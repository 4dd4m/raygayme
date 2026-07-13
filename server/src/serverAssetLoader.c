#include "serverAssetLoader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "file_io.h"

static ServerTerrainData terrainData = {0};

static char* duplicate_string(const char* source) {
    if (source == NULL) return NULL;

    size_t length = strlen(source);
    char* copy = malloc(length + 1);
    if (copy == NULL) return NULL;

    memcpy(copy, source, length + 1);
    return copy;
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

    *out = (ServerVec2i){
        .x = x->valueint,
        .z = z->valueint,
    };
    return 1;
}

static int parse_cell_size(const cJSON* value, float* cellSizeX, float* cellSizeZ) {
    if (cellSizeX == NULL || cellSizeZ == NULL) return 0;

    if (cJSON_IsNumber(value)) {
        *cellSizeX = (float)value->valuedouble;
        *cellSizeZ = (float)value->valuedouble;
        return 1;
    }

    if (cJSON_IsArray(value) && cJSON_GetArraySize(value) == 2) {
        const cJSON* x = cJSON_GetArrayItem(value, 0);
        const cJSON* z = cJSON_GetArrayItem(value, 1);
        if (!cJSON_IsNumber(x) || !cJSON_IsNumber(z)) return 0;

        *cellSizeX = (float)x->valuedouble;
        *cellSizeZ = (float)z->valuedouble;
        return 1;
    }

    return 0;
}

static int parse_verts(const cJSON* verts, ServerVec3** outVerts, int* outCount) {
    if (!cJSON_IsArray(verts) || outVerts == NULL || outCount == NULL) return 0;

    int count = cJSON_GetArraySize(verts);
    ServerVec3* parsedVerts = calloc((count > 0) ? (size_t)count : 1u, sizeof(ServerVec3));
    if (parsedVerts == NULL) return 0;

    for (int i = 0; i < count; i++) {
        if (!parse_vec3(cJSON_GetArrayItem(verts, i), &parsedVerts[i])) {
            free(parsedVerts);
            return 0;
        }
    }

    *outVerts = parsedVerts;
    *outCount = count;
    return 1;
}

static int parse_tris(const cJSON* tris, ServerTri** outTris, int* outCount) {
    if (!cJSON_IsArray(tris) || outTris == NULL || outCount == NULL) return 0;

    int count = cJSON_GetArraySize(tris);
    ServerTri* parsedTris = calloc((count > 0) ? (size_t)count : 1u, sizeof(ServerTri));
    if (parsedTris == NULL) return 0;

    for (int i = 0; i < count; i++) {
        const cJSON* tri = cJSON_GetArrayItem(tris, i);
        if (!cJSON_IsArray(tri) || cJSON_GetArraySize(tri) != 3) {
            free(parsedTris);
            return 0;
        }

        const cJSON* a = cJSON_GetArrayItem(tri, 0);
        const cJSON* b = cJSON_GetArrayItem(tri, 1);
        const cJSON* c = cJSON_GetArrayItem(tri, 2);
        if (!cJSON_IsNumber(a) || !cJSON_IsNumber(b) || !cJSON_IsNumber(c)) {
            free(parsedTris);
            return 0;
        }

        parsedTris[i] = (ServerTri){.a = a->valueint, .b = b->valueint, .c = c->valueint};
    }

    *outTris = parsedTris;
    *outCount = count;
    return 1;
}

static int parse_heights(const cJSON* heights, float** outHeights, int* outCount) {
    if (!cJSON_IsArray(heights) || outHeights == NULL || outCount == NULL) return 0;

    int count = cJSON_GetArraySize(heights);
    float* parsedHeights = calloc((count > 0) ? (size_t)count : 1u, sizeof(float));
    if (parsedHeights == NULL) return 0;

    for (int i = 0; i < count; i++) {
        const cJSON* height = cJSON_GetArrayItem(heights, i);
        if (!cJSON_IsNumber(height)) {
            free(parsedHeights);
            return 0;
        }

        parsedHeights[i] = (float)height->valuedouble;
    }

    *outHeights = parsedHeights;
    *outCount = count;
    return 1;
}

static void free_chunk(ServerTerrainChunk* chunk) {
    if (chunk == NULL) return;

    free(chunk->id);
    free(chunk->vertsSpace);
    free(chunk->verts);
    free(chunk->tris);
    free(chunk->heights);
    *chunk = (ServerTerrainChunk){0};
}

static int parse_chunk(const cJSON* chunkJson, ServerTerrainChunk* chunk) {
    if (!cJSON_IsObject(chunkJson) || chunk == NULL) return 0;

    const cJSON* id = cJSON_GetObjectItemCaseSensitive(chunkJson, "id");
    const cJSON* coord = cJSON_GetObjectItemCaseSensitive(chunkJson, "coord");
    const cJSON* origin = cJSON_GetObjectItemCaseSensitive(chunkJson, "origin");
    const cJSON* gridWidth = cJSON_GetObjectItemCaseSensitive(chunkJson, "gridWidth");
    const cJSON* gridDepth = cJSON_GetObjectItemCaseSensitive(chunkJson, "gridDepth");
    const cJSON* cellSize = cJSON_GetObjectItemCaseSensitive(chunkJson, "cellSize");
    const cJSON* vertsSpace = cJSON_GetObjectItemCaseSensitive(chunkJson, "vertsSpace");
    const cJSON* verts = cJSON_GetObjectItemCaseSensitive(chunkJson, "verts");
    const cJSON* tris = cJSON_GetObjectItemCaseSensitive(chunkJson, "tris");
    const cJSON* heights = cJSON_GetObjectItemCaseSensitive(chunkJson, "heights");
    const cJSON* bounds = cJSON_GetObjectItemCaseSensitive(chunkJson, "bounds");
    const cJSON* boundsMin = cJSON_GetObjectItemCaseSensitive(bounds, "min");
    const cJSON* boundsMax = cJSON_GetObjectItemCaseSensitive(bounds, "max");

    if (!cJSON_IsString(id) || id->valuestring == NULL || !cJSON_IsNumber(gridWidth) ||
        !cJSON_IsNumber(gridDepth) || !cJSON_IsString(vertsSpace) ||
        vertsSpace->valuestring == NULL) {
        return 0;
    }

    chunk->id = duplicate_string(id->valuestring);
    chunk->vertsSpace = duplicate_string(vertsSpace->valuestring);
    chunk->gridWidth = gridWidth->valueint;
    chunk->gridDepth = gridDepth->valueint;

    if (chunk->id == NULL || chunk->vertsSpace == NULL || !parse_vec2i(coord, &chunk->coord) ||
        !parse_vec3(origin, &chunk->origin) ||
        !parse_cell_size(cellSize, &chunk->cellSizeX, &chunk->cellSizeZ) ||
        !parse_verts(verts, &chunk->verts, &chunk->vertCount) ||
        !parse_tris(tris, &chunk->tris, &chunk->triCount) ||
        !parse_heights(heights, &chunk->heights, &chunk->heightCount) ||
        !parse_vec3(boundsMin, &chunk->bounds.min) ||
        !parse_vec3(boundsMax, &chunk->bounds.max)) {
        free_chunk(chunk);
        return 0;
    }

    if (chunk->heightCount != chunk->gridWidth * chunk->gridDepth) {
        fprintf(stderr, "Height count mismatch for chunk %s\n", chunk->id);
        free_chunk(chunk);
        return 0;
    }

    return 1;
}

static int file_exists(const char* path) {
    FILE* file = fopen(path, "r");
    if (file == NULL) return 0;

    fclose(file);
    return 1;
}

static char* load_heightmap_json() {
    const char* paths[] = {
        "client/assets/heightmaps.json",
        "client/assets/heighmaps.json",
        "../client/assets/heightmaps.json",
        "../client/assets/heighmaps.json",
    };

    for (int i = 0; i < 4; i++) {
        if (file_exists(paths[i])) {
            return LoadFile((char*)paths[i]);
        }
    }

    return NULL;
}

const ServerTerrainData* GetServerTerrainData() {
    return &terrainData;
}

void UnloadServerTerrainData() {
    for (int i = 0; i < terrainData.chunkCount; i++) {
        free_chunk(&terrainData.chunks[i]);
    }

    free(terrainData.chunks);
    terrainData = (ServerTerrainData){0};
}

void parse_server_json() {
    char* fileContent = load_heightmap_json();
    cJSON* json = NULL;

    UnloadServerTerrainData();

    if (fileContent == NULL) {
        fprintf(stderr, "Error while opening heightmaps JSON\n");
        return;
    }

    json = cJSON_Parse(fileContent);
    if (json == NULL) {
        fprintf(stderr, "Heightmaps JSON cannot be parsed: %s\n", cJSON_GetErrorPtr());
        goto error;
    }

    const cJSON* chunks = cJSON_GetObjectItemCaseSensitive(json, "chunks");
    if (!cJSON_IsArray(chunks)) {
        fprintf(stderr, "Heightmaps JSON does not contain a valid chunks array\n");
        goto error;
    }

    terrainData.chunkCount = cJSON_GetArraySize(chunks);
    terrainData.chunks =
        calloc((terrainData.chunkCount > 0) ? (size_t)terrainData.chunkCount : 1u,
               sizeof(ServerTerrainChunk));
    if (terrainData.chunks == NULL) goto error;

    for (int i = 0; i < terrainData.chunkCount; i++) {
        if (!parse_chunk(cJSON_GetArrayItem(chunks, i), &terrainData.chunks[i])) {
            fprintf(stderr, "Error while parsing heightmap chunk at index %d\n", i);
            goto error;
        }
    }

    printf("Parsed %d terrain chunk(s) from heightmaps JSON\n", terrainData.chunkCount);
    free(fileContent);
    cJSON_Delete(json);
    return;

error:
    UnloadServerTerrainData();
    free(fileContent);
    if (json != NULL) cJSON_Delete(json);
}

