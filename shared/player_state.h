#pragma once
#include <stddef.h>
#include <stdbool.h>

typedef struct NetVec3 {
    float x;
    float y;
    float z;
} NetVec3;

typedef enum PlayerMoveState {
    PLAYER_MOVE_IDLE,
    PLAYER_MOVE_WALKING,
    PLAYER_MOVE_RUNNING
} PlayerMoveState;

typedef struct PlayerNetState {
    int id;
    NetVec3 position;
    NetVec3 velocity;
    float yaw;
    PlayerMoveState moveState;
    bool isConnected;
} PlayerNetState;