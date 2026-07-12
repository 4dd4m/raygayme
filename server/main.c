#include <stdio.h>
#include <winsock2.h>
//
#include <stdbool.h>
#include <windows.h>

#include "../shared/player_state.h"

#define PORT 9999
#define MAX_PLAYERS 10
#define TICK 33

typedef struct ServerPlayer {
    PlayerNetState playerNetState;
    SOCKET socket;
    bool isConnected;

    double lastPacketTime;
    double lastInputTime;
} ServerPlayer;

ServerPlayer players[MAX_PLAYERS];

double GetServerTimeSeconds(void);

int n = 0;

int main() {
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
                    players[i].playerNetState.velocity = (NetVec3){0};
                    players[i].playerNetState.moveState = PLAYER_MOVE_IDLE;
                    players[i].playerNetState.yaw = 0.0f;

                    players[i].isConnected = true;

                    players[i].lastPacketTime = GetServerTimeSeconds();
                    players[i].lastInputTime = GetServerTimeSeconds();
                    newPlayerSlot = i;
                    printf("%d: Player has been connected! ID: %lld\n", ++n, i);
                    break;
                }
            }

            if (!slot_found) {
                printf("%d: Server is full\n", n);
                closesocket(new_socket);
            }

            // New Connection so send back the Id
            char welcomeBuffer[100];

            sprintf(welcomeBuffer, "WELCOME|%d|%f,%f,%f\n", newPlayerSlot,
                    players[newPlayerSlot].playerNetState.position.x,
                    players[newPlayerSlot].playerNetState.position.y,
                    players[newPlayerSlot].playerNetState.position.z);

            printf(welcomeBuffer);

            send(new_socket, welcomeBuffer, sizeof(welcomeBuffer), 0);
        }

        // Read incoming data
        for (size_t i = 0; i < MAX_PLAYERS; i++) {
            if (!players[i].isConnected) continue;

            char buffer[1024];
            int valread = recv(players[i].socket, buffer, sizeof(buffer) - 1, 0);

            if (valread > 0) {           // if something read from the player
                buffer[valread] = '\0';  // terminating the buffer
                sscanf(buffer, "%f, %f, %f", &players[i].playerNetState.position.x,
                       &players[i].playerNetState.position.y, &players[i].playerNetState.position.y);

                printf("%d: Player has been moved! ID: %lld\n", ++n, i);

                players[i].lastInputTime = GetServerTimeSeconds();
            }
            // nothing read from the player or player connection is not present
            else if (valread == 0 ||
                     (valread == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)) {
                printf("%d: Player has been disconnected! ID: %lld\n", ++n, i);
                players[i].isConnected = false;
            }
        }

        // BROADCAST WORLD STATE
        char broadcast_buffer[2048] = "";
        int offset = 0;

        for (size_t i = 0; i < MAX_PLAYERS; i++) {
            if (!players[i].isConnected) continue;

            offset +=
                sprintf(broadcast_buffer + offset, "%lld:%f,%f,%f|", i,
                        &players[i].playerNetState.position.x, &players[i].playerNetState.position.y,
                        &players[i].playerNetState.position.y);
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
    QueryPerformanceFrequency(&counter);

    // convert to double because: int / int = int
    return (double)counter.QuadPart / (double)frequency.QuadPart;  // i.e. 1432.548291 (ms)
}