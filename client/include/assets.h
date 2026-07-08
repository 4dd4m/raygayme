#pragma once
#include "raylib.h"

typedef struct {
    Model model;
    char* name;
} TreeAsset;

typedef struct {
    int id;
    TreeAsset* asset;
    Vector3 position;
    float rotation;
    int health;
    bool alive;
    double respawnTime;
} TreeInstance;

void LoadAssets();