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
extern IBotManager* botmanager;
extern IEngineTrace* enginetrace;
extern IPlayerInfoManager* playerinfomanager;
extern IVEngineServer* engine;

edict_t* bot_buffer[MAXBOTS];
JBotClass botc_buffer[MAXBOTS];

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
	sprintf_s(buffer, "JBot_%x",counter);
	
	edict_t* ptr = botmanager->CreateBot(buffer);
	if (!ptr) return 0;
	bot_buffer[counter] = ptr;
	botc_buffer[counter] = JBotClass();
	auto jclas = botc_buffer[counter];
	//basic init
	Player_JoinTeam(ptr, (counter%2)+2); //join red team
	jclas.edict = ptr;
	jclas.active = 1;
	jclas.Notify_Spawn();
#if 1
	//im so sorry
	CBaseEntity* entptr = ptr->GetIServerEntity()->GetBaseEntity();
	int nclass = 1;// (counter % 9) + 1;
	*((unsigned char*)(entptr)+0x1d4c) = nclass; //pick the scout class (m_iClass)
	*((unsigned char*)(entptr)+0x22c8) = nclass; //pick the scout class (m_iDesiredClass)
	botmanager->GetBotController(ptr)->RemoveAllItems(false);
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
		Player_Respawn(dic);
		jbot->Notify_Spawn();
		return;
	}
	auto* controller = botmanager->GetBotController(dic);
	CBotCmd commd;
	Q_memset(&commd, 0, sizeof(CBotCmd));
	jbot->current_position = controller->GetLocalOrigin();
	jbot->brain.current_cmd = commd;

	jbot->Update();
	jbot->Think();
	controller->RunPlayerMove(&jbot->brain.current_cmd);
	return;
}


void JBotManager::CallOnMapChange(const char* newmapname)
{
	JNavigation::Load(newmapname);
}

