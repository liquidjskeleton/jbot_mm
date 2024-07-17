#include "yuh.h"
#include <corecrt_math.h>

#define DEFAULT_RNG_SEED 0xbabadada
static u32 jrandom = DEFAULT_RNG_SEED;

u32 J_Random() noexcept
{
	jrandom ^= jrandom << 13;
	jrandom ^= jrandom >> 17;
	jrandom ^= jrandom << 5;
    return jrandom;
}

int J_RandomRange(int min, int max) noexcept
{
	const u32 rand = J_Random();
	int modulo = rand % (max + min);
	modulo -= min;
	return modulo;
}

int J_RandomRange(int max) noexcept
{
	return J_RandomRange(0,max);
}

int J_RandomRangeF(float min, float max, float scaling) noexcept
{
	scaling++;
	const u32 r = J_Random();
	const float f = (float)(r) / scaling;
	float resul = fmodf(f,min+max);
	resul -= min;
	return resul;
}

void J_Random_Seed(u32 seed) noexcept
{
	//seed must be a nonzero value
	if (seed == 0) seed = DEFAULT_RNG_SEED;
	jrandom = seed;
}


u32 static_counter;