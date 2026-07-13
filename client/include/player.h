#pragma once
#include <raylib.h>

#include "world.h"

typedef struct Player {
    int id;
    Asset* model;
    char* playerName;
    Vector3 position;
    bool hasPosition;
    Vector3 targetLocation;
    bool hasTarget;
    Ray ray;
    float movementSpeed;
    bool drawCollisionBox;
    BoundingBox boundingBox;
    bool isColliding;
} Player;

typedef struct MyCamera MyCamera;

void InitPlayer(Player* player);

// moves the player (with)
void MovePlayerOnTerrain(Player* player);

void UpdatePlayer(Player* player, MyCamera* camera, World* world);
float GetHeightFromMesh(Mesh mesh, Matrix transform, float worldX, float worldZ);
void DebugPlayerPosition(Player* player);
void DrawPlayer(Player* player, World* world);
void UpdatePlayerPosition(Player* player, PlayerNetState* localPlayerNetState);
