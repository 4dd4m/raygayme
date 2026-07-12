#include "network.h"

#include <stdbool.h>
#include <stdio.h>
#include <winsock2.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9999

static SOCKET client_socket = INVALID_SOCKET;

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

void Network_ReceiveData(PlayerNetState* world_players) {}

void Network_Shutdown(void) {}
