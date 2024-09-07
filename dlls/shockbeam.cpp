#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"effects.h"
#include	"customentity.h"
#include	"decals.h"
#include	"gamerules.h"
#include	"skill.h"
#include	"shockbeam.h"
#include	"game.h"

#if FEATURE_SHOCKBEAM

#define SHOCK_BEAM_LENGTH		64
#define SHOCK_BEAM_LENGTH_HALF	SHOCK_BEAM_LENGTH * 0.5f

#define SHOCK_BEAM_WIDTH		50

LINK_ENTITY_TO_CLASS(shock_beam, CShock)

TYPEDESCRIPTION	CShock::m_SaveData[] =
{
	DEFINE_FIELD(CShock, m_pBeam, FIELD_CLASSPTR),
	DEFINE_FIELD(CShock, m_pNoise, FIELD_CLASSPTR),
	DEFINE_FIELD(CShock, m_pSprite, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE(CShock, CBaseAnimating)

const NamedSoundScript CShock::impactSoundScript = {
	CHAN_WEAPON,
	{"weapons/shock_impact.wav"},
	FloatRange(0.8f, 0.9f),
	ATTN_NORM,
	"ShockBeam.Impact"
};

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
	pev->nextthink = gpGlobals->time + 0.01;
}

void CShock::Precache()
{
	PRECACHE_MODEL("sprites/flare3.spr");
	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_MODEL("models/shock_effect.mdl");
	RegisterAndPrecacheSoundScript(impactSoundScript);
}

void CShock::FlyThink()
{
	if (pev->waterlevel == 3)
	{
		entvars_t *pevOwner = VARS(pev->owner);
		EmitSoundScript(impactSoundScript);
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

void CShock::Shoot(entvars_t *pevOwner, const Vector angles, const Vector vecStart, const Vector vecVelocity, string_t soundList)
{
	CShock *pShock = GetClassPtr((CShock *)NULL);
	UTIL_SetOrigin(pShock->pev, vecStart);
	pShock->m_soundList = soundList;
	pShock->Spawn();

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

	CBaseMonster* pMonster = pOther->MyMonsterPointer();
	if (pMonster && pMonster->IsAlive())
	{
		pMonster->GlowShellOn( Vector( 0, 220, 255 ), .5f );
	}

	ClearEffects();
	if (!pOther->pev->takedamage)
	{
		// make a splat on the wall
		const int baseDecal = g_modFeatures.opfor_decals ? DECAL_OPFOR_SCORCH1 : DECAL_SMALLSCORCH1;
		UTIL_DecalTrace(&tr, baseDecal + RANDOM_LONG(0, 2));

		int iContents = UTIL_PointContents(pev->origin);

		// Create sparks
		if (iContents != CONTENTS_WATER)
		{
			UTIL_Sparks(tr.vecEndPos);
		}
	}
	else
	{
		int damageType = DMG_SHOCK;
		if (pMonster && !pMonster->IsAlive())
		{
			damageType |= DMG_CLUB;
		}
		entvars_t *pevOwner = VARS(pev->owner);
		entvars_t *pevAttacker = pevOwner ? pevOwner : pev;
		pOther->ApplyTraceAttack(pev, pevAttacker, pev->dmg, pev->velocity.Normalize(), &tr, damageType );
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

	// splat sound
	EmitSoundScript(impactSoundScript);

	pev->modelindex = 0;
	pev->solid = SOLID_NOT;
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.01; // let the sound play
}

void CShock::CreateEffects()
{
	m_pSprite = CSprite::SpriteCreate( "sprites/flare3.spr", pev->origin, FALSE );
	m_pSprite->SetAttachment( edict(), 0 );
	m_pSprite->pev->scale = 0.35;
	m_pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 170, kRenderFxDistort );
	//m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;
	//m_pSprite->pev->flags |= FL_SKIPLOCALHOST;

	m_pBeam = CBeam::BeamCreate( "sprites/lgtning.spr", 60 );

	if (m_pBeam)
	{
		UTIL_SetOrigin(m_pBeam->pev, pev->origin);

		m_pBeam->EntsInit( entindex(), entindex() );
		m_pBeam->SetStartAttachment( 1 );
		m_pBeam->SetEndAttachment( 2 );
		m_pBeam->SetBrightness( 180 );
		m_pBeam->SetScrollRate( 10 );
		m_pBeam->SetNoise( 0 );
		m_pBeam->SetFlags( BEAM_FSHADEOUT );
		m_pBeam->SetColor( 0, 253, 253 );
		//m_pBeam->RelinkBeam();
	}
	else
	{
		ALERT(at_console, "Could no create shockbeam beam!\n");
	}

	m_pNoise = CBeam::BeamCreate( "sprites/lgtning.spr", 20 );

	if (m_pNoise)
	{
		UTIL_SetOrigin(m_pNoise->pev, pev->origin);

		m_pNoise->EntsInit( entindex(), entindex() );
		m_pNoise->SetStartAttachment( 1 );
		m_pNoise->SetEndAttachment( 2 );
		m_pNoise->SetBrightness( 180 );
		m_pNoise->SetScrollRate( 30 );
		m_pNoise->SetNoise( 30 );
		m_pNoise->SetFlags( BEAM_FSHADEOUT );
		m_pNoise->SetColor( 255, 255, 157 );
		//m_pNoise->RelinkBeam();
	}
	else
	{
		ALERT(at_console, "Could no create shockbeam noise!\n");
	}
}

void CShock::ClearEffects()
{
	if (m_pBeam)
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}

	if (m_pNoise)
	{
		UTIL_Remove( m_pNoise );
		m_pNoise = NULL;
	}

	if (m_pSprite)
	{
		UTIL_Remove( m_pSprite );
		m_pSprite = NULL;
	}
}

void CShock::UpdateOnRemove()
{
	ClearEffects();
	CBaseAnimating::UpdateOnRemove();
}

#endif
