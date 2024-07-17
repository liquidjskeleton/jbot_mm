#pragma once

//basically cbase cause thats what i usually call these kinds of files but since this is the sauce engine they stole the name i stole so im naming it yuh
//because screw you

typedef unsigned char u8;
typedef  char s8;
typedef unsigned short u16;
typedef  short s16;
typedef  unsigned int u32;

#define MAX_PLAYERS 101


struct timestamp_t
{
	float relative_time;
	union { int idata; float fdata; } data;
};

u32 J_Random() noexcept;
int J_RandomRange(int min, int max) noexcept;
int J_RandomRange(int max) noexcept;
int J_RandomRangeF(float min, float max,float scaling) noexcept;
void J_Random_Seed(u32 seed) noexcept;

//just a counter. use for whatever you want!
extern u32 static_counter;