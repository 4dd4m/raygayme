#pragma once
#include <player_state.h>
#include <stdbool.h>
#include <winsock2.h>

#include "terrainAssetLoader.h"

#define MAX_PLAYERS 10

typedef struct ServerPlayer {
    PlayerNetState playerNetState;
    SOCKET socket;
    bool isConnected;
    double lastPacketTime;
    double lastInputTime;
    ServerVec2i chunkCoord;  // the chunk's coord the player stands on {0,0}
    int chunkIndex;          // index for terrainDat->chunks, if none: -1
} ServerPlayer;

typedef struct WorldState {
    const ServerTerrainData* terrainData;
    ServerPlayer players[MAX_PLAYERS];
    float serverDeltaTime;
    float tickPerMs;
} WorldState;

void set_up_non_blocking_socket();
SOCKET accept_new_connection();

void closeConnection();
void ioctl_socket();
