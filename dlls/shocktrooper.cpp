#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"schedule.h"
#include	"animation.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"
#include	"decals.h"
#include	"gamerules.h"
#include	"hgrunt.h"
#include	"mod_features.h"

#ifdef FEATURE_SHOCKTROOPER

#define SHOCK_BEAM_LENGTH		64
#define SHOCK_BEAM_LENGTH_HALF	SHOCK_BEAM_LENGTH * 0.5f

#define SHOCK_BEAM_WIDTH		50

//=========================================================
// Shockrifle projectile
//=========================================================
class CShock : public CBaseAnimating
{
public:
	void Spawn(void);
	void Precache();

	static void Shoot(entvars_t *pevOwner, const Vector angles, const Vector vecStart, const Vector vecVelocity);
	void Touch(CBaseEntity *pOther);
	void EXPORT FlyThink();

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	void CreateEffects();
	void ClearEffects();

	CBeam *m_pBeam;
	CBeam *m_pNoise;
	CSprite *m_pSprite;
};

LINK_ENTITY_TO_CLASS(shock_beam, CShock)

TYPEDESCRIPTION	CShock::m_SaveData[] =
{
	DEFINE_FIELD(CShock, m_pBeam, FIELD_CLASSPTR),
	DEFINE_FIELD(CShock, m_pNoise, FIELD_CLASSPTR),
	DEFINE_FIELD(CShock, m_pSprite, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE(CShock, CBaseAnimating)

void CShock::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->classname = MAKE_STRING("shock_beam");
	SET_MODEL(ENT(pev), "models/shock_effect.mdl");
	UTIL_SetOrigin(pev, pev->origin);

	if ( g_pGameRules->IsMultiplayer() )
		pev->dmg = gSkillData.plrDmgShockroachM;
	else
		pev->dmg = gSkillData.plrDmgShockroach;
	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));

	CreateEffects();
	SetThink( &CShock::FlyThink );
	pev->nextthink = gpGlobals->time;
}

void CShock::Precache()
{
	PRECACHE_MODEL("sprites/flare3.spr");
	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_MODEL("models/shock_effect.mdl");
	PRECACHE_SOUND("weapons/shock_impact.wav");
}

void CShock::FlyThink()
{
	if (pev->waterlevel == 3)
	{
		entvars_t *pevOwner = VARS(pev->owner);
		const int iVolume = RANDOM_FLOAT(0.8f, 1);
		const int iPitch = RANDOM_FLOAT(80, 110);
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/shock_impact.wav", iVolume, ATTN_NORM, 0, iPitch);
		RadiusDamage(pev->origin, pev, pevOwner ? pevOwner : pev, pev->dmg * 3, 144, CLASS_NONE, DMG_SHOCK | DMG_ALWAYSGIB );
		ClearEffects();
		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time;
	}
	else
	{
		pev->nextthink = gpGlobals->time + 0.05;
	}
}

void CShock::Shoot(entvars_t *pevOwner, const Vector angles, const Vector vecStart, const Vector vecVelocity)
{
	CShock *pShock = GetClassPtr((CShock *)NULL);
	pShock->Spawn();

	UTIL_SetOrigin(pShock->pev, vecStart);
	pShock->pev->velocity = vecVelocity;
	pShock->pev->owner = ENT(pevOwner);
	pShock->pev->angles = angles;

	pShock->pev->nextthink = gpGlobals->time;
}

void CShock::Touch(CBaseEntity *pOther)
{
	// Do not collide with the owner.
	if (ENT(pOther->pev) == pev->owner)
		return;

	TraceResult tr = UTIL_GetGlobalTrace( );
	int		iPitch, iVolume;

	// Lower the volume if touched entity is not a player.
	iVolume = (!pOther->IsPlayer())
		? RANDOM_FLOAT(0.4f, 0.5f)
		: RANDOM_FLOAT(0.8f, 1);

	iPitch = RANDOM_FLOAT(80, 110);

	// splat sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/shock_impact.wav", iVolume, ATTN_NORM, 0, iPitch);
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(pev->origin.x);	// X
		WRITE_COORD(pev->origin.y);	// Y
		WRITE_COORD(pev->origin.z);	// Z
		WRITE_BYTE( 8 );		// radius * 0.1
		WRITE_BYTE( 0 );		// r
		WRITE_BYTE( 255 );		// g
		WRITE_BYTE( 255 );		// b
		WRITE_BYTE( 10 );		// time * 10
		WRITE_BYTE( 10 );		// decay * 0.1
	MESSAGE_END( );

	ClearEffects();
	if (!pOther->pev->takedamage)
	{
		// make a splat on the wall
		UTIL_DecalTrace(&tr, DECAL_SMALLSCORCH1 + RANDOM_LONG(0, 2));

		int iContents = UTIL_PointContents(pev->origin);

		// Create sparks
		if (iContents != CONTENTS_WATER)
		{
			UTIL_Sparks(tr.vecEndPos);
		}
	}
	else
	{
		ClearMultiDamage();
		entvars_t *pevOwner = VARS(pev->owner);
		int damageType = DMG_ENERGYBEAM;
		if (pOther->pev->deadflag == DEAD_DEAD)
		{
			damageType |= DMG_ALWAYSGIB;
		}
		else
		{
			CBaseMonster* pMonster = pOther->MyMonsterPointer();
			if (pMonster)
				pMonster->GlowShellOn( Vector( 0, 220, 255 ), .5f );
		}
		pOther->TraceAttack(pev, pev->dmg, pev->velocity.Normalize(), &tr, damageType );
		ApplyMultiDamage(pev, pevOwner ? pevOwner : pev);
		if (pOther->IsPlayer() && (UTIL_PointContents(pev->origin) != CONTENTS_WATER))
		{
			const Vector position = tr.vecEndPos;
			MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY, NULL, pOther->pev );
				WRITE_BYTE( TE_SPARKS );
				WRITE_COORD( position.x );
				WRITE_COORD( position.y );
				WRITE_COORD( position.z );
			MESSAGE_END();
		}
	}
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

void CShock::CreateEffects()
{
	m_pSprite = CSprite::SpriteCreate( "sprites/flare3.spr", pev->origin, FALSE );
	m_pSprite->SetAttachment( edict(), 0 );
	m_pSprite->pev->scale = 0.4;
	m_pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 170, kRenderFxNoDissipation );
	m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;
	m_pSprite->pev->flags |= FL_SKIPLOCALHOST;

	m_pBeam = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );

	if (m_pBeam)
	{
		m_pBeam->EntsInit( entindex(), entindex() );
		m_pBeam->SetStartAttachment( 1 );
		m_pBeam->SetEndAttachment( 2 );
		m_pBeam->SetBrightness( 190 );
		m_pBeam->SetScrollRate( 20 );
		m_pBeam->SetNoise( 20 );
		m_pBeam->SetFlags( BEAM_FSHADEOUT );
		m_pBeam->SetColor( 0, 255, 255 );
		m_pBeam->pev->spawnflags = SF_BEAM_TEMPORARY;
		m_pBeam->RelinkBeam();
	}

	m_pNoise = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );

	if (m_pNoise)
	{
		m_pNoise->EntsInit( entindex(), entindex() );
		m_pNoise->SetStartAttachment( 1 );
		m_pNoise->SetEndAttachment( 2 );
		m_pNoise->SetBrightness( 190 );
		m_pNoise->SetScrollRate( 20 );
		m_pNoise->SetNoise( 65 );
		m_pNoise->SetFlags( BEAM_FSHADEOUT );
		m_pNoise->SetColor( 255, 255, 173 );
		m_pNoise->pev->spawnflags = SF_BEAM_TEMPORARY;
		m_pNoise->RelinkBeam();
	}
}

void CShock::ClearEffects()
{
	UTIL_Remove( m_pBeam );
	m_pBeam = NULL;

	UTIL_Remove( m_pNoise );
	m_pNoise = NULL;

	UTIL_Remove( m_pSprite );
	m_pSprite = NULL;
}

class CSporeGrenade : public CBaseMonster
{
public:
	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);

	static	TYPEDESCRIPTION m_SaveData[];

	void Precache(void);
	void Spawn(void);

	static CBaseEntity *ShootTimed(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time);
	static CBaseEntity *ShootContact(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity);

	void Explode(TraceResult *pTrace, int bitsDamageType);

	void EXPORT BounceTouch(CBaseEntity *pOther);
	void EXPORT ExplodeTouch(CBaseEntity *pOther);
	void EXPORT DangerSoundThink(void);
	void EXPORT Detonate(void);
	void EXPORT TumbleThink(void);

	void BounceSound(void);
	static void SpawnTrailParticles(const Vector& origin, const Vector& direction, int modelindex, int count, float speed, float noise);
	static void SpawnExplosionParticles(const Vector& origin, const Vector& direction, int modelindex, int count, float speed, float noise);

	CSprite* m_pSporeGlow;
};

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

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define STROOPER_VOL						0.35		// volume of grunt sounds
#define STROOPER_ATTN						ATTN_NORM	// attenutation of grunt sentences
#define STROOPER_LIMP_HEALTH				20
#define STROOPER_DMG_HEADSHOT				( DMG_BULLET | DMG_CLUB )	// damage types that can kill a grunt with a single headshot.
#define STROOPER_NUM_HEADS					2 // how many grunt heads are there?
#define STROOPER_MINIMUM_HEADSHOT_DAMAGE	15 // must do at least this much damage in one shot to head to score a headshot kill
#define	STROOPER_SENTENCE_VOLUME			(float)0.45 // volume of grunt sentences
#define STROOPER_MUZZLEFLASH	"sprites/muzzle_shock.spr"

#define STROOPER_SHOCKRIFLE			(1 << 0)
#define STROOPER_HANDGRENADE		(1 << 1)

#define STROOPER_GUN_GROUP					1
#define GUN_SHOCKRIFLE				0
#define STROOPER_GUN_NONE					1

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		STROOPER_AE_RELOAD			( 2 )
#define		STROOPER_AE_KICK			( 3 )
#define		STROOPER_AE_BURST1			( 4 )
#define		STROOPER_AE_BURST2			( 5 )
#define		STROOPER_AE_BURST3			( 6 )
#define		STROOPER_AE_GREN_TOSS		( 7 )
#define		STROOPER_AE_GREN_LAUNCH		( 8 )
#define		STROOPER_AE_GREN_DROP		( 9 )
#define		STROOPER_AE_CAUGHT_ENEMY	( 10 ) // shocktrooper established sight with an enemy (player only) that had previously eluded the squad.
#define		STROOPER_AE_DROP_GUN		( 11 ) // shocktrooper (probably dead) is dropping his shockrifle.


//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_STROOPER_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_STROOPER_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_STROOPER_COVER_AND_RELOAD,
	SCHED_STROOPER_SWEEP,
	SCHED_STROOPER_FOUND_ENEMY,
	SCHED_STROOPER_REPEL,
	SCHED_STROOPER_REPEL_ATTACK,
	SCHED_STROOPER_REPEL_LAND,
	SCHED_STROOPER_WAIT_FACE_ENEMY,
	SCHED_STROOPER_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_STROOPER_ELOF_FAIL,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_STROOPER_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_STROOPER_SPEAK_SENTENCE,
	TASK_STROOPER_CHECK_FIRE,
};

int g_fStrooperQuestion;	// true if an idle grunt asked a question. Cleared when someone answers.
int iStrooperMuzzleFlash;

//=========================================================
// shocktrooper
//=========================================================
class CStrooper : public CHGrunt
{
public:
	void Spawn(void);
	void Precache(void);
	int  DefaultClassify(void);
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin + Vector( -24, -24, 0 );
		pev->absmax = pev->origin + Vector( 24, 24, 72 );
	}

	void SetActivity(Activity NewActivity);

	void DeathSound(void);
	void PainSound(void);
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void GibMonster(void);
	void CheckAmmo();

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	int	Save(CSave &save);
	int Restore(CRestore &restore);

	void SpeakSentence();

	static TYPEDESCRIPTION m_SaveData[];

	BOOL m_bRightClaw;
	float m_rechargeTime;

	static const char *pGruntSentences[];

protected:
	float SentenceVolume();
	float SentenceAttn();
	const char* SentenceByNumber(int sentence);
	Schedule_t* ScheduleOnRangeAttack1();
};

LINK_ENTITY_TO_CLASS(monster_shocktrooper, CStrooper)

TYPEDESCRIPTION	CStrooper::m_SaveData[] =
{
	DEFINE_FIELD(CStrooper, m_bRightClaw, FIELD_BOOLEAN),
	DEFINE_FIELD(CStrooper, m_rechargeTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CStrooper, CHGrunt)

const char *CStrooper::pGruntSentences[] =
{
	"ST_GREN",		// grenade scared grunt
	"ST_ALERT",		// sees player
	"ST_MONST",		// sees monster
	"ST_COVER",		// running to cover
	"ST_THROW",		// about to throw grenade
	"ST_CHARGE",	// running out to get the enemy
	"ST_TAUNT",		// say rude things
	"ST_CHECK",
	"ST_QUEST",
	"ST_IDLE",
	"ST_CLEAR",
	"ST_ANSWER",
};

enum
{
	STROOPER_SENT_NONE = -1,
	STROOPER_SENT_GREN = 0,
	STROOPER_SENT_ALERT,
	STROOPER_SENT_MONSTER,
	STROOPER_SENT_COVER,
	STROOPER_SENT_THROW,
	STROOPER_SENT_CHARGE,
	STROOPER_SENT_TAUNT,
	STROOPER_SENT_CHECK,
	STROOPER_SENT_QUEST,
	STROOPER_SENT_IDLE,
	STROOPER_SENT_CLEAR,
	STROOPER_SENT_ANSWER,
} STROOPER_SENTENCE_TYPES;

void CStrooper::SpeakSentence( void )
{
	if( m_iSentence == STROOPER_SENT_NONE )
	{
		// no sentence cued up.
		return;
	}

	if( FOkToSpeak() )
	{
		SENTENCEG_PlayRndSz( ENT( pev ), pGruntSentences[m_iSentence], STROOPER_SENTENCE_VOLUME, STROOPER_ATTN, 0, m_voicePitch );
		JustSpoke();
	}
}

const char* CStrooper::SentenceByNumber(int sentence)
{
	return pGruntSentences[sentence];
}

Schedule_t* CStrooper::ScheduleOnRangeAttack1()
{
	if( InSquad() )
	{
		// if the enemy has eluded the squad and a squad member has just located the enemy
		// and the enemy does not see the squad member, issue a call to the squad to waste a
		// little time and give the player a chance to turn.
		if( MySquadLeader()->m_fEnemyEluded && !HasConditions( bits_COND_ENEMY_FACING_ME ) )
		{
			MySquadLeader()->m_fEnemyEluded = FALSE;
			return GetScheduleOfType( SCHED_STROOPER_FOUND_ENEMY );
		}
	}

	const bool preferAttack2 = HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && RANDOM_LONG(0,3) == 0;
	if (preferAttack2)
	{
		if( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
		{
			// throw a grenade if can and no engage slots are available
			return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
		}
		else if( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
		{
			// try to take an available ENGAGE slot
			return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
		}
	}
	else
	{
		if( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
		{
			// try to take an available ENGAGE slot
			return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
		}
		else if( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
		{
			// throw a grenade if can and no engage slots are available
			return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
		}
	}
	return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
}

float CStrooper::SentenceVolume()
{
	return STROOPER_SENTENCE_VOLUME;
}

float CStrooper::SentenceAttn()
{
	return STROOPER_ATTN;
}

#define STROOPER_GIB_COUNT 8
//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CStrooper::GibMonster(void)
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

	if (GetBodygroup(STROOPER_GUN_GROUP) != STROOPER_GUN_NONE)
	{// throw a shockroach if the shock trooper has one
		GetAttachment(0, vecGunPos, vecGunAngles);

		CBaseEntity* pRoach = DropItem("monster_shockroach", vecGunPos, vecGunAngles);

		if (pRoach)
		{
			pRoach->pev->owner = edict();

			if (m_hEnemy)
				pRoach->pev->angles = (pev->origin - m_hEnemy->pev->origin).Normalize();

			// Remove any pitch.
			pRoach->pev->angles.x = 0;
		}
	}

	EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM );

	if( CVAR_GET_FLOAT( "violence_agibs" ) != 0 )	// Should never get here, but someone might call it directly
	{
		CGib::SpawnRandomGibs( pev, 6, "models/strooper_gibs.mdl", STROOPER_GIB_COUNT );	// Throw alien gibs
	}
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

void CStrooper::CheckAmmo()
{
	if (m_cAmmoLoaded < m_cClipSize)
	{
		if (m_rechargeTime < gpGlobals->time)
		{
			m_cAmmoLoaded++;
			m_rechargeTime = gpGlobals->time + gSkillData.strooperRchgSpeed;
		}
	}
	if (m_cAmmoLoaded <= 0) {
		SetConditions( bits_COND_NO_AMMO_LOADED );
	}
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CStrooper::DefaultClassify(void)
{
	return CLASS_ALIEN_MILITARY;
}

BOOL CStrooper::CheckRangeAttack1(float flDot, float flDist)
{
	return m_cAmmoLoaded >= 1 && CHGrunt::CheckRangeAttack1(flDot, flDist);
}

BOOL CStrooper::CheckRangeAttack2( float flDot, float flDist )
{
	return CheckRangeAttack2Impl(gSkillData.strooperGrenadeSpeed, flDot, flDist);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CStrooper::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case STROOPER_AE_DROP_GUN:
	{
		// switch to body group with no gun.
		if (GetBodygroup(STROOPER_GUN_GROUP) != STROOPER_GUN_NONE)
		{
			Vector	vecGunPos;
			Vector	vecGunAngles;

			GetAttachment(0, vecGunPos, vecGunAngles);
			SetBodygroup(STROOPER_GUN_GROUP, STROOPER_GUN_NONE);

			Vector vecDropAngles = vecGunAngles;


			if (m_hEnemy)
				vecDropAngles = UTIL_VecToAngles(m_hEnemy->pev->origin - pev->origin);


			// Remove any pitch.
			vecDropAngles.x = 0;

			// now spawn a shockroach.
			const int originalSolid = pev->solid;
			pev->solid = SOLID_NOT;
			CBaseEntity* pRoach = CBaseEntity::Create( "monster_shockroach", vecGunPos, vecDropAngles, edict() );
			if (pRoach)
			{
				// Remove any pitch.
				pRoach->pev->angles.x = 0;
			}
			pev->solid = originalSolid;
		}
	}
	break;

	case STROOPER_AE_RELOAD:
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	case STROOPER_AE_GREN_TOSS:
	{
		UTIL_MakeVectors(pev->angles);
		CSporeGrenade::ShootTimed(pev, GetGunPosition(), m_vecTossVelocity, 3.5);

		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
		// !!!LATER - when in a group, only try to throw grenade if ordered.
	}
	break;

	case STROOPER_AE_GREN_LAUNCH:
	case STROOPER_AE_GREN_DROP:
		break;

	case STROOPER_AE_BURST1:
	{
		Vector	vecGunPos;
		Vector	vecGunAngles;

		GetAttachment(0, vecGunPos, vecGunAngles);

		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecGunPos );
			WRITE_BYTE( TE_SPRITE );
			WRITE_COORD( vecGunPos.x );	// pos
			WRITE_COORD( vecGunPos.y );
			WRITE_COORD( vecGunPos.z );
			WRITE_SHORT( iStrooperMuzzleFlash );		// model
			WRITE_BYTE( 4 );				// size * 10
			WRITE_BYTE( 196 );			// brightness
		MESSAGE_END();

		Vector vecShootDir;
		if (m_hEnemy)
		{
			if (m_hEnemy->IsPlayer())
			{
				vecShootDir = (m_hEnemy->EyePosition() - vecGunPos).Normalize();
				vecShootDir.z += RANDOM_FLOAT( -0.05, 0 );
			}
			else
			{
				vecShootDir = (m_hEnemy->Center() - vecGunPos).Normalize();
				vecShootDir.z += RANDOM_FLOAT( 0, 0.05 );
			}
		}
		else
		{
			ALERT(at_aiconsole, "Shooting with no enemy! Ammo: %d\n", m_cAmmoLoaded);
			vecShootDir = (m_vecEnemyLKP - vecGunPos).Normalize();
			vecShootDir.z += RANDOM_FLOAT( 0, 0.05 );
		}

		vecGunAngles = UTIL_VecToAngles(vecShootDir);
		CShock::Shoot(pev, vecGunAngles, vecGunPos, vecShootDir * 2000);
		m_cAmmoLoaded--;

		// Play fire sound.
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/shock_fire.wav", 1, ATTN_NORM);

		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
	}
	break;

	case STROOPER_AE_KICK:
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "zombie/claw_miss2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM + RANDOM_LONG( -5, 5 ) );
		CBaseEntity *pHurt = Kick();

		if (pHurt)
		{
			// SOUND HERE!
			UTIL_MakeVectors(pev->angles);
			pHurt->pev->punchangle.x = 15;
			pHurt->pev->punchangle.z = (m_bRightClaw) ? -10 : 10;

			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
			pHurt->TakeDamage(pev, pev, gSkillData.strooperDmgKick, DMG_CLUB);
		}

		m_bRightClaw = !m_bRightClaw;
	}
	break;

	case STROOPER_AE_CAUGHT_ENEMY:
	{
		if (FOkToSpeak())
		{
			SENTENCEG_PlayRndSz(ENT(pev), "ST_ALERT", STROOPER_SENTENCE_VOLUME, STROOPER_ATTN, 0, m_voicePitch);
			JustSpoke();
		}

	}

	default:
		CHGrunt::HandleAnimEvent(pEvent);
		break;
	}
}


//=========================================================
// Spawn
//=========================================================
void CStrooper::Spawn()
{
	Precache();

	SpawnHelper("models/strooper.mdl", gSkillData.strooperHealth, BLOOD_COLOR_GREEN);
	UTIL_SetSize( pev, Vector(-24, -24, 0), Vector(24, 24, 72) );

	if (pev->weapons == 0)
	{
		// initialize to original values
		pev->weapons = STROOPER_SHOCKRIFLE | STROOPER_HANDGRENADE;
	}

	m_cClipSize = gSkillData.strooperMaxCharge;

	m_cAmmoLoaded = m_cClipSize;

	m_bRightClaw = FALSE;

	CTalkMonster::g_talkWaitTime = 0;
	m_rechargeTime = gpGlobals->time + gSkillData.strooperRchgSpeed;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CStrooper::Precache()
{
	PrecacheMyModel("models/strooper.mdl");
	PRECACHE_MODEL("models/strooper_gibs.mdl");
	iStrooperMuzzleFlash = PRECACHE_MODEL(STROOPER_MUZZLEFLASH);
	PRECACHE_SOUND("shocktrooper/shock_trooper_attack.wav");

	PRECACHE_SOUND("shocktrooper/shock_trooper_die1.wav");
	PRECACHE_SOUND("shocktrooper/shock_trooper_die2.wav");
	PRECACHE_SOUND("shocktrooper/shock_trooper_die3.wav");
	PRECACHE_SOUND("shocktrooper/shock_trooper_die4.wav");

	PRECACHE_SOUND("shocktrooper/shock_trooper_pain1.wav");
	PRECACHE_SOUND("shocktrooper/shock_trooper_pain2.wav");
	PRECACHE_SOUND("shocktrooper/shock_trooper_pain3.wav");
	PRECACHE_SOUND("shocktrooper/shock_trooper_pain4.wav");
	PRECACHE_SOUND("shocktrooper/shock_trooper_pain5.wav");

	PRECACHE_SOUND("weapons/shock_fire.wav");

	PRECACHE_SOUND("zombie/claw_miss2.wav");

	UTIL_PrecacheOther("shock_beam");
	UTIL_PrecacheOther("monster_spore");
	UTIL_PrecacheOther("monster_shockroach");

	// get voice pitch
	if (RANDOM_LONG(0, 1))
		m_voicePitch = 109 + RANDOM_LONG(0, 7);
	else
		m_voicePitch = 100;

	m_iBrassShell = PRECACHE_MODEL("models/shell.mdl");// brass shell
}


//=========================================================
// PainSound
//=========================================================
void CStrooper::PainSound(void)
{
	if (gpGlobals->time > m_flNextPainTime)
	{
		switch (RANDOM_LONG(0, 4))
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_pain1.wav", 1, ATTN_NORM);
			break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_pain2.wav", 1, ATTN_NORM);
			break;
		case 2:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_pain3.wav", 1, ATTN_NORM);
			break;
		case 3:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_pain4.wav", 1, ATTN_NORM);
			break;
		case 4:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_pain5.wav", 1, ATTN_NORM);
			break;
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

int CStrooper::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if( bitsDamageType == DMG_BULLET ) {
		flDamage = flDamage * 0.6;
	} else if(FClassnameIs(pevInflictor, "monster_spore") || FClassnameIs(pevInflictor, "shock_beam")) {
		flDamage = flDamage * 0.8;
	}
	return CHGrunt::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

//=========================================================
// DeathSound
//=========================================================
void CStrooper::DeathSound(void)
{
	switch (RANDOM_LONG(0, 3))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_die1.wav", 1, ATTN_IDLE);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_die2.wav", 1, ATTN_IDLE);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_die3.wav", 1, ATTN_IDLE);
		break;
	case 3:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_die4.wav", 1, ATTN_IDLE);
		break;
	}
}


//=========================================================
// SetActivity
//=========================================================
void CStrooper::SetActivity(Activity NewActivity)
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR(ENT(pev));

	switch (NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		// shocktrooper is either shooting standing or shooting crouched
		if (m_fStanding)
		{
			// get aimable sequence
			iSequence = LookupSequence("standing_mp5");
		}
		else
		{
			// get crouching shoot
			iSequence = LookupSequence("crouching_mp5");
		}
		break;
	case ACT_RANGE_ATTACK2:
		// shocktrooper is going to throw a grenade.

		// get toss anim
		iSequence = LookupSequence("throwgrenade");
		break;

	case ACT_RUN:
		if (pev->health <= STROOPER_LIMP_HEALTH)
		{
			// limp!
			iSequence = LookupActivity(ACT_RUN_HURT);
		}
		else
		{
			iSequence = LookupActivity(NewActivity);
		}
		break;
	case ACT_WALK:
		if (pev->health <= STROOPER_LIMP_HEALTH)
		{
			// limp!
			iSequence = LookupActivity(ACT_WALK_HURT);
		}
		else
		{
			iSequence = LookupActivity(NewActivity);
		}
		break;
	case ACT_IDLE:
		if (m_MonsterState == MONSTERSTATE_COMBAT)
		{
			NewActivity = ACT_IDLE_ANGRY;
		}
		iSequence = LookupActivity(NewActivity);
		break;
	default:
		iSequence = LookupActivity(NewActivity);
		break;
	}

	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if (iSequence > ACTIVITY_NOT_AVAILABLE)
	{
		if (pev->sequence != iSequence || !m_fSequenceLoops)
		{
			pev->frame = 0;
		}

		pev->sequence = iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo();
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT(at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity);
		pev->sequence = 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// TraceAttack - reimplemented in shock trooper because they never have helmets
//=========================================================
void CStrooper::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CSquadMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

#endif
