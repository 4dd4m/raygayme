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

        UpdateMyCameraState(&client.Camera);
        UpdateWorld(&client.world);
        RenderWorldShadowMap(&client.world);

        BeginMode3D(client.Camera.Camera);
        DrawWorld(&client.world);
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
