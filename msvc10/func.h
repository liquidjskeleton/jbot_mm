#pragma once
#include <string>
#include <basetsd.h>
#include <Windows.h>
#include "edict.h"
#include "teamfortress.h"

class CBaseEntity;

//for compat
#define Player_GetTeam Entity_GetTeamNum
#define Player_GetHealth Entity_GetHealth
#define Player_GetMaxHealth Entity_GetMaxHealth

void Player_Respawn(edict_t* pEdict);
//int Player_GetTeam(edict_t* pEdict);
void Player_JoinTeam(edict_t* pEdict, int teamnum);
void Player_DoCommand(edict_t* pEdict, const char* concommand);
//int Player_GetHealth(edict_t* pEdict);
//int Player_GetMaxHealth(edict_t* pEdict);
//TFClass Player_GetTFClass(edict_t* pEdict);
//void Player_SetTFClass(edict_t* pEdict, TFClass tfclass);
edict_t* UserIdToEdict(int index);

int Entity_GetIndex(edict_t* pEdict);
//post netprops

int Entity_GetHealth(edict_t* pEdict);
int Entity_GetMaxHealth(edict_t* pEdict);
TFTeam Entity_GetTeamNum(edict_t* pEdict);
Vector Entity_GetPosition(edict_t* pEdict);
TFClass TFPlayer_GetTFClass(edict_t* pEdict);
void TFPlayer_SetTFClass(edict_t* pEdict, TFClass tfclass,bool desired);
bool TFPlayer_IsAlive(edict_t* pEdict);
//get the info for a player relative to a team (For spies and whatnot)
std::pair<TFTeam, TFClass> TFPlayer_GetRelativeInfo(edict_t* pEdict, TFTeam relative_team);
//only works for conds below 32
bool TFPlayer_CheckCond(edict_t* pEdict, TFCond cond);
bool TFPlayer_IsValidBotTarget(edict_t* pEdict);