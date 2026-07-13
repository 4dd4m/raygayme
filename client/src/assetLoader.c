#include "assetLoader.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <world.h>

#include "assetLoader.h"
#include "cJSON.h"
#include "file_io.h"
#include "raymath.h"

#define HERE printf("<><>><<>><><> I AM HERE ><><><><><><><><\n");

#define Debug 0

Asset Assets[ASSET_COUNT] = {0};

char* AssetPath[ASSET_COUNT] = {
    "assets/chunks/TERRAIN.glb",
    "assets/assets/ASSET_TREE.glb",
    "assets/assets/ASSET_TREE_TRUNK.glb",
    "assets/assets/ASSET_ROCK.glb",
    "assets/collisions/COLLISION_TREE.glb",
};

Asset* GetAsset(AssetId id) {
    if (Assets[id].isLoaded) {
        if (Debug) printf("Cached Asset >>> \n");
        return &Assets[id];
    }

    Assets[id].model = LoadModel(AssetPath[id]);
    Assets[id].isLoaded = true;

    if (Debug) printf("<<< Load Model\n");
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

    if (strcmp(type, "TERRAIN") == 0) {
        return TERRAIN;
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

    if (strcmp(type, "ASSET_ROCK") == 0) {
        return ASSET_ROCK;
    }

    fprintf(stderr, "Object %s has invalid type registered in GetAssetIdFromString()\n",
            id ? id : "(null)");
    return ASSET_COUNT;
}

WorldObject* LoadStaticAssetsForChunk(int chunkId, int* worldObjectCount) {
    char* fileContent = LoadFile("assets/objects.json");
    WorldObject* worldObjects = NULL;
    int objectCount = 0;
    int i = 0;
    if (worldObjectCount) {
        *worldObjectCount = 0;
    }
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

    objectCount = cJSON_GetArraySize(objects);
    worldObjects = malloc((objectCount > 0 ? (size_t)objectCount : 1u) * sizeof(WorldObject));
    if (worldObjects == NULL) {
        goto error;
    }

    if (Debug) printf("--- Found %d objects\n", objectCount);

    cJSON_ArrayForEach(object, objects) {
        cJSON* id = cJSON_GetObjectItemCaseSensitive(object, "id");
        cJSON* interactive = cJSON_GetObjectItemCaseSensitive(object, "interactive");
        cJSON* name = cJSON_GetObjectItemCaseSensitive(object, "name");
        cJSON* type = cJSON_GetObjectItemCaseSensitive(object, "type");
        cJSON* chunk = cJSON_GetObjectItemCaseSensitive(object, "chunk");
        bool isInteractive = false;
        int parsedChunk = 0;
        int parsedType = 0;

        positions = cJSON_GetObjectItemCaseSensitive(object, "position");

        if (cJSON_IsArray(positions) && cJSON_GetArraySize(positions) == 3) {
            position = (Vector3){
                .x = (double)cJSON_GetArrayItem(positions, 0)->valuedouble,
                .y = (double)cJSON_GetArrayItem(positions, 1)->valuedouble,
                .z = (double)cJSON_GetArrayItem(positions, 2)->valuedouble,
            };
        } else {
            goto error;
        }

        rotations = cJSON_GetObjectItemCaseSensitive(object, "rotation");

        if (cJSON_IsArray(rotations) && cJSON_GetArraySize(rotations) == 3) {
            rotation = (Vector3){
                .x = (double)cJSON_GetArrayItem(rotations, 0)->valuedouble,
                .y = (double)cJSON_GetArrayItem(rotations, 1)->valuedouble,
                .z = (double)cJSON_GetArrayItem(rotations, 2)->valuedouble,
            };
        } else {
            goto error;
        }

        if (!cJSON_IsString(id) || !cJSON_IsString(name) || !cJSON_IsNumber(type)) {
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

        if (cJSON_IsNumber(type)) {
            parsedType = type->valueint;
        } else if (cJSON_IsString(type) && chunk->valuestring != NULL) {
            parsedType = atoi(type->valuestring);
        } else {
            goto error;
        }

        if (Debug) printf("TYPEID: %d", parsedType);

        // calculate WorldObjectTransform
        Model* model = &(GetAsset(type->valueint)->model);
        Matrix transform =
            MatrixMultiply(model->transform, MatrixTranslate(position.x, position.y, position.z));

        // bounding box by default goes to origin, so pull it back where the object really
        BoundingBox box = GetModelBoundingBox(*model);
        box.min = Vector3Add(box.min, position);
        box.max = Vector3Add(box.max, position);

        WorldObject parsedObject = (WorldObject){.id = strdup(id->valuestring),
                                                 .interactive = isInteractive,
                                                 .name = strdup(name->valuestring),
                                                 .type = parsedType,
                                                 .chunk = parsedChunk,
                                                 .position = position,
                                                 .rotation = rotation,
                                                 .model = model,
                                                 .transform = transform,
                                                 .boundingBox = box,
                                                 .isMouseOver = true};

        if (Debug) {
            fprintf(stdout, "%s\t\t Type:%d\tId:%s | X: %.17g Y: %.17g Z: %.17g\n",
                    parsedObject.name, parsedObject.type, parsedObject.id, parsedObject.position.x,
                    parsedObject.position.y, parsedObject.position.z);
        }

        if (i < objectCount) {
            worldObjects[i++] = parsedObject;
        }
    }

    if (Debug) fprintf(stderr, ">>> WorldObjects parsing have been finished\n");
    if (worldObjectCount) {
        *worldObjectCount = i;
    }
    if (fileContent) free(fileContent);
    if (json) cJSON_Delete(json);
    return worldObjects;

error:
    fprintf(stderr, "Error while Parsing Objects.json\n");
    if (fileContent) free(fileContent);
    if (json) cJSON_Delete(json);
    if (worldObjects) free(worldObjects);
    return NULL;
}