#ifndef LOCUS_H
#define LOCUS_H

#include "cbase.h"

bool		TryCalcLocus_Position	( CBaseEntity *pEntity, CBaseEntity *pLocus, const char *szText , Vector& result );
bool		TryCalcLocus_Velocity	( CBaseEntity *pEntity, CBaseEntity *pLocus, const char *szText, Vector& result );
bool		TryCalcLocus_Ratio		( CBaseEntity *pLocus, const char *szText, float& result );

#endif
