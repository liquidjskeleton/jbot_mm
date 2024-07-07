#include "dynamic_offset.h"
#include <stdio.h>

#include "toolframework/iserverenginetools.h"
#include "toolframework/itoolentity.h"
#include <igameevents.h>
#include <iplayerinfo.h>
#include <sh_vector.h>
#include <ISmmAPI.h>

//unf, read header

void JDynamicOffset::RegisterOffsets(edict_t* classtoregister)
{
	IServerUnknown* unknown = classtoregister->GetUnknown();
	CBaseEntity* pEntity = unknown->GetBaseEntity();

	//get datasets from ent...

	//put into a big ol table...

	//???

	//profit
}
