#include "server.h"

#include <stdio.h>
#include <string.h>
#include <winsock2.h>

#define PORT 9999

static WSADATA wsa;
static SOCKET server_fd = INVALID_SOCKET;
static struct sockaddr_in server;
static u_long mode = 1;
static SOCKET new_socket = INVALID_SOCKET;

void set_up_non_blocking_socket() {
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("Server socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return;
    }

    ioctlsocket(server_fd, FIONBIO, &mode);

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Server bind failed: %d\n", WSAGetLastError());
        closesocket(server_fd);
        server_fd = INVALID_SOCKET;
        WSACleanup();
        return;
    }

    if (listen(server_fd, 3) == SOCKET_ERROR) {
        printf("Server listen failed: %d\n", WSAGetLastError());
        closesocket(server_fd);
        server_fd = INVALID_SOCKET;
        WSACleanup();
        return;
    }

    printf("Server Up!\n");
}

SOCKET accept_new_connection() {
    if (server_fd == INVALID_SOCKET) return INVALID_SOCKET;

    struct sockaddr_in address;
    int addrlen = sizeof(address);
    new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
    return new_socket;
}

void closeConnection() {
    if (server_fd != INVALID_SOCKET) {
        closesocket(server_fd);
        server_fd = INVALID_SOCKET;
    }
    WSACleanup();
}

void ioctl_socket() {
    if (new_socket != INVALID_SOCKET) {
        ioctlsocket(new_socket, FIONBIO, &mode);
    }
}
