#ifndef SPORE_H
#define SPORE_H

#include "mod_features.h"
#include "basemonster.h"

#if FEATURE_SPOREGRENADE
class CSprite;

class CSporeGrenade : public CBaseMonster
{
public:
	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);

	static	TYPEDESCRIPTION m_SaveData[];

	void Precache(void);
	void Spawn(void);

	static CBaseEntity *ShootTimed(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, bool ai);
	static CBaseEntity *ShootContact(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity);

	void Explode(TraceResult *pTrace);

	void EXPORT BounceTouch(CBaseEntity *pOther);
	void EXPORT ExplodeTouch(CBaseEntity *pOther);
	void EXPORT DangerSoundThink(void);
	void EXPORT Detonate(void);
	void EXPORT TumbleThink(void);

	void BounceSound(void);
	void DangerSound();
	static void SpawnTrailParticles(const Vector& origin, const Vector& direction, int modelindex, int count, float speed, float noise);
	static void SpawnExplosionParticles(const Vector& origin, const Vector& direction, int modelindex, int count, float speed, float noise);

	void UpdateOnRemove();

	CSprite* m_pSporeGlow;
};
#endif
#endif
