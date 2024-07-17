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

void JNavCell::CreateSnag(CNavArea* area, Vector position,Vector wishdir)
{
	auto cell = navhubex.GetCellFromArea(area);
	JNav_Snag snag = JNav_Snag();

	Vector best_direction = wishdir.Normalized() * 60;
	
	snag.position = position + best_direction;
	snag.size = 160;
	cell->snags.push_back(snag);
}

float JNavCell::Cell_Score(CNavArea* area, int id)
{
	float score = snags.size() + (is_spawn ? 50000.0f : 0.0f);
	//float score = 0.0f;
	return score;
}
