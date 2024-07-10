#pragma once
#include <vector>
#include <ISmmAPI.h>
#include "nav/nav.h"
#include "nav/CNavFile.h"
#include "nav_extended.h"
class CNavFile;
class CNavArea;
class JNavExtended;

typedef std::vector<CNavArea*> NavAreas;
typedef std::pair<float, CNavArea*> DisP;
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
};
static class JNavigation
{
public:
	static const char* TryFindLevelName();
	static void Load(const char* levelname);
	static JNav_PathfindResult Pathfind(CNavArea* start, CNavArea* end,Vector targetpos, Vector mypos, JNav_PathfindInput navinput);

	static Vector ClosestPointOnArea(const CNavArea* area, Vector target) noexcept;

	//get pos to move towards for bots
	static Vector GetMoveToPosition(CNavArea* area, Vector target);

	static bool NavAreaIsSlope(CNavArea* area);
};

extern CNavFile navhub;
extern JNavExtended navhubex;


CNavArea* nav_failsafe(Vector targetpos);