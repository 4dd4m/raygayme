#pragma once
#include "assetLoader.h"
#include "raylib.h"

typedef struct WorldObject {
    const char* id;
    bool interactive;
    const char* name;
    AssetId type;
    int chunk;
    Vector3 position;
    Vector3 rotation;
    Model* model;
} WorldObject;

#define MAX_CHUNKS 256

typedef unsigned int EntityId;

typedef struct Asset {
    Model model;
    bool isLoaded;
} Asset;

typedef struct ChunkCoord {
    int x;
    int z;
} ChunkCoord;

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
    Shader shadowShader;
    Shader shadowDepthShader;
    RenderTexture2D shadowMap;
    Vector3 lightDir;
    Camera3D lightCamera;
    Matrix lightViewProj;
    WorldObject* worldObjects;
    int worldObjectCount;
    int shadowLightViewProjLoc;
    int shadowLightDirLoc;
    int shadowMapLoc;
} World;

void InitWorld(World* world);
void UpdateWorld(World* world);
void DrawWorld(World* world);
void RenderWorldShadowMap(World* world);
void ShutdownWorld(World* world);
