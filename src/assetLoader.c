#include "assetLoader.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <world.h>

#include "cJSON.h"

Asset Assets[ASSET_COUNT] = {0};

char* AssetPath[ASSET_COUNT] = {"assets/Tree.glb", "assets/TreeTrunk.glb"};

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
    FILE* file = fopen("assets/terrain/objects.json", "r");

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
    // char* fileContent = LoadStaticObjectFile();
    // cJSON* json = cJSON_Parse(fileContent);
    // free(fileContent);
    // free(json);
}