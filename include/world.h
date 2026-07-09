#pragma once
#include "raylib.h"

#define MAX_CHUNKS 256

typedef unsigned int EntityId;

typedef struct Asset {
    Model model;
} Asset;

typedef struct ChunkCoord {
    int x;
    int z;
} ChunkCoord;

typedef struct WorldObject {
    EntityId id;
    Asset* asset;
    Vector3 position;
    float rotation;
    Vector3 scale;
} WorldObject;

typedef struct CollisionObject {
    Mesh mesh;
    BoundingBox boundingBox;
    Matrix transform;
} CollisionObject;

typedef struct Chunk {
    ChunkCoord coord;

    Mesh mesh;
    Model model;

    WorldObject* objects;
    int objectCount;

    CollisionObject* collisions;
    int collisionCount;

    bool isActive;
} Chunk;

typedef struct World {
    int id;
    Chunk chunks[MAX_CHUNKS];
    Chunk* activeChunk;
    int chunkCount;
    char* worldName;
} World;

void InitWorld(World* world);
void UpdateWorld(World* world);
void DrawWorld(World* world);
void ShutdownWorld(World* world);