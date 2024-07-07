#pragma once
#include "edict.h"
class CNavArea;
class JBotClass
{
public:
	bool active;
	CNavArea* last_area;
	edict_t* edict;
	edict_t* target;
	int team;
	int health;
	int maxhealth;
	float distance_moved_last_tick;
	float distance_moved_timer;
	Vector distance_moved_lastpos;

	Vector lookdir;
	JBotClass()
	{
		active = true;
		last_area = 0;
		edict = 0;
		lookdir = Vector(0, 1, 0);
	}

	void Target_Search();
	static JBotClass* Edict_To_JBot(edict_t* edic);
};

