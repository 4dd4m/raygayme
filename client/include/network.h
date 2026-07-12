#pragma once

#include <stdbool.h>

#include "../../shared/player_state.h"

bool Network_Init(const char* ip, int port);
void Network_SendData(float x, float y, float z);
void Network_ReceiveData(PlayerNetState* world_players);
void Network_Shutdown(void);