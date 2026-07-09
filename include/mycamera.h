#pragma once
#include "player.h"
#include "raylib.h"

typedef struct MyCamera {
    Camera3D Camera;
    float RotationSpeed;
    float ZoomSpeed;
    float MinZoom;
    float MaxZoom;

    Vector3 offset;

    float Radius;
    float Yaw;
    float Pitch;
} MyCamera;

typedef struct Player Player;

void InitCamera(MyCamera* camera, Player* player);
void UpdateMyCameraState(MyCamera* camera, Player* player);
void CalculateCameraOffset(MyCamera* camera);
void DebugCameraPosition(MyCamera* camera);
