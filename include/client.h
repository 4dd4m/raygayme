#pragma once
#include "MyCamera.h"
#include "player.h"
#include "raylib.h"
#include "world.h"

#define MAX_OBJECTS 1000
#define MAX_COLLISIONS 1000

typedef struct RenderConfig {
    int width;
    int height;
    const char* title;
    int targetFps;
} RenderConfig;

typedef struct Terrain {
    Model model;
} Terrain;

typedef struct Client {
    World world;
    Player player;
    MyCamera Camera;

    RenderConfig renderConfig;
} Client;

void InitClient(Client* client);
void UpdateClient(Client* client);
void DrawClient(Client* client);
void ShutdownClient(Client* client);