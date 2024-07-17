#pragma once


//tf classes
enum
{
	TF_CLASS_UNDEFINED = 0,
	TF_CLASS_SCOUT = 1,
	TF_CLASS_SNIPER = 2,
	TF_CLASS_SOLDIER = 3,
	TF_CLASS_DEMOMAN = 4,
	TF_CLASS_MEDIC = 5,
	TF_CLASS_HEAVYWEAPONS = 6,
	TF_CLASS_PYRO = 7,
	TF_CLASS_SPY = 8,
	TF_CLASS_ENGINEER = 9,
	TF_CLASS_CIVILIAN = 10, 
	TF_CLASS_COUNT_ALL = 11,
	TF_CLASS_RANDOM,
};
typedef int TFClass;

enum
{
	TF_TEAM_UNASSIGNED = 0,
	TF_TEAM_SPECTATOR = 1,
	TF_TEAM_RED = 2,
	TF_TEAM_BLUE = 3,
	//going any further will crash
};
typedef int TFTeam;
//fixme: OUTDATED!
enum
{
	TF_COND_AIMING = 0,		// Sniper aiming, Heavy minigun.
	TF_COND_ZOOMED,
	TF_COND_DISGUISING,
	TF_COND_DISGUISED,
	TF_COND_STEALTHED,
	TF_COND_INVULNERABLE,
	TF_COND_TELEPORTED,
	TF_COND_TAUNTING,
	TF_COND_INVULNERABLE_WEARINGOFF,
	TF_COND_STEALTHED_BLINK,
	TF_COND_SELECTED_TO_TELEPORT,

	// The following conditions all expire faster when the player is being healed
	// If you add a new condition that shouldn't have this behavior, add it before this section.
	TF_COND_HEALTH_BUFF,
	TF_COND_BURNING,

	// Add new conditions that should be affected by healing here

	TF_COND_LAST
};

typedef int TFCond;

enum
{
	TF_STATE_ACTIVE = 0,		// Happily running around in the game.
	TF_STATE_WELCOME,			// First entering the server (shows level intro screen).
	TF_STATE_OBSERVER,			// Game observer mode.
	TF_STATE_DYING,				// Player is dying.
	TF_STATE_COUNT
};


//fixme: OUTDATED!
//tf2 special damagetypes.
enum
{
	TF_DMG_CUSTOM_NONE = 0,
	TF_DMG_CUSTOM_HEADSHOT,
	TF_DMG_CUSTOM_BACKSTAB,
	TF_DMG_CUSTOM_BURNING,
	TF_DMG_WRENCH_FIX,
	TF_DMG_CUSTOM_MINIGUN,
	TF_DMG_CUSTOM_SUICIDE,
};
typedef unsigned char u8;
static u8 s_tfclassmaxhealth[TF_CLASS_COUNT_ALL] = {
	0, //undefined
	125, //scout
	125, //sniper
	200, //soldier
	175, //demo
	150, //medic
	300, //heavyweapons
	175, //pyro
	125, //spy
	125, //engineer
	200, //civilian
};
static u8 s_tfclassspeeds[TF_CLASS_COUNT_ALL] = {
	0, //undefined
	400, //scout
	300, //sniper
	240, //soldier
	280, //demo
	320, //medic
	230, //heavyweapons
	300, //pyro
	320, //spy
	300, //engineer
	240, //civilian
};

#define TF_SPEED_BACKWARDS_MULTIPLIER 0.9f