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
#if !defined(ITEMS_H)
#define ITEMS_H

#include "cbase.h"

class CPickup : public CBaseDelay
{
public:
	int ObjectCaps();
	void SetObjectCollisionBox();

	bool IsPickableByTouch();
	bool IsPickableByUse();

	void EXPORT FallThink( void );

	virtual Vector MyRespawnSpot() = 0;
	virtual float MyRespawnTime() = 0;

	CBaseEntity *Respawn( void );
	void EXPORT Materialize( void );
	virtual void OnMaterialize() = 0;
};

class CItem : public CPickup
{
public:
	void Spawn( void );
	void EXPORT ItemTouch( CBaseEntity *pOther );
	virtual BOOL MyTouch( CBasePlayer *pPlayer )
	{
		return FALSE;
	}
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void TouchOrUse( CBaseEntity* pOther );
	void SetMyModel( const char* model );
	void PrecacheMyModel( const char* model );

	Vector MyRespawnSpot();
	virtual float MyRespawnTime();
	void OnMaterialize();
};
#endif // ITEMS_H
