#include "mycamera.h"

#include <player.h>
#include <stdio.h>

#include "raylib.h"
#include "raymath.h"

char posBuffer[128];
char zoomBuffer[128];

void InitCamera(MyCamera* camera, Player* player) {
    printf("### InitCamera\n");
    camera->Camera.position = (Vector3){-20.4f, 35.87f, 74.83f};
    camera->Camera.target = player->position;
    camera->Camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera->Camera.fovy = 45.0f;
    camera->Camera.projection = CAMERA_PERSPECTIVE;
    camera->RotationSpeed = 0.01f;
    camera->ZoomSpeed = 1.0f;

    camera->Yaw = 0.0f;    // horizontal movement
    camera->Pitch = 0.3f;  // tilt/down/up movement
    camera->Radius = 30.0f;
    camera->MinZoom = 3.0f;
    camera->MaxZoom = 50.0f;

    CalculateCameraOffset(camera);
}

void UpdateMyCameraState(MyCamera* camera, Player* player) {
    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
        Vector2 delta = GetMouseDelta();

        camera->Yaw += delta.x * camera->RotationSpeed;
        camera->Pitch += delta.y * camera->RotationSpeed;
        camera->Pitch = Clamp(camera->Pitch, 0.0f, 1.55f);

        CalculateCameraOffset(camera);
    }

    camera->Camera.target = player->position;

    if (GetMouseWheelMove()) {
        Vector2 wheelMovement = GetMouseWheelMoveV();

        if (wheelMovement.y < 0.0f) {
            camera->Radius =
                Clamp(camera->Radius + camera->ZoomSpeed, camera->MinZoom, camera->MaxZoom);
            CalculateCameraOffset(camera);
        }

        if (wheelMovement.y > 0.0f) {
            camera->Radius =
                Clamp(camera->Radius - camera->ZoomSpeed, camera->MinZoom, camera->MaxZoom);
            CalculateCameraOffset(camera);
        }
    }
    camera->Camera.position = Vector3Add(camera->Camera.target, camera->offset);
}

void CalculateCameraOffset(MyCamera* camera) {
    camera->offset.x = camera->Radius * cosf(camera->Pitch) * sinf(camera->Yaw);
    camera->offset.y = camera->Radius * sinf(camera->Pitch);
    camera->offset.z = camera->Radius * cosf(camera->Pitch) * cosf(camera->Yaw);
}

void DebugCameraPosition(MyCamera* camera) {  // must be NOT called in BeginMode3D environment!
    sprintf(
        posBuffer,
        "Cam Pos: X: %.2f Y: %.2f Z:%.2f\nCam Off Ox: %.2f Oy: %.2f Oz:%.2f, Yaw: %.2f, Pitch: %.2f",
        camera->Camera.position.x, camera->Camera.position.y, camera->Camera.position.z,
        camera->offset.x, camera->offset.y, camera->offset.z, camera->Yaw, camera->Pitch);
    DrawText(posBuffer, 0, 0, 10, WHITE);
}