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
#include "terrainAssetLoader.h"

WorldState worldState = {0};

double GetServerTimeSeconds(void);
void UpdatePlayerPosition(PlayerNetState* netState, float delta);
ServerVec2i GetChunkCoordByXandZ(float rayX, float rayZ);

int n = 0;

int main() {
    worldState.terrainData = GetInitialTerrainData();
    if (worldState.terrainData == NULL) {
        printf("Initial terrain data load fail\n");
        exit(1);
    }

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
                    worldState.players[i].playerNetState.position = (NetVec3){48.0f, 0, 48.0f};
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

                    // ID|startXYZ|chunkX,chunkZ
                    int length = snprintf(NULL, 0, "WELCOME|%d|%f,%f,%f|%d,%d\n", (int)i,
                                          worldState.players[i].playerNetState.position.x,
                                          worldState.players[i].playerNetState.position.y,
                                          worldState.players[i].playerNetState.position.z,
                                          worldState.players[i].chunkCoord.x,
                                          worldState.players[i].chunkCoord.z);

                    if (length > 0) {
                        char* welcomeBuffer = malloc(length + 1);
                        if (welcomeBuffer == NULL) {
                            worldState.players[i].isConnected = false;
                            break;
                        }

                        int written = snprintf(
                            welcomeBuffer, length + 1, "WELCOME|%d|%f,%f,%f|%d,%d\n", (int)i,
                            worldState.players[i].playerNetState.position.x,
                            worldState.players[i].playerNetState.position.y,
                            worldState.players[i].playerNetState.position.z,
                            worldState.players[i].chunkCoord.x, worldState.players[i].chunkCoord.z);

                        if (written != length) {
                            free(welcomeBuffer);
                            worldState.players[i].isConnected = false;
                            break;
                        }

                        send(new_socket, welcomeBuffer, strlen(welcomeBuffer), 0);
                        free(welcomeBuffer);
                        break;
                    }
                }
            }

            if (!slot_found) {
                printf("%d: Server is full\n", n);
                closesocket(new_socket);
            }
        }

        //
        // Receive data from the client
        //
        for (size_t i = 0; i < MAX_PLAYERS; i++) {
            if (!worldState.players[i].isConnected) continue;

            char buffer[1024];
            int valread = recv(worldState.players[i].socket, buffer, sizeof(buffer) - 1, 0);

            if (valread > 0) {
                buffer[valread] = '\0';

                //
                // Parse client move request
                //
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
                } else {
                    worldState.players[i].isConnected = false;
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

        //
        // Broadcast
        //
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

    ServerVec2i actualChunk = GetChunkCoordByXandZ(netState->position.x, netState->position.z);

    // printf("Next position ChunkId: %d, %d\n", actualChunk.x, actualChunk.z);

    //
    // Player enters into a new chunk
    //
    if (netState->chunkCoord.x != actualChunk.x || netState->chunkCoord.z != actualChunk.z) {
        int newChunkIndex = LoadServerTerrainChunkByCoord(actualChunk);
        if (newChunkIndex >= 0) {
            printf("REQUEST NEW CHUNK DATA\n");
            netState->chunkCoord.x = actualChunk.x;
            netState->chunkCoord.z = actualChunk.z;
            printf("CHUNK LOADED\n");
        }
    }
}

ServerVec2i GetChunkCoordByXandZ(float rayX, float rayZ) {
    return (ServerVec2i){.x = floor((rayX + 50.0f) / 100.0f),
                          .z = floor((rayZ + 50.0f) / 100.0f)};
}
