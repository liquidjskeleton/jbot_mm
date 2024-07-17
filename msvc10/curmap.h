#pragma once
#include "yuh.h"
#include <vector>
#include "teamfortress.h"
#include <ISmmAPI.h>
struct edict_t;
class CBaseEntity;
class Vector;
enum
{
	TF_GAMEMODE_UNKNOWN, //assume tdm then
	TF_GAMEMODE_CTF,
	TF_GAMEMODE_CONTROL_POINTS, //5cp, powerhouse
	TF_GAMEMODE_ATTACK_DEFEND, //cp maps with one team owning all points by default
	TF_GAMEMODE_TC, //no shot im adding tc
	TF_GAMEMODE_KOTH, 
	TF_GAMEMODE_PL, //not supported rn..
	TF_GAMEMODE_ARENA,
	TF_GAMEMODE_PLR, //not supported rn..
	TF_GAMEMODE_DOMINATION, //cp_standin and only cp_standin.
	TF_GAMEMODE_MEDIEVAL, //im not supporting this at all ever
	NUMBER_OF_TF_GAMEMODES,
};
typedef u8 TFGamemode;
//some stuff that bots should keep track of

enum {
	MO_CATEGORY_PICKUP_HEALTH = 0x10, //health kits, map consts or not
	MO_CATEGORY_PICKUP_AMMO = 0x20, //
	//use 0x30 for stuff that provide both health and ammo
	MO_CATEGORY_BUILDING = 0x40, //engineer buildings

	MO_CATEGORY_OBJECTIVE = 0x100, //cap points, ctf flag.. whatnot..
};

enum
{
	MO_OBJECT_OBJECTIVE_CONTROLPOINT = 0x100, //stationary always
	MO_OBJECT_OBJECTIVE_FLAG = 0x101, //moves around a lot
	MO_OBJECT_OBJECTIVE_CART = 0x102, //moves around but in a predictable way

	MO_OBJECT_PICKUP_HEALTHKIT_SMALL = 0x10,
	MO_OBJECT_PICKUP_HEALTHKIT_MEDIUM = 0x11,
	MO_OBJECT_PICKUP_HEALTHKIT_FULL = 0x12,

	MO_OBJECT_PICKUP_AMMO_SMALL = 0x20,
	MO_OBJECT_PICKUP_AMMO_MEDIUM = 0x21,
	MO_OBJECT_PICKUP_AMMO_FULL = 0x22,

	MO_OBJECT_RESUPPLY = 0x32, //

	MO_OBJECT_DISPENSER = 0x40,
	MO_OBJECT_SENTRY = 0x41,
	MO_OBJECT_TELEPORTER = 0x42,
	MO_OBJECT_TELEPORTER_ENTRANCE = 0x43,
	MO_OBJECT_TELEPORTER_EXIT = 0x44,
};

struct MapObjectData
{ public:
	bool m_bIsActive;  //(for healthkits and ammopacks, and contrl points not in this round)
	u16 m_iObjectType; //what is this object
	edict_t* m_hEntity;  //reference to the object
	TFTeam m_iTeam; //which team currently owns this object?
	Vector m_Position; //(you can get this in the edict but we need fast access to this for pathfinding)
	edict_t* m_hOwner;  //(for ctf flags and engineer buildings)

	//(for control points)
	bool m_bLocked;
	int m_iCPIndex;

};

enum
{
	JICURMAP_NOTIFY_POINT_CAPTURED,
	JICURMAP_NOTIFY_POINT_LOCKED,
	JICURMAP_NOTIFY_POINT_UNLOCKED,
	JICURMAP_NOTIFY_FLAG_STOLEN,
	JICURMAP_NOTIFY_FLAG_DROPPED,
	JICURMAP_NOTIFY_FLAG_CAPTURED,
};

typedef std::vector<struct MapObjectData> MapObjectDataVector;
static class JI_CurMap
{
public:
	static TFGamemode s_iCurrentGamemode;

	static void Determine_Gamemode();
	//call on map refresh please
	static void Map_Reload();
	//regenerate dynamic objects and refresh static mapobjects
	static void Obj_Reload();

	//static map objects are map objects placed by the mapper, and will always be there
	static MapObjectDataVector* GetStaticMapObjects();
	//dynamic map objects are objects like engineer buildings that can or cannot exist at a given point in time so these tend to update more frequently
	static MapObjectDataVector* GetDynamicMapObjects();


private:
	//to check each tick whether or not we need to reload dynamic objects so we dont do it every time
	static u32 counter_timestamp;
	static TFGamemode Determine_Gamemode_Internal();
};