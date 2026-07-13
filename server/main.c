#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

typedef int SOCKET;

#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#endif

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

static bool SocketLayer_Init(void);
static void SocketLayer_Cleanup(void);
static void Socket_Close(SOCKET socket_handle);
static bool Socket_SetNonBlocking(SOCKET socket_handle);
static bool Socket_WouldBlock(void);
static void ServerSleepMs(unsigned int milliseconds);
double GetServerTimeSeconds(void);

int n = 0;

int main() {
    // turn terminal buffering off
    setvbuf(stdout, NULL, _IONBF, 0);

    // creating the server main socket
    SOCKET server_fd;
    struct sockaddr_in server;

    if (!SocketLayer_Init()) {
        fprintf(stderr, "Socket initialization failed\n");
        return 1;
    }

    // AF_INET -> ipv4, SOCK_STREAM -> TCP (UDP -> SOCK_DGRAM)
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        fprintf(stderr, "Server socket creation failed\n");
        SocketLayer_Cleanup();
        return 1;
    }

    if (!Socket_SetNonBlocking(server_fd)) {
        fprintf(stderr, "Could not switch server socket to non-blocking mode\n");
        Socket_Close(server_fd);
        SocketLayer_Cleanup();
        return 1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;          // IPV4
    server.sin_addr.s_addr = INADDR_ANY;  // receive data from all network devices
    server.sin_port = htons(PORT);        // port conversion

    // bind
    if (bind(server_fd, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        fprintf(stderr, "Bind failed\n");
        Socket_Close(server_fd);
        SocketLayer_Cleanup();
        return 1;
    }

    if (listen(server_fd, 3) == SOCKET_ERROR) {  // 3 players can queue up (connecting)
        fprintf(stderr, "Listen failed\n");
        Socket_Close(server_fd);
        SocketLayer_Cleanup();
        return 1;
    }

    printf("Server Up!\n");

    while (1) {  // main server loop
        double start_time = GetServerTimeSeconds();

        // handling new connections
        struct sockaddr_in address;
    #ifdef _WIN32
        int addrlen = sizeof(address);
    #else
        socklen_t addrlen = sizeof(address);
    #endif
        SOCKET new_socket = accept(server_fd, (struct sockaddr*)&address,
                                   &addrlen);  // accept() accepting connections
        // the code will not wait here for connections. If no one wants to connect, just pass

        if (new_socket != INVALID_SOCKET) {
            int newPlayerSlot = 0;
            bool slot_found = false;
            Socket_SetNonBlocking(new_socket);

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
                    printf("%d: Player has been connected! ID: %zu\n", ++n, i);
                    break;
                }
            }

            if (!slot_found) {
                printf("%d: Server is full\n", n);
                Socket_Close(new_socket);
                continue;
            }

            // New Connection so send back the Id
            char welcomeBuffer[100];

            sprintf(welcomeBuffer, "WELCOME|%d|%f,%f,%f\n", newPlayerSlot,
                    players[newPlayerSlot].playerNetState.position.x,
                    players[newPlayerSlot].playerNetState.position.y,
                    players[newPlayerSlot].playerNetState.position.z);

            printf("%s", welcomeBuffer);

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
                       &players[i].playerNetState.position.y, &players[i].playerNetState.position.z);

                printf("%d: Player has been moved! ID: %zu\n", ++n, i);

                players[i].lastInputTime = GetServerTimeSeconds();
            }
            // nothing read from the player or player connection is not present
            else if (valread == 0 ||
                     (valread == SOCKET_ERROR && !Socket_WouldBlock())) {
                printf("%d: Player has been disconnected! ID: %zu\n", ++n, i);
                Socket_Close(players[i].socket);
                players[i].isConnected = false;
            }
        }

        // BROADCAST WORLD STATE
        char broadcast_buffer[2048] = "";
        int offset = 0;

        for (size_t i = 0; i < MAX_PLAYERS; i++) {
            if (!players[i].isConnected) continue;

            offset +=
                sprintf(broadcast_buffer + offset, "%zu:%f,%f,%f|", i,
                        players[i].playerNetState.position.x, players[i].playerNetState.position.y,
                        players[i].playerNetState.position.z);
        }

        if (offset > 0) {
            for (size_t i = 0; i < MAX_PLAYERS; i++) {
                if (!players[i].isConnected) continue;
                send(players[i].socket, broadcast_buffer, strlen(broadcast_buffer), 0);
            }
        }

        // Calculate next tick
        unsigned int elapsed = (unsigned int)((GetServerTimeSeconds() - start_time) * 1000.0);
        if (elapsed < TICK) {
            ServerSleepMs(TICK - elapsed);
        }
    }

    Socket_Close(server_fd);
    SocketLayer_Cleanup();
    return 0;
}

static bool SocketLayer_Init(void) {
#ifdef _WIN32
    WSADATA wsa;

    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
#else
    return true;
#endif
}

static void SocketLayer_Cleanup(void) {
#ifdef _WIN32
    WSACleanup();
#endif
}

static void Socket_Close(SOCKET socket_handle) {
#ifdef _WIN32
    closesocket(socket_handle);
#else
    close(socket_handle);
#endif
}

static bool Socket_SetNonBlocking(SOCKET socket_handle) {
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(socket_handle, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(socket_handle, F_GETFL, 0);
    if (flags == -1) {
        return false;
    }

    return fcntl(socket_handle, F_SETFL, flags | O_NONBLOCK) != -1;
#endif
}

static bool Socket_WouldBlock(void) {
#ifdef _WIN32
    return WSAGetLastError() == WSAEWOULDBLOCK;
#else
    return errno == EWOULDBLOCK || errno == EAGAIN;
#endif
}

static void ServerSleepMs(unsigned int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    struct timespec request = {
        .tv_sec = milliseconds / 1000,
        .tv_nsec = (long)(milliseconds % 1000) * 1000000L,
    };

    nanosleep(&request, NULL);
#endif
}

double GetServerTimeSeconds(void) {
#ifdef _WIN32
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
#else
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)now.tv_sec + (double)now.tv_nsec / 1000000000.0;
#endif
}
