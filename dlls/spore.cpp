#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"
#include	"decals.h"
#include	"gamerules.h"
#include	"skill.h"
#include	"spore.h"

#if FEATURE_SPOREGRENADE
LINK_ENTITY_TO_CLASS(monster_spore, CSporeGrenade)

TYPEDESCRIPTION	CSporeGrenade::m_SaveData[] =
{
	DEFINE_FIELD(CSporeGrenade, m_pSporeGlow, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE(CSporeGrenade, CBaseMonster)

int gSporeExplode, gSporeExplodeC, g_sModelIndexTinySpit;

void CSporeGrenade::Precache(void)
{
	PRECACHE_MODEL("models/spore.mdl");
	PRECACHE_MODEL("sprites/glow02.spr");
	g_sModelIndexTinySpit = PRECACHE_MODEL("sprites/tinyspit.spr");
	gSporeExplode = PRECACHE_MODEL ("sprites/spore_exp_01.spr");
	gSporeExplodeC = PRECACHE_MODEL ("sprites/spore_exp_c_01.spr");
	PRECACHE_SOUND("weapons/splauncher_bounce.wav");
	PRECACHE_SOUND("weapons/splauncher_impact.wav");
}

void CSporeGrenade::Explode(TraceResult *pTrace, int bitsDamageType)
{
	pev->solid = SOLID_NOT;// intangible
	pev->takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if (pTrace->flFraction != 1.0)
	{
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * (pev->dmg - 24) * 0.6);
	}

	Vector vecSpraySpot = pTrace->vecEndPos;
	float flSpraySpeed = RANDOM_LONG(10, 15);

	// If the trace is pointing up, then place
	// spawn position a few units higher.
	if (pTrace->vecPlaneNormal.z > 0)
	{
		vecSpraySpot = vecSpraySpot + (pTrace->vecPlaneNormal * 8);
		flSpraySpeed *= 2; // Double the speed to make them fly higher
						   // in the air.
	}

	// Spawn small particles at the explosion origin.
	SpawnExplosionParticles(
		vecSpraySpot,				// position
		pTrace->vecPlaneNormal,			// direction
		g_sModelIndexTinySpit,			// modelindex
		RANDOM_LONG(40, 50),				// count
		flSpraySpeed,					// speed
		RANDOM_FLOAT(600, 640));		// noise

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SPRITE );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( RANDOM_LONG( 0, 1 ) ? gSporeExplode : gSporeExplodeC );
		WRITE_BYTE( 25  ); // scale * 10
		WRITE_BYTE( 155  ); // framerate
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD( pev->origin.x );	// X
		WRITE_COORD( pev->origin.y );	// Y
		WRITE_COORD( pev->origin.z );	// Z
		WRITE_BYTE( 12 );		// radius * 0.1
		WRITE_BYTE( 0 );		// r
		WRITE_BYTE( 180 );		// g
		WRITE_BYTE( 0 );		// b
		WRITE_BYTE( 20 );		// time * 10
		WRITE_BYTE( 20 );		// decay * 0.1
	MESSAGE_END( );

	// Play explode sound.
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/splauncher_impact.wav", 1, ATTN_NORM);

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0);
	entvars_t *pevOwner;
	if (pev->owner)
		pevOwner = VARS(pev->owner);
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	RadiusDamage(pev, pevOwner, pev->dmg, CLASS_NONE, bitsDamageType);

	// Place a decal on the surface that was hit.
	UTIL_DecalTrace(pTrace, DECAL_YBLOOD5 + RANDOM_LONG(0, 1));

	UTIL_Remove(this);
	if (m_pSporeGlow)
	{
		UTIL_Remove(m_pSporeGlow);
		m_pSporeGlow = NULL;
	}
}

void CSporeGrenade::Detonate(void)
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	vecSpot = pev->origin + Vector(0, 0, 8);
	UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -40), ignore_monsters, ENT(pev), &tr);

	Explode(&tr, DMG_BLAST);
}


void CSporeGrenade::BounceSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/splauncher_bounce.wav", 0.4, ATTN_NORM);
}

void CSporeGrenade::TumbleThink(void)
{
	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->dmgtime - 1 < gpGlobals->time)
	{
		CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin + pev->velocity * (pev->dmgtime - gpGlobals->time), 400, 0.1);
	}

	if (pev->dmgtime <= gpGlobals->time)
	{
		SetThink(&CSporeGrenade::Detonate);
	}
	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.5;
	}

	// Spawn particles.
	SpawnTrailParticles(
		pev->origin,					// position
		-pev->velocity.Normalize(),		// dir
		g_sModelIndexTinySpit,			// modelindex
		RANDOM_LONG( 2, 4 ),			// count
		RANDOM_FLOAT(10, 15),			// speed
		RANDOM_FLOAT(2, 3) * 100);		// noise ( client will divide by 100 )
}

//
// Contact grenade, explode when it touches something
//
void CSporeGrenade::ExplodeTouch(CBaseEntity *pOther)
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine(vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT(pev), &tr);

	Explode(&tr, DMG_BLAST);
}

void CSporeGrenade::DangerSoundThink(void)
{
	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5, pev->velocity.Length(), 0.2);
	pev->nextthink = gpGlobals->time + 0.2;

	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.5;
	}

	// Spawn particles.
	SpawnTrailParticles(
		pev->origin,					// position
		-pev->velocity.Normalize(),		// dir
		g_sModelIndexTinySpit,			// modelindex
		RANDOM_LONG( 5, 10),				// count
		RANDOM_FLOAT(10, 15),			// speed
		RANDOM_FLOAT(2, 3) * 100);		// noise ( client will divide by 100 )
}

void CSporeGrenade::BounceTouch(CBaseEntity *pOther)
{
	if (pev->flags & FL_ONGROUND)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.03;
	}
	else
	{
		// play bounce sound
		BounceSound();
	}
}

void CSporeGrenade::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE;

	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/spore.mdl");
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	pev->gravity = 0.5; // 0.5
	//pev->friction = 0.8; // 0.8

	pev->dmg = gSkillData.plrDmgSpore;

	m_pSporeGlow = CSprite::SpriteCreate("sprites/glow02.spr", pev->origin, FALSE);

	if (m_pSporeGlow)
	{
		m_pSporeGlow->SetTransparency(kRenderGlow, 150, 158, 19, 155, kRenderFxNoDissipation);
		m_pSporeGlow->SetAttachment(edict(), 0);
		m_pSporeGlow->SetScale(.75f);
	}
}

CBaseEntity* CSporeGrenade::ShootTimed(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time)
{
	CSporeGrenade *pGrenade = GetClassPtr((CSporeGrenade *)NULL);
	pGrenade->Spawn();
	UTIL_SetOrigin(pGrenade->pev, vecStart);
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles(pGrenade->pev->velocity);
	pGrenade->pev->owner = ENT(pevOwner);

	pGrenade->SetTouch(&CSporeGrenade::BounceTouch);	// Bounce if touched

	pGrenade->pev->dmgtime = gpGlobals->time + time;
	pGrenade->SetThink(&CSporeGrenade::TumbleThink);
	pGrenade->pev->nextthink = gpGlobals->time + 0.1;
	if (time < 0.1)
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = Vector(0, 0, 0);
	}

	// Tumble through the air
	// pGrenade->pev->avelocity.x = -400;

	pGrenade->pev->gravity = 0.5;
	pGrenade->pev->friction = 0.8;

	SET_MODEL(ENT(pGrenade->pev), "models/spore.mdl");
	pGrenade->pev->dmg = gSkillData.plrDmgSpore;

	return pGrenade;
}

CBaseEntity *CSporeGrenade::ShootContact(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity)
{
	CSporeGrenade *pGrenade = GetClassPtr((CSporeGrenade *)NULL);
	pGrenade->Spawn();
	pGrenade->pev->movetype = MOVETYPE_FLY;
	UTIL_SetOrigin(pGrenade->pev, vecStart);
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles(pGrenade->pev->velocity);
	pGrenade->pev->owner = ENT(pevOwner);

	// make monsters afaid of it while in the air
	pGrenade->SetThink(&CSporeGrenade::DangerSoundThink);
	pGrenade->pev->nextthink = gpGlobals->time;

	// Explode on contact
	pGrenade->SetTouch(&CSporeGrenade::ExplodeTouch);

	pGrenade->pev->dmg = gSkillData.plrDmgSpore;

	return pGrenade;
}

void CSporeGrenade::SpawnTrailParticles(const Vector& origin, const Vector& direction, int modelindex, int count, float speed, float noise)
{
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, origin);
	WRITE_BYTE(TE_SPRITE_SPRAY);
		WRITE_COORD(origin.x);				// pos
		WRITE_COORD(origin.y);
		WRITE_COORD(origin.z);
		WRITE_COORD(direction.x);			// dir
		WRITE_COORD(direction.y);
		WRITE_COORD(direction.z);
		WRITE_SHORT(modelindex);			// model
		WRITE_BYTE(count);					// count
		WRITE_BYTE(speed);					// speed
		WRITE_BYTE(noise);					// noise ( client will divide by 100 )
	MESSAGE_END();
}

void CSporeGrenade::SpawnExplosionParticles(const Vector& origin, const Vector& direction, int modelindex, int count, float speed, float noise)
{

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, origin);
		WRITE_BYTE(TE_SPRITE_SPRAY);
		WRITE_COORD(origin.x);				// pos
		WRITE_COORD(origin.y);
		WRITE_COORD(origin.z);
		WRITE_COORD(direction.x);			// dir
		WRITE_COORD(direction.y);
		WRITE_COORD(direction.z);
		WRITE_SHORT(modelindex);			// model
		WRITE_BYTE(count);					// count
		WRITE_BYTE(speed);					// speed
		WRITE_BYTE(noise);					// noise ( client will divide by 100 )
	MESSAGE_END();
}
#endif
