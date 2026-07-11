#include "assetLoader.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <world.h>

#include "cJSON.h"

Asset Assets[ASSET_COUNT] = {0};

char* AssetPath[ASSET_COUNT] = {"assets/static_assets/ASSET_TREE.glb",
                                "assets/static_assets/ASSET_TREE_TRUNK.glb"};

Asset* GetAsset(AssetId id) {
    if (Assets[id].isLoaded) return &Assets[id];

    Assets[id].model = LoadModel(AssetPath[id]);
    Assets[id].isLoaded = true;

    return &Assets[id];
}

void UnloadAsset(AssetId id) {
    if (!Assets[id].isLoaded) return;

    UnloadModel(Assets[id].model);
    Assets[id] = (Asset){0};
}

void UnloadAllAssets() {
    for (size_t i = 0; i < ASSET_COUNT; i++) {
        if (Assets[i].isLoaded) {
            UnloadAsset((AssetId)i);
        }
    }
}

char* LoadStaticObjectFile() {
    FILE* file = fopen("assets/objects.json", "r");

    if (!file) {
        perror("Error opening static objects file");
        return NULL;
    }

    // check file length
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (length <= 0) {
        perror("Object file size problem");
        fclose(file);
        return NULL;
    }

    // allocate memory
    char* buffer = malloc(length + 1);
    if (!buffer) {
        fclose(file);
        perror("Memory allocation has been failed");
        return NULL;
    }

    // read file into the buffer
    size_t read_size = fread(buffer, 1, length, file);
    buffer[read_size] = '\0';

    printf(">>> Static File Json hasbeen loaded with a size of %lld\n", read_size);

    fclose(file);

    return buffer;
}

void LoadStaticAssetsForChunk(int chunkId) {
    char* fileContent = LoadStaticObjectFile();
    cJSON* json = cJSON_Parse(fileContent);
    if (json == NULL) {
        fprintf(stderr, "Objects file cannot be parsed\n");
        goto error;
    }

    const cJSON* objects = NULL;
    const cJSON* object = NULL;
    const cJSON* positions = NULL;
    Vector3 position = {0};
    const cJSON* rotations = NULL;
    Vector3 rotation = {0};

    objects = cJSON_GetObjectItemCaseSensitive(json, "objects");

    WorldObject* worldObjects = malloc(cJSON_GetArraySize(objects) * sizeof(WorldObject));
    if (worldObjects == NULL) {
        goto error;
    }

    printf(">>> Found %d objects\n", cJSON_GetArraySize(objects));

    cJSON_ArrayForEach(object, objects) {
        cJSON* id = cJSON_GetObjectItemCaseSensitive(object, "id");
        cJSON* interactive = cJSON_GetObjectItemCaseSensitive(object, "interactive");
        cJSON* name = cJSON_GetObjectItemCaseSensitive(object, "name");
        cJSON* type = cJSON_GetObjectItemCaseSensitive(object, "type");
        cJSON* chunk = cJSON_GetObjectItemCaseSensitive(object, "chunk");

        positions = cJSON_GetObjectItemCaseSensitive(object, "position");

        if (cJSON_IsArray(positions) && cJSON_GetArraySize(positions) == 3) {
            position = (Vector3){
                .x = (float)cJSON_GetArrayItem(positions, 0)->valuedouble,
                .y = (float)cJSON_GetArrayItem(positions, 1)->valuedouble,
                .z = (float)cJSON_GetArrayItem(positions, 2)->valuedouble,
            };
        } else {
            goto error;
        }

        rotations = cJSON_GetObjectItemCaseSensitive(object, "rotation");

        if (cJSON_IsArray(rotations) && cJSON_GetArraySize(rotations) == 3) {
            rotation = (Vector3){
                .x = (float)cJSON_GetArrayItem(rotations, 0)->valuedouble,
                .y = (float)cJSON_GetArrayItem(rotations, 1)->valuedouble,
                .z = (float)cJSON_GetArrayItem(rotations, 2)->valuedouble,
            };
        } else {
            goto error;
        }

        WorldObject parsedObject = (WorldObject){
            .id = id->valuestring,
            .interactive = strcmp(interactive->valuestring, "true") == 0 ? true : false,
            .name = name->valuestring,
            .type = GetAssetIdFromString(id->valuestring, type->valuestring),
            .chunk = chunk->valueint,
            .position = position,
            .rotation = rotation,
            .model = &(GetAsset((AssetId)type->valueint)->model)};

        fprintf(stdout, ">>> Asset loaded:%s\t\t Type:%d\tId:%s\n", parsedObject.name,
                parsedObject.type, parsedObject.id);
    }

    fprintf(stderr, ">>> WorldObjects parsing have been finished\n");
    goto end;

error:
    fprintf(stderr, "Error while Parsing Objects.json\n");
    free(fileContent);
    free(json);
    free(worldObjects);

end:
    free(fileContent);
    free(json);
    // to remove
    free(worldObjects);
}

static AssetId GetAssetIdFromString(const char* id, const char* type) {
    if (type == NULL) {
        fprintf(stderr, "Object %s has invalid Type", id);
        exit(1);
    }

    if (strcmp(type, "ASSET_TREE") == 0) {
        return ASSET_TREE;
    }

    if (strcmp(type, "ASSET_TREE_TRUNK") == 0) {
        return ASSET_TREE_TRUNK;
    }

    if (strcmp(type, "COLLISION") == 0) {
        return COLLISION;
    }

    fprintf(stderr, "Object %s has valid type registered in GetAssetIdFromString()", id);
    exit(1);
}
