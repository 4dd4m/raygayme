#pragma once

typedef enum AssetId { ASSET_TREE, ASSET_TREE_TRUNK, COLLISION_TREE, ASSET_COUNT } AssetId;

typedef struct Asset Asset;
typedef struct WorldObject WorldObject;

Asset* GetAsset(AssetId id);
void UnloadAsset(AssetId id);
void UnloadAllAssets();
WorldObject* LoadStaticAssetsForChunk(int chunkId, int* worldObjectCount);

char* LoadStaticObjectFile();
