#include "nav_extended.h"
#include "navigation.h"
#include "help.h"

JNavExtended navhubex;

void JNavExtended::LoadCells()
{
	if (navhub.m_areas.empty()) return;
	cells = JNavCells(navhub.m_areas.size());//new JNavCell[navhub.m_areas.size()];
		
}

JNavCell* JNavExtended::GetCellFromArea(CNavArea* area)
{
	for (int i = 0; i < navhub.m_areas.size(); i++)
	{
		if (&navhub.m_areas[i] == area)
		{
			return GetCellFromAreaIndex(i);
		}
	}
	return nullptr;
}

JNavCell* JNavExtended::GetCellFromAreaIndex(int index)
{
	return &cells.at(index);
}

void JNavCell::CreateSnag(CNavArea* area, Vector position)
{
	auto cell = navhubex.GetCellFromArea(area);
	JNav_Snag snag = JNav_Snag();

	Vector best_direction;
	float biggest_size = -1;
	for (int i = 0; i < area->m_connections.size(); i++)
	{
		float size = VectorLength(area->m_connections[i].area->m_center - position);
		if (size > biggest_size)
		{
			biggest_size = size;
			best_direction = area->m_connections[i].area->m_center;
		}
	}

	snag.position = position - best_direction;
	snag.size = 120;
	cell->snags.push_back(snag);
}

float JNavCell::Cell_Score(CNavArea* area, int id)
{
	float score = snags.size() + (is_spawn ? 50000.0f : 0.0f);
	//float score = 0.0f;
	return score;
}
