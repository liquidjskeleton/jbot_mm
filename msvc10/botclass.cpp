#include "botclass.h"
#include "func.h"
#include <IEngineTrace.h>
#include "botman.h"
#include "mathlib/mathlib.h"
#include "eiface.h"
#include "../../../../hl2sdk-tf2/game/shared/in_buttons.h"
#include "help.h"

#define SMALL_MOVEDISTANCE 8.0f

extern IBotManager* botmanager;
extern IEngineTrace* enginetrace;
extern IPlayerInfoManager* playerinfomanager;
extern IVEngineServer* engine;

extern edict_t* bot_buffer[MAXBOTS];
extern JBotClass botc_buffer[MAXBOTS];
extern bool j_stopexecution;

ConVar jbotdebugcvar("jbot_debug","0",0,"1 = print new paths\n2 = print when new pathfinding is checked\n3 = print when pathfinding is unavailable\n");

#define IS_IN_DEBUG_MODE jbotdebugcvar.GetInt()
#define ifdebug if (IS_IN_DEBUG_MODE) 
void debug_vector(Vector vec)
{
	jprintf("(debugvector) %f, %f, %f\n",vec.x,vec.y,vec.z);
}

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

void JBotClass::PathfindIfNeeded(Vector position)
{
	if (brain.movetothisposition == position) return;
	float dis = VectorLength(position - brain.last_pathfind_position);
	if (brain.movetothisposition.IsZero()) {
		dis = 99999.9f;
	}
	//dont waste resources trying to calculate a new path if they havent moved
	//also hacky way to see if we've respawned or something wack
	if (dis <= 100 || distancemoved > 800) return;
	CNavArea* theynavarea = nav_failsafe(position);
	if (theynavarea == brain.last_pathfind_navarea) return;
	brain.last_pathfind_navarea = theynavarea;
	Pathfind(position);
}

void JBotClass::PathfindToTargetIfNeeded()
{
	if (!target) return;


	//todo check if the navarea they're on hasnt changed
	PathfindIfNeeded(target_position);
}

void JBotClass::LookAt(Vector lookhere,float smoothness)
{
	const Vector smoth = lookhere;// (lookhere * (1 - smoothness)) + (lookdir * smoothness);
	//if (IS_IN_DEBUG_MODE)
	//{
	//	jprintf("Lookat is: ");
	//	debug_vector(smoth);
	//}
	QAngle ang;
	VectorAngles(smoth, ang);
	if (ang.x >= 89.9f)
	{
		//refuse
		ang.x = 89.0f;
	}
	brain.current_cmd.viewangles = ang;
	lookdir = smoth.Normalized();
}

void JBotClass::Pathfind_MoveAlong()
{
	if (IS_IN_DEBUG_MODE == 2)
		jprintf("pathfind call\n");
	if (!brain.last_successful_pathfind_result.canpathfind)
	{
		if (IS_IN_DEBUG_MODE)
		jprintf("cant pathfind\n");
		return;
	}
	Vector pf_gohere;
	Vector vdif;
	if (brain.last_successful_pathfind_result.disp.size() == 1)
	{
		//we are in the same navarea as the target, so just move towards them
		vdif = target_position - current_position;
		brain.movetothisposition = target_position;
		goto label_finish;
		return;
	}
	if (brain.current_pathind_index == 0)
	{
		pf_gohere = this->brain.movetothisposition;
		brain.current_pathind_index = this->brain.last_successful_pathfind_result.disp.size() - 1;
		this->brain.movetothisposition = pf_gohere;
	}
	else
	{
		pf_gohere = this->brain.movetothisposition;
	}
	 
	vdif = pf_gohere - current_position;

	float distance = VectorLength(vdif);

	if (distance < 120.0f)
	{
		//we made it
		brain.current_pathind_index--;
		int curind = brain.current_pathind_index - 1;
		if (curind == -1) curind = 0;
		auto it = this->brain.last_successful_pathfind_result.disp[curind];
		jprintf("made it to %i (id=%i)\n", brain.current_pathind_index, it.second->m_id);
		if (curind == 0)
		{
			brain.last_successful_pathfind_result.canpathfind = false; //cheap but hi
		}
		brain.movetothisposition = JNavigation::GetMoveToPosition(it.second,target_position);
	}
	label_finish:
	//go to it
	Vector vdiflook = vdif;
	//vdiflook.z /= 40.0f;
	vdiflook.NormalizeInPlace();
	LookAt(vdiflook,0.98f);
	wishdir = (vdif).Normalized();

}

void JBotClass::Notify_Death()
{
	health = 0;
}

void JBotClass::Notify_Spawn()
{
	brain = JBotBrain(); 
	const Vector pos = playerinfomanager->GetPlayerInfo(edict)->GetAbsOrigin();
	current_position = pos; //y not 
	distance_moved_lastpos = pos;
	distance_moved_timer = -2; 
	distance_moved_last_tick = 1;
	distance_moved_snag_timer = 0;
	CNavArea* spawnarea = nav_failsafe(pos);
	if (spawnarea)
	{
		JNavCell* jnav = navhubex.GetCellFromArea(spawnarea);
		if (jnav)
		{
			jnav->is_spawn = true;
		}
	}
}

void JBotClass::TranslateForward()
{
#define WALK_SPEED 400
	Vector whereilookingrn;
	const Vector whereiwannalook = wishdir.Normalized();
	AngleVectors(brain.current_cmd.viewangles, &whereilookingrn);
	const float dot = whereiwannalook.Dot(whereilookingrn);
	brain.current_cmd.forwardmove = dot * WALK_SPEED;
	brain.current_cmd.sidemove = ((1 - dot) * WALK_SPEED);
}

void JBotClass::DBG_Print_Status()
{
	if (0 && IS_IN_DEBUG_MODE)
	{
		jprintf("--v jbot! debug status\n");
		jprintf("brain:%x\n", &brain);
		jprintf("Cmd:Movedir (relative):%f,%f,%f\n", brain.current_cmd.forwardmove, brain.current_cmd.sidemove, brain.current_cmd.upmove);
		jprintf("Cmd:ViewAngles:%f,%f,%f\n", brain.current_cmd.viewangles.x, brain.current_cmd.viewangles.y, brain.current_cmd.viewangles.z);
		jprintf("--^ jbot! debug status\n");
	}
	
}

void JBotClass::Update()
{
	if (target)
	{
		auto* infmang = playerinfomanager; 
		auto* plyinfo = infmang->GetPlayerInfo(target); 
		if (plyinfo) 
		{
			target_position = plyinfo->GetAbsOrigin(); 
			if (plyinfo->GetTeamIndex() <= 2)
			{
				//they joined spectator
				target = 0;
			}
		}
		else
		{
			//what the hell?
			target = 0;
		}
	}
	Vector dismov = current_position - distance_moved_lastpos;
	dismov.z = 0;
	distancemoved = VectorLength(dismov);
	distance_moved_lastpos = current_position;
	distance_moved_last_tick = distancemoved;
	if (distancemoved < SMALL_MOVEDISTANCE)
	{
		distance_moved_timer += 0.05;
	}
	else
	{
		distance_moved_timer = 0;
	}
	brain.current_cmd.Reset();
}

void JBotClass::Think()
{
	if (!target) { Target_Search(); }
	if (target)
	{
		PathfindToTargetIfNeeded();
	}

	bool has_pathfind = this->brain.last_successful_pathfind_result.canpathfind;

	if (has_pathfind)
	{
		Pathfind_MoveAlong();
	}
	else
	{
		if (IS_IN_DEBUG_MODE == 3) jprintf("no path?\n");
	}

	if (last_area)
	{
		//am i in the air
		if (current_position.z < (fmaxf(last_area->m_nwCorner.z, last_area->m_seCorner.z) - 5))
		{
			brain.current_cmd.buttons |= IN_DUCK;
		}
	}
	CNavArea* curarea = last_area;
	
	JNavCell* curcell = navhubex.GetCellFromArea(curarea);


	if (distance_moved_timer > 8.0f && curcell && !curcell->is_spawn && has_pathfind)
	{
		brain.current_cmd.buttons |= IN_JUMP;
		if (distance_moved_last_tick > SMALL_MOVEDISTANCE)
		{
			distance_moved_timer = -4.0f;
			distance_moved_snag_timer = 0;
		}
		else
		{
			distance_moved_timer = 3.0f;
			distance_moved_snag_timer++;
		}
		if (distance_moved_snag_timer > 30)
		{
			//alright this has gone on long enough
			distance_moved_snag_timer = 20;
			jprint("new snag\n");
			CNavArea* area = nav_failsafe(current_position);
			if (!area) return;
			curcell->CreateSnag(area,current_position);
			last_area = area;
		}
	}
	//check for snags and if there are any move away from them
	if (curcell)
	{
		auto snags = curcell->snags;
		for (int i = 0; i < snags.size(); i++)
		{
			Vector dis = snags[i].position - current_position;
			if (dis.Length() < snags[i].size)
			{
				wishdir -= dis.Normalized();
			}
		}
	}
	//LookAt(wishdir, 0.3f);

	if (target)
	{
		float distance = VectorLength(current_position - target_position);
		if (distance < 800.0f && distance >= 2.0f)
		{
			LookAt(target_position - current_position,0.1f);
			brain.current_cmd.buttons |= IN_ATTACK;
			brain.current_cmd.weaponselect = 1;
		}
		if (distance < 200.0f)
		{
			brain.current_cmd.weaponselect = 2;
		}
		if (distance < 50.0f)
		{
			brain.current_cmd.weaponselect = 3;
		}
	}

	TranslateForward();
	DBG_Print_Status();
}

void JBotClass::Pathfind(Vector position)
{
	if (IS_IN_DEBUG_MODE == 2)
	{
		jprintf("new pathfind call requested\nmypos:");
		debug_vector(current_position); jprintf("pf_targetpos:");
		debug_vector(position);
	}
	brain.current_pathind_index = 0;
	CNavArea* myarea = 0, * theyarea = 0;
	Vector theypos = position;
	Vector mypos = current_position;
	auto* theyjbot = Edict_To_JBot(target);
	for (int i = 0; i < navhub.m_areas.size(); i++)
	{

		if (navhub.m_areas[i].Contains(mypos)) { myarea = &navhub.m_areas[i]; }
		if (navhub.m_areas[i].Contains(theypos)) { theyarea = &navhub.m_areas[i]; }
	}
	JNav_PathfindResult canpath;
	if (myarea) {
		last_area = myarea;
	}
	if (!myarea && last_area)
	{
		myarea = last_area;
	}


	if (!theyarea && theyjbot && theyjbot->last_area)
	{
		theyarea = theyjbot->last_area;
	}
	else if (theyjbot)
	{
		theyjbot->last_area = theyarea;
	}

	if (!theyarea)
	{
		theyarea = nav_failsafe(theypos);
		if (!theyarea)
		{
			ifdebug jprintf("pathfinding: failed to find a nav area for destination even with failsafe. no idea how this happened\n");
		}
	}

	if (!myarea || !theyarea) {
		return;
	}

	Vector dir;
	float distance;
	QAngle lookdir;

	JNav_PathfindInput pfinput;
	pfinput.jnav_close_enough = !brain.last_pathfind_result.canpathfind;
	if (IS_IN_DEBUG_MODE == 5)
	{
		printf("pathfinding use close enough : %i\n",pfinput.jnav_close_enough);
	}

#define PATH_DEBUGGING 1
	canpath = JNavigation::Pathfind(myarea, theyarea, theypos, mypos, pfinput);
	brain.last_pathfind_result = canpath;
#if PATH_DEBUGGING 
	static bool pf_debug = 1;
#endif
	if (canpath.canpathfind)
	{
		int desindex = canpath.disp.size() - 1;
		//if (canpath.disp.size() > 1) desindex--;
		const DisP des = canpath.disp[desindex];
		jprint("new path!\n");
		const Vector closestpoint = JNavigation::ClosestPointOnArea(myarea, des.second->m_center);
		brain.last_pathfind_position = theypos;
		brain.movetothisposition = closestpoint;
		brain.last_pathfind_navarea = des.second;
		dir = closestpoint - current_position;
		distance = VectorLength(dir);
		brain.last_successful_pathfind_result = canpath;
		if (dir.z < 0) {
			brain.current_cmd.buttons |= IN_JUMP;
		}
		//dir = dir.Normalized();
		//dir *= 200.0f;
#if PATH_DEBUGGING //for debugging :)
		if (pf_debug || (IS_IN_DEBUG_MODE==1)) {
			jprintf("-------\n");
			jprintf("start navid = %i\n", myarea->m_id);
			jprintf("end navid = %i\n", theyarea->m_id);
			jprintf("total navs = %i\n", canpath.disp.size());
			for (int i = 0; i < canpath.disp.size(); i++)
			{
				auto dis = canpath.disp[i];
				jprintf("- %i , navid = %i, score = %f, position = %f,%f,%f  \n", i, dis.second->m_id, dis.first, dis.second->m_center.x, dis.second->m_center.y, dis.second->m_center.z);
			}
			pf_debug = 0;
			//j_stopexecution = true;
		}
#endif
	}
	else
	{
		//label_failedtopathfind:
		//dir = theyinf->GetAbsOrigin() - controller->GetLocalOrigin();
#if PATH_DEBUGGING
		if (IS_IN_DEBUG_MODE == 1)
		{
			jprintf("-------\n");
			jprintf("cant path\n");
			jprintf("canpath.canpathfind = %i\n", *(char*)&canpath.canpathfind);
			jprintf("canpath.disp.size() = %i\n", canpath.disp.size());
			for (int i = 0; i < canpath.disp.size(); i++)
			{
				auto dis = canpath.disp[i];
				if (!dis.second)
				{
					jprintf("- %i = NULLREF (first=%f)\n", i, dis.first);
					continue;
				}
				jprintf("- %i , navid = %i, score = %f, position = %f,%f,%f  \n", i, dis.second->m_id, dis.first, dis.second->m_center.x, dis.second->m_center.y, dis.second->m_center.z);
			}

		}
#endif
	}
	//if (distance) //go the distance
	//{
	//	Vector lookatthisdir = dir;
	//	//lookatthisdir *= 30;
	//	VectorAngles(lookatthisdir, lookdir);
	//	lookdir.x /= 20.0f;
	//	brain.current_cmd.viewangles = lookdir;
	//}
}

