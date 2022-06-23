#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"
#include "shake.h"
#include "displacerball.h"
#include "scripted.h"

extern edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer );

LINK_ENTITY_TO_CLASS(displacer_ball, CDisplacerBall)

TYPEDESCRIPTION	CDisplacerBall::m_SaveData[] =
{
	DEFINE_FIELD(CDisplacerBall, m_iBeams, FIELD_INTEGER),
	DEFINE_ARRAY(CDisplacerBall, m_pBeam, FIELD_CLASSPTR, 8),
	DEFINE_FIELD(CDisplacerBall, m_hDisplacedTarget, FIELD_EHANDLE),
};

IMPLEMENT_SAVERESTORE(CDisplacerBall, CBaseEntity);

void CDisplacerBall::Spawn(void)
{
	Precache();
	pev->classname = MAKE_STRING("displacer_ball");

	pev->movetype = MOVETYPE_FLY;

	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "sprites/exit1.spr");
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	pev->frame = 0;
	pev->scale = 0.75f;

	SetTouch(&CDisplacerBall::BallTouch);
	SetThink(&CDisplacerBall::FlyThink);
	pev->nextthink = gpGlobals->time + 0.2f;

	m_iBeams = 0;
}

void CDisplacerBall::Precache()
{
	PRECACHE_MODEL("sprites/exit1.spr");
	PRECACHE_MODEL("sprites/lgtning.spr");
	m_iTrail = PRECACHE_MODEL("sprites/disp_ring.spr");

	PRECACHE_SOUND("weapons/displacer_impact.wav");
	PRECACHE_SOUND("weapons/displacer_teleport.wav");
}

void CDisplacerBall::FlyThink()
{
	ArmBeam( -1 );
	ArmBeam( 1 );
	pev->nextthink = gpGlobals->time + 0.05;
}

void CDisplacerBall::ArmBeam( int iSide )
{
	//This method is identical to the Alien Slave's ArmBeam, except it treats m_pBeam as a circular buffer.
	if( m_iBeams > 7 )
		m_iBeams = 0;

	TraceResult tr;
	float flDist = 1.0;

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = gpGlobals->v_forward * 32.0 + iSide * gpGlobals->v_right * 16.0 + gpGlobals->v_up * 36.0 + pev->origin;
	Vector vecAim = gpGlobals->v_up * RANDOM_FLOAT( -1.0, 1.0 );
	Vector vecEnd = (iSide * gpGlobals->v_right * RANDOM_FLOAT( 0.0, 1.0 ) + vecAim) * 512.0 + vecSrc;
	UTIL_TraceLine( &vecSrc.x, &vecEnd.x, dont_ignore_monsters, ENT( pev ), &tr );

	if( flDist > tr.flFraction )
		flDist = tr.flFraction;

	// Couldn't find anything close enough
	if( flDist == 1.0 )
		return;

	// The beam might already exist if we've created all beams before.
	if( !m_pBeam[ m_iBeams ] )
		m_pBeam[ m_iBeams ] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );

	if( !m_pBeam[ m_iBeams ] )
		return;

	CBaseEntity* pHit = Instance( tr.pHit );

	entvars_t *pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	if( pHit && pHit->pev->takedamage != DAMAGE_NO )
	{
		//Beam hit something, deal radius damage to it
		m_pBeam[ m_iBeams ]->EntsInit( pHit->entindex(), entindex() );
		m_pBeam[ m_iBeams ]->SetColor( 255, 255, 255 );
		m_pBeam[ m_iBeams ]->SetBrightness( 255 );
		m_pBeam[ m_iBeams ]->SetNoise( 10 );

		RadiusDamage( tr.vecEndPos, pev, pevOwner, 25, 15, CLASS_NONE, DMG_ENERGYBEAM );
	}
	else
	{
		m_pBeam[ m_iBeams ]->PointEntInit( tr.vecEndPos, entindex() );
		m_pBeam[ m_iBeams ]->SetColor( 96, 128, 16 );
		m_pBeam[ m_iBeams ]->SetBrightness( 255 );
		m_pBeam[ m_iBeams ]->SetNoise( 80 );
	}
	m_iBeams++;
}

void CDisplacerBall::Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, Vector vecAngles )
{
	CDisplacerBall *pSpit = GetClassPtr((CDisplacerBall *)NULL);
	pSpit->Spawn();
	UTIL_SetOrigin(pSpit->pev, vecStart);
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->angles = vecAngles;
	pSpit->pev->owner = ENT(pevOwner);
}

void CDisplacerBall::SelfCreate(entvars_t *pevOwner,Vector vecStart)
{
	CDisplacerBall *pSelf = GetClassPtr((CDisplacerBall *)NULL);
	UTIL_SetOrigin(pSelf->pev, vecStart);
	pSelf->Spawn();
	pSelf->ClearBeams();

	pSelf->pev->owner = ENT(pevOwner);
	pSelf->Circle();
	pSelf->SetTouch( NULL );
	pSelf->SetThink(&CDisplacerBall::KillThink);
	pSelf->pev->nextthink = gpGlobals->time + ( g_pGameRules->IsMultiplayer() ? 0.2f : 0.5f );
}

void CDisplacerBall::BallTouch(CBaseEntity *pOther)
{
	pev->velocity = g_vecZero;

	TraceResult tr;
	Vector vecSpot;
	Vector vecSrc;
	CBaseEntity *pTarget = NULL;

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/displacer_impact.wav", 0.9f, ATTN_NORM);

	if( ( g_pGameRules->IsMultiplayer() && !g_pGameRules->IsCoOp() ) && pOther->IsPlayer() )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)pOther;
		pTarget = GetClassPtr( (CBaseEntity *)VARS( EntSelectSpawnPoint( pPlayer ) ) );


		if( pTarget )
		{
			// UTIL_ScreenFade( pPlayer, Vector( 0, 200, 0 ), 0.5, 0.5, 255, FFADE_IN );
			Vector tmp = pTarget->pev->origin;
			UTIL_CleanSpawnPoint( tmp, 100 );

			EMIT_SOUND( pPlayer->edict(), CHAN_BODY, "weapons/displacer_self.wav", 1, ATTN_NORM );

			// make origin adjustments (origin in center, not at feet)
			tmp.z -= pPlayer->pev->mins.z + 36;
			tmp.z++;

			pPlayer->pev->flags &= ~FL_ONGROUND;

			UTIL_SetOrigin( pPlayer->pev, tmp );

			pPlayer->pev->angles = pTarget->pev->angles;

			pPlayer->pev->v_angle = pTarget->pev->angles;

			pPlayer->pev->fixangle = TRUE;

			pPlayer->pev->velocity = pOther->pev->basevelocity = g_vecZero;
		}
	}

	pev->movetype = MOVETYPE_NONE;

	Circle();

	m_hDisplacedTarget = 0;

	if (pOther && pOther->IsDisplaceable())
	{
		m_hDisplacedTarget = pOther;

		CBaseMonster* pMonster = pOther->MyMonsterPointer();
		if (!pMonster || !pMonster->m_pCine || pMonster->m_pCine->CanInterrupt())
		{
			CBaseEntity* pAttacker = CBaseEntity::Instance( pev->owner );

			if (pMonster && pMonster->m_pCine)
				pMonster->m_pCine->CancelScript();

			pOther->pev->health = 0;
			pOther->Killed(pev, pAttacker ? pAttacker->pev : pev, GIB_NEVER);
		}
	}

	pev->basevelocity = g_vecZero;
	pev->velocity = g_vecZero;

	SetTouch(NULL);
	pev->solid = SOLID_NOT;

	UTIL_SetOrigin(pev, pev->origin);

	SetThink(&CDisplacerBall::KillThink);
	pev->nextthink = gpGlobals->time + ( g_pGameRules->IsMultiplayer() ? 0.2f : 0.5f );
}

void CDisplacerBall::Circle( void )
{
	// portal circle
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_BEAMCYLINDER);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z + 800.0f); // reach damage radius over .2 seconds
		WRITE_SHORT(m_iTrail);
		WRITE_BYTE(0); // startframe
		WRITE_BYTE(0); // framerate
		WRITE_BYTE(3); // life
		WRITE_BYTE(16);  // width
		WRITE_BYTE(0);   // noise
		WRITE_BYTE(255);   // r, g, b
		WRITE_BYTE(255);   // r, g, b
		WRITE_BYTE(255);   // r, g, b
		WRITE_BYTE(255); // brightness
		WRITE_BYTE(0);		// speed
	MESSAGE_END();
	UTIL_MuzzleLight( pev->origin, 160.0f, 255, 180, 96, 1.0f, 100.0f );
}

void CDisplacerBall::KillThink( void )
{
	CBaseEntity* pTarget = m_hDisplacedTarget;
	if (pTarget)
	{
		CBaseMonster* pMonster = pTarget->MyMonsterPointer();
		if (!pMonster || !pMonster->m_pCine || pMonster->m_pCine->CanInterrupt()) {
			pTarget->SetThink(&CBaseEntity::SUB_Remove);
		}
	}

	SetThink( &CDisplacerBall::ExplodeThink );
	pev->nextthink = gpGlobals->time + 0.2f;
}

void CDisplacerBall::ExplodeThink( void )
{
	ClearBeams();

	pev->effects |= EF_NODRAW;

	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/displacer_teleport.wav", VOL_NORM, ATTN_NORM);

	CBaseEntity* pAttacker = CBaseEntity::Instance( pev->owner );
	pev->owner = NULL;

	::RadiusDamage( pev->origin, pev, pAttacker ? pAttacker->pev : pev, gSkillData.plrDmgDisplacer, gSkillData.plrDisplacerRadius, CLASS_NONE, DMG_ALWAYSGIB | DMG_BLAST );

	UTIL_Remove( this );
}

void CDisplacerBall::ClearBeams( void )
{
	for( int i = 0;i < 8; i++ )
	{
		if( m_pBeam[i] )
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
}
