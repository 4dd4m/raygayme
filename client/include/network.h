#pragma once

#include <stdbool.h>

#include "../../shared/player_state.h"

bool Network_Init(const char* ip, int port);
bool Network_IsConnected(void);
void Network_SendData(float x, float y, float z);
void Network_ReceiveData(PlayerNetState* world_players, PlayerNetState* local_player_state);
void Network_Shutdown(void);
void ParseWelcome(char* buffer, PlayerNetState* world_players, PlayerNetState* localPlayerNetState);
void ParseSnapShot(char* buffer, PlayerNetState* world_players, PlayerNetState* localPlayerNetState);
