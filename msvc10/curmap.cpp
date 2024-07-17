#include "curmap.h"
#include "edict.h"
#include <IEngineTrace.h>
#include <toolframework/itoolentity.h>
#include <ISmmAPI.h>
class CBaseEntity;
#include "netprops.h"
#include "help.h"

//lol
TFGamemode JI_CurMap::s_iCurrentGamemode;
u32 JI_CurMap::counter_timestamp;

static MapObjectDataVector static_mapobjects;
static MapObjectDataVector dynamic_mapobjects;

extern IVEngineServer* engine;
extern IServerPluginHelpers* helpers;
extern CGlobalVars* gpGlobals;
extern IServerTools* serverTools;
extern IServerGameEnts* serverGameEnts;

ConVar cv_printgamemode("jbot_curmap_show_gamemode","1",0,"print the determined gamemode when its calculated");
ConVar cv_dumpmapobjects("jbot_curmap_dump_map_objects","0",0,"1 = dump static\n 2 = dump static & dynamic");

#define ENM2STR(THEENUM) #THEENUM
const char* gmstringarray[NUMBER_OF_TF_GAMEMODES] = {ENM2STR(TF_GAMEMODE_UNKNOWN), ENM2STR(TF_GAMEMODE_CTF), ENM2STR(TF_GAMEMODE_CONTROL_POINTS),ENM2STR(TF_GAMEMODE_ATTACK_DEFEND),ENM2STR(TF_GAMEMODE_TC),ENM2STR(TF_GAMEMODE_KOTH),ENM2STR(TF_GAMEMODE_PL),ENM2STR(TF_GAMEMODE_ARENA), ENM2STR(TF_GAMEMODE_PLR), ENM2STR(TF_GAMEMODE_DOMINATION),  ENM2STR(TF_GAMEMODE_MEDIEVAL), };

TFGamemode JI_CurMap::Determine_Gamemode_Internal()
{
#define check_ent_exists(entname) if (serverTools->FindEntityByClassname(0,entname))
	s_iCurrentGamemode = TF_GAMEMODE_UNKNOWN;

	//some custom gamemodes use arena logic as well.. (i think vscript saxton hale does?)
	check_ent_exists("tf_logic_arena") return TF_GAMEMODE_ARENA;
	check_ent_exists("tf_logic_koth") return TF_GAMEMODE_KOTH;
	//bug: mvm & sd are also ctf by this
	check_ent_exists("item_teamflag") return TF_GAMEMODE_CTF;

	if (!serverTools->FindEntityByClassname(0, "team_control_point")) return TF_GAMEMODE_UNKNOWN;
	//control point map..
	TFTeam curteamfullowner = TF_TEAM_SPECTATOR;
	CBaseEntity* cur = 0;
	while (cur = serverTools->FindEntityByClassname(cur, "team_control_point"))
	{
		const TFTeam defaultowner = *JI_NetProps::GetNetProp<TFTeam>(cur, "m_iDefaultOwner");
		if (curteamfullowner == TF_TEAM_SPECTATOR) { curteamfullowner = defaultowner; continue; }
		if (curteamfullowner != defaultowner)
		{
			return TF_GAMEMODE_CONTROL_POINTS;
		}
	}

	if (curteamfullowner != TF_TEAM_UNASSIGNED) return TF_GAMEMODE_ATTACK_DEFEND;

	return TF_GAMEMODE_DOMINATION;
}
void JI_CurMap::Determine_Gamemode()
{
	s_iCurrentGamemode = Determine_Gamemode_Internal();
	if (cv_printgamemode.GetBool())
	jprintf("jicurmap: determined gamemode for this map is %s\n",gmstringarray[s_iCurrentGamemode]);
}

static void dump_ents(const char* classname, MapObjectDataVector* vector, u16 objtype)
{
	CBaseEntity* ptr = 0;
	while (ptr = serverTools->FindEntityByClassname(ptr, classname))
	{
		MapObjectData objdata;
		memset(&objdata, 0, sizeof(MapObjectData));
		objdata.m_bIsActive = 1;
		objdata.m_hEntity = serverGameEnts->BaseEntityToEdict(ptr);
		objdata.m_iObjectType = objtype;
		switch (objtype)
		{
		case MO_OBJECT_OBJECTIVE_CONTROLPOINT:
			//note: control points are terrible i hate them i hate this system
			objdata.m_iTeam = *JI_NetProps::GetNetProp<TFTeam>(ptr, "m_iDefaultOwner");
			objdata.m_bLocked = *JI_NetProps::GetNetProp<bool>(ptr, "m_bLocked");
			objdata.m_iCPIndex = *JI_NetProps::GetNetProp<int>(ptr, "m_iPointIndex");
		case MO_OBJECT_OBJECTIVE_CART:
			//plr not supported yet so just assume blue always
			objdata.m_iTeam = TF_TEAM_BLUE;
		default:
			objdata.m_iTeam = *JI_NetProps::GetNetProp<int>(ptr, JNP_PROPNAME_TEAMNUM);
		}

		objdata.m_Position = *JI_NetProps::GetNetProp<Vector>(ptr, JNP_PROPNAME_ABSORIGIN);
		//m_hOwner is never set...
		u8 dynamic = vector == &dynamic_mapobjects;
		if (cv_dumpmapobjects.GetInt() >= dynamic + 1 )
		{
			jprintf("pushed back id %x (classname=%s) (dynamic?=%i)\n", objtype, classname, dynamic);
			jprintf("--stats: team=%i,cpindex=%i,active=%i", objdata.m_iTeam, objdata.m_iCPIndex, objdata.m_bIsActive);
			jprintf("position= setpos %.1f %.1f %.1f", objdata.m_Position.x, objdata.m_Position.y, objdata.m_Position.z);

			jprint("\n");
		}
		
		vector->push_back(objdata);
	}
}

//create curmapobjects
void JI_CurMap::Map_Reload()
{
	
	Determine_Gamemode();
	static_mapobjects.clear();
	static_mapobjects.reserve(50);
	dump_ents("team_control_point", &static_mapobjects, MO_OBJECT_OBJECTIVE_CONTROLPOINT);
	dump_ents("item_teamflag", &static_mapobjects, MO_OBJECT_OBJECTIVE_FLAG);
	//dump_ents("the name of the cart entity or some identifier", &static_mapobjects, MO_OBJECT_OBJECTIVE_CART);
	dump_ents("item_healthkit_full", &static_mapobjects, MO_OBJECT_PICKUP_HEALTHKIT_FULL);
	dump_ents("item_healthkit_medium", &static_mapobjects, MO_OBJECT_PICKUP_HEALTHKIT_MEDIUM);
	dump_ents("item_healthkit_small", &static_mapobjects, MO_OBJECT_PICKUP_HEALTHKIT_SMALL);	

	dump_ents("item_ammopack_full", &static_mapobjects, MO_OBJECT_PICKUP_AMMO_FULL);
	dump_ents("item_ammopack_medium", &static_mapobjects, MO_OBJECT_PICKUP_AMMO_MEDIUM);
	dump_ents("item_ammopack_small", &static_mapobjects, MO_OBJECT_PICKUP_AMMO_SMALL);
	Obj_Reload(); //:)
	counter_timestamp = static_counter;
	J_Random_Seed(counter_timestamp - static_mapobjects.size());
}

void JI_CurMap::Obj_Reload()
{
	dynamic_mapobjects.clear();
	dynamic_mapobjects.reserve(40);
	dump_ents("obj_sentrygun",&dynamic_mapobjects,MO_OBJECT_SENTRY);
	dump_ents("obj_teleporter",&dynamic_mapobjects,MO_OBJECT_TELEPORTER);
	dump_ents("obj_dispenser",&dynamic_mapobjects,MO_OBJECT_DISPENSER);
	//fixme: doesnt account for player dropped ammo packs / small health kits created by candy cane, dropped sandwiches.. etc..

}

MapObjectDataVector* JI_CurMap::GetStaticMapObjects()
{
	if (static_counter != counter_timestamp)
	{
		Obj_Reload();
		counter_timestamp = static_counter;
	}
	return &static_mapobjects;
}

MapObjectDataVector* JI_CurMap::GetDynamicMapObjects()
{
	if (static_counter != counter_timestamp)
	{
		Obj_Reload();
		counter_timestamp = static_counter;
	}
	return &dynamic_mapobjects;
}

void getobjstuff(u16 objectiveid, MapObjectDataVector* desvec)
{
	MapObjectDataVector* vec = JI_CurMap::GetStaticMapObjects();
	for (int i = 0; i < vec->size(); i++)
	{
		if (vec->at(i).m_iObjectType == objectiveid)
		{
			//yippee
			desvec->push_back(vec->at(i));
		}
	}
}
