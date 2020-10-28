#ifndef LOCUS_H
#define LOCUS_H

#include "cbase.h"

Vector		CalcLocus_Position	( CBaseEntity *pEntity, CBaseEntity *pLocus, const char *szText , bool *evaluated = 0 );
Vector		CalcLocus_Velocity	( CBaseEntity *pEntity, CBaseEntity *pLocus, const char *szText, bool *evaluated = 0 );
float		CalcLocus_Ratio		( CBaseEntity *pLocus, const char *szText, bool* evaluated = 0 );

#endif
