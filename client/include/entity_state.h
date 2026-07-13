#pragma once

#include "../../shared/player_state.h"
#include "assetLoader.h"
#include "raylib.h"

typedef struct RemotePlayer {
    PlayerNetState state;

    Asset* model;
    char* playerName;

    Vector3 previousPosition;
    Vector3 targetPosition;
    float interpolationTime;
} RemotePlayer;