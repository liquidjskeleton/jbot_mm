#pragma once

//unfinished as sourcemod registers offsets differently than i do, like around 1000 off which is strange as fuck so imma hold off on that

struct edict_t;
static class JDynamicOffset
{
	static void RegisterOffsets(edict_t* classtoregister);
};

