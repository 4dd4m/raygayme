#include "terrainAssetLoader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "file_io.h"

#define HEIGHTMAPS_DIR "../assets/heightmaps"

static ServerTerrainData terrainData = {0};

static char* duplicate_string(const char* source) {
    if (source == NULL) return NULL;

    size_t length = strlen(source);
    char* copy = malloc(length + 1);
    if (copy == NULL) return NULL;

    memcpy(copy, source, length + 1);
    return copy;
}

static char* build_chunk_file_name(ServerVec2i coord, const char* extension) {
    // build the Chunk_X_X.extension filename
    int length = snprintf(NULL, 0, "Chunk%d_%d.%s", coord.x, coord.z,
                          extension);  // assembling the filenames length
    if (length < 0) return NULL;

    char* fileName = malloc((size_t)length + 1u);  // memory for the filename
    if (fileName == NULL) return NULL;

    // snprintf always \0 terminates the string
    snprintf(fileName, (size_t)length + 1u, "Chunk%d_%d.%s", coord.x, coord.z,
             extension);  // copy the filename to the buffer
    return fileName;
}

static char* join_path(const char* directory, const char* fileName) {
    if (directory == NULL || fileName == NULL) return NULL;

    size_t directoryLength = strlen(directory);
    size_t fileNameLength = strlen(fileName);
    int needsSlash = directoryLength > 0 && directory[directoryLength - 1] != '/' &&
                     directory[directoryLength - 1] != '\\';
    size_t length = directoryLength + (needsSlash ? 1u : 0u) + fileNameLength;

    char* path = malloc(length + 1);
    if (path == NULL) return NULL;

    strcpy(path, directory);
    if (needsSlash) strcat(path, "/");
    strcat(path, fileName);
    return path;
}

static int parse_vec3(const cJSON* array, ServerVec3* out) {
    if (!cJSON_IsArray(array) || cJSON_GetArraySize(array) != 3 || out == NULL) return 0;

    const cJSON* x = cJSON_GetArrayItem(array, 0);
    const cJSON* y = cJSON_GetArrayItem(array, 1);
    const cJSON* z = cJSON_GetArrayItem(array, 2);
    if (!cJSON_IsNumber(x) || !cJSON_IsNumber(y) || !cJSON_IsNumber(z)) return 0;

    *out = (ServerVec3){
        .x = (float)x->valuedouble,
        .y = (float)y->valuedouble,
        .z = (float)z->valuedouble,
    };
    return 1;
}

static int parse_vec2i(const cJSON* array, ServerVec2i* out) {
    if (!cJSON_IsArray(array) || cJSON_GetArraySize(array) != 2 || out == NULL) return 0;

    const cJSON* x = cJSON_GetArrayItem(array, 0);
    const cJSON* z = cJSON_GetArrayItem(array, 1);
    if (!cJSON_IsNumber(x) || !cJSON_IsNumber(z)) return 0;

    *out = (ServerVec2i){.x = x->valueint, .z = z->valueint};
    return 1;
}

static int parse_cell_size(const cJSON* chunkJson, float* cellSizeX, float* cellSizeZ) {
    const cJSON* x = cJSON_GetObjectItemCaseSensitive(chunkJson, "cellSizeX");
    const cJSON* z = cJSON_GetObjectItemCaseSensitive(chunkJson, "cellSizeZ");

    if (cJSON_IsNumber(x) && cJSON_IsNumber(z)) {
        *cellSizeX = (float)x->valuedouble;
        *cellSizeZ = (float)z->valuedouble;
        return 1;
    }

    const cJSON* legacyCellSize = cJSON_GetObjectItemCaseSensitive(chunkJson, "cellSize");
    if (cJSON_IsNumber(legacyCellSize)) {
        *cellSizeX = (float)legacyCellSize->valuedouble;
        *cellSizeZ = (float)legacyCellSize->valuedouble;
        return 1;
    }

    if (cJSON_IsArray(legacyCellSize) && cJSON_GetArraySize(legacyCellSize) == 2) {
        const cJSON* legacyX = cJSON_GetArrayItem(legacyCellSize, 0);
        const cJSON* legacyZ = cJSON_GetArrayItem(legacyCellSize, 1);
        if (!cJSON_IsNumber(legacyX) || !cJSON_IsNumber(legacyZ)) return 0;

        *cellSizeX = (float)legacyX->valuedouble;
        *cellSizeZ = (float)legacyZ->valuedouble;
        return 1;
    }

    return 0;
}

static int load_height_file(const char* heightPath, ServerTerrainChunk* chunk) {
    if (heightPath == NULL || chunk == NULL || chunk->gridWidth <= 0 || chunk->gridDepth <= 0) {
        return 0;
    }

    chunk->heightCount = chunk->gridWidth * chunk->gridDepth;
    chunk->heights = calloc((size_t)chunk->heightCount, sizeof(float));
    if (chunk->heights == NULL) return 0;

    FILE* file = fopen(heightPath, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open height file: %s\n", heightPath);
        return 0;
    }

    size_t readCount = fread(chunk->heights, sizeof(float), (size_t)chunk->heightCount, file);
    fclose(file);

    if (readCount != (size_t)chunk->heightCount) {
        fprintf(stderr, "Height file has wrong size: %s\n", heightPath);
        return 0;
    }

    return 1;
}

static void free_chunk(ServerTerrainChunk* chunk) {
    if (chunk == NULL) return;

    free(chunk->id);
    free(chunk->heightFile);
    free(chunk->heights);
    *chunk = (ServerTerrainChunk){0};
}

static int parse_chunk_json(const cJSON* chunkJson, const char* heightFileName,
                            const char* heightPath, ServerTerrainChunk* chunk) {
    if (!cJSON_IsObject(chunkJson) || heightFileName == NULL || heightPath == NULL ||
        chunk == NULL) {
        return 0;
    }

    const cJSON* id = cJSON_GetObjectItemCaseSensitive(chunkJson, "id");
    const cJSON* coord = cJSON_GetObjectItemCaseSensitive(chunkJson, "coord");
    const cJSON* origin = cJSON_GetObjectItemCaseSensitive(chunkJson, "origin");
    const cJSON* gridWidth = cJSON_GetObjectItemCaseSensitive(chunkJson, "gridWidth");
    const cJSON* gridDepth = cJSON_GetObjectItemCaseSensitive(chunkJson, "gridDepth");
    const cJSON* bounds = cJSON_GetObjectItemCaseSensitive(chunkJson, "bounds");
    const cJSON* boundsMin = cJSON_GetObjectItemCaseSensitive(bounds, "min");
    const cJSON* boundsMax = cJSON_GetObjectItemCaseSensitive(bounds, "max");

    if (!cJSON_IsString(id) || id->valuestring == NULL || !cJSON_IsNumber(gridWidth) ||
        !cJSON_IsNumber(gridDepth)) {
        return 0;
    }

    chunk->id = duplicate_string(id->valuestring);
    chunk->heightFile = duplicate_string(heightFileName);
    chunk->gridWidth = gridWidth->valueint;
    chunk->gridDepth = gridDepth->valueint;

    if (chunk->id == NULL || chunk->heightFile == NULL || !parse_vec2i(coord, &chunk->coord) ||
        !parse_vec3(origin, &chunk->origin) ||
        !parse_cell_size(chunkJson, &chunk->cellSizeX, &chunk->cellSizeZ) ||
        !parse_vec3(boundsMin, &chunk->bounds.min) || !parse_vec3(boundsMax, &chunk->bounds.max) ||
        !load_height_file(heightPath, chunk)) {
        free_chunk(chunk);
        return 0;
    }

    return 1;
}

static int load_chunk_by_coord(ServerVec2i coord, ServerTerrainChunk* chunk) {
    char* jsonFileName = build_chunk_file_name(coord, "json");
    char* heightFileName = build_chunk_file_name(coord, "height");
    char* jsonPath = join_path(HEIGHTMAPS_DIR, jsonFileName);      // Chunk_X_X.json
    char* heightPath = join_path(HEIGHTMAPS_DIR, heightFileName);  // Chink_X_X.height
    char* fileContent = NULL;
    cJSON* json = NULL;
    int success = 0;

    if (jsonFileName == NULL || heightFileName == NULL || jsonPath == NULL || heightPath == NULL) {
        goto cleanup;
    }

    fileContent = LoadFile(jsonPath);
    if (fileContent == NULL) {
        fprintf(stderr, "Could not open terrain chunk JSON: %s\n", jsonPath);
        goto cleanup;
    }

    json = cJSON_Parse(fileContent);
    if (json == NULL) {
        fprintf(stderr, "Terrain chunk JSON cannot be parsed: %s\n", cJSON_GetErrorPtr());
        goto cleanup;
    }

    // parses both json and height into the chunk
    success = parse_chunk_json(json, heightFileName, heightPath, chunk);
    if (!success) {
        fprintf(stderr, "Error while parsing terrain chunk %d,%d\n", coord.x, coord.z);
    }

cleanup:
    free(jsonFileName);
    free(heightFileName);
    free(jsonPath);
    free(heightPath);
    free(fileContent);
    if (json != NULL) cJSON_Delete(json);
    return success;
}

const ServerTerrainData* GetServerTerrainData() { return &terrainData; }

void UnloadServerTerrainData() {
    for (int i = 0; i < terrainData.chunkCount; i++) {
        free_chunk(&terrainData.chunks[i]);
    }

    free(terrainData.chunks);
    terrainData = (ServerTerrainData){0};
}

void LoadServerTerrainData() {
    UnloadServerTerrainData();  // unload first to avoid garbage memory data

    terrainData.chunkCount = 1;
    terrainData.chunks = calloc(1u, sizeof(ServerTerrainChunk));

    if (terrainData.chunks == NULL) {  // if the allocation has been failed
        terrainData.chunkCount = 0;
        return;
    }

    ServerVec2i spawnChunk = {0, 0};  // first initial chunk

    if (!load_chunk_by_coord(spawnChunk,
                             &terrainData.chunks[0])) {  // load the chunk, unload if fails
        UnloadServerTerrainData();
        return;
    }

    printf("Parsed %d terrain chunk(s) from chunk files\n", terrainData.chunkCount);
}

ServerTerrainData* GetInitialTerrainData() {
    LoadServerTerrainData();

    if (terrainData.chunkCount == 0) {
        return NULL;
    }

    return &terrainData;
}
