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
#define SHOW_HEIGHT_GRID_DEBUG false
#define SHOW_HEIGHT_GRID_MESH_DIFF_DEBUG false
#define CHUNK_WORLD_SIZE 100.0f

static void SetModelShader(Model* model, Shader shader) {
    for (int i = 0; i < model->materialCount; i++) {
        model->materials[i].shader = shader;
    }
}

Matrix GetWorldObjectTransform(const WorldObject* obj) {
    Matrix scale = MatrixScale(obj->scale.x, obj->scale.y, obj->scale.z);
    Matrix rotation = MatrixRotateY(obj->rotation.y);
    Matrix translation = MatrixTranslate(obj->position.x, obj->position.y, obj->position.z);
    Matrix objectTransform = MatrixMultiply(MatrixMultiply(scale, rotation), translation);

    return MatrixMultiply(obj->model->transform, objectTransform);
}

void DrawWorldObjectModel(const WorldObject* obj, Color tint) {
    DrawModelEx(*obj->model, obj->position, (Vector3){0.0f, 1.0f, 0.0f}, obj->rotation.y * RAD2DEG, obj->scale, tint);
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
    Matrix lightProj =
        MatrixOrtho(-LIGHT_ORTHO_SIZE, LIGHT_ORTHO_SIZE, -LIGHT_ORTHO_SIZE, LIGHT_ORTHO_SIZE, 0.1f, 100.0f);
    world->lightViewProj = MatrixMultiply(lightProj, lightView);
}

void InitWorld(World* world, ServerVec2i chunkCoord) {
    // printf("### Initializing World\n");
    world->chunkCount = 0;

    GetInitialTerrainData();

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
    world->shadowDepthShader = LoadShader("../assets/shaders/shadow_depth.vs", "../assets/shaders/shadow_depth.fs");

    // outline shader
    world->outlineShader = LoadShader("../assets/shaders/outline.vs", "../assets/shaders/outline.fs");
    world->outlineShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(world->outlineShader, "mvp");
    world->outlineColorLoc = GetShaderLocation(world->outlineShader, "outlineColor");
    world->outlineSizeLoc = GetShaderLocation(world->outlineShader, "outlineSize");

    // emerald green
    Vector4 outlineColor = {255.0f / 255.0f, 51.0f / 255.0f, 0.0f / 255.0f, 1.0f};

    // shader layer above the hull
    float outlineSize = 0.006f;
    SetShaderValue(world->outlineShader, world->outlineColorLoc, &outlineColor, SHADER_UNIFORM_VEC4);
    SetShaderValue(world->outlineShader, world->outlineSizeLoc, &outlineSize, SHADER_UNIFORM_FLOAT);

    world->shadowShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(world->shadowShader, "mvp");
    world->shadowShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(world->shadowShader, "matModel");
    world->shadowShader.locs[SHADER_LOC_MAP_DIFFUSE] = GetShaderLocation(world->shadowShader, "texture0");
    world->shadowShader.locs[SHADER_LOC_COLOR_DIFFUSE] = GetShaderLocation(world->shadowShader, "colDiffuse");
    world->shadowLightViewProjLoc = GetShaderLocation(world->shadowShader, "lightViewProj");
    world->shadowLightDirLoc = GetShaderLocation(world->shadowShader, "lightDir");
    world->shadowMapLoc = GetShaderLocation(world->shadowShader, "shadowMap");

    world->shadowDepthShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(world->shadowDepthShader, "mvp");

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

    //
    // Shadow pass on chunks
    //
    for (int chunkIndex = 0; chunkIndex < world->chunkCount; chunkIndex++) {
        Chunk* chunk = &world->chunks[chunkIndex];
        SetModelShader(&chunk->model, world->shadowDepthShader);
        DrawModel(chunk->model, (Vector3){0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
    }
    if (world->worldObjects && world->worldObjectCount > 0) {
        for (int i = 0; i < world->worldObjectCount; i++) {
            WorldObject obj = world->worldObjects[i];
            if (obj.model) {
                SetModelShader(obj.model, world->shadowDepthShader);
                DrawWorldObjectModel(&obj, WHITE);
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
    SetShaderValue(world->shadowShader, world->shadowLightDirLoc, &world->lightDir, SHADER_UNIFORM_VEC3);

    DrawGrid(20, 5);

    Ray staticRay = GetScreenToWorldRay(GetMousePosition(), camera);

    //
    // Set mousover on interactive objects
    //
    for (int i = 0; i < world->worldObjectCount; i++) {
        //
        // Bug: Overlapped interactive object. Raycast should choose the closer one.
        //
        WorldObject* obj = &world->worldObjects[i];
        obj->isMouseOver = false;

        if (obj->model == NULL || obj->model->meshCount <= 0) {
            continue;
        }

        for (int meshIndex = 0; meshIndex < obj->model->meshCount; meshIndex++) {
            Matrix transform = GetWorldObjectTransform(obj);
            RayCollision collision = GetRayCollisionMesh(staticRay, obj->model->meshes[meshIndex], transform);

            if (collision.hit && obj->interactive) {
                obj->isMouseOver = true;
                break;
            }
        }
    }

    //
    // Draw Chunks
    //
    for (int chunkIndex = 0; chunkIndex < world->chunkCount; chunkIndex++) {
        Chunk* chunk = &world->chunks[chunkIndex];
        DrawModel(chunk->model, (Vector3){0.0f, 0.0f, 0.0f}, 1.0f, GRAY);
    }

    //
    // draw all world objects
    //
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
                DrawWorldObjectModel(&obj, WHITE);

                SetModelShader(obj.model, world->shadowShader);
                DrawWorldObjectModel(&obj, WHITE);
            } else {
                SetModelShader(obj.model, world->shadowShader);
                DrawWorldObjectModel(&obj, WHITE);
            }

            break;
        }
    }

    //
    // Draw actual height grid
    //

    if (SHOW_HEIGHT_GRID_DEBUG) {
        ServerVec3* heights = GetHeightMapGridPointsByChunIndex(0);
        if (heights != NULL) {
            const ServerTerrainData* terrainData = GetServerTerrainData();
            int heightCount = 0;
            if (terrainData != NULL && terrainData->chunkCount > 0) {
                heightCount = terrainData->chunks[0].heightCount;
            }

            for (int i = 0; i < heightCount; i++) {

                Vector3 centerpos = {
                    .x = heights[i].x,
                    .y = heights[i].y,
                    .z = heights[i].z,
                };

                DrawSphereEx(centerpos, 0.06f, 2, 2, RED);

                if (SHOW_HEIGHT_GRID_MESH_DIFF_DEBUG && i % 25 == 0) {
                    Ray ray = {
                        .position = {centerpos.x, 1000.0f, centerpos.z},
                        .direction = {0.0f, -1.0f, 0.0f},
                    };
                    RayCollision closestHit = {0};

                    for (int chunkIndex = 0; chunkIndex < world->chunkCount; chunkIndex++) {
                        Chunk* chunk = &world->chunks[chunkIndex];
                        RayCollision hit = GetRayCollisionMesh(ray, chunk->model.meshes[0], chunk->model.transform);
                        if (hit.hit && (!closestHit.hit || hit.distance < closestHit.distance)) {
                            closestHit = hit;
                        }
                    }

                    if (closestHit.hit && fabsf(closestHit.point.y - centerpos.y) > 0.05f) {
                        DrawLine3D(centerpos, closestHit.point, SKYBLUE);
                        DrawSphereEx(closestHit.point, 0.08f, 4, 4, SKYBLUE);
                    }
                }
            }
            free(heights);
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
