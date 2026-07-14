#include "player.h"

#include <mycamera.h>
#include <stddef.h>
#include <stdio.h>

#include "network.h"
#include "raymath.h"

char playerBuffer[128];

void InitPlayer(Player* player) {
    player->id = 0;
    player->position = (Vector3){40.0f, 0.0f, 10.0f};
    player->hasPosition = true;
    player->ray = (Ray){0};
    player->playerName = "Adam";
    player->model = NULL;
    player->drawCollisionBox = true;
    player->movementSpeed = 3.0f;
}

void UpdatePlayer(Player* player, MyCamera* camera, World* world) {
    bool isInteractiveClicked = false;

    player->boundingBox = (BoundingBox){
        .min = {player->position.x - 0.5f, player->position.y, player->position.z - 0.5f},
        .max = {player->position.x + 0.5f, player->position.y + 1.7f, player->position.z + 0.5f}};

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        player->ray = GetScreenToWorldRay(GetMousePosition(), camera->Camera);

        // if clicked and the ray has > 0.f
        if (fabsf(player->ray.direction.y) > 0.0001f) {
            for (int i = 0; i < world->worldObjectCount; i++) {
                WorldObject* obj = &world->worldObjects[i];

                if (!obj->interactive) {
                    continue;
                }

                if (obj->model == NULL || obj->model->meshCount <= 0) {
                    continue;
                }

                RayCollision interactiveRayCollision =
                    GetRayCollisionMesh(player->ray, obj->model->meshes[0], obj->transform);

                if (interactiveRayCollision.hit) {
                    isInteractiveClicked = true;
                    player->hasTarget = true;
                    player->targetLocation = world->worldObjects[i].position;
                    Network_SendMovement(
                        player->id, (NetVec3){player->targetLocation.x, player->targetLocation.y,
                                              player->targetLocation.z});
                    break;
                }
            }

            if (!isInteractiveClicked) {  // raycost to terrain
                RayCollision hit =
                    GetRayCollisionMesh(player->ray, world->activeChunk->model.meshes[0],
                                        world->activeChunk->model.transform);

                if (hit.hit) {
                    player->targetLocation = hit.point;
                    player->hasTarget = true;
                    Network_SendMovement(
                        player->id, (NetVec3){player->targetLocation.x, player->targetLocation.y,
                                              player->targetLocation.z});
                } else {
                    player->hasTarget = false;
                    player->targetLocation = (Vector3){0};
                }
            }

        } else {
            player->hasTarget = false;
        }
    }

    if (player->hasTarget) {
        // subtract vector but this has length info
        Vector3 direction = Vector3Subtract(player->targetLocation, player->position);

        float distance = Vector3Length(direction);

        if (distance > 0.01f) {
            direction = Vector3Normalize(direction);

            Vector3 nextPos = Vector3Add(
                player->position, Vector3Scale(direction, player->movementSpeed * GetFrameTime()));

            // get world active chunk mesh instance

            nextPos.y = GetHeightFromMesh(world->activeChunk->model.meshes[0],
                                          world->activeChunk->model.transform, nextPos.x, nextPos.z);

            player->isColliding = false;

            // check collision agains world objects
            for (int i = 0; i < world->worldObjectCount; i++) {
                switch (world->worldObjects[i].type) {
                    case COLLISION_TREE:
                        if (CheckCollisionBoxes(player->boundingBox,
                                                world->worldObjects[i].boundingBox)) {
                            player->isColliding = true;
                            break;
                        }
                        break;
                }
            }

            if (!player->isColliding) {
                // player->position = nextPos;
            }
        }
    }
}

void DrawPlayer(Player* player, World* world) {
    // this is player. always visible
    if (player->hasPosition) {
        DrawCylinder(player->position, 0.5f, 0.5f, 1.7f, 32, RED);
    }

    if (player->hasTarget && !player->isColliding) {
        DrawCylinder(player->targetLocation, 0.2f, 0.2f, 0.1f, 32, BLUE);
    }

    if (player->drawCollisionBox &&
        (player->boundingBox.max.x != 0.0f && player->boundingBox.max.y != 0.0f &&
         player->boundingBox.max.z != 0.0f && player->boundingBox.min.z != 0.0f &&
         player->boundingBox.min.z != 0.0f && player->boundingBox.min.z != 0.0f)) {
        DrawBoundingBox(player->boundingBox, YELLOW);
    }
}

void UpdatePlayerPosition(Player* player, PlayerNetState* localPlayerNetState) {
    player->position.x = localPlayerNetState->position.x;
    player->position.y = localPlayerNetState->position.y;
    player->position.z = localPlayerNetState->position.z;
    player->chunkCoord.x = localPlayerNetState->chunkCoord.x;
    player->chunkCoord.z = localPlayerNetState->chunkCoord.z;
}

void MovePlayerOnTerrain(Player* player) {
    if (fabsf(player->ray.direction.y) > 0.0001f) {
        float t = -player->ray.position.y / player->ray.direction.y;

        Vector3 hit = {player->ray.position.x + player->ray.direction.x * t, 0.0f,
                       player->ray.position.z + player->ray.direction.z * t};

        player->targetLocation = hit;
        player->hasTarget = true;
    }
}

float GetHeightFromMesh(Mesh mesh, Matrix transform, float worldX, float worldZ) {
    // gets the mesh height from a mesh in a given worldX and worldZ coordinates

    // Matrix inverseMatrix = MatrixInvert(transform);
    //  with the inverse matrix we "translate" the world coordinates back to the mesh's local
    //  coordinates
    // Vector3 local = Vector3Transform((Vector3){worldX, 0, worldZ}, inverseMatrix);

    Ray ray = {.position = {worldX, 1000, worldZ}, .direction = {0, -1, 0}};

    RayCollision collision = GetRayCollisionMesh(ray, mesh, transform);

    if (collision.hit) {
        return collision.point.y;
    } else {
        return 0.0f;
    }

    return collision.hit ? collision.point.y : 0.0f;
}

void DebugPlayerPosition(Player* player) {
    sprintf(playerBuffer, "Player POS: X: %.2f Y: %.2f Z:%.2f\n", player->position.x,
            player->position.y, player->position.z);
    DrawText(playerBuffer, 0, 25, 10, WHITE);
}
