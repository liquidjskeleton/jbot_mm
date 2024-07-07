#pragma once
#include <vector>
#include <ISmmAPI.h>
#include "nav/nav.h"
#include "nav/CNavFile.h"
class CNavFile;
class CNavArea;
typedef std::vector<CNavArea*> NavAreas;
typedef std::pair<float, CNavArea*> DisP;
typedef std::vector<DisP> NavAreaDisP;
struct JNav_PathfindResult
{
	bool canpathfind;
	NavAreaDisP disp;
};
static class JNavigation
{
public:
	static void Load(const char* levelname);
	static JNav_PathfindResult Pathfind(CNavArea* start, CNavArea* end,Vector targetpos, Vector mypos);
};

extern CNavFile navhub;