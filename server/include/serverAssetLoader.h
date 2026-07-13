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

typedef struct ServerTri {
    int a;
    int b;
    int c;
} ServerTri;

typedef struct ServerTerrainBounds {
    ServerVec3 min;
    ServerVec3 max;
} ServerTerrainBounds;

typedef struct ServerTerrainChunk {
    char* id;
    ServerVec2i coord;
    ServerVec3 origin;
    int gridWidth;
    int gridDepth;
    float cellSizeX;
    float cellSizeZ;
    char* vertsSpace;
    ServerVec3* verts;
    int vertCount;
    ServerTri* tris;
    int triCount;
    float* heights;
    int heightCount;
    ServerTerrainBounds bounds;
} ServerTerrainChunk;

typedef struct ServerTerrainData {
    ServerTerrainChunk* chunks;
    int chunkCount;
} ServerTerrainData;

const ServerTerrainData* GetServerTerrainData();
void UnloadServerTerrainData();
void parse_server_json();
