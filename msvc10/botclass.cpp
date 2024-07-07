#include "botclass.h"
#include "func.h"
#include <IEngineTrace.h>
#include "botman.h"
#include "mathlib/mathlib.h"
#include "eiface.h"

extern IBotManager* botmanager;
extern IEngineTrace* enginetrace;
extern IPlayerInfoManager* playerinfomanager;
extern IVEngineServer* engine;

extern edict_t* bot_buffer[MAXBOTS];
extern JBotClass botc_buffer[MAXBOTS];

void JBotClass::Target_Search()
{
	//find a target
	target = 0;

	for (int i = 0; i < 101; i++)
	{
		edict_t* it = engine->PEntityOfEntIndex(i);
		if (!it || it == edict || it->IsFree()) continue;
#undef GetClassName
		if (strcmp("player", it->GetClassName())) continue;
		int theyteam = Player_GetTeam(it);
		if (theyteam < 2 || theyteam == team) continue;
		if (Player_GetHealth(it) <= 0) continue;
		target = it;
	}
}

JBotClass* JBotClass::Edict_To_JBot(edict_t* edic)
{
	for (int i = 0; i < MAXBOTS; i++)
	{
		if (bot_buffer[i] == edic) { return &botc_buffer[i]; }
	}
	return 0;
}
