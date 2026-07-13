#include "network.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
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

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9999
#define MAX_PLAYERS 10

static SOCKET client_socket = INVALID_SOCKET;
PlayerNetState remote_players[MAX_PLAYERS];

static bool SocketLayer_Init(void) {
#ifdef _WIN32
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("[Network]: Winsock initialization fault\n");
        return false;
    }
#endif

    return true;
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

static const char* Socket_GetLastErrorText(void) {
#ifdef _WIN32
    static char message[128];
    snprintf(message, sizeof(message), "WSA error %d", WSAGetLastError());
    return message;
#else
    return strerror(errno);
#endif
}

bool Network_Init(const char* ip, int port) {
    struct sockaddr_in server;

    if (client_socket != INVALID_SOCKET) {
        return true;
    }

    if (!SocketLayer_Init()) {
        return false;
    }

    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (client_socket == INVALID_SOCKET) {
        fprintf(stderr, "[Network]: Socket creation fault: %s\n", Socket_GetLastErrorText());
        SocketLayer_Cleanup();
        return false;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_port = htons(port);

    fprintf(stderr, "[Network]: Connecting to the server: %s:%d\n", ip, port);
    if (connect(client_socket, (struct sockaddr*)&server, sizeof(server)) < 0) {
        fprintf(stderr, "[Network]: Connection error: %s\n", Socket_GetLastErrorText());
        Socket_Close(client_socket);
        client_socket = INVALID_SOCKET;
        SocketLayer_Cleanup();
        return false;
    }

    fprintf(stderr, "[Network]: Connected to the server\n");

    if (!Socket_SetNonBlocking(client_socket)) {
        fprintf(stderr, "[Network]: Could not switch socket to non-blocking mode: %s\n",
                Socket_GetLastErrorText());
        Socket_Close(client_socket);
        client_socket = INVALID_SOCKET;
        SocketLayer_Cleanup();
        return false;
    }

    return true;
}

bool Network_IsConnected(void) { return client_socket != INVALID_SOCKET; }

void Network_SendData(float x, float y, float z) {}

void Network_ReceiveData(PlayerNetState* world_players, PlayerNetState* player) {
    if (client_socket == INVALID_SOCKET) return;

    char recvBuffer[2048];
    int valread = recv(client_socket, recvBuffer, sizeof(recvBuffer) - 1, 0);

    if (valread > 0) {  // if buffer has anything inside it
        recvBuffer[valread] = '\0';

        if (strncmp(recvBuffer, "WELCOME|", 8) == 0) {
            ParseWelcome(recvBuffer, world_players, player);
        } else {
            ParseSnapShot(recvBuffer, world_players, player);
        }

    } else if (valread == 0 ||
               (valread == SOCKET_ERROR && !Socket_WouldBlock())) {
        fprintf(stderr, "[Network]: Unexpected Disconnect\n");
        Network_Shutdown();
    }
}

void Network_Shutdown(void) {
    if (client_socket != INVALID_SOCKET) {
        Socket_Close(client_socket);
        client_socket = INVALID_SOCKET;

        SocketLayer_Cleanup();
        fprintf(stderr, "[Network]: Connection has been closed by the client\n");
    }
}

void ParseWelcome(char* buffer, PlayerNetState* world_players, PlayerNetState* localPlayerNetState) {
    fprintf(stderr, "[Network]: Welcome received\n");

    if (sscanf(buffer, "WELCOME|%d|%f,%f,%f\n", &localPlayerNetState->id,
               &localPlayerNetState->position.x, &localPlayerNetState->position.y,
               &localPlayerNetState->position.z) == 4) {
        fprintf(stderr, "[Network]: Received ID: %d Player Coordinates: X:%f Y:%f Z:%f\n",
                localPlayerNetState->id, localPlayerNetState->position.x,
                localPlayerNetState->position.y, localPlayerNetState->position.z);
    }
    // else uninitailized localPlayerNetState will put the player on 0.0.0
    localPlayerNetState->isConnected = true;
}

void ParseSnapShot(char* buffer, PlayerNetState* world_players,
                   PlayerNetState* localPlayerNetState) {
    // change everybody to offline
    for (size_t i = 0; i < MAX_PLAYERS; i++) {
        remote_players[i].isConnected = false;

        char* token = strtok(buffer, "|");

        while (token != NULL) {
            int id;
            float x, y, z;

            if (sscanf(token, "%d:%f,%f,%f", &id, &x, &y, &z) == 4) {
                if (id >= 0 && id < MAX_PLAYERS) {
                    remote_players[id].id = id;

                    remote_players[id].position = (NetVec3){x, y, z};
                    remote_players[id].isConnected = true;
                }
            }
            token = strtok(NULL, "|");
        }
    }
}
