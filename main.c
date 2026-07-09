#include <math.h>

#include "client.h"
#include "debug.h"
#include "raylib.h"

DebugConfig debug = {0};

int main(void) {
    Client client;
    InitClient(&client);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(client.Camera.Camera);
        UpdateMyCameraState(&client.Camera);
        UpdateWorld(&client.world);
        UpdatePlayer(&client.player, &client.Camera, &client.world);

        EndMode3D();

        DebugCameraPosition(&client.Camera);
        DebugPlayerPosition(&client.player);

        EndDrawing();
    }

    CloseWindow();
    ShutdownWorld(&client.world);
    return 0;
}