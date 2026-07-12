#include "network.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <winsock2.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9999
#define MAX_PLAYERS 10

static SOCKET client_socket = INVALID_SOCKET;
PlayerNetState remote_players[MAX_PLAYERS];

bool Network_Init(const char* ip, int port) {
    WSADATA wsa;
    struct sockaddr_in server;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("[Network]: Winsock initialization fault\n");
        return false;
    }

    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (client_socket == INVALID_SOCKET) {
        printf("[Network]: Socket creation fault\n");
        WSACleanup();
        return false;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_port = htons(port);

    printf("[Network]: Connecting to the server: %s:%d\n", ip, port);
    if (connect(client_socket, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printf("[Network]: Connection error\n");
        closesocket(client_socket);
        WSACleanup();
        return false;
    }

    printf("[Network]: Connected to the server\n");

    // change to non blocking mode
    u_long mode = 1;

    ioctlsocket(client_socket, FIONBIO, &mode);

    return true;
}

void Network_SendData(float x, float y, float z) {}

void Network_ReceiveData(PlayerNetState* world_players) {
    if (client_socket == INVALID_SOCKET) return;

    char recvBuffer[2048];
    int valread = recv(client_socket, recvBuffer, sizeof(recvBuffer) - 1, 0);

    if (valread > 0) {  // if buffer has anything inside it
        recvBuffer[valread] = '\0';

        // change everybody to offline
        for (size_t i = 0; i < MAX_PLAYERS; i++) {
            remote_players[i].isConnected = false;

            char* token = strtok(recvBuffer, "|");

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
    } else if (valread == 0 ||
               (valread == SOCKET_ERROR &&
                WSAGetLastError() !=
                    WSAEWOULDBLOCK)) {  // buffer has no content or connection has been dropped
        printf("[Network]: Unexpected Disconnect\n");
        Network_Shutdown();
    }
}

void Network_Shutdown(void) {
    if (client_socket != INVALID_SOCKET) {
        closesocket(client_socket);
        client_socket = INVALID_SOCKET;

        WSACleanup();
        printf("[Network]: Connection has been closed by the client\n");
    }
}
