#pragma once
#include "yuh.h"
#include <string>
#include "edict.h"
typedef u8* offsetptr_t;
typedef int offset_t;
class CBaseEntity;
struct edict_t;

//Note: These also include datamaps 


//helpers to not worry about if its m_i or m_n or just m_ cause sauce engine
#define JNP_PROPNAME_HEALTH "m_iHealth"
#define JNP_PROPNAME_HEALTH_MAX "m_iMaxHealth"
#define JNP_PROPNAME_TEAMNUM "m_iTeamNum"
#define JNP_PROPNAME_TFCLASS "m_iClass"
#define JNP_PROPNAME_TFCLASS_DESIRED "m_iDesiredPlayerClass"
#define JNP_PROPNAME_DISGUISETEAM "m_nDisguiseTeam"
#define JNP_PROPNAME_DISGUISECLASS "m_nDisguiseClass"
#define JNP_PROPNAME_CONDS1 "m_nPlayerCond"
#define JNP_PROPNAME_CONDS2 "m_nPlayerCondEx" //conds past ex arent implemented yet
#define JNP_PROPNAME_CONDS3 "m_nPlayerCondEx2"
#define JNP_PROPNAME_CONDS4 "m_nPlayerCondEx3"
#define JNP_PROPNAME_CONDS5 "m_nPlayerCondEx4"
#define JNP_PROPNAME_LIFESTATE "m_lifeState"
#define JNP_PROPNAME_OBSERVERMODE "m_iObserverMode"
#define JNP_PROPNAME_ABSORIGIN "m_vecOrigin"

struct edict_t;

static class JI_NetProps
{ public:
	//returns the offset relative to the CBaseEntity of the edict
	static offset_t FindNetProp(std::string propname);

	template <class T>
	static inline T* GetNetProp(edict_t* reference, std::string propname)
	{
		offset_t offset = FindNetProp(propname);
		offsetptr_t haha = reinterpret_cast<offsetptr_t>(reference->GetIServerEntity()->GetBaseEntity());
		haha += offset;
		return reinterpret_cast<T*>(haha);
	}

	template <class T>
	static inline T* GetNetProp(CBaseEntity* reference, std::string propname)
	{
		offset_t offset = FindNetProp(propname);
		offsetptr_t haha = reinterpret_cast<offsetptr_t>(reference);
		haha += offset;
		return reinterpret_cast<T*>(haha);
	}


};

