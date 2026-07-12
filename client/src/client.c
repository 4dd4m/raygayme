#include "client.h"

#include <stdio.h>
#include <stdlib.h>

#include "network.h"

typedef struct RenderConfig {
    int width;
    int height;
    const char* title;
    int targetFps;
} RenderConfig;

Client* CreateClient(void) {
    Client* client = malloc(sizeof(*client));
    if (client == NULL) {
        return NULL;
    }

    InitClient(client);
    return client;
}

void InitClient(Client* client) {
    client->renderConfig = malloc(sizeof(RenderConfig));
    if (client->renderConfig == NULL) {
        return;
    }

    *client->renderConfig =
        (RenderConfig){.width = 2500, .height = 1200, .targetFps = 165, .title = "My FavGame"};

    InitWindow(client->renderConfig->width, client->renderConfig->height,
               client->renderConfig->title);
    SetTargetFPS(client->renderConfig->targetFps);

    bool isConnected = Network_Init("127.0.0.1", 9999);
    if (!isConnected) {
        printf("[Client]: Network init failed\n");
        exit(1);
    }

    InitPlayer(&client->player);
    InitWorld(&client->world);
    InitCamera(&client->Camera, &client->player);
}

void DestroyClient(Client* client) {
    if (client == NULL) {
        return;
    }

    free(client->renderConfig);
    free(client);
}
