#pragma once
#include "mycamera.h"
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

typedef enum ClientState {
    CLIENT_STATE_CONNECT_SCREEN,
    CLIENT_STATE_CONNECTING,
    CLIENT_STATE_LOADING,
    CLIENT_STATE_IN_GAME,
    CLIENT_STATE_CONNECTION_FAILED
} ClientState;

typedef struct Client {
    World world;
    Player player;
    MyCamera Camera;
    RenderConfig* renderConfig;
    ClientState clientState;
    const char* connectionError;
} Client;

Client* CreateClient(void);

void InitPLayerWorldCamera(Client* client);
void InitClientWindow(Client* client);
void UpdateClient(Client* client);
void DrawClient(Client* client);
void ShutdownClient(Client* client);
void DestroyClient(Client* client);
