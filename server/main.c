#include <stdio.h>
#include <winsock2.h>
//
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "player_state.h"
#include "server.h"
#include "serverAssetLoader.h"

WorldState worldState = {0};

double GetServerTimeSeconds(void);
void UpdatePlayerPosition(PlayerNetState* netState, float delta);
void load_server_terrain_data();

int n = 0;

int main() {
    load_server_terrain_data();
    double lastTime = GetServerTimeSeconds();
    worldState.serverDeltaTime = 0.0f;
    worldState.tickPerMs = 33;

    setvbuf(stdout, NULL, _IONBF, 0);

    set_up_non_blocking_socket();

    while (1) {
        double now = GetServerTimeSeconds();
        worldState.serverDeltaTime = now - lastTime;
        lastTime = now;

        DWORD start_time = GetTickCount();
        SOCKET new_socket = accept_new_connection();

        if (new_socket != INVALID_SOCKET) {
            bool slot_found = false;

            ioctl_socket();

            for (size_t i = 0; i < MAX_PLAYERS; i++) {
                if (!worldState.players[i].isConnected) {
                    slot_found = true;
                    worldState.players[i].socket = new_socket;
                    worldState.players[i].playerNetState.id = i;
                    worldState.players[i].playerNetState.position = (NetVec3){0, 0, -17.38};
                    worldState.players[i].playerNetState.velocity = 0;
                    worldState.players[i].playerNetState.moveState = PLAYER_MOVE_IDLE;
                    worldState.players[i].playerNetState.yaw = 0.0f;
                    worldState.players[i].playerNetState.hasTarget = 0.0f;
                    worldState.players[i].isConnected = true;
                    worldState.players[i].lastPacketTime = GetServerTimeSeconds();
                    worldState.players[i].lastInputTime = GetServerTimeSeconds();
                    worldState.players[i].chunkCoord = (ServerVec2i){0, 0};
                    worldState.players[i].chunkIndex = 0;

                    printf("%d: Player has been connected! ID: %lld\n", ++n, i);

                    char welcomeBuffer[100];
                    sprintf(welcomeBuffer, "WELCOME|%llu|%f,%f,%f\n", (unsigned long long)i,
                            worldState.players[i].playerNetState.position.x,
                            worldState.players[i].playerNetState.position.y,
                            worldState.players[i].playerNetState.position.z);

                    printf("%s", welcomeBuffer);
                    send(new_socket, welcomeBuffer, strlen(welcomeBuffer), 0);
                    break;
                }
            }

            if (!slot_found) {
                printf("%d: Server is full\n", n);
                closesocket(new_socket);
            }
        }

        for (size_t i = 0; i < MAX_PLAYERS; i++) {
            if (!worldState.players[i].isConnected) continue;

            char buffer[1024];
            int valread = recv(worldState.players[i].socket, buffer, sizeof(buffer) - 1, 0);

            if (valread > 0) {
                buffer[valread] = '\0';
                if (sscanf(buffer, "MOVE|%d|%f,%f,%f", &worldState.players[i].playerNetState.id,
                           &worldState.players[i].playerNetState.targetPosition.x,
                           &worldState.players[i].playerNetState.targetPosition.y,
                           &worldState.players[i].playerNetState.targetPosition.z) == 4) {
                    printf("%d: Player %lld asks to move to: X:%f, y:%f, z:%f\n", ++n, i,
                           worldState.players[i].playerNetState.targetPosition.x,
                           worldState.players[i].playerNetState.targetPosition.y,
                           worldState.players[i].playerNetState.targetPosition.z);

                    worldState.players[i].playerNetState.hasTarget = true;
                    worldState.players[i].playerNetState.velocity = 3.0f;
                }

                worldState.players[i].lastInputTime = GetServerTimeSeconds();
            } else if (valread == 0) {
                printf("%d: Player has been disconnected gracefully! ID: %lld\n", ++n, i);
                closesocket(worldState.players[i].socket);
                worldState.players[i].isConnected = false;
            } else {
                int error = WSAGetLastError();

                if (error == WSAEWOULDBLOCK) {
                    continue;
                }

                printf("%d: Player connection has been lost! ID: %lld\n", ++n, i);
                closesocket(worldState.players[i].socket);
                worldState.players[i].isConnected = false;
            }
        }

        for (size_t i = 0; i < MAX_PLAYERS; i++) {
            if (!worldState.players[i].isConnected) continue;
            UpdatePlayerPosition(&worldState.players[i].playerNetState, worldState.serverDeltaTime);
        }

        char broadcast_buffer[2048] = "";
        int offset = 0;

        for (size_t i = 0; i < MAX_PLAYERS; i++) {
            if (!worldState.players[i].isConnected) continue;

            offset += sprintf(broadcast_buffer + offset, "SNAPSHOT|");
            offset += sprintf(broadcast_buffer + offset, "%lld:%f,%f,%f|", i,
                              worldState.players[i].playerNetState.position.x,
                              worldState.players[i].playerNetState.position.y,
                              worldState.players[i].playerNetState.position.z);
        }

        if (offset > 0) {
            for (size_t i = 0; i < MAX_PLAYERS; i++) {
                if (!worldState.players[i].isConnected) continue;
                send(worldState.players[i].socket, broadcast_buffer, strlen(broadcast_buffer), 0);
            }
        }

        DWORD elapsed = GetTickCount() - start_time;
        if (elapsed < worldState.tickPerMs) {
            Sleep((DWORD)(worldState.tickPerMs - elapsed));
        }
    }

    closeConnection();
    UnloadServerTerrainData();
    return 0;
}

double GetServerTimeSeconds(void) {
    static LARGE_INTEGER frequency;
    static bool initialized = false;

    if (!initialized) {
        QueryPerformanceFrequency(&frequency);
        initialized = true;
    }

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    return (double)counter.QuadPart / (double)frequency.QuadPart;
}

void UpdatePlayerPosition(PlayerNetState* netState, float delta) {
    if (!netState->hasTarget) return;

    NetVec3 target = netState->targetPosition;
    NetVec3 position = netState->position;
    NetVec3 toTarget = (NetVec3){
        .x = target.x - position.x,
        .y = target.y - position.y,
        .z = target.z - position.z,
    };

    float toTargetLength =
        sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);

    if (toTargetLength < 0.001f) {
        printf("Player toTargetLength too small\n");
        netState->position = target;
        netState->velocity = 0.0f;
        netState->hasTarget = false;
        return;
    }

    NetVec3 toTargetNormalized = (NetVec3){
        .x = toTarget.x / toTargetLength,
        .y = toTarget.y / toTargetLength,
        .z = toTarget.z / toTargetLength,
    };

    float distance = netState->velocity * delta;

    if (distance >= toTargetLength) {
        printf("Avoid overshooting\n");
        netState->position = target;
        netState->velocity = 0.0f;
        netState->hasTarget = false;
        return;
    }

    NetVec3 toMove = (NetVec3){
        .x = toTargetNormalized.x * distance,
        .y = toTargetNormalized.y * distance,
        .z = toTargetNormalized.z * distance,
    };

    netState->position.x += toMove.x;
    netState->position.y += toMove.y;
    netState->position.z += toMove.z;
}

void load_server_terrain_data() {
    parse_server_json();
    worldState.terrainData = GetServerTerrainData();
    if (worldState.terrainData == NULL || worldState.terrainData->chunkCount <= 0) {
        printf("Terrain heightmaps are not loaded. Exit\n");
        exit(1);
    }

    printf("Loaded %d terrain chunk(s)\n", worldState.terrainData->chunkCount);
}
