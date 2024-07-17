#include "netprops.h"
#include "yuh.h"
#include <string>
#include "help.h"

enum NP_PropType //these dont really do anything but they're neat
{
	PT_int = -1,
	PT_float = -2,
	PT_bool = -3,
	PT_vector = -4,
	PT_handle = -5,
	PT_array = -6,
};
#define NP_CLS_NONEASY_MARKER 0x100

enum NP_ClassNameEnum
{
	//easy peasy objects
	CN_CBaseEntity,
	CN_CBasePlayer,
	CN_CTFPlayer,
	CN_CBaseCombatWeapon,
	CN_CTFEconEntity, //CTFWeapon and CTFWearable both inherit this class
	CN_CBaseAnimating,
	CN_CTFObjectiveResource, //dude I THINK this is unused.
	CN_CTeamControlPoint, //control point

	//objects that are housed inside another object. this is setup like this cus its easier to write code for and probably is faster
	CN_CTFPlayerClass = NP_CLS_NONEASY_MARKER, //this is related to TF_ClassType
	CN_CTFPlayerShared,
	CN_CTFEconEntity_ScriptCreatedItem,
};

typedef s16 NP_ClassName;
	
struct NP_Pair
{
	NP_ClassName classname;
	std::string propname;
	int offset;
	NP_ClassName holds;



	NP_Pair(const NP_ClassName& classname, const std::string& propname, int offset, const NP_ClassName& holds)
		: classname(classname), propname(propname), offset(offset), holds(holds)
	{
	}
};
typedef struct NP_Pair np_p;

//you can get these values from sm_dump_netprops, or some other method. but these tend to break through updates (good thing this is tf2)
static np_p netprop_pairs[] = {
	//np_p(<What class this prop belongs to>,<the string name of the class>,<the offset RELATIVE TO OWNER>,<the return value (only useful for nonez obj)>)
	np_p(CN_CBaseEntity,"m_iMaxHealth",240,PT_int),
	np_p(CN_CBaseEntity,"m_iHealth",244,PT_int),
	np_p(CN_CBaseEntity,"m_lifeState",248,PT_int),
	np_p(CN_CBaseEntity,"m_iTeamNum",556,PT_int),
	np_p(CN_CBaseEntity,"m_vecOrigin",832,PT_vector),
	np_p(CN_CBaseAnimating,"m_nSkin",924,PT_int),
	//   Table: m_hMyWeapons (offset 1904) (type m_hMyWeapons)

	//these are int types in code
	np_p(CN_CBasePlayer,"m_hMyWeapons",1904,PT_array), 
	np_p(CN_CBasePlayer,"m_hActiveWeapon",2096,PT_handle),

	np_p(CN_CBasePlayer,"m_iObserverMode",3176,PT_int),
	np_p(CN_CBasePlayer,"m_flMaxspeed",3844,PT_float),

	np_p(CN_CTFPlayer,"m_PlayerClass",8900,CN_CTFPlayerClass), //a struct that holds player class info
		np_p(CN_CTFPlayerClass,"m_iClass",4,PT_int), //the actual tfclass
	np_p(CN_CTFPlayer,"m_Shared",6880,CN_CTFPlayerShared), //a struct that holds misc tf2 player info
		np_p(CN_CTFPlayerShared,"m_nPlayerCond",204,PT_int), //first 32 bits of tf conditions
		np_p(CN_CTFPlayerShared,"m_nDisguiseTeam",256,PT_int), 
		np_p(CN_CTFPlayerShared,"m_nDisguiseClass",260,PT_int),
		np_p(CN_CTFPlayerShared,"m_iDesiredPlayerClass",620,PT_int),
	np_p(CN_CBaseCombatWeapon,"m_iClip1",1552,PT_int),
	np_p(CN_CBaseCombatWeapon,"m_iClip2",1556,PT_int),
	np_p(CN_CTFEconEntity,"m_Item",96,CN_CTFEconEntity_ScriptCreatedItem), //for tf weapons
	np_p(CN_CTFEconEntity_ScriptCreatedItem,"m_iItemDefinitionIndex",4,PT_int),
	np_p(CN_CTFEconEntity_ScriptCreatedItem,"m_iEntityQuality",8,PT_int),
	np_p(CN_CTFEconEntity_ScriptCreatedItem,"m_iAccountID",32,PT_int),
	np_p(CN_CTFEconEntity_ScriptCreatedItem,"m_bInitialized",84,PT_bool),
#if 0
	np_p(CN_CTFObjectiveResource,"m_iNumControlPoints",908,PT_int),
	//array of 8 * 8. very strange. to get the team do (cpindex + (team * 8)
	np_p(CN_CTFObjectiveResource,"m_bTeamCanCap",2872,PT_array), 
	np_p(CN_CTFObjectiveResource,"m_iOwner",3696,PT_int),
#endif
	np_p(CN_CTeamControlPoint,"m_iDefaultOwner",1200,PT_int),
	np_p(CN_CTeamControlPoint,"m_bLocked",1221,PT_bool),
	np_p(CN_CTeamControlPoint,"m_iPointIndex",1444,PT_int),
};

static constexpr int number_of_netprop_pairs = (sizeof(netprop_pairs) / sizeof(np_p));

static void recursive_npfind(std::string propname,NP_ClassName classname, offset_t* offsetptr)
{
	for (int i = 0; i < number_of_netprop_pairs; i++)
	{
		const np_p* cur = &netprop_pairs[i];
		if (cur->holds == classname)
		{
			//this is it
			*offsetptr += cur->offset;
			recursive_npfind(propname, cur->classname, offsetptr);
			break;
		}
	}
}

offset_t JI_NetProps::FindNetProp(std::string propname)
{
	//find what class owns this property
	np_p* owner = 0;
	for (int i = 0; i < number_of_netprop_pairs; i++)
	{
		if (!strcmp(propname.c_str(), netprop_pairs[i].propname.c_str()))
		{
			//this is it
			owner = &netprop_pairs[i];
			break;
		}
	}
	if (!owner)
	{
		char buffa[500];
		//i dont use c++ exceptions so i dont know if this displays a beautiful windows message box or not
		sprintf_s(buffa, "netprop %s isnt registered",propname.c_str());
		jprint(buffa); //failsafe in case im wrong
		throw std::exception(buffa);
		return 0; //do some error message or something
	}

	offset_t finaloffset = owner->offset;
	//check if the owner class is a easy peasy type
	if ((owner->classname < NP_CLS_NONEASY_MARKER))
	{
		//alright! now lets just dump this output
		return owner->offset;
	}

	//this is a class that is housed inside another class
	recursive_npfind(propname,owner->classname,&finaloffset);

	return finaloffset;
}
