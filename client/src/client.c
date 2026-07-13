#include "client.h"

#include <stdio.h>
#include <stdlib.h>

#include "network.h"

Client* CreateClient(void) {
    Client* client = malloc(sizeof(*client));
    if (client == NULL) {
        return NULL;
    }

    client->renderConfig = malloc(sizeof(RenderConfig));
    if (client->renderConfig == NULL) {
        return NULL;
    }

    client->clientState = CLIENT_STATE_CONNECT_SCREEN;
    client->connectionError = NULL;
    client->localPlayerNetState = (PlayerNetState){0};

    *client->renderConfig =
        (RenderConfig){.width = 1280, .height = 720, .targetFps = 165, .title = "My FavGame"};

    return client;
}

void InitClientWindow(Client* client) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(client->renderConfig->width, client->renderConfig->height,
               client->renderConfig->title);
    SetTargetFPS(client->renderConfig->targetFps);
}

void InitPLayerWorldCamera(Client* client) {
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
