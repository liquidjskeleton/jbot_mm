#include "navigation.h"
#include "mathlib/mathlib.h"
#include <algorithm>

#include "help.h"
CNavFile navhub("arena_well");
void JNavigation::Load(const char* levelname)
{
	jprintf("name=%s\n", levelname);
	navhub = CNavFile(levelname);
}


struct NavR
{
	bool* checks;
	NavAreaDisP disp_list;
	bool failsafe;
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
float navigation_scoring(CNavArea* end, CNavArea* area)
{
	Vector vecdif = area->m_center - end->m_center;
	float length = VectorLength(vecdif);
	int connections = area->m_connections.size();
	float fconnections = connections;

	float score = length - ((3 -fconnections) * 10.0f) - (vecdif.z*8);
	return score;
}

//check the one with the lowest distance to target first and then go through the others thats what imn tryina do here
//todo: move some of the vectors to the heap maybe.. this may be bad for huge meshes
bool checknavarea_recursive(CNavArea* it, CNavArea* end,NavR* navr)
{
	//first, flag self
	//jprintf("currently on id %i/%i\n", navareaid(it),navhub.m_areas.size());
	//jdelay;
	navr->checks[navareaid(it)] = 1;
	int connections = it->m_connections.size();
	//sort connections by distance
	NavAreaDisP vec;
	for (int i = 0; i < connections; i++)
	{
		CNavArea* area = it->m_connections[i].area;

		//check if the area is the end
		if (area == end)
		{
			//we found it boys
			navr->disp_list.push_back(DisP(0,end));
			return true;
		}

		//check if the area is flagged already
		if (navr->checks[navareaid(area)]) { continue; }
		navr->checks[navareaid(area)] = 1;
		float score = navigation_scoring(end, area);
		//Vector vecdif = area->m_center - end->m_center;
		//float length = VectorLength(vecdif);
		DisP disp = DisP(score,area);
		vec.push_back(disp);
	}
	if (vec.empty()) return false; //no connections will work
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
		}
	}
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

JNav_PathfindResult JNavigation::Pathfind(CNavArea* start, CNavArea* end,Vector targetpos, Vector mypos)
{
	JNav_PathfindResult result = JNav_PathfindResult();
	result.canpathfind = false;
	int numofareas = navhub.m_areas.size();
	//jprintf("there are %i areas\n", numofareas);
	if(numofareas == 0)
	{
		//jprint("No nav mesh\n");
		
		return result;
	}
	if (start == end) { result.canpathfind = true; result.disp.push_back(DisP(0, end)); return result; }
	if (!start)
	{
		start = nav_failsafe(mypos); //we are DESPERATE
	}

	if (!end)
	{
		end = nav_failsafe(targetpos);
	}
	//jdelay
	NavR navr = NavR();
	navr.checks = new bool[numofareas];
	Q_memset(navr.checks, 0, numofareas);
	//jprintf("starting pathing. start:%i,end:%i\n", start,end);
	//jdelay
	//jdelay
	bool res = checknavarea_recursive(start, end, &navr);
	delete[] navr.checks;
	//jprintf("finished pathing. result:%i\n", res);
	result.canpathfind = true; result.disp = navr.disp_list; return result;
}
