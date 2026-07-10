#pragma once
#include "world.h"

typedef enum AssetId { ASSET_TREE, ASSET_TREE_TRUNK, ASSET_COUNT } AssetId;

Asset* GetAsset(AssetId id);
void UnloadAsset(AssetId id);
void UnloadAllAssets();
void LoadStaticAssetsForChunk(int chunkId);

char* LoadStaticObjectFile();