#pragma once
#include "teamfortress.h"
#include "yuh.h"
#include <ISmmAPI.h>

#define BOT_RELATIONSHIPS 0

#if BOT_RELATIONSHIPS
class JBotRelationship
{

};
#endif
class JBotProfile
{ public:
	TFTeam m_iTeamNum;
#if BOT_RELATIONSHIPS
	JBotRelationship m_relationships[MAX_PLAYERS];
#endif
	JBotProfile() {}
	JBotProfile(TFTeam curteam)
	{
		Q_memset(this, 0, sizeof(JBotProfile));
		m_iTeamNum = curteam;
	}
};

