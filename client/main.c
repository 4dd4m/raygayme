#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "cJSON.h"
#include "client.h"
#include "network.h"
#include "raylib.h"

void DrawConnectScreen(Client* client);
void UpdateConnectScreen(Client* client);
void DrawConnectingScreen(Client* lient);
void DrawConnectionFailedScreen(Client* client);
void UpdateClient(Client* client);
void DrawClient(Client* client);
void DrawGame(Client* client);
static Rectangle GetConnectButtonBounds(Client* client);

int main(void) {
    SetTraceLogLevel(LOG_ERROR);

    Client* client = CreateClient();
    if (client == NULL) {
        perror("Client cannot be created\n");
        exit(1);
    }
    InitClientWindow(client);

    while (!WindowShouldClose()) {
        UpdateClient(client);

        BeginDrawing();
        ClearBackground(BLACK);

        DrawClient(client);

        EndDrawing();
    }

    if (client->clientState == CLIENT_STATE_IN_GAME) {  // only chut down if they were initialized
        ShutdownWorld(&client->world);
    }

    Network_Shutdown();
    CloseWindow();
    DestroyClient(client);
    return 0;
}

void UpdateClient(Client* client) {
    switch (client->clientState) {
        case CLIENT_STATE_CONNECT_SCREEN:
            UpdateConnectScreen(client);
            break;

        case CLIENT_STATE_CONNECTING:
            // blocking call, can freeze on timeout
            bool isConnected = Network_Init("127.0.0.1", 9999);
            if (isConnected) {
                client->clientState = CLIENT_STATE_LOADING;
            } else {
                client->clientState = CLIENT_STATE_CONNECTION_FAILED;
            }
            break;

        case CLIENT_STATE_LOADING:
            InitPLayerWorldCamera(client);
            client->clientState = CLIENT_STATE_IN_GAME;
            break;

        case CLIENT_STATE_IN_GAME:
            UpdateMyCameraState(&client->Camera, &client->player);
            UpdateWorld(&client->world);
            UpdatePlayer(&client->player, &client->Camera, &client->world);
            break;

        default:
            break;
    }
}

void DrawClient(Client* client) {
    switch (client->clientState) {
        case CLIENT_STATE_CONNECT_SCREEN:
            DrawConnectScreen(client);
            break;

        case CLIENT_STATE_CONNECTING:
            DrawConnectingScreen(client);
            break;

        case CLIENT_STATE_CONNECTION_FAILED:
            DrawConnectionFailedScreen(client);
            break;

        case CLIENT_STATE_IN_GAME:
            DrawGame(client);

            break;

        default:
            break;
    }
}

void DrawGame(Client* client) {
    RenderWorldShadowMap(&client->world);

    BeginMode3D(client->Camera.Camera);
    DrawWorld(&client->world, client->Camera.Camera);
    DrawPlayer(&client->player, &client->world);

    EndMode3D();

    DebugCameraPosition(&client->Camera);
    DebugPlayerPosition(&client->player);
}

void DrawConnectScreen(Client* client) {
    Rectangle connectButton = GetConnectButtonBounds(client);

    Vector2 mouse = GetMousePosition();
    bool isHovered = CheckCollisionPointRec(mouse, connectButton);

    DrawText("Dummy Login:", 40, 40, 32, WHITE);
    DrawText("When clicked, it will connect to server", 40, 80, 32, WHITE);

    DrawRectangleRec(connectButton, isHovered ? GRAY : DARKGRAY);
    DrawRectangleLinesEx(connectButton, 2.0f, WHITE);

    DrawText("CONNECT", (int)(connectButton.x + 45), (int)(connectButton.y + 15), 24, WHITE);
}

void UpdateConnectScreen(Client* client) {
    Rectangle connectButton = GetConnectButtonBounds(client);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mouse = GetMousePosition();

        if (CheckCollisionPointRec(mouse, connectButton)) {
            client->clientState = CLIENT_STATE_CONNECTING;
        }
    }
}

void DrawConnectingScreen(Client* client) {  // no UpdateConnecting because this is a static screen
    int textOffset = 40;
    DrawText("Connecting...", (client->renderConfig->width / 2.0f) - textOffset,
             (client->renderConfig->height / 2.0f) - textOffset, 24, WHITE);
}

void DrawConnectionFailedScreen(Client* client) {
    DrawText("Connection failed", 40, 40, 32, RED);
    DrawText("Press ESC to close", 40, 80, 24, WHITE);
}

static Rectangle GetConnectButtonBounds(Client* client) {  // to adopt variable size window
    float button_width = 220.0f;
    float button_height = 56.0f;

    // center - cetner rectangle
    return (Rectangle){.x = client->renderConfig->width / 2.0f - button_width / 2.0f,
                       .y = client->renderConfig->height / 2.0f - button_height / 2.0f,
                       .width = button_width,
                       .height = button_height};
}