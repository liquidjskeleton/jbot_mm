#pragma once
#include <stdio.h>
#include "edict.h"
#include "toolframework/iserverenginetools.h"
#include "toolframework/itoolentity.h"
#include "iplayerinfo.h"

#define MAXBOTS 100
static class JBotManager
{ public:
	static edict_t* CreateBot();
	static void Bot_ThinkAll();
	static void RemoveBot(edict_t* num);
	static void CallOnMapChange(const char* newmapname);
private:
	static void Bot_Think(int index);
};
