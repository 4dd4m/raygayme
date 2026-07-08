#include "mycamera.h"
#include "raylib.h"

typedef enum GameState{
    STATE_MENU = 0,
    STATE_GAME = 1,
    STATE_END = 2
} GameState;

typedef struct Game{
    Camera Camera;
    Vector3 CylinderPos;
    Vector3 TargetLocation;
    float MovementSpeed;
    bool HasTarget;
    Ray Ray;
} Game;

void InitGame(Game* g);
void UpdateGame(Game* g);
void DrawGame(Game* g);
void UpdateMyCamera();