#include "func.h"

#include <ISmmPlugin.h>
#include <igameevents.h>
#include <iplayerinfo.h>
#include <toolframework/itoolentity.h>

extern IServerGameDLL* server;
extern IServerGameClients* gameclients;
extern IVEngineServer* engine;
extern IServerPluginHelpers* helpers;
extern IGameEventManager2* gameevents;
extern IServerPluginCallbacks* vsp_callbacks;
extern IPlayerInfoManager* playerinfomanager;
extern ICvar* icvar;
extern CGlobalVars* gpGlobals;
extern IServerTools* serverTools;

void Player_Respawn(edict_t* pEdict)
{
	serverTools->DispatchSpawn(pEdict->GetIServerEntity()->GetBaseEntity());
}

int Player_GetTeam(edict_t* pEdict)
{
	IPlayerInfo* playerinfo = playerinfomanager->GetPlayerInfo(pEdict);
	return playerinfo->GetTeamIndex();
}

void Player_JoinTeam(edict_t* pEdict, int teamnum)
{
	IPlayerInfo* playerinfo = playerinfomanager->GetPlayerInfo(pEdict);
	return playerinfo->ChangeTeam(teamnum);
}

void Player_DoCommand(edict_t* pEdict, const char* concommand)
{
	engine->ClientCommand(pEdict, concommand);
}

int Player_GetHealth(edict_t* pEdict)
{
	IPlayerInfo* playerinfo = playerinfomanager->GetPlayerInfo(pEdict);
	return playerinfo->GetHealth();
}

int Player_GetMaxHealth(edict_t* pEdict)
{
	IPlayerInfo* playerinfo = playerinfomanager->GetPlayerInfo(pEdict);
	return playerinfo->GetMaxHealth();
}