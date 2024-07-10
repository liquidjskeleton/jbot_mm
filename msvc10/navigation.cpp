#include "navigation.h"
#include "mathlib/mathlib.h"
#include <algorithm>
#include "mathlib/vector.h"
extern bool j_stopexecution;
#include "help.h"
CNavFile navhub;
extern CGlobalVars* gpGlobals;

const char* JNavigation::TryFindLevelName()
{
	return gpGlobals->mapname.ToCStr();
}
void JNavigation::Load(const char* levelname)
{
	if (!levelname) return;
	jprintf("name=%s\n", levelname);
	navhub = CNavFile(levelname);
	navhubex = JNavExtended(levelname);
	j_stopexecution = false;
}

ConVar pfdebug("jbot_debug_pathfinding","0",0,"this will spam your console btw\nnonzero = notify new pathfind calls\n1=pf status\n2=area scoring\n3=notify area tags and skips\n4=show recursive steps\n");
#define ISINPFDEBUG pfdebug.GetInt()
#define PFDEBUG_STATUS 1
#define PFDEBUG_SCORING 2
#define PFDEBUG_SKIPS 3
#define PFDEBUG_CHECKCHECK 4

#define PFDEBUG_SKIP_MSG(area,message) 		if (ISINPFDEBUG == PFDEBUG_SKIPS){	jprintf("skipping %i due to: %s\n",area->m_id,message); }

typedef unsigned char u8;

struct NavR
{
	u8* checks;
	NavAreaDisP disp_list;
	Vector targetpos;
	//(for jnav_close_enough)
	float ce_lowest_score;
	unsigned char ce_is_pass;
};

int navareaid(CNavArea* area)
{
	for (int i = 0; i < navhub.m_areas.size(); i++)
	{
		if (area == &navhub.m_areas[i]) return i;
	}
	return 0;
}


//is this a good target to pathfind towards?
//prefer flat movement, and prefer areas with more connections with other navareas
//LOWER = BETTER FOR SCORE
float navigation_scoring(CNavArea* end, CNavArea* area,Vector targetpos,int index)
{
	//Vector vecdif = area->m_center - end->m_center;
	Vector point = JNavigation::ClosestPointOnArea(area, targetpos);
	Vector vecdif = targetpos - point;
	
	float zdif = fabs(vecdif.z);
	float length = VectorLength(vecdif);
	
	float navareasize = VectorLength(area->m_nwCorner - area->m_seCorner);

	if (JNavigation::NavAreaIsSlope(area)) //slopes usually get us to where we're going
	{
		zdif = -10.0f;
	}
	else
	{
		//we HATE drops/jumps. prefer FLAT ground
		zdif = 0.0f;
	}

	float cell_score = navhubex.GetCellFromAreaIndex(index)->Cell_Score(area, index);

	//float score = length + (zdif * 200) - navareasize;// -((3 - fconnections) * 10.0f) - (vecdif.z * 8);
	float score = length + (zdif) + cell_score;
	if (ISINPFDEBUG==PFDEBUG_SCORING)
	{
		jprintf("%i score is %f\n",area->m_id,score);
	}
	return score;
}

std::string pfdebug_idar[3] = {"AREA_UNCHECKED","AREA_CHECKED","AREA_CHECKED_INVALID"};

//check the one with the lowest distance to target first and then go through the others thats what imn tryina do here
//todo: move some of the vectors to the heap maybe.. this may be bad for huge meshes
static bool checknavarea_recursive(CNavArea* it, CNavArea* end,NavR* navr)
{
	if (ISINPFDEBUG==PFDEBUG_STATUS)
	jprintf("currently on id %i (going to %i)\n", it->m_id, end->m_id);

	if (ISINPFDEBUG == PFDEBUG_CHECKCHECK)
	{
		jprintf("%i ....v\n", it->m_id);
	}
	//first, flag self
	//jdelay;
	const int it_areaid = navareaid(it);
	navr->checks[it_areaid] = 1;
	const int connections = it->m_connections.size();
	//sort connections by distance
	NavAreaDisP vec;
	for (int i = 0; i < connections; i++)
	{
		CNavArea* area = it->m_connections[i].area;

		//check if the area is the end
		if (area == end)
		{
			//we found it boys
			navr->disp_list.push_back(DisP(-1,end));
			PFDEBUG_SKIP_MSG(it, "is the end");
			return true;
		}

		//check if the area is flagged already
		const int area_areaid = navareaid(area);
		if (navr->checks[it_areaid]) {

			if (pfdebug.GetInt() == PFDEBUG_SKIPS) {
				jprintf("skipping %i:%i (ind:%i) due to: %s\n", it->m_id,area->m_id, area_areaid,"already checked this area");
			}; 
			if (ISINPFDEBUG == PFDEBUG_CHECKCHECK)
			{
				jprintf("%i:%i (ind:%i) checkval=%s\n", it->m_id, area->m_id, area_areaid, pfdebug_idar[navr->checks[it_areaid]]);
			}
			
			continue;
		}
		if (pfdebug.GetInt() == PFDEBUG_SKIPS)
		{
			jprintf("tagging area %i:%i (ind:%i) as checked\n",it->m_id,area->m_id, area_areaid);
		}
		navr->checks[it_areaid] = 2;

#if 0
		float zdif = it->m_minZ - area->m_minZ;
		//jprintf("(%i) zdif=%f\n",area->m_id,zdif);

		//is this higher than we can jump?
		if (zdif < -72)
		{
			continue; //yeah... dont...
		}
#endif

		float score = navigation_scoring(end, area, navr->targetpos, it_areaid);
		//negative navigation scores mean its reaaalllll baddd
		float absscore = fabs(score);
		if (!navr->ce_is_pass)
		{
			if (absscore < navr->ce_lowest_score)
			navr->ce_lowest_score = score;
		}
		else
		{
			if (absscore <= navr->ce_lowest_score)
			{
				navr->disp_list.push_back(DisP(-score, area));
				return true;
			}
		}
		//if (!score) { continue; } //this is really bad
		//Vector vecdif = area->m_center - end->m_center;
		//float length = VectorLength(vecdif);
		DisP disp = DisP(score,area);
		vec.push_back(disp);
	}
	if (vec.empty())
	{
		//no connections will work
		PFDEBUG_SKIP_MSG(it,"no valid connections");
		return false;
	}
	//use lambda magic to sort the vector. yoinked ts off stackoverflow btw
	std::sort(vec.begin(), vec.end(), [](DisP& left, DisP& right) {
		return left.first < right.first;
		});
	//alright. now iterate the vector :)
	for (int i = 0; i < vec.size(); i++)
	{
#define curp vec[i]

		bool result = checknavarea_recursive(curp.second, end,navr);
		if (result)
		{
			//hooly shit we found a path
			navr->disp_list.push_back(curp);
			return true;
		}
	}
	PFDEBUG_SKIP_MSG(it, "no good connections");
	return false;
}

//returns: a new cnavarea_end that is as close to the destination as possible
CNavArea* nav_failsafe(Vector targetpos)
{
	float smallest_distance = 9999999.9f;
	CNavArea* outpt = 0;
	for (int i = 0; i < navhub.m_areas.size(); i++)
	{
		CNavArea* end = (&navhub.m_areas[i]);
		Vector vecdif = targetpos - end->m_center;
		float length = VectorLength(vecdif);
		if (length < smallest_distance)
		{
			outpt = end;
		}
	}
	return outpt;
}
#define PFDEBUG_PFNOTIFY if (ISINPFDEBUG) { jprintf("new call to pf recursive!\nstartid=%i,\nendid=%i,\nnavr.ce_is_pass=%i\n",start->m_id,end->m_id,navr.ce_is_pass); }


JNav_PathfindResult JNavigation::Pathfind(CNavArea* start, CNavArea* end,Vector targetpos, Vector mypos, JNav_PathfindInput navinput)
{
	JNav_PathfindResult result = JNav_PathfindResult(); //canpath
	result.canpathfind = false;
	result.disp.clear();
	result.disp.reserve(50);
	int numofareas = navhub.m_areas.size();
	//jprintf("there are %i areas\n", numofareas);
	if(numofareas == 0)
	{
		//jprint("No nav mesh\n");
		return result;
	}
	if (!start)
	{
		start = nav_failsafe(mypos); //we are DESPERATE
	}

	if (!end)
	{
		end = nav_failsafe(targetpos);
	}
	if (!start && !end) { result.canpathfind = false; return result; }
	if (start == end) { result.canpathfind = true; result.disp.push_back(DisP(0, end)); return result; }
	//jdelay
	NavR navr = NavR();
	navr.checks = new u8[numofareas];
	Q_memset(navr.checks, 0, numofareas);
	navr.ce_is_pass = false;
	navr.ce_lowest_score = 9999999999.9f;
	navr.targetpos = targetpos;
	navr.disp_list.clear();
	navr.disp_list.reserve(50);
	PFDEBUG_PFNOTIFY;
	bool res = checknavarea_recursive(start, end, &navr);
	if (navinput.jnav_close_enough)
	{
		Q_memset(navr.checks, 0, numofareas);
		navr.ce_is_pass = true; PFDEBUG_PFNOTIFY;
		res = checknavarea_recursive(start, end, &navr);
		if (!res)
		{
			//oh my god dude
			Q_memset(navr.checks, 0, numofareas);
			navr.ce_is_pass = 2; PFDEBUG_PFNOTIFY;
			res = checknavarea_recursive(start, end, &navr);
		}
	}
	delete[] navr.checks;
	//jprintf("finished pathing. result:%i\n", res);
	result.canpathfind = res; result.disp = navr.disp_list; return result;
}

Vector JNavigation::ClosestPointOnArea(const CNavArea* area, Vector target) noexcept
{
	const float fatty = 82.0f;
	const float nfatty = -fatty;
	if (!area) return target;

	Vector closestpoint = target;
	float cpz = target.z;
	if (closestpoint.x < area->m_nwCorner.x + fatty) { closestpoint.x = area->m_nwCorner.x + fatty; cpz = area->m_nwCorner.z; }
	if (closestpoint.x > area->m_seCorner.x + nfatty) {closestpoint.x = area->m_seCorner.x + nfatty; cpz = area->m_seCorner.z; }
	if (closestpoint.y < area->m_nwCorner.y + fatty) {closestpoint.y = area->m_nwCorner.y + fatty; cpz = area->m_nwCorner.z; }
	if (closestpoint.y > area->m_seCorner.y + nfatty) {closestpoint.y = area->m_seCorner.y + nfatty; cpz = area->m_seCorner.z; }
	//if (closestpoint.z > area->m_maxZ) closestpoint.z = area->m_maxZ;
	//if (closestpoint.z < area->m_minZ) closestpoint.z = area->m_minZ;

	return closestpoint;
}


Vector JNavigation::GetMoveToPosition(CNavArea* area, Vector target)
{
#if 0
	const float ignoredis = 300;
	Vector dif = target - area->m_center;
	float distancetotarget = VectorLength(dif);
	if (dif.z < 0)
	{
		distancetotarget += dif.z * 300.0f; //treat jumps differently :)
	}
	distancetotarget = ignoredis - distancetotarget;
	if (distancetotarget < 0) distancetotarget = 0;

	Vector targit = VectorLerp(area->m_center,target,distancetotarget / ignoredis);

#else
	Vector targit = area->m_center;
#endif
	
	Vector mpos = JNavigation::ClosestPointOnArea(area, targit);
	if (NavAreaIsSlope(area))
	{
		//ugh
		
	}
	return mpos;
}

bool JNavigation::NavAreaIsSlope(CNavArea* area)
{
	return area->m_nwCorner.z != area->m_seCorner.z;
}