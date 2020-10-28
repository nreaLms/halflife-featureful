/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#pragma once
#ifndef ITEMS_H
#define ITEMS_H

#include "cbase.h"

class CItem : public CBaseDelay
{
public:
	void Spawn( void );
	CBaseEntity *Respawn( void );
	void EXPORT ItemTouch( CBaseEntity *pOther );
	void EXPORT Materialize( void );
	void EXPORT FallThink( void );
	virtual BOOL MyTouch( CBasePlayer *pPlayer )
	{
		return FALSE;
	}
	int ObjectCaps();
	void SetObjectCollisionBox();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void TouchOrUse( CBaseEntity* pOther );
	void SetMyModel( const char* model );
	void PrecacheMyModel( const char* model );
};
#endif // ITEMS_H
