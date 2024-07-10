#pragma once
#include "edict.h"
#include "navigation.h"
class CNavArea;
class CBotCmd;
#include "iplayerinfo.h"
typedef CBotCmd JUserCmd;

//the brain is life-exclusive. it gets reset when they die
class JBotBrain
{
public:
	Vector movetothisposition;
	bool movingrn;
	Vector last_pathfind_position;
	CNavArea* last_pathfind_navarea;
	JNav_PathfindResult last_pathfind_result;
	JNav_PathfindResult last_successful_pathfind_result;
	int current_pathind_index;
	JUserCmd current_cmd;
	int statictimer;

	JBotBrain()
	{
		Q_memset(this, 0, sizeof(JBotBrain));
	}


};
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
	//for snags
	float distancemoved;
	float distance_moved_last_tick;
	float distance_moved_timer;
	Vector distance_moved_lastpos;
	int distance_moved_snag_timer;

	Vector lookdir; //where i wanna look
	Vector wishdir; //where i wanna go

	Vector target_position;
	Vector current_position;
	JBotBrain brain;

	JBotClass()
	{
		active = true;
		last_area = 0;
		edict = 0;
		lookdir = Vector(0, 1, 0);
		brain = JBotBrain();
		target_position = Vector(0, 0, 0);
		target = 0;
	}

	void Target_Search();
	static JBotClass* Edict_To_JBot(edict_t* edic);

	void Update();
	void Think();
	void Pathfind(Vector position);
	void PathfindIfNeeded(Vector position);

	void PathfindToTargetIfNeeded();
	void LookAt(Vector lookhere,float smoothness = 0.0f);
	void Pathfind_MoveAlong();

	void Notify_Death();
	void Notify_Spawn();
private:
	void TranslateForward();
	void DBG_Print_Status();
};

