#pragma once
#include <raylib.h>

#include "world.h"

typedef struct Player {
    int id;
    Asset* model;
    char* playerName;
    Vector3 position;
    Vector3 targetLocation;
    Ray ray;
    bool hasTarget;
    float movementSpeed;
    bool drawCollisionBox;
} Player;

typedef struct MyCamera MyCamera;

void InitPlayer(Player* player);

// moves the player (with)
void MovePlayer(Player* player);

void UpdatePlayer(Player* player, MyCamera* camera, World* world);
float GetHeightFromMesh(Mesh mesh, Matrix transform, float worldX, float worldZ);
void DebugPlayerPosition(Player* player);
