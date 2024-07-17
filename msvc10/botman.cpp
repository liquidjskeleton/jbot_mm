#include "botman.h"
#include "func.h"
#include "../game/shared/in_buttons.h"
#include <stdio.h>
#include <IEngineTrace.h>
#include "eiface.h"
#include "mathlib/mathlib.h"
#include "navigation.h"
#include "help.h"
#include "botclass.h"
#include "curmap.h"
#include "netprops.h"
//#include "cdll_int.h"
extern IBotManager* botmanager;
extern IEngineTrace* enginetrace;
extern IPlayerInfoManager* playerinfomanager;
extern IVEngineServer* engine;

extern IServerPluginHelpers* helpers;
extern CGlobalVars* gpGlobals;
extern IServerTools* serverTools;
extern IServerGameEnts* serverGameEnts;

edict_t* bot_buffer[MAXBOTS];
JBotClass botc_buffer[MAXBOTS];


#define PF_DEBUG_MARKER "passtime_ball"

edict_t* JBotManager::CreateBot()
{
	if (!botmanager) return 0;

	int counter = -1;
	for (int i = 0; i < MAXBOTS; i++)
	{
		if (!bot_buffer[i])
		{
			counter = i;
			break;
		}
	}
	if (counter == -1)
	{
		return 0;
	}



	char buffer[50];
	sprintf_s(buffer, "JBOT_%x",counter);
	
	edict_t* ptr = botmanager->CreateBot(buffer);
	if (!ptr) return 0;
	bot_buffer[counter] = ptr;
	botc_buffer[counter] = JBotClass();
	auto jclas = botc_buffer[counter];
	//basic init
	Player_JoinTeam(ptr, (counter%2)+2); //join red team
	jclas.edict = ptr;
	jclas.active = 1;
	//jclas.Notify_Spawn();
#if 0
	//im so sorry
	CBaseEntity* entptr = ptr->GetIServerEntity()->GetBaseEntity();
	int nclass = 1;// (counter % 9) + 1;
	*((unsigned char*)(entptr)+0x1d4c) = nclass; //pick the scout class (m_iClass)
	*((unsigned char*)(entptr)+0x22c8) = nclass; //pick the scout class (m_iDesiredClass)
	botmanager->GetBotController(ptr)->RemoveAllItems(false);
#else
	TFClass desiredclass = TF_CLASS_SCOUT;
	TFPlayer_SetTFClass(ptr,desiredclass,true);
#endif
	return ptr;
}

void JBotManager::Bot_ThinkAll()
{
	for (int i = 0; i < MAXBOTS; i++)
	{
		if (bot_buffer[i])
		{
			if (bot_buffer[i]->IsFree() ||
				!bot_buffer[i]->GetIServerEntity()
				) 
			{
				bot_buffer[i] = 0; botc_buffer[i].active = false; continue;
			}
			Bot_Think(i);
		}
	}
}

void JBotManager::RemoveBot(edict_t* num)
{
	for (int i = 0; i < MAXBOTS; i++)
	{
		if (bot_buffer[i] == num) { bot_buffer[i] = 0; botc_buffer[i].active = false; break; }
	}
	//done?
}
extern bool j_stopexecution;
void JBotManager::Notify_RestartEverything()
{
	for (int i = 0; i < MAXBOTS; i++)
	{
		bot_buffer[i] = 0;
		botc_buffer[i].active = 0;
	}
}

extern ConVar jbotdebugcvar;
void JBotManager::Debug_VisualizePath(int botindex)
{
	if (!jbotdebugcvar.GetBool()) return;
	CBaseEntity* t = 0;
	while (t = serverTools->FindEntityByClassname(t, PF_DEBUG_MARKER))
	{
		serverTools->RemoveEntity(t);
	}

	if (botc_buffer[botindex].active)
	{
		auto& vec = botc_buffer[botindex].brain.last_successful_pathfind_result.disp;
		for (int i = 0; i < vec.size(); i++)
		{
			CBaseEntity* e = serverTools->CreateEntityByName(PF_DEBUG_MARKER);
			Vector position = vec[i].second->m_center;
			position.z += 8;
			serverTools->DispatchSpawn(e);
			*JI_NetProps::GetNetProp<Vector>(e, JNP_PROPNAME_ABSORIGIN) = position;
		}
	}
}

void dbgvispathfunc(const CCommand& command)
{
	int index = atoi(command.Arg(1));
	JBotManager::Debug_VisualizePath(index);
}
ConCommand dbgvispath("jbot_debug_visualizepath", dbgvispathfunc,"?");

void JBotManager::Bot_Think(int index)
{
	edict_t* dic = bot_buffer[index]; 
	JBotClass* jbot = &botc_buffer[index];
	jbot->edict = dic;
	int health = Player_GetHealth(dic);
	int maxhealth = Player_GetMaxHealth(dic);
	int team = Player_GetTeam(dic);
	jbot->team = team;
	jbot->health = health;
	jbot->maxhealth = maxhealth;
	if (health <= 0)
	{
		jbot->Notify_Death();
		//Player_Respawn(dic);
		//jbot->Notify_Spawn();
		return;
	}
	auto* controller = botmanager->GetBotController(dic);
	CBotCmd commd;
	Q_memset(&commd, 0, sizeof(CBotCmd)); //is this needed?
	commd.Reset();
	jbot->current_position = controller->GetLocalOrigin();
	jbot->brain.current_cmd = commd;

	jbot->Update();
	if (jbot->health)
	{
		jbot->Think();
	}
	else
	{
		jbot->Think_Dead();
	}
	controller->RunPlayerMove(&jbot->brain.current_cmd);
	return;
}


void JBotManager::CallOnMapChange(const char* newmapname)
{
	JI_CurMap::Map_Reload();
	JNavigation::Load(newmapname);
}

void JBotManager::Notify_Death(edict_t* victim)
{
	if (!victim) return;
	for (int i = 0; i < MAXBOTS; i++)
	{
		if (!bot_buffer[i]) return;
		if (bot_buffer[i] == victim)
		{
			botc_buffer[i].Notify_Death();
		}
		else
		{
			botc_buffer[i].Notify_Death_Other(victim);
		}
	}
}

void JBotManager::Notify_Spawn(edict_t* victim)
{
	if (!victim) return;
	for (int i = 0; i < MAXBOTS; i++)
	{
		if (bot_buffer[i] == victim)
		{
			botc_buffer[i].Notify_Spawn();
			return;
		}
	}
}

void JBotManager::Notify_Disconnect(edict_t* dudewhodidit)
{
	if (!dudewhodidit) return;
	//this func checks if its a bot
	RemoveBot(dudewhodidit);
}

void JBotManager::Notify_RoundEnd()
{
}

