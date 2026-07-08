#pragma once
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

void InitCamera(MyCamera* camera);
void UpdateMyCameraState(MyCamera* camera);
void CalculateCameraOffset(MyCamera* camera);