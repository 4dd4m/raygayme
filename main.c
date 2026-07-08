#include <math.h>
#include "raylib.h"
#include "debug.h"
#include "game.h"

GameState state = STATE_GAME;

DebugConfig debug = {0}; 

int main(void) {
    const int screenWidth = 2500;
    const int screenHeight = 900;

    InitWindow(screenWidth, screenHeight, "Test Project");
    SetTargetFPS(165); 

    Game game;
    InitGame(&game);
    
    while (!WindowShouldClose())
    {
        UpdateMyCamera();
        UpdateGame(&game);

        BeginDrawing();
            ClearBackground(BLACK);
            DrawGame(&game);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}