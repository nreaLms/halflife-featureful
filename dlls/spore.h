#ifndef SPORE_H
#define SPORE_H

#include "mod_features.h"

#if FEATURE_SPOREGRENADE
#include "ggrenade.h"

// Contact/Timed spore grenade
class CSpore : public CGrenade
{
public:
	enum SporeType
	{
		ROCKET = 1,
		GRENADE = 2
	};

public:
	int Save(CSave& save);
	int Restore(CRestore& restore);

	static TYPEDESCRIPTION m_SaveData[];

	void Precache();
	void Spawn();
	void UpdateOnRemove();

	void EXPORT IgniteThink();
	void EXPORT FlyThink();
	void EXPORT GibThink();
	void EXPORT RocketTouch(CBaseEntity* pOther);
	void EXPORT MyBounceTouch(CBaseEntity* pOther);

	static CSpore* CreateSpore(const Vector& vecOrigin, const Vector& vecAngles, const Vector &vecDirection, CBaseEntity* pOwner, SporeType sporeType, bool bIsAI = false, bool bPuked = false, EntityOverrides entityOverrides = EntityOverrides());
	static CSpore* ShootContact(CBaseEntity *pOwner, const Vector& vecOrigin, const Vector &vecAngles , const Vector &vecVelocity);
	static CSpore* ShootTimed(CBaseEntity *pOwner, const Vector &vecOrigin, const Vector& vecAngles, const Vector &vecVelocity, bool bIsAI = false);

	static float SporeRocketSpeed() { return 1200.0f; }
	static float SporeGrenadeSpeed() { return 800.0f; }

	static const NamedSoundScript bounceSoundScript;
	static const NamedSoundScript impactSoundScript;

private:
	int m_iBlow;
	int m_iBlowSmall;

	int m_iSpitSprite;
	int m_iTrail;

	SporeType m_SporeType;

	float m_flIgniteTime;
	float m_flSoundDelay;

	BOOL m_bIsAI;
	EHANDLE m_hSprite;
	BOOL m_bPuked;
};
#endif
#endif
