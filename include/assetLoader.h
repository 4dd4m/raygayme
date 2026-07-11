#pragma once

typedef enum AssetId { ASSET_TREE, ASSET_TREE_TRUNK, ASSET_COUNT } AssetId;

typedef struct Asset Asset;
typedef struct WorldObject WorldObject;

Asset* GetAsset(AssetId id);
void UnloadAsset(AssetId id);
void UnloadAllAssets();
void LoadStaticAssetsForChunk(int chunkId);

char* LoadStaticObjectFile();
