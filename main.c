#include <math.h>

#include "cJSON.h"
#include "client.h"
#include "raylib.h"

int main(void) {
    Client* client = CreateClient();

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        UpdateMyCameraState(&client->Camera, &client->player);
        UpdateWorld(&client->world);
        RenderWorldShadowMap(&client->world);

        BeginMode3D(client->Camera.Camera);
        DrawWorld(&client->world);
        UpdatePlayer(&client->player, &client->Camera, &client->world);

        EndMode3D();

        DebugCameraPosition(&client->Camera);
        DebugPlayerPosition(&client->player);

        EndDrawing();
    }

    CloseWindow();
    ShutdownWorld(&client->world);
    DestroyClient(client);
    return 0;
}
