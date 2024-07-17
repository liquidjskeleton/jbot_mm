#include "func.h"

#include <ISmmPlugin.h>
#include <igameevents.h>
#include <iplayerinfo.h>
#include <toolframework/itoolentity.h>
#include "edict.h"
#include "eiface.h"
#include "../../../../hl2sdk-tf2/game/shared/in_buttons.h"
#include <ISmmAPI.h>
#include "help.h"
#include "netprops.h"
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

//int Player_GetTeam(edict_t* pEdict)
//{
//	IPlayerInfo* playerinfo = playerinfomanager->GetPlayerInfo(pEdict);
//	return playerinfo->GetTeamIndex();
//}

void Player_JoinTeam(edict_t* pEdict, int teamnum)
{
	IPlayerInfo* playerinfo = playerinfomanager->GetPlayerInfo(pEdict);
	return playerinfo->ChangeTeam(teamnum);
}

void Player_DoCommand(edict_t* pEdict, const char* concommand)
{
	engine->ClientCommand(pEdict, concommand);
}

//int Player_GetHealth(edict_t* pEdict)
//{
//	IPlayerInfo* playerinfo = playerinfomanager->GetPlayerInfo(pEdict);
//	return playerinfo->GetHealth();
//}
//
//int Player_GetMaxHealth(edict_t* pEdict)
//{
//	IPlayerInfo* playerinfo = playerinfomanager->GetPlayerInfo(pEdict);
//	return playerinfo->GetMaxHealth();
//}
typedef unsigned char u8;
//*((unsigned char*)(entptr)+0x1d4c) = nclass; //pick the scout class (m_iClass)
//*((unsigned char*)(entptr)+0x22c8) = nclass; //pick the scout class (m_iDesiredClass)

//this shit gonna break next update fuck you
//TFClass Player_GetTFClass(edict_t* pEdict)
//{
//	u8* entptr = reinterpret_cast<u8*>(pEdict->GetIServerEntity()->GetBaseEntity());
//	return *reinterpret_cast<TFClass*>((u8*)(entptr+0x1d4c));
//}
//
//void Player_SetTFClass(edict_t* pEdict,TFClass tfclass)
//{
//	u8* entptr = reinterpret_cast<u8*>(pEdict->GetIServerEntity()->GetBaseEntity());
//	*(entptr + 0x1d4c) = tfclass;
//	*(entptr + 0x22c8) = tfclass;
//}

edict_t* UserIdToEdict(int index)
{
	for (int i = 1; i < 100; i++)
	{
		edict_t* it = engine->PEntityOfEntIndex(i);
		if (!it || it->IsFree()) continue;
#undef GetClassName
		if (strcmp("player", it->GetClassNameA())) continue;
		int uid = engine->GetPlayerUserId(it);
		if (uid == index) return it;
	}
	jprintf("unknown userid %i\n");
	return nullptr;
}

int Entity_GetIndex(edict_t* pEdict)
{
	return engine->IndexOfEdict(pEdict);
}

int Entity_GetHealth(edict_t* pEdict)
{
	return *JI_NetProps::GetNetProp<int>(pEdict, JNP_PROPNAME_HEALTH);
}

int Entity_GetMaxHealth(edict_t* pEdict)
{
	return *JI_NetProps::GetNetProp<int>(pEdict, JNP_PROPNAME_HEALTH_MAX);
}

TFTeam Entity_GetTeamNum(edict_t* pEdict)
{
	return *JI_NetProps::GetNetProp<TFTeam>(pEdict, JNP_PROPNAME_TEAMNUM);
}

Vector Entity_GetPosition(edict_t* pEdict)
{
	return *JI_NetProps::GetNetProp<Vector>(pEdict, JNP_PROPNAME_ABSORIGIN);
}

TFClass TFPlayer_GetTFClass(edict_t* pEdict)
{
	return *JI_NetProps::GetNetProp<TFClass>(pEdict, JNP_PROPNAME_TFCLASS);
}

void TFPlayer_SetTFClass(edict_t* pEdict, TFClass tfclass, bool desired)
{
	*JI_NetProps::GetNetProp<TFClass>(pEdict, JNP_PROPNAME_TFCLASS) = tfclass;
	if (desired)
	*JI_NetProps::GetNetProp<TFClass>(pEdict, JNP_PROPNAME_TFCLASS_DESIRED) = tfclass;
}

bool TFPlayer_IsAlive(edict_t* pEdict)
{
	int health = Entity_GetHealth(pEdict);
	if (health <= 0) return false;
	const int lifestate = *JI_NetProps::GetNetProp<u8>(pEdict, JNP_PROPNAME_LIFESTATE);
	const int observermode = *JI_NetProps::GetNetProp<u8>(pEdict, JNP_PROPNAME_OBSERVERMODE);
	//lifestate help:
	//512 = alive, in world
	//2 = dead?
	//(0 = alive for observermode)
	return (!lifestate) && !observermode;
}

std::pair<TFTeam, TFClass> TFPlayer_GetRelativeInfo(edict_t* pEdict, TFTeam relative_team)
{
	TFClass theyclass = TFPlayer_GetTFClass(pEdict);
	TFTeam theyteam = Entity_GetTeamNum(pEdict);
	std::pair<TFTeam, TFClass> trueinfo = std::pair<TFTeam, TFClass>(theyteam, theyclass);
	if (theyteam == relative_team) return trueinfo;
	if (theyclass != TF_CLASS_SPY || !TFPlayer_CheckCond(pEdict,TF_COND_DISGUISED)) return trueinfo;
	//check what theyre disguised as
	theyclass = *JI_NetProps::GetNetProp<TFClass>(pEdict, JNP_PROPNAME_DISGUISECLASS);
	theyteam = *JI_NetProps::GetNetProp<TFTeam>(pEdict, JNP_PROPNAME_DISGUISETEAM);
	
	return std::pair<TFTeam, TFClass>(theyteam, theyclass);
}

bool TFPlayer_CheckCond(edict_t* pEdict, TFCond cond)
{
	int condsbitmask = *JI_NetProps::GetNetProp<int>(pEdict, JNP_PROPNAME_CONDS1);
	int mask = (1 << cond);
	return condsbitmask & mask;
}

bool TFPlayer_IsValidBotTarget(edict_t* pEdict)
{
	//if (!pEdict) return false;
	if (Entity_GetTeamNum(pEdict) <= TF_TEAM_SPECTATOR) return false;
	if (!TFPlayer_IsAlive(pEdict)) return false;
	TFClass clas = TFPlayer_GetTFClass(pEdict);
	if (!clas) return false; //havent picked a class yet
	return true;
}
