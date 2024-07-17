#pragma once
#include "edict.h"
#include "navigation.h"
class CNavArea;
class CBotCmd;
#include "iplayerinfo.h"
typedef CBotCmd JUserCmd;
#define DO_NOT_USE_PLEASE
#include "botprofile.h"
//the brain is life-exclusive. it gets reset when they die
class JBotBrain
{
public:
	bool valid_state; //to prevent jbot_dumpstatus from crashing when reading from freshly added jbot
	Vector movetothisposition;
	bool movingrn;
	//pathfind memory
	Vector last_pathfind_position;
	CNavArea* last_pathfind_navarea;
	JNav_PathfindResult last_pathfind_result;
	JNav_PathfindResult last_successful_pathfind_result;
	int current_pathind_index; //which part of the path are we on right now?
	int current_pathind_end; //usually zero
	JUserCmd current_cmd;
	int statictimer;
	float internaltimer; //increments every tick
	float lastpath_timestamp; //relative to internaltimer

	//called on respawn, or something else
	JBotBrain()
	{
		Q_memset(this, 0, sizeof(JBotBrain));
	}


};
class JBotClass
{
public:
	bool active;
	bool alive;
	CNavArea* last_area;
	edict_t* edict;
	edict_t* target;
	bool target_is_player;
	int team;
	int health;
	int maxhealth;
	TFClass tfclass;
	int maxspeed;
	//for snags
	float nav_timer;
	int nav_snag_timer;

	Vector lookdir; //where i wanna look
	Vector wishdir; //where i wanna go

	Vector target_position;
	Vector current_position;
	JBotBrain brain;
	JBotProfile profile;

	//called when a bot is added to a server
	JBotClass()
	{
		active = true;
		last_area = 0;
		edict = 0;
		lookdir = Vector(0, 1, 0);
		brain = JBotBrain();
		brain.valid_state = false;
		target_position = Vector(0, 0, 0);
		target = 0;
		alive = 0;
		tfclass = TF_CLASS_SCOUT;
		profile = JBotProfile(TF_TEAM_UNASSIGNED);
		target_is_player = false;
	}

	inline float HealthPercentage() const noexcept { return (float)health / maxhealth; }

	void Target_Search_Any(); //combo of below funcs for easy
	void Target_Search(); //locate players
	void Target_Search_Obj(); //locate objectives and health and stuff
	bool CanSeeObject(edict_t* dic);
	bool CanSee(Vector location);
	bool IsInFov(Vector direction);
	static JBotClass* Edict_To_JBot(edict_t* edic);

	DO_NOT_USE_PLEASE bool CheckForRespawn(); 
	void Update();
	void Think();
	void Think_Dead();
	void Pathfind(Vector position);
	void PathfindIfNeeded(Vector position);
	bool IsAreaOnCurrentPath(CNavArea* area,int* number);

	void PathfindToTargetIfNeeded();
	void LookAt(Vector lookhere,float smoothness = 0.0f);
	void Pathfind_MoveAlong();

	void Notify_Death();
	void Notify_Death_Other(edict_t* guywhodied); //called when someone else dies
	void Notify_Spawn();
private:
	void TranslateForward();
	void DBG_Print_Status();
};

