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

ConVar pfdebug("jbot_debug_pathfinding","0",0,"this will SPAM your console btw. I MEAN IT!!!\nnonzero = notify new pathfind calls\n1=pf status\n2=area scoring\n3=notify area tags and skips\n4=show recursive steps\n");
#define ISINPFDEBUG pfdebug.GetInt()
#define PFDEBUG_STATUS 1
#define PFDEBUG_SCORING 2
#define PFDEBUG_SKIPS 3
#define PFDEBUG_CHECKCHECK 4

#define PFDEBUG_SKIP_MSG(area,message) 		if (ISINPFDEBUG == PFDEBUG_SKIPS){	jprintf("skipping %i due to: %s\n",area->m_id,message); }

ConVar cv_jnav_faraway("jbot_pf_far_away","4000.0",0,"if we're far away from the target we will prefer larger areas to pathfind towards");
ConVar cv_jnav_fatty("jbot_pf_fatty","48.9",0,"tolerance for nav areas. This should be around 49, set to a larger amount if guys are getting stuck on walls");

#define JNAV_FAR_AWAY cv_jnav_faraway.GetFloat() //2000.0f
#define JNAV_FATTY cv_jnav_fatty.GetFloat() //48.9f //leeway for ClosestPointOnArea. this should be the size of a tf player bounding box

typedef unsigned char u8;

struct NavR
{
	//bools have the same size as a u8, but c++ treats them in a strange special way unlike plain ol c so im using u8s here
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
		zdif = fabsf(zdif) * 500.0f;
	}

	float cell_score = navhubex.GetCellFromAreaIndex(index)->Cell_Score(area, index);

	//float score = length + (zdif * 200) - navareasize;// -((3 - fconnections) * 10.0f) - (vecdif.z * 8);
	float local_score = length + zdif;

	//avoid smaller nav meshes if we're real far away from the target
	float lensizedif = length / JNAV_FAR_AWAY;
	lensizedif = clamp(lensizedif, 0, 1);
	local_score -= navareasize * lensizedif;

	float score = local_score + cell_score;
	if (ISINPFDEBUG==PFDEBUG_SCORING)
	{
		jprintf("%i score is %f\n",area->m_id,score);
	}
	return score;
}

#define AREA_UNCHECKED 0
#define AREA_CHECKED 1
#define AREA_TOO_HIGH 2
#define COUNT_AREA 3
std::string pfdebug_idar[COUNT_AREA] = {"AREA_UNCHECKED","AREA_CHECKED","AREA_TOO_HIGH"};

//check the one with the lowest distance to target first and then go through the others thats what imn tryina do here
//todo: move some of the vectors to the heap maybe.. this may be bad for huge meshes
static bool checknavarea_recursive(CNavArea* it, CNavArea* end,NavR* navr)
{
	if (ISINPFDEBUG==PFDEBUG_STATUS)
	jprintf("currently on id %i (going to %i)\n", it->m_id, end->m_id);

	const int it_areaid = navareaid(it);
	if (ISINPFDEBUG == PFDEBUG_CHECKCHECK)
	{
		jprintf("%i (%s) ....v\n", it->m_id, pfdebug_idar[navr->checks[it_areaid]].c_str());
	}
	//first, flag self
	//jdelay;
	navr->checks[it_areaid] = AREA_CHECKED;
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
		if (ISINPFDEBUG == PFDEBUG_CHECKCHECK)
		{
			jprintf("%i:%i (ind:%i) checkval=%s\n", it->m_id, area->m_id, area_areaid, pfdebug_idar[navr->checks[area_areaid]].c_str());
		}
		if (navr->checks[area_areaid]) {

			if (pfdebug.GetInt() == PFDEBUG_SKIPS) {
				jprintf("skipping %i:%i (ind:%i) due to: %s\n", it->m_id,area->m_id, area_areaid,"already checked this area");
			}; 
			continue;
		}
		if (pfdebug.GetInt() == PFDEBUG_SKIPS)
		{
			jprintf("tagging area %i:%i (ind:%i) as checked\n",it->m_id,area->m_id, area_areaid);
		}

		navr->checks[area_areaid] = AREA_CHECKED;

#if 1
		float zdif = fminf(area->m_nwCorner.z,area->m_seCorner.z) - fmaxf(it->m_nwCorner.z, it->m_seCorner.z);
		//jprintf("(%i) zdif=%f\n",area->m_id,zdif);

		//is this higher than we can jump?
		if (zdif > 72)
		{
			navr->checks[area_areaid] = AREA_TOO_HIGH;
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
		if (JNavigation::IsOnArea(end,(targetpos)))
		{
			//okay we found it goodie
			return end;
		}
		Vector vecdif = targetpos - end->m_center;
		float length = VectorLength(vecdif);
		if (length < smallest_distance)
		{
			outpt = end;
			smallest_distance = length;
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

bool JNavigation::IsOnArea(CNavArea* area, Vector pos) noexcept
{
	return area->IsOverlapping(pos,fabsf(JNAV_FATTY)) && !pos.z > area->m_maxZ && !pos.z < area->m_minZ;
}

Vector JNavigation::ClosestPointOnArea(const CNavArea* area, Vector target) noexcept
{
	const float fatty = -JNAV_FATTY;
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

//AXIS
//these are here to sort of figure out the connection point between two navs

struct axis_single
{
	float big;
	float sml;
	float difference;

	void loadconsts() noexcept
	{
		difference = big - sml;
	}
};
struct axis_pair
{
	//0 =x , 1 = y
	struct axis_single pair[2];
	//void init() { Q_memset(this, 0, sizeof(struct axis_pair)); } //this may be a waste as these get set through rip area.. although as of writing im still stting this up
	void rip_area(const CNavArea* area) noexcept
	{
		//idea: if se corner is always gonna be further in the x direction then we dont need any of these fmaxf and fminf calls
		pair[0].big = fmaxf(area->m_seCorner.x, area->m_nwCorner.x);
		pair[0].sml = fminf(area->m_seCorner.x, area->m_nwCorner.x);	
		pair[0].loadconsts();
		pair[1].big = fmaxf(area->m_seCorner.y, area->m_nwCorner.y);
		pair[1].sml = fminf(area->m_seCorner.y, area->m_nwCorner.y);
		pair[1].loadconsts();
	}
	axis_pair(const CNavArea* area) noexcept
	{
		rip_area(area);
	}
};

enum NAV_DIRECTION
{
	NDIR_NORTH,
	NDIR_EAST,
	NDIR_SOUTH,
	NDIR_WEST,
};

typedef struct axis_pair jpair_t;

//maybe not exactly 0.5 maybe some sqrt thing as this is normalized? tinker w this
#define DIFDOT_THRESHOLD 0.335f

Vector JNavigation::nav_connection_position(CNavArea* first, CNavArea* next)
{
	if (first == next) return first->m_center; //shouldnt happen
	Vector a1c, a2c;
	a1c = first->m_center;
	a2c = next->m_center;
	Vector dif = (a2c - a1c).Normalized();

#if 1
	//if (dif.z) return ND_UNAPPLICABLE;
	//Vector fordot = Vector(0, 1, 0);
	float dot = dif.y;// dif.Dot(fordot);
	NAV_DIRECTION direction;
	if (fabsf(dif.y) > DIFDOT_THRESHOLD) 
	{
		direction = dif.y > 0 ? NDIR_NORTH : NDIR_SOUTH;
	}
	else
	{
		direction = dif.x > 0 ? NDIR_EAST : NDIR_WEST;
	}

	Vector vdir;

	switch (direction)
	{
	case NDIR_NORTH: vdir = Vector(0, 1, 0); break;
	case NDIR_SOUTH: vdir = Vector(0, -1, 0); break;
	case NDIR_EAST: vdir = Vector(1,0, 0); break;
	case NDIR_WEST: vdir = Vector(-1, 0, 0); break;
	}
	vdir *= 8000.0f;
#else
	Vector vdir = dif;
#endif

	//this blows but i hope it works
	CNavArea* clamparea = first;
	CNavArea* centerarea = next;

	//
	if (next->m_center.z != first->m_center.z)
	{
		clamparea = next;

	}

	return ClosestPointOnArea(clamparea, centerarea->m_center + (vdir));
}


Vector JNavigation::GetMoveToPosition(CNavArea* area, CNavArea* next, Vector target)
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
	
	Vector mpos = JNavigation::nav_connection_position(area,next);
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