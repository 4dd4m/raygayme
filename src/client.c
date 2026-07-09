#include "client.h"

void InitClient(Client* client) {
    client->renderConfig.width = 2500;
    client->renderConfig.height = 900;
    client->renderConfig.targetFps = 165;
    client->renderConfig.title = "My Favourite game";

    InitWindow(client->renderConfig.width, client->renderConfig.height, client->renderConfig.title);
    SetTargetFPS(client->renderConfig.targetFps);

    InitCamera(&client->Camera);
    InitWorld(&client->world);
    InitPlayer(&client->player);
}