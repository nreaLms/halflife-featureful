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
LINK_ENTITY_TO_CLASS(spore, CSporeGrenade)

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

void CSporeGrenade::Explode(TraceResult *pTrace)
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

	RadiusDamage(pev, pevOwner, pev->dmg, CLASS_NONE, DMG_BLAST);

	// Place a decal on the surface that was hit.
#if FEATURE_OPFOR_DECALS
	UTIL_DecalTrace(pTrace, DECAL_SPR_SPLT1 + RANDOM_LONG(0, 2));
#else
	UTIL_DecalTrace(pTrace, DECAL_YBLOOD5 + RANDOM_LONG(0, 1));
#endif

	UTIL_Remove(this);
}

void CSporeGrenade::Detonate(void)
{
	TraceResult tr;
	Vector vecSpot = pev->origin + Vector(0, 0, 8);
	UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -40), ignore_monsters, ENT(pev), &tr);

	Explode(&tr);
}


void CSporeGrenade::BounceSound(void)
{
	DangerSound();
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/splauncher_bounce.wav", 0.25, ATTN_NORM);
}

void CSporeGrenade::DangerSound()
{
	CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5, pev->velocity.Length(), 0.2);
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

	Explode(&tr);
}

void CSporeGrenade::DangerSoundThink(void)
{
	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	DangerSound();
	pev->nextthink = gpGlobals->time + 0.2;

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
	if ( !pOther->pev->takedamage )
	{
		if (!(pev->flags & FL_ONGROUND)) {
			if (pev->dmg_save < gpGlobals->time) {
				BounceSound();
				pev->dmg_save = gpGlobals->time + 0.1;
			}
		} else {
			pev->velocity = pev->velocity * 0.9;
		}
		if (pev->flags & FL_SWIM)
		{
			pev->velocity = pev->velocity * 0.5;
		}
	}
	else
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		Explode(&tr);
	}
}

void CSporeGrenade::Spawn(void)
{
	Precache();
	pev->classname = MAKE_STRING("spore");
	pev->movetype = MOVETYPE_BOUNCE;

	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/spore.mdl");
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	//pev->gravity = 0.5;

	pev->dmg = gSkillData.plrDmgSpore;

	m_pSporeGlow = CSprite::SpriteCreate("sprites/glow02.spr", pev->origin, FALSE);

	if (m_pSporeGlow)
	{
		m_pSporeGlow->SetTransparency(kRenderGlow, 150, 158, 19, 155, kRenderFxNoDissipation);
		m_pSporeGlow->SetAttachment(edict(), 0);
		m_pSporeGlow->SetScale(.75f);
	}
}

CBaseEntity* CSporeGrenade::ShootTimed(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, bool ai)
{
	CSporeGrenade *pGrenade = GetClassPtr((CSporeGrenade *)NULL);
	UTIL_SetOrigin(pGrenade->pev, vecStart);
	pGrenade->Spawn();
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles(pGrenade->pev->velocity);
	pGrenade->pev->owner = ENT(pevOwner);

	pGrenade->SetTouch(&CSporeGrenade::BounceTouch);	// Bounce if touched

	float lifetime = 2.0;
	if (ai) {
		lifetime = 4.0;
		pGrenade->pev->gravity = 0.5;
		pGrenade->pev->friction = 0.9;
	}
	pGrenade->pev->dmgtime = gpGlobals->time + lifetime;
	pGrenade->SetThink(&CSporeGrenade::TumbleThink);
	pGrenade->pev->nextthink = gpGlobals->time + 0.1;
	if (lifetime < 0.1)
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = Vector(0, 0, 0);
	}

	return pGrenade;
}

CBaseEntity *CSporeGrenade::ShootContact(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity)
{
	CSporeGrenade *pGrenade = GetClassPtr((CSporeGrenade *)NULL);
	UTIL_SetOrigin(pGrenade->pev, vecStart);
	pGrenade->Spawn();
	pGrenade->pev->movetype = MOVETYPE_FLY;
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles(pGrenade->pev->velocity);
	pGrenade->pev->owner = ENT(pevOwner);

	// make monsters afraid of it while in the air
	pGrenade->SetThink(&CSporeGrenade::DangerSoundThink);
	pGrenade->pev->nextthink = gpGlobals->time;

	// Explode on contact
	pGrenade->SetTouch(&CSporeGrenade::ExplodeTouch);

	pGrenade->pev->gravity = 0.5;
	pGrenade->pev->friction = 0.7;

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

void CSporeGrenade::UpdateOnRemove()
{
	CBaseMonster::UpdateOnRemove();
	if (m_pSporeGlow)
	{
		UTIL_Remove(m_pSporeGlow);
		m_pSporeGlow = NULL;
	}
}

//=========================================================
// Opposing Forces Spore Ammo
//=========================================================
#define		SACK_GROUP			1
#define		SACK_EMPTY			0
#define		SACK_FULL			1

class CSporeAmmo : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT BornThink ( void );
	void EXPORT IdleThink ( void );
	void EXPORT AmmoTouch ( CBaseEntity *pOther );
	int  TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );

	int	Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	virtual int SizeForGrapple() { return GRAPPLE_FIXED; }

	int m_iExplode;
	BOOL borntime;
	float m_flTimeSporeIdle;
};


typedef enum
{
	SPOREAMMO_IDLE = 0,
	SPOREAMMO_SPAWNUP,
	SPOREAMMO_SNATCHUP,
	SPOREAMMO_SPAWNDOWN,
	SPOREAMMO_SNATCHDOWN,
	SPOREAMMO_IDLE1,
	SPOREAMMO_IDLE2,
} SPOREAMMO;

LINK_ENTITY_TO_CLASS( ammo_spore, CSporeAmmo )

TYPEDESCRIPTION	CSporeAmmo::m_SaveData[] =
{
	DEFINE_FIELD( CSporeAmmo, m_flTimeSporeIdle, FIELD_TIME ),
	DEFINE_FIELD( CSporeAmmo, borntime, FIELD_BOOLEAN ),
};
IMPLEMENT_SAVERESTORE( CSporeAmmo, CBaseEntity )

void CSporeAmmo :: Precache( void )
{
	PRECACHE_MODEL("models/spore_ammo.mdl");
	m_iExplode = PRECACHE_MODEL ("sprites/spore_exp_c_01.spr");
	PRECACHE_SOUND("weapons/spore_ammo.wav");
	UTIL_PrecacheOther ( "spore" );
}
//=========================================================
// Spawn
//=========================================================
void CSporeAmmo :: Spawn( void )
{
	Precache( );
	SET_MODEL(ENT(pev), "models/spore_ammo.mdl");
	UTIL_SetSize(pev, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ));
	pev->takedamage = DAMAGE_YES;
	pev->solid			= SOLID_BBOX;
	pev->movetype		= MOVETYPE_NONE;
	pev->framerate		= 1.0;
	pev->animtime		= gpGlobals->time + 0.1;

	pev->sequence = SPOREAMMO_IDLE1;
	pev->body = 1;

	Vector vecOrigin = pev->origin;
	vecOrigin.z += 16;
	UTIL_SetOrigin( pev, vecOrigin );

	pev->angles.x -= 90;// :3

	SetThink (&CSporeAmmo::IdleThink);
#if FEATURE_SPORELAUNCHER
	SetTouch (&CSporeAmmo::AmmoTouch);
#endif

	m_flTimeSporeIdle = gpGlobals->time + 20;
	pev->nextthink = gpGlobals->time + 0.1;
}

//=========================================================
// Override all damage
//=========================================================
int CSporeAmmo::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if (!borntime) // rigth '!borntime'  // blast in anytime 'borntime || !borntime'
	{
		Vector vecSrc = pev->origin + gpGlobals->v_forward * -32;

		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
			WRITE_COORD( vecSrc.x );	// Send to PAS because of the sound
			WRITE_COORD( vecSrc.y );
			WRITE_COORD( vecSrc.z );
			WRITE_SHORT( m_iExplode );
			WRITE_BYTE( 25  ); // scale * 10
			WRITE_BYTE( 12  ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NOSOUND );
		MESSAGE_END();


		//ALERT( at_console, "angles %f %f %f\n", pev->angles.x, pev->angles.y, pev->angles.z );

		Vector angles = pev->angles;
		angles.x -= 90;
		angles.y += 180;

		Vector vecLaunchDir = angles;

		vecLaunchDir.x += RANDOM_FLOAT( -20, 20 );
		vecLaunchDir.y += RANDOM_FLOAT( -20, 20 );
		vecLaunchDir.z += RANDOM_FLOAT( -20, 20 );

		UTIL_MakeVectors( vecLaunchDir );
		CSporeGrenade::ShootTimed(pevAttacker, vecSrc, gpGlobals->v_forward * 800, false);

		pev->framerate		= 1.0;
		pev->animtime		= gpGlobals->time + 0.1;
		pev->sequence		= SPOREAMMO_SNATCHDOWN;
		pev->body			= 0;
		borntime			= 1;
		m_flTimeSporeIdle = gpGlobals->time + 1;
		SetThink (&CSporeAmmo::IdleThink);
		return 1;
	}
	return 0;
}

//=========================================================
// Thinking begin
//=========================================================
void CSporeAmmo :: BornThink ( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if ( m_flTimeSporeIdle > gpGlobals->time )
		return;

	pev->sequence = 3;
	pev->framerate		= 1.0;
	pev->animtime		= gpGlobals->time + 0.1;
	pev->body = 1;
	borntime = 0;
	SetThink (&CSporeAmmo::IdleThink);

	m_flTimeSporeIdle = gpGlobals->time + 16;
}

void CSporeAmmo :: IdleThink ( void )
{

	pev->nextthink = gpGlobals->time + 0.1;
	if ( m_flTimeSporeIdle > gpGlobals->time )
		return;

	if (borntime)
	{
		pev->sequence = SPOREAMMO_IDLE;

		m_flTimeSporeIdle = gpGlobals->time + 10;
		SetThink(&CSporeAmmo::BornThink);
		return;
	}
	else
	{
		pev->sequence = SPOREAMMO_IDLE1;
	}
}

void CSporeAmmo :: AmmoTouch ( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() )
		return;

	if (borntime)
		return;

	int bResult = (pOther->GiveAmmo( AMMO_SPORE_GIVE, "spores" ) != -1);
	if (bResult)
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/spore_ammo.wav", 1, ATTN_NORM);

		pev->framerate		= 1.0;
		pev->animtime		= gpGlobals->time + 0.1;
		pev->sequence = SPOREAMMO_SNATCHDOWN;
		pev->body = 0;
		borntime = 1;
		m_flTimeSporeIdle = gpGlobals->time + 1;
		SetThink (&CSporeAmmo::IdleThink);
	}
}

#endif
