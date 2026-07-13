#pragma once

#include <stdbool.h>
// Keep raylib out of this header to avoid Windows API name collisions in network.c.
#include "player_state.h"

bool Network_Init(const char* ip, int port);
void Network_SendMovement(int playerId, NetVec3 target);
void Network_ReceiveData(PlayerNetState* world_players, PlayerNetState* local_player_state);
void Network_Shutdown(void);
void ParseWelcome(char* buffer, PlayerNetState* world_players, PlayerNetState* localPlayerNetState);
void ParseSnapShot(char* buffer, PlayerNetState* world_players, PlayerNetState* localPlayerNetState);