#pragma once
#include "mycamera.h"
#include "player.h"
#include "raylib.h"
#include "world.h"

#define MAX_OBJECTS 1000
#define MAX_COLLISIONS 1000

typedef struct RenderConfig RenderConfig;

typedef struct Client {
    World world;
    Player player;
    MyCamera Camera;
    RenderConfig* renderConfig;
} Client;

Client* CreateClient(void);

void InitClient(Client* client);
void UpdateClient(Client* client);
void DrawClient(Client* client);
void ShutdownClient(Client* client);
void DestroyClient(Client* client);
