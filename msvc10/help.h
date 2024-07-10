#pragma once
#ifndef JHELPME //WHAT???? HELP ME!!! (click like to save the chicekn nuget
#include <ISmmAPI.h>

extern ISmmAPI* g_SMAPI;

#define jprint META_CONPRINT
#define jprintf META_CONPRINTF
#define jprints for (int i=0;i<10;i++){META_CONPRINT("-----------");}
#define jlogerror //while(1){} //cheap way to halt the console
#define jdelay {jprint("(intentional delay)\n");unsigned long i=1; while(i){i++;}} //cheap way to delay the inevitable
#define jhelp_start jhelp
#define jhelp jprintf("helpme! %i\n",__LINE__); jdelay;

//
#define reintrp(value,castto) (*reinterpret_cast<castto*>(&value))

#else

#define jprint 
#define jprintf 
#define jprints 
#endif