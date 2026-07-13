#pragma once
#include "player_state.h"
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
    Matrix transform;
    BoundingBox boundingBox;
    bool isMouseOver;
} WorldObject;

#define MAX_CHUNKS 256
#define MAX_PLAYERS 10

typedef unsigned int EntityId;

typedef struct Asset {
    Model model;
    bool isLoaded;
} Asset;

typedef struct ChunkCoord {
    int x;
    int y;
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
    Model model;  // terrain

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
    Shader outlineShader;
    RenderTexture2D shadowMap;
    Vector3 lightDir;
    Camera3D lightCamera;
    Matrix lightViewProj;
    WorldObject* worldObjects;
    Vector3 pointToClickCursor;
    int worldObjectCount;
    // some shader
    int shadowLightViewProjLoc;
    int shadowLightDirLoc;
    int shadowMapLoc;
    int outlineColorLoc;
    int outlineSizeLoc;
    PlayerNetState players[MAX_PLAYERS];
} World;

void InitWorld(World* world);
void UpdateWorld(World* world);
void DrawWorld(World* world, Camera3D camera);
void RenderWorldShadowMap(World* world);
void ShutdownWorld(World* world);
