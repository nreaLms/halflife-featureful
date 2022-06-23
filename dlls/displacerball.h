#ifndef DISPLACERBALL_H
#define DISPLACERBALL_H

#include "mod_features.h"
#include "cbase.h"

#if FEATURE_DISPLACER

class CBeam;
//=========================================================
// Displacement field
//=========================================================
class CDisplacerBall : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache();

	static void Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, Vector vecAngles);
	static float BallSpeed() { return 500.0f; }
	static void SelfCreate(entvars_t *pevOwner, Vector vecStart);

	void EXPORT BallTouch(CBaseEntity *pOther);
	void EXPORT ExplodeThink( void );
	void EXPORT KillThink( void );
	void Circle( void );

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	CBeam* m_pBeam[8];

	void EXPORT FlyThink( void );
	void ClearBeams( void );
	void ArmBeam( int iSide );

	int m_iBeams;

	EHANDLE m_hDisplacedTarget;

	int m_iTrail;
};
#endif
#endif
