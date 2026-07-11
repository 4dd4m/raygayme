#include "client.h"

#include <stdlib.h>

Client* CreateClient(void) {
    Client* client = malloc(sizeof(*client));
    if (client == NULL) {
        return NULL;
    }

    InitClient(client);
    return client;
}

void InitClient(Client* client) {
    client->renderConfig.width = 2500;
    client->renderConfig.height = 900;
    client->renderConfig.targetFps = 165;
    client->renderConfig.title = "My Favourite game";

    InitWindow(client->renderConfig.width, client->renderConfig.height, client->renderConfig.title);
    SetTargetFPS(client->renderConfig.targetFps);

    InitPlayer(&client->player);
    InitWorld(&client->world);
    InitCamera(&client->Camera, &client->player);
}

void DestroyClient(Client* client) {
    if (client == NULL) {
        return;
    }

    free(client);
}
