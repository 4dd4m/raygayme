#pragma once

typedef struct ServerVec2i {
    int x;
    int z;
} ServerVec2i;

typedef struct ServerVec3 {
    float x;
    float y;
    float z;
} ServerVec3;

typedef struct ServerTerrainBounds {
    ServerVec3 min;
    ServerVec3 max;
} ServerTerrainBounds;

typedef struct ServerTerrainChunk {
    char *id;
    char *heightFile;
    ServerVec2i coord;
    ServerVec3 origin;
    int gridWidth;
    int gridDepth;
    float *heights;
    int heightCount;
    ServerTerrainBounds bounds;
} ServerTerrainChunk;

typedef struct ServerTerrainData {
    ServerTerrainChunk *chunks;
    int chunkCount;
} ServerTerrainData;

const ServerTerrainData *GetServerTerrainData();
ServerTerrainData *GetInitialTerrainData();
void UnloadServerTerrainData();
void LoadServerTerrainData();

int LoadServerTerrainChunkByCoord(ServerVec2i coord);

float GetYcoordByChunkIndex(float playerWorldX, float playerWorldZ, int chunkIndex);
