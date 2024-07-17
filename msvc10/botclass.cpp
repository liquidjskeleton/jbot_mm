#include "botclass.h"
#include "func.h"
#include <IEngineTrace.h>
#include "botman.h"
#include "mathlib/mathlib.h"
#include "eiface.h"
#include "../../../../hl2sdk-tf2/game/shared/in_buttons.h"
#include "help.h"
#include "netprops.h"
#include "curmap.h"
#define SMALL_MOVEDISTANCE 30.0f
#define BOT_FOV_FAKE 54.0f
#define BOT_FOV_REAL (1.0f - (BOT_FOV_FAKE / 100.0f));

extern IBotManager* botmanager;
extern IEngineTrace* enginetrace;
extern IPlayerInfoManager* playerinfomanager;
extern IVEngineServer* engine;

extern edict_t* bot_buffer[MAXBOTS];
extern JBotClass botc_buffer[MAXBOTS];
extern bool j_stopexecution;

ConVar jbotdebugcvar("jbot_debug","0",0,"NOTE: using these debug commands will slow down your server and spam your console ESPECIALLY with large amounts of bots\nnonzero = print misc debug info\n1 = print new paths\n2 = print when new pathfinding is checked\n3 = print when pathfinding is unavailable\n4 = print PF_MoveAlong status\n5 = print when using pf_close_enough mode");
ConVar jb_lookatdir("jbot_debug_lookatdir","0",0,"look where im moving except when im shooting then look at them\n");
ConVar jb_snags("jbot_snags","0",0,"experimental: allow snags\n");
void jbotlog();
ConCommand jblog("jbot_dumpstatus", jbotlog);
ConVar cv_jb_lookatwdir("jbot_lookatwdir","1",0,"for testing: makes bots always look at the direction they want to move.");
extern ConVar cv_jnav_faraway;


#define IS_IN_DEBUG_MODE jbotdebugcvar.GetInt()
#define ifdebug if (IS_IN_DEBUG_MODE) 
#define ALLOW_SNAGS jb_snags.GetBool()

#define try_to_jump if (fmodf(brain.internaltimer,0.3f) <= 0.15f) brain.current_cmd.buttons |= IN_JUMP;

#define DEBUG_PRINTNEWPATHS 1
#define DEBUG_PRINTCHECKS 2
#define DEBUG_PRINTWHENNOPATHS 3
#define DEBUG_PRINTMOVEALONG 4
#define DEBUG_PRINTWHENCLOSEENOUGH 5

#define PF_DISTANCE_CLOSE cv_pf_distanceclose.GetFloat()//40.0f
#define PF_DISTANCE_VERY_CLOSE cv_pf_distanceveryclose.GetFloat() //1.0f

ConVar cv_pf_distanceclose("jbot_pf_distance_close", "42.0", 0, "how close a bot has to be to progress");
ConVar cv_pf_distanceveryclose("jbot_pf_distance_very_close", "5.8", 0, "how close a bot has to be to ignore all checks and force a progression");


void debug_vector(Vector vec)
{
	jprintf("(debugvector) %f, %f, %f\n",vec.x,vec.y,vec.z);
}

void JBotClass::Target_Search_Any()
{
	Target_Search();
	if (!target)
		Target_Search_Obj();

}

void JBotClass::Target_Search()
{
	//find a target
	edict_t* lasttarget = target;
	target = 0;
	target_position.Zero();
	target_is_player = false;

	for (int i = 0; i < 101; i++)
	{
		edict_t* it = engine->PEntityOfEntIndex(i);
		if (!it || it == edict || it->IsFree()) continue;
#undef GetClassName
		if (strcmp("player", it->GetClassName())) continue;
		
		JBotClass* jbot = Edict_To_JBot(it);
		if (jbot && !jbot->alive) continue;
		if (!TFPlayer_IsValidBotTarget(it)) continue;

		//okay
		Vector itpos = Entity_GetPosition(it);
		if (CanSeeObject(it) && IsInFov(itpos - current_position))
		{
			if (target != lasttarget) brain.last_successful_pathfind_result.canpathfind = false;
			
			target = it;
			target_position = itpos;
			target_is_player = true;
			break;
		}
	}
}

void JBotClass::Target_Search_Obj()
{
	target = 0;
	const MapObjectDataVector objdat = *JI_CurMap::GetStaticMapObjects();
	for (int i = 0; i < objdat.size(); i++)
	{
		MapObjectData dt = objdat[i];

		if (dt.m_iObjectType & MO_CATEGORY_OBJECTIVE)
		{
			TFTeam itteam = dt.m_iTeam;
			TFTeam idealteam = team;
			if (JI_CurMap::s_iCurrentGamemode == TF_GAMEMODE_CTF)
			{
				idealteam = team == 2 ? 3 : 2;
			}
			if (itteam != TF_TEAM_UNASSIGNED && itteam != idealteam)
			{
				continue;
			}
			target = dt.m_hEntity;
			target_position = dt.m_Position;
			target_is_player = false;
			Pathfind(dt.m_Position);
			return;
		}
	}
}
#define eyepos Vector(0,0,64)

bool JBotClass::CanSeeObject(edict_t* dic)
{
	Vector itpos = Entity_GetPosition(dic);
	Ray_t ray = Ray_t();
	//rays use 2 points not directions
	ray.Init(current_position + eyepos,itpos + eyepos);
	trace_t trace; // = trace_t();
	//enginetrace->ClipRayToEntity(ray, MASK_BLOCKLOS, dic->GetIServerEntity(),&trace);
	auto tracefilter = CTraceFilterWorldAndPropsOnly();
	enginetrace->TraceRay(ray, MASK_BLOCKLOS, &tracefilter, &trace);
	const CBaseEntity* ent = trace.m_pEnt;
	//if (!ent) return true;
	return !ent; //did we not collide w world
	//return ent != dic->GetUnknown()->GetBaseEntity();
	//return trace.m_pEnt == (dic->GetUnknown()->GetBaseEntity());
}

//dont use rn
bool JBotClass::CanSee(Vector location)
{
	Vector dir = location - current_position;
	Ray_t ray = Ray_t();
	ray.Init(current_position + eyepos, location);
	trace_t trace; // = trace_t();
	enginetrace->TraceRay(ray, MASK_BLOCKLOS, 0, &trace);
	float len = VectorLength(trace.startpos - trace.endpos);
	return fabsf(len - dir.Length()) <= 0.1f;
}

bool JBotClass::IsInFov(Vector direction)
{
	Vector angv;
	AngleVectors(brain.current_cmd.viewangles, &angv);
	float dot = angv.Dot(direction.Normalized());
	return dot >= BOT_FOV_REAL;
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
	if (!target_is_player) return;
	float dis = VectorLength(position - brain.last_pathfind_position);
	if (brain.movetothisposition.IsZero()) {
		dis = 99999.9f;
	}
	//dont waste resources trying to calculate a new path if they havent moved
	//also hacky way to see if we've respawned or something wack
	if (dis <= 100) return;
	CNavArea* theynavarea = nav_failsafe(position);
	if (theynavarea == brain.last_pathfind_navarea) return;
	brain.last_pathfind_navarea = theynavarea;
	int lole;
	if (IsAreaOnCurrentPath(theynavarea, &lole))
	{
		brain.current_pathind_end = lole;
		return;
	}
	float tdis = VectorLength(target_position - position);
	if (tdis > (cv_jnav_faraway.GetFloat() * 0.5f) && brain.internaltimer - brain.lastpath_timestamp < 20.0f) return;
	Pathfind(position);
}

bool JBotClass::IsAreaOnCurrentPath(CNavArea* area, int* number)
{
	if (!brain.last_successful_pathfind_result.canpathfind) return false;
	for (int i = 0; i < brain.last_successful_pathfind_result.disp.size(); i++)
	{
		if (brain.last_successful_pathfind_result.disp[i].second == area)
		{
			*number = i;
			return true;
		}
	}
	*number = 0;
	return false;
}

void JBotClass::PathfindToTargetIfNeeded()
{
	if (!target) return;


	//todo check if the navarea they're on hasnt changed
	PathfindIfNeeded(target_position);
}

void JBotClass::LookAt(Vector lookhere,float smoothness)
{
	const Vector smoth = (lookhere * (1 - smoothness)) + (lookdir * smoothness);
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
	if (IS_IN_DEBUG_MODE == DEBUG_PRINTCHECKS)
		jprintf("pathfind movealong call\n");
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
		if (IS_IN_DEBUG_MODE)
		{
			jprintf("path of size 1. redundant & discarding it rn to save resources\n");
		}
		goto label_finish;
		return;
	}
	if (brain.current_pathind_index == -1)
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
	Vector vdif2d = vdif;
	vdif2d.z = 0;
	float distance = VectorLength(vdif2d);

	if (distance < 30.0f && vdif.z < -8)
	{
		try_to_jump;
		//brain.current_cmd.buttons |= IN_JUMP; //jump jump
		//brain.current_cmd.buttons |= IN_DUCK; 
	}
	nav_timer += 0.05; //yee
	int curind = brain.current_pathind_index - 1;
	if (curind == -1) curind = 0;
	auto& it_current = this->brain.last_successful_pathfind_result.disp[brain.current_pathind_index];
	auto& it = this->brain.last_successful_pathfind_result.disp[curind];

	//we are basically on the seam between two navs but detected to be on the first one

	const bool is_on_nav = last_area == it.second || (distance < PF_DISTANCE_VERY_CLOSE);

	if (IS_IN_DEBUG_MODE == DEBUG_PRINTMOVEALONG) jprintf("isonnav = %i , distance check = %i ; distance=%.1f\n",is_on_nav, distance < PF_DISTANCE_CLOSE,distance);
	if (is_on_nav && distance < PF_DISTANCE_CLOSE)
	{
		if (IS_IN_DEBUG_MODE) jprintf("pathmovealong: Progressing\n");
		//we made it
		brain.current_pathind_index--;
		nav_timer = 0;
		nav_snag_timer = 0;
		//auto it = this->brain.last_successful_pathfind_result.disp[curind];
		auto next = this->brain.last_successful_pathfind_result.disp[curind - 1];
		if (curind - 1 <= -1) { next = it; }
		float distancetotarget = VectorLength(target_position - current_position);
		jprintf("made it to %i (id=%i) (next=%i) (distancetowp%.2f)(distancetotpos=%.2f)\n", brain.current_pathind_index, it.second->m_id,next.second->m_id,distance,distancetotarget);
		if (curind == brain.current_pathind_end)
		{
			brain.last_successful_pathfind_result.canpathfind = false; //cheap but hi
		}
		brain.movetothisposition = JNavigation::GetMoveToPosition(it.second,next.second,target_position);
		vdif = brain.movetothisposition - current_position;
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
	alive = 0;
	JNavCell* curcell = navhubex.GetCellFromArea(last_area);
	if (curcell && team >= 2 && maxhealth)
	{
		curcell->team_data[team - 2].danger += 1.0f;
	}
}

void JBotClass::Notify_Death_Other(edict_t* guywhodied)
{
	if (guywhodied == target)
	{
		target = 0;
	}
	//need a way to make sure this gets called once
#if 0
	JBotClass* guy = Edict_To_JBot(guywhodied);
	if (!guy && !guywhodied->IsFree() && guywhodied->GetIServerEntity())
	{
		//handle jbot stuff for them :)
		JNavCell* curcell = navhubex.GetCellFromArea(nav_failsafe(Entity_GetPosition(guywhodied)));
		TFTeam theyteam = Entity_GetTeamNum(guywhodied);
		if (theyteam == 0)

		if (curcell)
		{
			curcell->team_data[theyteam - 2].danger += 1.0f;
		}
	}
#endif
}

void JBotClass::Notify_Spawn()
{
	jprintf("respawn detected\n");
	brain = JBotBrain(); 
	brain.valid_state = true;
	alive = 1;
	auto plyrinfo = playerinfomanager->GetPlayerInfo(edict);
	if (!plyrinfo) return; //havent been "connected" yet. chances are we'll get another roll later
	const Vector pos = plyrinfo->GetAbsOrigin();
	current_position = pos; //y not 
	nav_timer = 0;
	nav_snag_timer = 0;
	last_area = 0;
	target = 0;
	CNavArea* spawnarea = nav_failsafe(pos);
	if (spawnarea)
	{
		JNavCell* jnav = navhubex.GetCellFromArea(spawnarea);
		if (jnav)
		{
			jnav->is_spawn = true;
		}
	}
	tfclass = J_RandomRange(TF_CLASS_SCOUT,TF_CLASS_ENGINEER);
	if (tfclass < 1 || tfclass >= 10)
	{
		jprintf("tfclass was %i\n", tfclass);
		tfclass = 10;
	}
	tfclass = TFPlayer_GetTFClass(edict);
}

void JBotClass::TranslateForward()
{
//#define WALK_SPEED 400
	const float WALK_SPEED = s_tfclassspeeds[tfclass];
#if 0
	Vector whereilookingrn;
	const Vector whereiwannalook = wishdir.Normalized();
	Vector whereiwannalook_right = whereiwannalook;
	whereiwannalook_right.x = -whereiwannalook.y;
	whereiwannalook_right.y = -whereiwannalook.x;
	AngleVectors(brain.current_cmd.viewangles, &whereilookingrn);
	if (wishdir.IsZero()) return;
	const float dot = whereiwannalook.Dot(whereilookingrn);
	const float dot_side = whereiwannalook.Dot(whereiwannalook_right);
	brain.current_cmd.forwardmove = dot * WALK_SPEED;
	brain.current_cmd.sidemove = (dot_side * WALK_SPEED);
#else
	if (cv_jb_lookatwdir.GetBool())
	{
		const Vector whereiwannalook = wishdir.Normalized();
		QAngle ang;
		VectorAngles(whereiwannalook, ang);
		if (ang.x >= 89) ang.x = 89;
		brain.current_cmd.viewangles = ang;
		if (wishdir.IsZero()) return;
		brain.current_cmd.forwardmove = WALK_SPEED;
		brain.current_cmd.sidemove = 0 * WALK_SPEED;
	}
	else
	{
		if (brain.current_cmd.viewangles.x >= 89) brain.current_cmd.viewangles.x = 89;
		if (wishdir.IsZero()) return;
		Vector whereimlooking;
		AngleVectors(brain.current_cmd.viewangles,&whereimlooking);
		whereimlooking.z = 0;
		whereimlooking.NormalizeInPlace();
		Vector normwdir = wishdir;
		normwdir.z = 0;
		normwdir.NormalizeInPlace();
		float dot = normwdir.Dot(whereimlooking);
		float forwardmove = dot;
		brain.current_cmd.forwardmove = forwardmove * WALK_SPEED;
		float sidemove = (1 - fabsf(dot)) * (dot<0?1:-1);
		brain.current_cmd.sidemove = sidemove * WALK_SPEED;
	}
#endif
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

DO_NOT_USE_PLEASE bool JBotClass::CheckForRespawn()
{

}

void JBotClass::Update()
{
	if (target && target_is_player)
	{
		auto* infmang = playerinfomanager; 
		auto* plyinfo = infmang->GetPlayerInfo(target); 
		if (plyinfo) 
		{
			target_position = plyinfo->GetAbsOrigin(); 
			if (plyinfo->GetTeamIndex() <= 2 || plyinfo->GetHealth() <= 0)
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
	//if (CheckForRespawn())
	//{
	//	Notify_Spawn();
	//}
	//this isnt being set properly for some reason so im just gonna set it here okay have fun
	last_area = nav_failsafe(current_position);
	brain.internaltimer += (1 / 60.0f);
	brain.current_cmd.Reset();
}

void JBotClass::Think()
{
	if (!target)
	{
		Target_Search_Any();
	}
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
		if (IS_IN_DEBUG_MODE == DEBUG_PRINTWHENNOPATHS) jprintf("no path?\n");
	}

#if 0
	if (last_area)
	{
		//am i in the air
		if (current_position.z < (fmaxf(last_area->m_nwCorner.z, last_area->m_seCorner.z) - 5))
		{
			brain.current_cmd.buttons |= IN_DUCK;
		}
	}
#endif
	CNavArea* curarea = last_area;
	
	JNavCell* curcell = navhubex.GetCellFromArea(curarea);

	if (curcell && !curcell->is_spawn && has_pathfind)
	{
		if ((brain.statictimer % 20) == 0)
		{
			curcell->team_data[team - 2].popularity += 0.02f;
		}

		if (nav_timer > 8.0f)
		{
			if (fmodf(nav_timer,1.0f) <= 0.2f)
			brain.current_cmd.buttons |= IN_JUMP;

			//still still
			nav_snag_timer++;
			//move around a bit.. try to nudge out
			Vector twiddle;
			twiddle.x = -twiddle.y;
			twiddle.y = -wishdir.x;
			twiddle.z = 0;
			twiddle *= sinf(brain.internaltimer) * ((nav_timer - 8.0f) * 0.5f) * 0.3f;
			//if (fmod(distance_moved_snag_timer,2.5988888f) > 2.5) //fmod is weird cus of float inaccuracy and idk i dont want any weird bugs
			//{
			//	//uhh... move towards center?
			//	Vector dif = brain.movetothisposition;
			//	brain.movetothisposition
			//}
			if (nav_snag_timer > 8000 && ALLOW_SNAGS)
			{
				//alright this has gone on long enough
				nav_timer = -1;
				jprint("new snag\n");
				CNavArea* area = nav_failsafe(current_position);
				if (!area) return;
				curcell->CreateSnag(area, current_position, wishdir);
				last_area = area;
			}
		}
	}
	else
	{
		nav_timer = 0.0f;
	}
	//check for snags and if there are any move away from them
	if (curcell)
	{
		auto snags = curcell->snags;
		for (int i = 0; i < snags.size(); i++)
		{
			Vector dis = snags[i].position - current_position;
			float len = dis.Length();
			if (len < snags[i].size)
			{
				wishdir += dis * (snags[i].size - len);
			}
		}
	}


	LookAt(wishdir, 0.8f);

	if (target_is_player && target && CanSeeObject(target))
	{
		LookAt(target_position);
		brain.current_cmd.buttons |= IN_ATTACK;
	}

	TranslateForward();
	DBG_Print_Status();
}

void JBotClass::Think_Dead()
{
	//...
}

void JBotClass::Pathfind(Vector position)
{
	if (IS_IN_DEBUG_MODE == DEBUG_PRINTCHECKS)
	{
		jprintf("new pathfind call requested\nmypos:");
		debug_vector(current_position); jprintf("pf_targetpos:");
		debug_vector(position);
	}
	brain.current_pathind_index = -1;
	brain.current_pathind_end = 0;
	CNavArea* myarea = 0, * theyarea = 0;
	Vector theypos = position;
	Vector mypos = current_position;
	auto* theyjbot = target_is_player ? Edict_To_JBot(target) : 0;
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
	pfinput.team = (TFTeam)team; //raaah i hate this language
	if (IS_IN_DEBUG_MODE == DEBUG_PRINTWHENCLOSEENOUGH)
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

		float distoprev = VectorLength(closestpoint - brain.movetothisposition);
		//what is this?
		if (distoprev < 80.0f && nav_timer <= 0.3f)
		{
			float dot = brain.movetothisposition.Dot(closestpoint);
			if (dot < 0.2f)
			{
				brain.movetothisposition = closestpoint;
			}
		}
		else
		{
			brain.movetothisposition = closestpoint;
		}
		

		brain.last_pathfind_navarea = des.second;
		dir = closestpoint - current_position;
		distance = VectorLength(dir);
		brain.last_successful_pathfind_result = canpath;
		brain.lastpath_timestamp = brain.internaltimer;
		//dir = dir.Normalized();
		//dir *= 200.0f;
#if PATH_DEBUGGING //for debugging :)
		if ((IS_IN_DEBUG_MODE==1)) {
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
		if (IS_IN_DEBUG_MODE == DEBUG_PRINTNEWPATHS)
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

void jbotlog() //jbot_dumpstatus
{
#define printspos(area) jprintf(".. pf: setpos for (%s) will be printed below\n",#area); jprintf("setpos %.2f %.2f %.2f\n",area.x, area.y, area.z + 8);
	for (int i = 0; i < MAXBOTS; i++)
	{
		if (bot_buffer[i] && botc_buffer[i].active)
		{
			auto* jbot = &botc_buffer[i];
			jprintf("bot %i (%x)\n", i, i);
			if (!botc_buffer[i].brain.valid_state)
			{
				jprintf("[WARNING:] JBOT is NOT in a valid state. did you just add them? dont do that we would have crashed if not for this check\n");
				continue;
			}
			jprintf(".. pathfindstatus=%i\n",jbot->brain.last_successful_pathfind_result.canpathfind);
			if (jbot->brain.last_successful_pathfind_result.canpathfind)
			{
				jprintf(".. pf: current index=%i\n", jbot->brain.current_pathind_index);
				jprintf(".. pf: current end=%i\n", jbot->brain.current_pathind_end);
				auto* area = &jbot->brain.last_successful_pathfind_result.disp[jbot->brain.current_pathind_index];
				jprintf(".. pf: current target area score=%f\n", area->first);
				jprintf(".. pf: current target area nav id=%i\n", area->second->m_id);
				jprintf(".. pf: current target area nav center setpos will be printed below\n");
				jprintf("setpos %.2f %.2f %.2f\n",area->second->m_center.x,area->second->m_center.y,area->second->m_center.z + 8);
				jprintf(".. pf: current brain:whereishouldwalkto will be printed below\n");
				jprintf("setpos %.2f %.2f %.2f\n",jbot->brain.movetothisposition.x, jbot->brain.movetothisposition.y, jbot->brain.movetothisposition.z + 8);
				jprintf(".. pf: brain pf status dist and whatnots now\n");
				float distance = VectorLength(jbot->current_position - jbot->brain.movetothisposition);
				jprintf(".. brainpf: Distance to MTPos = %.2f (Needs to be below %.1f to progress)\n",distance, PF_DISTANCE_CLOSE);
				jprintf(".. brainpf: [notes] PF_DISTANCE_VERY_CLOSE is %f.\n",PF_DISTANCE_VERY_CLOSE);
				if (jbot->last_area)
				{
					jprintf(".. brainpf: MTNav = %i, Current = %i\n", area->second->m_id, jbot->last_area->m_id);
					printspos(jbot->last_area->m_center);
				}
				else
				jprintf(".. brainpf: MTNav = %i, Current = NULLREF\n",area->second->m_id);
				

			}
		}
	}
}
