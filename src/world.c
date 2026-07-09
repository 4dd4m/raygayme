#include "world.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void InitWorld(World* world) {
    printf("### Initializing World\n");
    world->chunkCount = 0;

    // load starting chunk
    Chunk* chunk = &world->chunks[0];
    chunk->coord.x = 0;
    chunk->coord.z = 0;
    chunk->isActive = true;

    chunk->model = LoadModel("assets/terrain/terrain.obj");

    world->chunkCount = 1;

    world->activeChunk = chunk;

    world->activeChunk->collisionCount++;

    chunk->collisions = malloc(sizeof(CollisionObject) * world->activeChunk->collisionCount);

    Model initialCollision = LoadModel("assets/terrain/collision.glb");

    chunk->collisions[0] =
        (CollisionObject){initialCollision.meshes[0], GetMeshBoundingBox(initialCollision.meshes[0]),
                          initialCollision.transform};
}

void UpdateWorld(World* world) {
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