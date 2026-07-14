#include "world.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assetLoader.h"
#include "raymath.h"
#define SHADOWMAP_SIZE 2048
#define LIGHT_DISTANCE 25.0f
#define LIGHT_ORTHO_SIZE 20.0f
#define SHOW_COLLISIONS true
#define CHUNK_WORLD_SIZE 100.0f

static void SetModelShader(Model* model, Shader shader) {
    for (int i = 0; i < model->materialCount; i++) {
        model->materials[i].shader = shader;
    }
}

static Vector3 GetChunkWorldPosition(const Chunk* chunk) {
    return (Vector3){chunk->coord.x * CHUNK_WORLD_SIZE, chunk->coord.y,
                     chunk->coord.z * CHUNK_WORLD_SIZE};
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

void InitWorld(World* world, ServerVec2i chunkCoord) {
    // printf("### Initializing World\n");
    world->chunkCount = 0;

    Chunk* chunk = &world->chunks[world->chunkCount];

    int i = LoadChunkByCoords(chunk, chunkCoord);
    if (i == 0) {
        printf("Active chunk cannot be loaded");
        return;
    }

    world->chunkCount = 1;
    world->activeChunk = chunk;

    ServerVec2i neighbours[8];
    int neighbourCount = GetNeighbourChunkCoords(chunkCoord, neighbours);

    for (int neighbourIndex = 0; neighbourIndex < neighbourCount; neighbourIndex++) {
        if (world->chunkCount >= MAX_CHUNKS) {
            break;
        }

        Chunk* neighbourChunk = &world->chunks[world->chunkCount];
        if (LoadChunkByCoords(neighbourChunk, neighbours[neighbourIndex])) {
            world->chunkCount++;
        }
    }

    world->activeChunk->collisionCount = 0;
    world->worldObjects = LoadChunkAssetsByCoords(chunkCoord, &world->worldObjectCount);

    printf("%d chunks loaded\n", world->chunkCount);
    printf("%d world object loaded\n", world->worldObjectCount);

    // SET Shaders

    world->shadowShader = LoadShader("../assets/shaders/shadow.vs", "../assets/shaders/shadow.fs");
    world->shadowDepthShader =
        LoadShader("../assets/shaders/shadow_depth.vs", "../assets/shaders/shadow_depth.fs");

    // outline shader
    world->outlineShader =
        LoadShader("../assets/shaders/outline.vs", "../assets/shaders/outline.fs");
    world->outlineShader.locs[SHADER_LOC_MATRIX_MVP] =
        GetShaderLocation(world->outlineShader, "mvp");
    world->outlineColorLoc = GetShaderLocation(world->outlineShader, "outlineColor");
    world->outlineSizeLoc = GetShaderLocation(world->outlineShader, "outlineSize");

    // emerald green
    Vector4 outlineColor = {255.0f / 255.0f, 51.0f / 255.0f, 0.0f / 255.0f, 1.0f};

    // shader layer above the hull
    float outlineSize = 0.006f;
    SetShaderValue(world->outlineShader, world->outlineColorLoc, &outlineColor, SHADER_UNIFORM_VEC4);
    SetShaderValue(world->outlineShader, world->outlineSizeLoc, &outlineSize, SHADER_UNIFORM_FLOAT);

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
    for (int chunkIndex = 0; chunkIndex < world->chunkCount; chunkIndex++) {
        SetModelShader(&world->chunks[chunkIndex].model, world->shadowShader);
    }
    if (world->worldObjects && world->worldObjectCount > 0) {
        for (int i = 0; i < world->worldObjectCount; i++) {
            if (world->worldObjects[i].model) {
                SetModelShader(world->worldObjects[i].model, world->shadowShader);
            }
        }
    }
}

void UpdateWorld(World* world) { (void)world; }

void RenderWorldShadowMap(World* world) {
    UpdateLightView(world);

    BeginTextureMode(world->shadowMap);
    ClearBackground(WHITE);
    BeginMode3D(world->lightCamera);

    for (int chunkIndex = 0; chunkIndex < world->chunkCount; chunkIndex++) {
        Chunk* chunk = &world->chunks[chunkIndex];
        SetModelShader(&chunk->model, world->shadowDepthShader);
        DrawModel(chunk->model, GetChunkWorldPosition(chunk), 1.0f, WHITE);
    }
    if (world->worldObjects && world->worldObjectCount > 0) {
        for (int i = 0; i < world->worldObjectCount; i++) {
            WorldObject obj = world->worldObjects[i];
            if (obj.model) {
                SetModelShader(obj.model, world->shadowDepthShader);
                DrawModel(*obj.model, obj.position, 1.0f, WHITE);
                SetModelShader(obj.model, world->shadowShader);
            }
        }
    }
    for (int chunkIndex = 0; chunkIndex < world->chunkCount; chunkIndex++) {
        SetModelShader(&world->chunks[chunkIndex].model, world->shadowShader);
    }

    EndMode3D();
    EndTextureMode();
}

void DrawWorld(World* world, Camera3D camera) {
    SetShaderValueMatrix(world->shadowShader, world->shadowLightViewProjLoc, world->lightViewProj);
    SetShaderValue(world->shadowShader, world->shadowLightDirLoc, &world->lightDir,
                   SHADER_UNIFORM_VEC3);

    DrawGrid(10, 10);

    Ray staticRay = GetScreenToWorldRay(GetMousePosition(), camera);

    for (int i = 0; i < world->worldObjectCount; i++) {
        WorldObject* obj = &world->worldObjects[i];
        obj->isMouseOver = false;

        if (obj->model == NULL || obj->model->meshCount <= 0) {
            continue;
        }

        for (int meshIndex = 0; meshIndex < obj->model->meshCount; meshIndex++) {
            RayCollision collision =
                GetRayCollisionMesh(staticRay, obj->model->meshes[meshIndex], obj->transform);

            if (collision.hit && obj->interactive) {
                obj->isMouseOver = true;
                break;
            }
        }
    }

    // draw terrain
    for (int chunkIndex = 0; chunkIndex < world->chunkCount; chunkIndex++) {
        Chunk* chunk = &world->chunks[chunkIndex];
        DrawModel(chunk->model, GetChunkWorldPosition(chunk), 1.0f, GRAY);
    }

    // draw all world objects
    for (int i = 0; i < world->worldObjectCount; i++) {
        WorldObject obj = world->worldObjects[i];

        switch (obj.type) {
            case COLLISION_TREE:
                if (SHOW_COLLISIONS) {
                    DrawBoundingBox(obj.boundingBox, RED);
                }
                break;
            default:

                if (obj.isMouseOver) {
                    SetModelShader(obj.model, world->outlineShader);
                    DrawModel(*obj.model, obj.position, 1.0f, WHITE);

                    SetModelShader(obj.model, world->shadowShader);
                    DrawModel(*obj.model, obj.position, 1.0f, WHITE);
                } else {
                    SetModelShader(obj.model, world->shadowShader);
                    DrawModel(*obj.model, obj.position, 1.0f, WHITE);
                }

                break;
        }
    }

    // world absolute origin
    DrawCylinder((Vector3){0}, 0.2f, 0.2f, 50.0f, 4, YELLOW);
}

void ShutdownWorld(World* world) {
    for (int i = 0; i < world->chunkCount; i++) {
        Chunk* chunk = &world->chunks[i];
        if (chunk->collisions != NULL) {
            free(chunk->collisions);
            chunk->collisions = NULL;
        }
        if (chunk->objects != NULL) {
            free(chunk->objects);
            chunk->objects = NULL;
        }
        if (chunk->model.meshCount > 0) {
            UnloadModel(chunk->model);
        }
    }
    if (world->shadowMap.texture.id != 0) {
        UnloadRenderTexture(world->shadowMap);
    }
    if (world->shadowShader.id != 0) {
        UnloadShader(world->shadowShader);
    }
    if (world->shadowDepthShader.id != 0) {
        UnloadShader(world->shadowDepthShader);
    }
    if (world->outlineShader.id != 0) {
        UnloadShader(world->outlineShader);
    }

    free(world->worldObjects);
}




