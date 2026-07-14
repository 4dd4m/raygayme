#pragma once

#include "terrainAssetLoader.h"

typedef struct Asset Asset;
typedef struct Chunk Chunk;
typedef struct WorldObject WorldObject;

typedef enum AssetId {
    TERRAIN,
    ASSET_TREE,
    ASSET_TREE_TRUNK,
    ASSET_ROCK,
    COLLISION_TREE,
    ASSET_COUNT
} AssetId;

Asset* GetAsset(AssetId id);
void UnloadAsset(AssetId id);
void UnloadAllAssets();
WorldObject* LoadStaticAssetsForChunk(int chunkId, int* worldObjectCount);
WorldObject* LoadChunkAssetsByCoords(ServerVec2i coords, int* worldObjectCount);
int LoadChunkByCoords(Chunk* chunk, ServerVec2i coords);
int GetNeighbourChunkCoords(ServerVec2i current, ServerVec2i* out);