#include "assetLoader.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <world.h>

#include "assetLoader.h"
#include "cJSON.h"

Asset Assets[ASSET_COUNT] = {0};

char* AssetPath[ASSET_COUNT] = {"assets/static_assets/ASSET_TREE.glb",
                                "assets/static_assets/ASSET_TREE_TRUNK.glb",
                                "assets/collisions/COLLISION_TREE.glb"};

Asset* GetAsset(AssetId id) {
    if (Assets[id].isLoaded) {
        printf("Cached Asset >>> ");
        return &Assets[id];
    }

    Assets[id].model = LoadModel(AssetPath[id]);
    Assets[id].isLoaded = true;

    printf("<<< Load Model");
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

static AssetId GetAssetIdFromString(const char* id, const char* type) {
    if (type == NULL) {
        fprintf(stderr, "Object %s has invalid Type: %s\n", id ? id : "(null)", type);
        return ASSET_COUNT;
    }

    if (strcmp(type, "ASSET_TREE") == 0) {
        return ASSET_TREE;
    }

    if (strcmp(type, "ASSET_TREE_TRUNK") == 0) {
        return ASSET_TREE_TRUNK;
    }

    if (strcmp(type, "COLLISION_TREE") == 0) {
        return COLLISION_TREE;
    }

    fprintf(stderr, "Object %s has invalid type registered in GetAssetIdFromString()\n",
            id ? id : "(null)");
    return ASSET_COUNT;
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

    printf("!!! Static File Json has been loaded with a size of %lld\n", read_size);

    fclose(file);

    return buffer;
}

WorldObject* LoadStaticAssetsForChunk(int chunkId) {
    char* fileContent = LoadStaticObjectFile();
    if (fileContent == NULL) {
        goto error;
    }
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
    if (!cJSON_IsArray(objects)) {
        fprintf(stderr, "Objects file does not contain a valid objects array\n");
        goto error;
    }

    int objectCount = cJSON_GetArraySize(objects);
    WorldObject* worldObjects =
        malloc((objectCount > 0 ? (size_t)objectCount : 1u) * sizeof(WorldObject));
    if (worldObjects == NULL) {
        goto error;
    }

    printf("--- Found %d objects\n", objectCount);

    int i = 0;
    cJSON_ArrayForEach(object, objects) {
        cJSON* id = cJSON_GetObjectItemCaseSensitive(object, "id");
        cJSON* interactive = cJSON_GetObjectItemCaseSensitive(object, "interactive");
        cJSON* name = cJSON_GetObjectItemCaseSensitive(object, "name");
        cJSON* type = cJSON_GetObjectItemCaseSensitive(object, "type");
        cJSON* chunk = cJSON_GetObjectItemCaseSensitive(object, "chunk");
        bool isInteractive = false;
        int parsedChunk = 0;

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

        if (!cJSON_IsString(id) || !cJSON_IsString(name) || !cJSON_IsString(type)) {
            goto error;
        }

        if (cJSON_IsTrue(interactive)) {
            isInteractive = true;
        } else if (cJSON_IsFalse(interactive)) {
            isInteractive = false;
        } else if (cJSON_IsString(interactive) && interactive->valuestring != NULL) {
            isInteractive = strcmp(interactive->valuestring, "true") == 0;
        }

        if (cJSON_IsNumber(chunk)) {
            parsedChunk = chunk->valueint;
        } else if (cJSON_IsString(chunk) && chunk->valuestring != NULL) {
            parsedChunk = atoi(chunk->valuestring);
        } else {
            goto error;
        }

        AssetId assetId = GetAssetIdFromString(id->valuestring, type->valuestring);
        if (assetId < 0 || assetId >= ASSET_COUNT) {
            goto error;
        }
        Model* model = &(GetAsset(assetId)->model);

        WorldObject parsedObject = (WorldObject){.id = strdup(id->valuestring),
                                                 .interactive = isInteractive,
                                                 .name = strdup(name->valuestring),
                                                 .type = assetId,
                                                 .chunk = parsedChunk,
                                                 .position = position,
                                                 .rotation = rotation,
                                                 .model = model};

        fprintf(stdout, "%s\t\t Type:%d\tId:%s\n", parsedObject.name, parsedObject.type,
                parsedObject.id);

        if (i < objectCount) {
            worldObjects[i++] = parsedObject;
        }
    }

    fprintf(stderr, ">>> WorldObjects parsing have been finished\n");
    goto end;

error:
    fprintf(stderr, "Error while Parsing Objects.json\n");
    if (fileContent) free(fileContent);
    if (json) cJSON_Delete(json);
    if (worldObjects) free(worldObjects);

end:
    if (fileContent) free(fileContent);
    if (json) cJSON_Delete(json);
    // to remove
    return worldObjects;
}
