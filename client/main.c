#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "cJSON.h"
#include "client.h"
#include "network.h"
#include "raylib.h"

void DrawConnectScreen(Client* client);
void UpdateConnectScreen(Client* client);
void StartConnection(Client* client);
void DrawConnectingScreen(Client* lient);
void DrawStatusScreen(Client* client, const char* message);
void DrawConnectionFailedScreen(Client* client);
void UpdateClient(Client* client);
void DrawClient(Client* client);
void DrawGame(Client* client);
static Rectangle GetConnectButtonBounds(Client* client);

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    SetTraceLogLevel(LOG_ERROR);
    fprintf(stderr, "[Client]: Starting build %s %s\n", __DATE__, __TIME__);

    Client* client = CreateClient();
    if (client == NULL) {
        perror("Client cannot be created\n");
        exit(1);
    }
    InitClientWindow(client);
    fprintf(stderr, "[Client]: Window initialized\n");
    fprintf(stderr, "[Client]: Screen size reported by raylib: %dx%d\n", GetScreenWidth(),
            GetScreenHeight());

    while (!WindowShouldClose()) {
        UpdateClient(client);

        BeginDrawing();
        ClearBackground((Color){24, 28, 35, 255});

        DrawClient(client);

        DrawFPS(GetScreenWidth() - 100, 10);

        EndDrawing();
    }

    if (client->clientState == CLIENT_STATE_IN_GAME) {  // only chut down if they were initialized
        ShutdownWorld(&client->world);
    }

    Network_Shutdown();
    CloseWindow();
    DestroyClient(client);
    fprintf(stderr, "[Client]: Shutdown complete\n");
    return 0;
}

void UpdateClient(Client* client) {
    static double welcomeWaitStartedAt = 0.0;

    switch (client->clientState) {
        case CLIENT_STATE_CONNECT_SCREEN:
            welcomeWaitStartedAt = 0.0;
            UpdateConnectScreen(client);
            break;

        case CLIENT_STATE_CONNECTING:
            // blocking call, can freeze on timeout
            fprintf(stderr, "[Client]: Connecting to server\n");
            bool isConnected = Network_Init("127.0.0.1", 9999);
            if (isConnected) {
                welcomeWaitStartedAt = GetTime();
                fprintf(stderr, "[Client]: Connected, initializing player\n");
                client->clientState = CLIENT_SATE_INIT_PLAYER;
            } else {
                client->connectionError = "Could not connect to server";
                fprintf(stderr, "[Client]: Connection failed\n");
                client->clientState = CLIENT_STATE_CONNECTION_FAILED;
            }
            break;

        case CLIENT_SATE_INIT_PLAYER:
            fprintf(stderr, "[Client]: Initializing player\n");
            InitPlayer(&client->player);
            fprintf(stderr, "[Client]: Waiting for server welcome\n");
            client->clientState = CLIENT_STATE_WAITING_FOR_WELCOME;
            break;

        case CLIENT_STATE_WAITING_FOR_WELCOME:
            Network_ReceiveData(client->world.players, &client->localPlayerNetState);
            if (client->localPlayerNetState.isConnected) {
                welcomeWaitStartedAt = 0.0;
                fprintf(stderr, "[Client]: Server welcome received\n");
                UpdatePlayerPosition(&client->player, &client->localPlayerNetState);
                client->clientState = CLIENT_STATE_LOADING;
            } else if (!Network_IsConnected()) {
                client->connectionError = "Disconnected while waiting for server welcome";
                fprintf(stderr, "[Client]: Disconnected while waiting for welcome\n");
                client->clientState = CLIENT_STATE_CONNECTION_FAILED;
            } else if (welcomeWaitStartedAt > 0.0 && GetTime() - welcomeWaitStartedAt > 5.0) {
                client->connectionError = "Timed out waiting for server welcome";
                fprintf(stderr, "[Client]: Timed out waiting for server welcome\n");
                Network_Shutdown();
                client->clientState = CLIENT_STATE_CONNECTION_FAILED;
            }
            break;

        case CLIENT_STATE_LOADING:
            fprintf(stderr, "[Client]: Loading world\n");
            InitPLayerWorldCamera(client);
            fprintf(stderr, "[Client]: Entering game\n");
            client->clientState = CLIENT_STATE_IN_GAME;
            break;

        case CLIENT_STATE_IN_GAME:
            Network_ReceiveData(client->world.players, &client->localPlayerNetState);
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

        case CLIENT_SATE_INIT_PLAYER:
            DrawStatusScreen(client, "Initializing player...");
            break;

        case CLIENT_STATE_WAITING_FOR_WELCOME:
            DrawStatusScreen(client, "Waiting for server welcome...");
            break;

        case CLIENT_STATE_LOADING:
            DrawStatusScreen(client, "Loading world...");
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
    static bool logged = false;
    if (!logged) {
        fprintf(stderr, "[Client]: Showing connect screen\n");
        fprintf(stderr, "[Client]: Connect screen size: %dx%d\n", GetScreenWidth(),
                GetScreenHeight());
        logged = true;
    }

    Rectangle connectButton = GetConnectButtonBounds(client);

    Vector2 mouse = GetMousePosition();
    bool isHovered = CheckCollisionPointRec(mouse, connectButton);

    DrawRectangle(0, 0, GetScreenWidth(), 110, (Color){46, 55, 70, 255});
    DrawRectangle(28, 30, 18, 18, GREEN);
    DrawText("Dummy Login", 60, 22, 32, RAYWHITE);
    DrawText("Click CONNECT or press Enter/Space", 60, 62, 22, RAYWHITE);

    DrawRectangleRec(connectButton, isHovered ? (Color){88, 100, 118, 255}
                                              : (Color){64, 75, 92, 255});
    DrawRectangleLinesEx(connectButton, 3.0f, RAYWHITE);

    DrawText("CONNECT", (int)(connectButton.x + 45), (int)(connectButton.y + 15), 24, RAYWHITE);
}

void UpdateConnectScreen(Client* client) {
    Rectangle connectButton = GetConnectButtonBounds(client);

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        fprintf(stderr, "[Client]: Connect requested by keyboard\n");
        StartConnection(client);
        return;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mouse = GetMousePosition();
        fprintf(stderr, "[Client]: Mouse click at %.0f,%.0f\n", mouse.x, mouse.y);

        if (CheckCollisionPointRec(mouse, connectButton)) {
            fprintf(stderr, "[Client]: Connect button clicked\n");
            StartConnection(client);
        }
    }
}

void StartConnection(Client* client) { client->clientState = CLIENT_STATE_CONNECTING; }

void DrawConnectingScreen(Client* client) {  // no UpdateConnecting because this is a static screen
    DrawStatusScreen(client, "Connecting...");
}

void DrawStatusScreen(Client* client, const char* message) {
    int textWidth = MeasureText(message, 24);
    DrawText(message, (GetScreenWidth() - textWidth) / 2, (GetScreenHeight() / 2) - 12, 24,
             WHITE);
}

void DrawConnectionFailedScreen(Client* client) {
    DrawText("Connection failed", 40, 40, 32, RED);
    if (client->connectionError != NULL) {
        DrawText(client->connectionError, 40, 80, 24, WHITE);
        DrawText("Press ESC to close", 40, 120, 24, WHITE);
    } else {
        DrawText("Press ESC to close", 40, 80, 24, WHITE);
    }
}

static Rectangle GetConnectButtonBounds(Client* client) {  // to adopt variable size window
    float button_width = 220.0f;
    float button_height = 56.0f;

    return (Rectangle){.x = 60.0f,
                       .y = 145.0f,
                       .width = button_width,
                       .height = button_height};
}
