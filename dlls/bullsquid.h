/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// bullsquid - big, spotty tentacle-mouthed meanie.
//=========================================================

#ifndef BULLSQUID_H
#define BULLSQUID_H

#include "monsters.h"

//=========================================================
// Bullsquid's spit projectile
//=========================================================
class CSquidSpit : public CBaseEntity
{
public:
	void Spawn(void);
	void Precache();

	static void Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, string_t soundList = iStringNull);
	static float SpitSpeed() { return 900.0f; }
	void Touch(CBaseEntity *pOther);
	void EXPORT Animate(void);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	int  m_maxFrame;
	
protected:
	void SpawnHelper(const char* className);
};

class CSquidToxicSpit : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache();

	static void Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity , string_t soundList = iStringNull);
	static float SpitSpeed() { return 600.0f; }
	void Touch( CBaseEntity *pOther );
	void EXPORT Animate( void );
	CBaseMonster* GetSpitOwner();

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	int m_maxFrame;

	int m_iImpactSprite;
	int m_iFleckSprite;
};

#endif // BULLSQUID_H
