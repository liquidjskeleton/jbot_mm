#pragma once
#include <vector>
#include <ISmmAPI.h>

//extended functionality for valve's nav meshes for my bots to use smiley face
struct JNavCell;
class JNavExtended;
struct JNavCell_TeamData;

typedef std::vector<JNavCell> JNavCells;


class CNavArea;

struct JNav_Snag
{
	Vector position;
	float size;
};
typedef std::vector<JNav_Snag> JNavSnags;

struct JNavCell_TeamData
{
	float danger;
};
struct JNavCell
{
public:
	bool is_spawn;
	JNavCell_TeamData team_data[2];
	JNavSnags snags;

	void CreateSnag(CNavArea* area, Vector position);
	float Cell_Score(CNavArea* area, int id);
};
class JNavExtended
{
public:
	const char* map_name;
	JNavCells cells;

	JNavExtended() {};
	JNavExtended(const char* levelname)
	{
		map_name = levelname;
		LoadCells();
	}

	void LoadCells();
	JNavCell* GetCellFromArea(CNavArea* area);
	//NOT area->m_id. THIS IS THE INDEX IN THE NAV MESH!!!!!
	JNavCell* GetCellFromAreaIndex(int index);
};

