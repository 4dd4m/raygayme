#include "game.h"
#include <math.h>
#include <stdio.h>
#include "raylib.h"
#include "mycamera.h"
#include "rlgl.h"
#include "raymath.h"

#define NULLVECTOR (Vector3){0.0f, 0.0f, 0.0f} 

MyCamera camera;
Model Terrain;
Model CollisionModel;

typedef struct {
    BoundingBox box;
} CollisionObject;

CollisionObject collisions[100];
int collisionCount = 0;

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

float GetHeightFromMesh(Mesh mesh, Matrix transform, float worldX, float worldZ);


void InitGame(Game* g){
    
    InitCamera(&camera);

    g->CylinderPos = (Vector3){-2.5f, 0.0f, -2.5f};
    g->TargetLocation = NULLVECTOR;
    g->MovementSpeed = 3.0f;
    g->HasTarget = false;
    g->Ray = (Ray){0};

    Terrain = LoadModel("assets/terrain/terrain.glb");

    CollisionModel = LoadModel("assets/terrain/collision.glb");

    for(int i = 0; i < CollisionModel.meshCount; i++)
    {
        Mesh mesh = CollisionModel.meshes[i];

        collisions[collisionCount].box = GetMeshBoundingBox(mesh);

        collisionCount++;
    }

    printf("collisionCount: %d\n", collisionCount);

    SetWindowState(FLAG_MSAA_4X_HINT);
}

void UpdateGame(Game* g){
    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
        g->Ray = GetScreenToWorldRay(GetMousePosition(), camera.Camera);
    }

    if(fabsf(g->Ray.direction.y) > 0.0001f)
    {
        float t = -g->Ray.position.y / g->Ray.direction.y;

        g->TargetLocation = (Vector3){
            g->Ray.position.x + g->Ray.direction.x * t,
            0.0f,
            g->Ray.position.z + g->Ray.direction.z * t
        };

        g->HasTarget = true;
    }
    else{
        g->HasTarget = false;
    }
}

void DrawGame(Game* g){
    BeginMode3D(camera.Camera);
    //DrawGrid(50, 1.0f);

    // draw collision boxes

    for(int i = 0; i < CollisionModel.meshCount; i++)
    {
        DrawBoundingBox(GetMeshBoundingBox(CollisionModel.meshes[i]), RED);
    }

    if(g->HasTarget){

        // subtract vector but this has length info
        Vector3 direction = Vector3Subtract(
            g->TargetLocation,
            g->CylinderPos
        );

        float distance = Vector3Length(direction);


        if(distance > 0.01f)
        {
            direction = Vector3Normalize(direction);
            
            Vector3 nextPos = Vector3Add(g->CylinderPos, Vector3Scale(direction, g->MovementSpeed * GetFrameTime()));

            nextPos.y = GetHeightFromMesh(
                Terrain.meshes[0], 
                Terrain.transform,
                nextPos.x,
                nextPos.z
            );

            BoundingBox playerBox = {
                .min = {
                   nextPos.x - 0.5f,
                    nextPos.y,
                    nextPos.z - 0.5f
                },
                .max = {
                    nextPos.x + 0.5f,
                    nextPos.y + 1.7f,
                    nextPos.z + 0.5f
                }
            };

            DrawBoundingBox(playerBox, YELLOW);

            bool isCollided = false;

            for(int i = 0; i < collisionCount; i++)
            {
                if(CheckCollisionBoxes(playerBox, collisions[i].box))
                {
                    isCollided = true;
                }
            }

            if(!isCollided) {
                g->CylinderPos = nextPos;
                DrawCylinder(g->TargetLocation, 0.2f, 0.2f, 0.01f, 32, RED);
            }
        }
    }

    DrawCylinder(g->CylinderPos, 0.5f, 0.5f, 1.7f, 32, RED);

    DrawModelWires(Terrain, (Vector3){0}, 1.0f, GRAY);

    
    // render red cylinder on click
    if(fabsf(g->Ray.direction.y) > 0.0001f){

        float t = -g->Ray.position.y / g->Ray.direction.y;

        Vector3 hit = {
            g->Ray.position.x + g->Ray.direction.x * t,
            0.0f,
            g->Ray.position.z + g->Ray.direction.z * t
        };

        g->TargetLocation = hit;
        g->HasTarget = true;
    }

    

    EndMode3D();
}

void UpdateMyCamera(){
    UpdateMyCameraState(&camera);
}


float GetHeightFromMesh(Mesh mesh, Matrix transform, float worldX, float worldZ){
    Matrix inverseMatrix = MatrixInvert(transform);
    // world to local transform
    Vector3 local = Vector3Transform((Vector3){worldX, 0, worldZ}, inverseMatrix);

    Ray ray = {.position = {worldX, 1000, worldZ}, .direction = {0,-1,0}};

    RayCollision collision = GetRayCollisionMesh(ray, mesh, transform);

    return collision.hit ? collision.point.y : 0.0f;
}