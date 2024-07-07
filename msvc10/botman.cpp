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
	//basic init
	Player_JoinTeam(ptr, (counter%2)+2); //join red team

#if 1
	//im so sorry
	CBaseEntity* entptr = ptr->GetIServerEntity()->GetBaseEntity();
	int nclass = 4;// (counter % 9) + 1;
	*((unsigned char*)(entptr)+0x1d4c) = nclass; //pick the scout class (m_iClass)
	*((unsigned char*)(entptr)+0x22c8) = nclass; //pick the scout class (m_iDesiredClass)
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
		Player_Respawn(dic);
		return;
	}
	auto* controller = botmanager->GetBotController(dic);
	CBotCmd commd;
	Q_memset(&commd, 0, sizeof(CBotCmd));
	
	float distancemoved = VectorLength(controller->GetLocalOrigin() - jbot->distance_moved_lastpos);

	jbot->distance_moved_lastpos = controller->GetLocalOrigin();
	if (distancemoved < 20)
	{
		jbot->distance_moved_timer += 0.05;
	}
	else
	{
		jbot->distance_moved_timer = 0;
	}
	
	Vector movedir = Vector();

	jbot->Target_Search();
	
	if (jbot->target)
	{
		IPlayerInfo* theyinf = playerinfomanager->GetPlayerInfo(jbot->target);
		
		JBotClass* theyjbot = JBotClass::Edict_To_JBot(jbot->target);
		
#if 0
		Vector dir;
		dir = theyinf->GetAbsOrigin() - controller->GetLocalOrigin();
		commd.forwardmove = 200;
		commd.sidemove = 0;
		QAngle lookdir;
		VectorAngles(dir, lookdir);
		commd.buttons |= IN_ATTACK;
		commd.viewangles = lookdir;
#else
		CNavArea* myarea = 0, *theyarea = 0;
		Vector theypos = theyinf->GetAbsOrigin();
		Vector mypos = controller->GetLocalOrigin();
		for (int i = 0; i < navhub.m_areas.size(); i++)
		{

			if (navhub.m_areas[i].Contains(mypos)) { myarea = &navhub.m_areas[i]; }
			if (navhub.m_areas[i].Contains(theypos)) { theyarea = &navhub.m_areas[i]; }
		}
		
		Vector dir;

		commd.forwardmove = 0;
		commd.sidemove = 0;
		QAngle lookdir;
		bool shoot;
		float distance = 1.0f;
		JNav_PathfindResult canpath;
		if (myarea) {
			jbot->last_area = myarea;
		}
		if (!myarea && jbot->last_area)
		{
			myarea = jbot->last_area;
		}

		
		if (!theyarea && theyjbot && theyjbot->last_area)
		{
			theyarea = theyjbot->last_area;
		}
		else if (theyjbot)
		{
			theyjbot->last_area = theyarea;
		}

		
		canpath = JNavigation::Pathfind(myarea,theyarea,theypos,mypos);

		if (canpath.canpathfind && canpath.disp.size()>1)
		{
			int searchid = canpath.disp.size() - 1;
			
			Vector closestpoint = canpath.disp[searchid].second->m_center;
			//rare
			if (myarea)
			{
				if (closestpoint.x < myarea->m_nwCorner.x) closestpoint.x = myarea->m_nwCorner.x;
				if (closestpoint.x > myarea->m_seCorner.x) closestpoint.x = myarea->m_seCorner.x;
				if (closestpoint.y < myarea->m_nwCorner.y) closestpoint.y = myarea->m_nwCorner.y;
				if (closestpoint.y > myarea->m_seCorner.y) closestpoint.y = myarea->m_seCorner.y;
			}

			dir = closestpoint - controller->GetLocalOrigin();
			dir = dir.Normalized();
			dir *= 200.0f;
		}
		else
		{
			//label_failedtopathfind:
			//dir = theyinf->GetAbsOrigin() - controller->GetLocalOrigin();
		}
		if (distance) //go the distance
		{
			VectorAngles(dir, lookdir);
			movedir = dir.Normalized();
			lookdir.x /= 20.0f;
			commd.viewangles = lookdir;
		}
		
#endif
	}
	else
	{
		Player_DoCommand(dic, "taunt");
	}
label_nofail:
	Vector nuang;
	AngleVectors(commd.viewangles, &nuang);

#define LOOKSMOOTH 0.07f

	float len = jbot->lookdir.Length();
	jbot->lookdir += nuang *= LOOKSMOOTH;
	jbot->lookdir = ((jbot->lookdir.Normalized()) * len);
	QAngle nuangles;
	VectorAngles(jbot->lookdir,nuangles);
	commd.viewangles = nuangles;

	if (jbot->distance_moved_timer > 14)
	{
		commd.buttons |= IN_JUMP;
		jbot->distance_moved_timer = 0;
	}
	if (commd.viewangles.x >= 89.9f) commd.viewangles.x = 89;
	commd.viewangles.z = 0;

#if 0
	//translate movedir into where i acutlay want to go MULT the move speed
	Vector actualwishdir = nuang.Cross(movedir * 280.0f);
	commd.forwardmove = actualwishdir.y;
	commd.sidemove = actualwishdir.x;
#else
	//ugh 
	commd.forwardmove = 280.0f;
#endif
	controller->RunPlayerMove(&commd);
	return;
}


void JBotManager::CallOnMapChange(const char* newmapname)
{
	JNavigation::Load(newmapname);
}