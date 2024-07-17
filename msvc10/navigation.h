#pragma once
#include <vector>
#include <ISmmAPI.h>
#include "nav/nav.h"
#include "nav/CNavFile.h"
#include "nav_extended.h"
#include "teamfortress.h"
class CNavFile;
class CNavArea;
class JNavExtended;

#define DISP_IS_STRUCT 0

//enable for extended functionality in disps
#if DISP_IS_STRUCT
struct disp_struc
{
	//from std::pair
	float first;
	CNavArea* second;

	disp_struc(float one, CNavArea* two)
	{
		first = one; second = two;
	}
};
typedef disp_struc DisP;
#else
typedef std::pair<float, CNavArea*> DisP;
#endif
typedef std::vector<CNavArea*> NavAreas;
typedef std::vector<DisP> NavAreaDisP;
struct JNav_PathfindResult
{
	bool canpathfind;
	NavAreaDisP disp;
};
struct JNav_PathfindInput
{
	//twice as slow as normal pathfinding, but will always give you a valid path
	bool jnav_close_enough;
	TFTeam team;
	TFClass class_profile; //this will be changed eventually
};
static class JNavigation
{
public:
	static const char* TryFindLevelName();
	static void Load(const char* levelname);
	static JNav_PathfindResult Pathfind(CNavArea* start, CNavArea* end,Vector targetpos, Vector mypos, JNav_PathfindInput navinput);

	static bool IsOnArea(CNavArea* area, Vector pos) noexcept;
	static Vector ClosestPointOnArea(const CNavArea* area, Vector target) noexcept;

	//get pos to move towards for bots
	static Vector GetMoveToPosition(CNavArea* area, CNavArea* next,Vector target);

	static bool NavAreaIsSlope(CNavArea* area);
private:
	static Vector nav_connection_position(CNavArea* area1, CNavArea* area2);
};

extern CNavFile navhub;
extern JNavExtended navhubex;


CNavArea* nav_failsafe(Vector targetpos);