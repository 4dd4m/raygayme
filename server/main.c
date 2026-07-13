#include <stdio.h>
#include <winsock2.h>
//
#include <math.h>
#include <stdbool.h>
#include <windows.h>

#include "../shared/player_state.h"

#define PORT 9999
#define MAX_PLAYERS 10
#define TICK 33  // tick duration in a sec

typedef struct ServerPlayer {
    PlayerNetState playerNetState;
    SOCKET socket;
    bool isConnected;

    double lastPacketTime;
    double lastInputTime;
} ServerPlayer;

ServerPlayer players[MAX_PLAYERS];

double GetServerTimeSeconds(void);
void UpdatePlayerPosition(PlayerNetState* netState, float delta);

int n = 0;

int main() {
    double lastTime = GetServerTimeSeconds();  // base for delta calculation
    float serverDeltaTime = 0.0f;
    // turn terminal buffering off
    setvbuf(stdout, NULL, _IONBF, 0);
    // windows specific, just access to the networking layer
    WSADATA wsa;

    // creating the server main socket
    SOCKET server_fd;
    struct sockaddr_in server;

    // Winsock inicializálás (Windows specifikus)
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // AF_INET -> ipv4, SOCK_STREAM -> TCP (UDP -> SOCK_DGRAM)
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // non-blocking
    u_long mode = 1;
    ioctlsocket(server_fd, FIONBIO, &mode);

    server.sin_family = AF_INET;          // IPV4
    server.sin_addr.s_addr = INADDR_ANY;  // receive data from all network devices
    server.sin_port = htons(PORT);        // port conversion

    // bind
    bind(server_fd, (struct sockaddr*)&server, sizeof(server));
    listen(server_fd, 3);  // 3 players can queue up (connecting)
    printf("Server Up!\n");

    while (1) {  // main server loop
        double now = GetServerTimeSeconds();
        serverDeltaTime = now - lastTime;
        lastTime = now;

        DWORD start_time =
            GetTickCount();  // get sytstem time to calcualte processTime - tick = nextTick

        // handling new connections
        struct sockaddr_in address;
        int addrlen = sizeof(address);
        SOCKET new_socket = accept(server_fd, (struct sockaddr*)&address,
                                   &addrlen);  // accept() accepting connections
        // the code will not wait here for connections. If no one wants to connect, just pass

        if (new_socket != INVALID_SOCKET) {
            int newPlayerSlot = 0;
            bool slot_found = false;
            ioctlsocket(new_socket, FIONBIO, &mode);

            // search for inactive players
            for (size_t i = 0; i < MAX_PLAYERS; i++) {
                if (!players[i].isConnected) {
                    slot_found = true;
                    players[i].socket = new_socket;
                    players[i].playerNetState.id = i;
                    players[i].playerNetState.position = (NetVec3){0, 0, -17.38};
                    players[i].playerNetState.velocity = 0;
                    players[i].playerNetState.moveState = PLAYER_MOVE_IDLE;
                    players[i].playerNetState.yaw = 0.0f;
                    players[i].playerNetState.hasTarget = 0.0f;

                    players[i].isConnected = true;

                    players[i].lastPacketTime = GetServerTimeSeconds();
                    players[i].lastInputTime = GetServerTimeSeconds();
                    printf("%d: Player has been connected! ID: %lld\n", ++n, i);

                    // New Connection so send back the Id
                    char welcomeBuffer[100];

                    sprintf(welcomeBuffer, "WELCOME|%d|%f,%f,%f\n", i,
                            players[i].playerNetState.position.x,
                            players[i].playerNetState.position.y,
                            players[i].playerNetState.position.z);

                    printf(welcomeBuffer);

                    send(new_socket, welcomeBuffer, sizeof(welcomeBuffer), 0);
                    break;
                }
            }

            if (!slot_found) {
                printf("%d: Server is full\n", n);
                closesocket(new_socket);
            }
        }

        // Read incoming data
        for (size_t i = 0; i < MAX_PLAYERS; i++) {
            if (!players[i].isConnected) continue;

            char buffer[1024];
            int valread = recv(players[i].socket, buffer, sizeof(buffer) - 1, 0);

            if (valread > 0) {           // if something read from the player
                buffer[valread] = '\0';  // terminating the buffer
                if (sscanf(buffer, "MOVE|%d|%f,%f,%f", &players[i].playerNetState.id,
                           &players[i].playerNetState.targetPosition.x,
                           &players[i].playerNetState.targetPosition.y,
                           &players[i].playerNetState.targetPosition.z) == 4) {
                    printf("%d: Player %lld asks to move to: X:%f, y:%f, z:%f\n", ++n, i,
                           players[i].playerNetState.targetPosition.x,
                           players[i].playerNetState.targetPosition.y,
                           players[i].playerNetState.targetPosition.z);

                    players[i].playerNetState.hasTarget = true;
                    players[i].playerNetState.velocity = 3.0f;
                }

                players[i].lastInputTime = GetServerTimeSeconds();
            } else if (valread == 0) {
                printf("%d: Player has been disconnected gracefully! ID: %lld\n", ++n, i);
            }
            // nothing read from the player or player connection is not present
            else {
                int error = WSAGetLastError();

                if (error == WSAEWOULDBLOCK) {
                    // no buffer data
                    continue;
                } else {
                    printf("%d: Player connection has been lost! ID: %lld\n", ++n, i);
                    closesocket(players[i].socket);
                    players[i].isConnected = false;
                }
            }
        }

        for (size_t i = 0; i < MAX_PLAYERS; i++) {
            if (!players[i].isConnected) continue;

            UpdatePlayerPosition(&players[i].playerNetState, serverDeltaTime);
        }

        // BROADCAST WORLD STATE
        char broadcast_buffer[2048] = "";
        int offset = 0;

        for (size_t i = 0; i < MAX_PLAYERS; i++) {
            if (!players[i].isConnected) continue;

            offset += sprintf(broadcast_buffer + offset, "SNAPSHOT|");

            offset += sprintf(
                broadcast_buffer + offset, "%lld:%f,%f,%f|", i, players[i].playerNetState.position.x,
                players[i].playerNetState.position.y, players[i].playerNetState.position.z);

            // printf("%f - SNAPSHOT|%lld:%f,%f,%f|\n", serverDeltaTime, i,
            //        &players[i].playerNetState.position.x, &players[i].playerNetState.position.y,
            //        &players[i].playerNetState.position.y);
        }

        if (offset > 0) {
            for (size_t i = 0; i < MAX_PLAYERS; i++) {
                if (!players[i].isConnected) continue;
                send(players[i].socket, broadcast_buffer, strlen(broadcast_buffer), 0);
            }
        }

        // Calculate next tick
        DWORD elapsed = GetTickCount() - start_time;
        if (elapsed < TICK) {
            Sleep(TICK - elapsed);
        }
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}

double GetServerTimeSeconds(void) {
    // static are created once, and keep their value
    static LARGE_INTEGER frequency;  // processor frequency
    static bool initialized = false;

    if (!initialized) {
        QueryPerformanceFrequency(&frequency);  // since server machine turned on (in millisec)
        initialized = true;
    }

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    // convert to double because: int / int = int
    return (double)counter.QuadPart / (double)frequency.QuadPart;  // i.e. 1432.548291 (ms)
}

void UpdatePlayerPosition(PlayerNetState* netState, float delta) {
    if (!netState->hasTarget) {
        // printf("Player has not target\n");
        return;
    }

    NetVec3 target = netState->targetPosition;
    NetVec3 position = netState->position;

    NetVec3 toTarget =
        (NetVec3){.x = target.x - position.x,
                  .y = target.y - position.y,
                  .z = target.z - position.z};  // this vector will point to position -> target

    float toTargetLength =
        sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);

    if (toTargetLength < 0.001f) {
        printf("Player toTargetLength too small\n");
        netState->position = target;
        netState->velocity = 0.0f;
        netState->hasTarget = false;
        return;
    }

    NetVec3 toTargetNormalized = (NetVec3){.x = toTarget.x / toTargetLength,
                                           .y = toTarget.y / toTargetLength,
                                           .z = toTarget.z / toTargetLength};

    float distance = netState->velocity * delta;

    if (distance >= toTargetLength) {  // avoid overshooting the target
        printf("Avoid overshooting\n");
        netState->position = target;
        netState->velocity = 0.0f;
        netState->hasTarget = false;
        return;
    }

    NetVec3 toMove = (NetVec3){// allowed movement
                               .x = toTargetNormalized.x * distance,
                               .y = toTargetNormalized.y * distance,
                               .z = toTargetNormalized.z * distance};

    netState->position.x += toMove.x;
    netState->position.y += toMove.y;
    netState->position.z += toMove.z;

    // printf("%d, %f, %f \n", netState->position.x, netState->position.y, netState->position.z);
    // printf("Movement has been calulated\n");
}