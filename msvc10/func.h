#pragma once
#include <string>
#include <basetsd.h>
#include <Windows.h>
#include "edict.h"

class CBaseEntity;

void Player_Respawn(edict_t* pEdict);
int Player_GetTeam(edict_t* pEdict);
void Player_JoinTeam(edict_t* pEdict, int teamnum);
void Player_DoCommand(edict_t* pEdict, const char* concommand);
int Player_GetHealth(edict_t* pEdict);
int Player_GetMaxHealth(edict_t* pEdict);