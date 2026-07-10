#include "world.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assetLoader.h"
#include "raymath.h"

#define SHADOWMAP_SIZE 2048
#define LIGHT_DISTANCE 25.0f
#define LIGHT_ORTHO_SIZE 20.0f

static void SetModelShader(Model* model, Shader shader) {
    for (int i = 0; i < model->materialCount; i++) {
        model->materials[i].shader = shader;
    }
}

static void UpdateLightView(World* world) {
    Vector3 target = (Vector3){0.0f, 0.0f, 0.0f};
    world->lightDir = Vector3Normalize(world->lightDir);

    world->lightCamera.position = Vector3Add(target, Vector3Scale(world->lightDir, -LIGHT_DISTANCE));
    world->lightCamera.target = target;
    world->lightCamera.up = (Vector3){0.0f, 1.0f, 0.0f};
    world->lightCamera.fovy = LIGHT_ORTHO_SIZE;
    world->lightCamera.projection = CAMERA_ORTHOGRAPHIC;

    Matrix lightView = GetCameraMatrix(world->lightCamera);
    Matrix lightProj = MatrixOrtho(-LIGHT_ORTHO_SIZE, LIGHT_ORTHO_SIZE, -LIGHT_ORTHO_SIZE,
                                   LIGHT_ORTHO_SIZE, 0.1f, 100.0f);
    world->lightViewProj = MatrixMultiply(lightProj, lightView);
}

void InitWorld(World* world) {
    printf("### Initializing World\n");
    world->chunkCount = 0;

    // load starting chunk
    Chunk* chunk = &world->chunks[0];
    chunk->coord.x = 0;
    chunk->coord.z = 0;
    chunk->isActive = true;

    chunk->model = LoadModel("assets/terrain.glb");

    world->chunkCount = 1;

    world->activeChunk = chunk;

    world->activeChunk->collisionCount++;

    LoadStaticAssetsForChunk(0);

    chunk->collisions = malloc(sizeof(CollisionObject) * world->activeChunk->collisionCount);

    // Model initialCollision = LoadModel("assets/terrain/collision.glb");

    // chunk->collisions[0] =
    //     (CollisionObject){initialCollision.meshes[0],
    //     GetMeshBoundingBox(initialCollision.meshes[0]),
    //                       initialCollision.transform};

    // SET Shaders

    world->shadowShader = LoadShader("assets/shaders/shadow.vs", "assets/shaders/shadow.fs");
    world->shadowDepthShader =
        LoadShader("assets/shaders/shadow_depth.vs", "assets/shaders/shadow_depth.fs");

    world->shadowShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(world->shadowShader, "mvp");
    world->shadowShader.locs[SHADER_LOC_MATRIX_MODEL] =
        GetShaderLocation(world->shadowShader, "matModel");
    world->shadowShader.locs[SHADER_LOC_MAP_DIFFUSE] =
        GetShaderLocation(world->shadowShader, "texture0");
    world->shadowShader.locs[SHADER_LOC_COLOR_DIFFUSE] =
        GetShaderLocation(world->shadowShader, "colDiffuse");
    world->shadowLightViewProjLoc = GetShaderLocation(world->shadowShader, "lightViewProj");
    world->shadowLightDirLoc = GetShaderLocation(world->shadowShader, "lightDir");
    world->shadowMapLoc = GetShaderLocation(world->shadowShader, "shadowMap");

    world->shadowDepthShader.locs[SHADER_LOC_MATRIX_MVP] =
        GetShaderLocation(world->shadowDepthShader, "mvp");

    world->shadowMap = LoadRenderTexture(SHADOWMAP_SIZE, SHADOWMAP_SIZE);
    SetTextureFilter(world->shadowMap.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(world->shadowMap.texture, TEXTURE_WRAP_CLAMP);

    world->lightDir = (Vector3){-1.0f, -1.0f, -0.6f};
    UpdateLightView(world);

    SetShaderValueTexture(world->shadowShader, world->shadowMapLoc, world->shadowMap.texture);
    SetModelShader(&world->activeChunk->model, world->shadowShader);
}

void UpdateWorld(World* world) { (void)world; }

void RenderWorldShadowMap(World* world) {
    UpdateLightView(world);

    BeginTextureMode(world->shadowMap);
    ClearBackground(WHITE);
    BeginMode3D(world->lightCamera);

    SetModelShader(&world->activeChunk->model, world->shadowDepthShader);
    DrawModel(world->activeChunk->model,
              (Vector3){world->activeChunk->coord.x, 0.0f, world->activeChunk->coord.z}, 1.0f,
              WHITE);
    SetModelShader(&world->activeChunk->model, world->shadowShader);

    EndMode3D();
    EndTextureMode();
}

void DrawWorld(World* world) {
    SetShaderValueMatrix(world->shadowShader, world->shadowLightViewProjLoc, world->lightViewProj);
    SetShaderValue(world->shadowShader, world->shadowLightDirLoc, &world->lightDir,
                   SHADER_UNIFORM_VEC3);

    DrawGrid(10, 10);

    if (world->chunkCount > 0) {
        for (int i = 0; i < world->chunkCount; i++) {
            Chunk currentChunk = world->chunks[i];

            if (currentChunk.isActive) {
                for (int x = 0; x < currentChunk.collisionCount; x++) {
                    DrawBoundingBox(currentChunk.collisions[x].boundingBox, RED);
                }

                DrawModel(currentChunk.model,
                          (Vector3){currentChunk.coord.x, 0.0f, currentChunk.coord.z}, 1.0f, GRAY);
            }
        }
    }
}

void ShutdownWorld(World* world) {
    for (int i = 0; i < world->chunkCount; i++) {
        free(&world->chunks[i]);
    }
}
