/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
// Alien slave monster
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"squadmonster.h"
#include	"schedule.h"
#include	"effects.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"mod_features.h"

// whether vortigaunts can spawn familiars (snarks and headcrabs)
#define FEATURE_ISLAVE_FAMILIAR 1
// whether vortigaunt can do coil attack
#define FEATURE_ISLAVE_COIL 1
// whether vortigaunts get health and energy when attack enemies
#define FEATURE_ISLAVE_ENERGY 1
// whether vortigaunts play some effects on idle
#define FEATURE_ISLAVE_IDLEARMBEAM 1
// whether vortigaunt can have glowing hands
#define FEATURE_ISLAVE_HANDGLOW 1
// whether vortigaunt marked as squadleader has a different beam color
#define FEATURE_ISLAVE_LEADER_COLOR 1

// free energy dependent abilities

// whether vortigaunts can heal allies
#define FEATURE_ISLAVE_HEAL (1 && FEATURE_ISLAVE_ENERGY)
// whether vortigaunts can revive other vortigaunts
#define FEATURE_ISLAVE_REVIVE (1 && FEATURE_ISLAVE_ENERGY)
// whether vortigaunts can have damage boost on melee attacks
#define FEATURE_ISLAVE_ARMBOOST (1 && FEATURE_ISLAVE_ENERGY)

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_ISLAVE_COVER_AND_SUMMON_FAMILIAR = LAST_COMMON_SCHEDULE + 1,
	SCHED_ISLAVE_SUMMON_FAMILIAR,
	SCHED_ISLAVE_HEAL_OR_REVIVE
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_ISLAVE_SUMMON_FAMILIAR = LAST_COMMON_TASK + 1,
	TASK_ISLAVE_HEAL_OR_REVIVE_ATTACK
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		ISLAVE_AE_CLAW			( 1 )
#define		ISLAVE_AE_CLAWRAKE		( 2 )
#define		ISLAVE_AE_ZAP_POWERUP		( 3 )
#define		ISLAVE_AE_ZAP_SHOOT		( 4 )
#define		ISLAVE_AE_ZAP_DONE		( 5 )

#define		ISLAVE_MAX_BEAMS		8

#define ISLAVE_HAND_SPRITE_NAME "sprites/glow02.spr"

#define ISLAVE_ZAP_RED 180
#define ISLAVE_ZAP_GREEN 255
#define ISLAVE_ZAP_BLUE 96

#define ISLAVE_LEADER_ZAP_RED 150
#define ISLAVE_LEADER_ZAP_GREEN 255
#define ISLAVE_LEADER_ZAP_BLUE 120

#define ISLAVE_ARMBEAM_RED 96
#define ISLAVE_ARMBEAM_GREEN 128
#define ISLAVE_ARMBEAM_BLUE 16

#define ISLAVE_LEADER_ARMBEAM_RED 72
#define ISLAVE_LEADER_ARMBEAM_GREEN 180
#define ISLAVE_LEADER_ARMBEAM_BLUE 72

#define ISLAVE_LIGHT_RED 255
#define ISLAVE_LIGHT_GREEN 180
#define ISLAVE_LIGHT_BLUE 96

#define ISLAVE_ELECTROONLY	(1 << 0)
#define ISLAVE_SNARKS		(1 << 1)
#define ISLAVE_HEADCRABS	(1 << 2)

#define ISLAVE_COIL_ATTACK_RADIUS 196

#define ISLAVE_SPAWNFAMILIAR_DELAY 6

enum {
	ISLAVE_LEFT_ARM = -1,
	ISLAVE_RIGHT_ARM = 1
};

static bool IsVortWounded(CBaseEntity* pEntity)
{
	return pEntity->pev->health <= pEntity->pev->max_health / 2;
}

static bool CanBeRevived(CBaseEntity* pEntity)
{
	if ( pEntity != NULL && pEntity->pev->deadflag == DEAD_DEAD && !FBitSet(pEntity->pev->flags, FL_KILLME) ) {
		Vector vecDest = pEntity->pev->origin + Vector( 0, 0, 38 );
		TraceResult trace;
		UTIL_TraceHull( vecDest, vecDest, dont_ignore_monsters, human_hull, pEntity->edict(), &trace );
	
		return !trace.fStartSolid;
	}
	return false;
}

class CISlave : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void UpdateOnRemove();
	void SetYawSpeed( void );
	int ISoundMask( void );
	int DefaultClassify( void );
	int IRelationship( CBaseEntity *pTarget );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL CheckRangeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack2( float flDot, float flDist );
	BOOL CheckHealOrReviveTargets( float flDist = 784, bool mustSee = false );
	bool IsValidHealTarget( CBaseEntity* pEntity );
	void CallForHelp( const char *szClassname, float flDist, EHANDLE hEnemy, Vector &vecLocation );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );

	void DeathSound( void );
	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );

	void Killed( entvars_t *pevAttacker, int iGib );

	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );
	void PrescheduleThink();
	virtual int SizeForGrapple() { return GRAPPLE_MEDIUM; }
	void SpawnFamiliar(const char *entityName, const Vector& origin, int hullType);
	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType( int Type );
	CUSTOM_SCHEDULES

	int Save( CSave &save ); 
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	void ClearBeams();
	void ArmBeam(int side );
	void ArmBeamMessage(int side );
	void WackBeam( int side, CBaseEntity *pEntity );
	void ZapBeam( int side );
	void BeamGlow( void );
	void HandsGlowOn(int brightness = 224);
	void HandGlowOn(CSprite* handGlow, int brightness = 224);
	void StartMeleeAttackGlow(int side);
	bool CanUseGlowArms();
	void HandGlowOff(CSprite* handGlow);
	void HandsGlowOff();
	void CreateSummonBeams();
	void RemoveSummonBeams();
	void CoilBeam();
	void MakeDynamicLight(const Vector& vecSrc, int radius, int t);

	Vector HandPosition(int side);

	CSprite* CreateHandGlow(int attachment);
	CBeam* CreateSummonBeam(const Vector& vecEnd, int attachment);

	float HealPower();
	void SpendEnergy(float energy);
	bool HasFreeEnergy();
	bool CanRevive();
	int HealOther(CBaseEntity* pEntity);
	bool CanSpawnFamiliar();

	Vector GetZapColor();
	Vector GetArmBeamColor(int& brightness);

	inline int AttachmentFromSide(int side) {
		return side < 0 ? 2 : 1;
	}

	int m_iBravery;

	CBeam *m_pBeam[ISLAVE_MAX_BEAMS];

	int m_iBeams;
	float m_flNextAttack;

	int m_voicePitch;

	EHANDLE m_hDead;
	EHANDLE m_hWounded;
	EHANDLE m_hWounded2;
	
	int m_iLightningTexture;
	int m_iTrailTexture;

	CSprite	*m_handGlow1;
	CSprite	*m_handGlow2;
	CBeam *m_handsBeam1;
	CBeam *m_handsBeam2;
	
	float m_flSpawnFamiliarTime;
	float m_freeEnergy;
	short m_clawStrikeNum;
	bool m_lastAttackWasCoil;

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];
	static const char *pGlowArmSounds[];
};

LINK_ENTITY_TO_CLASS( monster_alien_slave, CISlave )
LINK_ENTITY_TO_CLASS( monster_vortigaunt, CISlave )

TYPEDESCRIPTION	CISlave::m_SaveData[] =
{
	DEFINE_FIELD( CISlave, m_iBravery, FIELD_INTEGER ),

	DEFINE_ARRAY( CISlave, m_pBeam, FIELD_CLASSPTR, ISLAVE_MAX_BEAMS ),
	DEFINE_FIELD( CISlave, m_iBeams, FIELD_INTEGER ),
	DEFINE_FIELD( CISlave, m_flNextAttack, FIELD_TIME ),

	DEFINE_FIELD( CISlave, m_voicePitch, FIELD_INTEGER ),

	DEFINE_FIELD( CISlave, m_hDead, FIELD_EHANDLE ),
	DEFINE_FIELD( CISlave, m_hWounded, FIELD_EHANDLE ),
	DEFINE_FIELD( CISlave, m_hWounded2, FIELD_EHANDLE ),
	DEFINE_FIELD( CISlave, m_flSpawnFamiliarTime, FIELD_FLOAT ),
	DEFINE_FIELD( CISlave, m_freeEnergy, FIELD_FLOAT ),
	DEFINE_FIELD( CISlave, m_clawStrikeNum, FIELD_SHORT )
};

IMPLEMENT_SAVERESTORE( CISlave, CSquadMonster )

const char *CISlave::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CISlave::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CISlave::pPainSounds[] =
{
	"aslave/slv_pain1.wav",
	"aslave/slv_pain2.wav",
};

const char *CISlave::pDeathSounds[] =
{
	"aslave/slv_die1.wav",
	"aslave/slv_die2.wav",
};

const char *CISlave::pGlowArmSounds[] =
{
	"debris/zap3.wav",
	"debris/zap8.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CISlave::DefaultClassify( void )
{
	return CLASS_ALIEN_MILITARY;
}

int CISlave::IRelationship( CBaseEntity *pTarget )
{
	if( ( pTarget->IsPlayer() ) )
		if( ( pev->spawnflags & SF_MONSTER_WAIT_UNTIL_PROVOKED ) && ! ( m_afMemory & bits_MEMORY_PROVOKED ) )
			return R_NO;
	return CBaseMonster::IRelationship( pTarget );
}

void CISlave::CallForHelp( const char *szClassname, float flDist, EHANDLE hEnemy, Vector &vecLocation )
{
	// ALERT( at_aiconsole, "help " );

	// skip ones not on my netname
	if( FStringNull( pev->netname ) )
		return;

	CBaseEntity *pEntity = NULL;

	while( ( pEntity = UTIL_FindEntityByString( pEntity, "netname", STRING( pev->netname ) ) ) != NULL)
	{
		float d = ( pev->origin - pEntity->pev->origin ).Length();
		if( d < flDist )
		{
			CBaseMonster *pMonster = pEntity->MyMonsterPointer();
			if( pMonster )
			{
				pMonster->m_afMemory |= bits_MEMORY_PROVOKED;
				pMonster->PushEnemy( hEnemy, vecLocation );
			}
		}
	}
}

//=========================================================
// ALertSound - scream
//=========================================================
void CISlave::AlertSound( void )
{
	if( m_hEnemy != 0 )
	{
		SENTENCEG_PlayRndSz( ENT( pev ), "SLV_ALERT", 0.85, ATTN_NORM, 0, m_voicePitch );

		CallForHelp( "monster_alien_slave", 512, m_hEnemy, m_vecEnemyLKP );
	}
}

//=========================================================
// IdleSound
//=========================================================
void CISlave::IdleSound( void )
{
	if( RANDOM_LONG( 0, 2 ) == 0 )
	{
		SENTENCEG_PlayRndSz( ENT( pev ), "SLV_IDLE", 0.85, ATTN_NORM, 0, m_voicePitch );
	}
#if FEATURE_ISLAVE_IDLEARMBEAM
	int side = RANDOM_LONG( 0, 1 ) * 2 - 1;

	ArmBeamMessage( side );

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + gpGlobals->v_right * 2 * side;
	MakeDynamicLight(vecSrc, 8, 10);

	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "debris/zap1.wav", 1, ATTN_NORM, 0, 100 );
#endif
}

//=========================================================
// PainSound
//=========================================================
void CISlave::PainSound( void )
{
	if( RANDOM_LONG( 0, 2 ) == 0 )
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pPainSounds[RANDOM_LONG( 0, ARRAYSIZE( pPainSounds ) - 1 )], 1.0, ATTN_NORM, 0, m_voicePitch );
	}
}

//=========================================================
// DieSound
//=========================================================
void CISlave::DeathSound( void )
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pDeathSounds[RANDOM_LONG( 0, ARRAYSIZE( pDeathSounds ) - 1 )], 1.0, ATTN_NORM, 0, m_voicePitch );
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CISlave::ISoundMask( void )
{
	return bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER;
}

void CISlave::Killed( entvars_t *pevAttacker, int iGib )
{
	ClearBeams();
	UTIL_Remove(m_handGlow1);
	m_handGlow1 = NULL;
	UTIL_Remove(m_handGlow2);
	m_handGlow2 = NULL;
	CSquadMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CISlave::SetYawSpeed( void )
{
	int ys;

	switch( m_Activity )
	{
	case ACT_WALK:		
		ys = 50;	
		break;
	case ACT_RUN:		
		ys = 70;
		break;
	case ACT_IDLE:		
		ys = 50;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CISlave::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	// ALERT( at_console, "event %d : %f\n", pEvent->event, pev->frame );
	switch( pEvent->event )
	{
		case ISLAVE_AE_CLAW:
		{
			m_clawStrikeNum++;
			int damageType = DMG_SLASH;
			float damage = gSkillData.slaveDmgClaw;
			if (CanUseGlowArms()) {
				if ( m_clawStrikeNum == 1 ) {
					HandGlowOff(m_handGlow1);
					StartMeleeAttackGlow(ISLAVE_LEFT_ARM);
				}
				if ( m_clawStrikeNum == 2 ) {
					HandGlowOff(m_handGlow2);
					StartMeleeAttackGlow(ISLAVE_RIGHT_ARM);
				}
				if ( m_clawStrikeNum == 3 ) {
					HandGlowOff(m_handGlow1);
				}
				damageType |= DMG_SHOCK;
				damage *= 1.5;
			}
			// SOUND HERE!
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, damage, damageType );
			if( pHurt )
			{
				if( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) )
				{
					pHurt->pev->punchangle.z = 18;
					pHurt->pev->punchangle.x = 5;
				}
				// Play a random attack hit sound
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackHitSounds ) - 1 )], 1.0, ATTN_NORM, 0, m_voicePitch );
			}
			else
			{
				// Play a random attack miss sound
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackMissSounds ) - 1 )], 1.0, ATTN_NORM, 0, m_voicePitch );
			}
		}
			break;
		case ISLAVE_AE_CLAWRAKE:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.slaveDmgClawrake, DMG_SLASH );
			if( pHurt )
			{
				if( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
				}
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackHitSounds ) - 1 )], 1.0, ATTN_NORM, 0, m_voicePitch );
			}
			else
			{
				EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackMissSounds ) - 1 )], 1.0, ATTN_NORM, 0, m_voicePitch );
			}
		}
			break;
		case ISLAVE_AE_ZAP_POWERUP:
		{
			// speed up attack depending on difficulty level
			pev->framerate = gSkillData.slaveZapRate;

			UTIL_MakeAimVectors( pev->angles );

			if( m_iBeams == 0 )
			{
				Vector vecSrc = pev->origin + gpGlobals->v_forward * 2;
				MakeDynamicLight(vecSrc, 12, (int)(20/pev->framerate));
			}
			if( CanRevive() )
			{
				WackBeam( ISLAVE_LEFT_ARM, m_hDead );
				WackBeam( ISLAVE_RIGHT_ARM, m_hDead );
			}
			else
			{
				ArmBeam( ISLAVE_LEFT_ARM );
				ArmBeam( ISLAVE_RIGHT_ARM );
				BeamGlow();
			}

			EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "debris/zap4.wav", 1, ATTN_NORM, 0, 100 + m_iBeams * 10 );
			pev->skin = m_iBeams / 2;
		}
			break;
		case ISLAVE_AE_ZAP_SHOOT:
		{
			ClearBeams();

			if( CanRevive() )
			{
				if( CanBeRevived(m_hDead) )
				{
					m_lastAttackWasCoil = false;

					CBaseEntity *revivedVort = m_hDead;
					if (revivedVort) {
						revivedVort->pev->health = 0;
						revivedVort->Spawn();

						CBaseMonster* monster = revivedVort->MyMonsterPointer();
						if (monster) {
							CISlave* islave = (CISlave*)monster;
							// revived vort starts with zero energy
							islave->m_freeEnergy = 0;
						}

						WackBeam( ISLAVE_LEFT_ARM, revivedVort );
						WackBeam( ISLAVE_RIGHT_ARM, revivedVort );
						m_hDead = NULL;
						EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 130, 160 ) );

						SpendEnergy(pev->max_health);
					}

					/*
					CBaseEntity *pEffect = Create( "test_effect", pNew->Center(), pev->angles );
					pEffect->Use( this, this, USE_ON, 1 );
					*/
					break;
				}
				else {
					ALERT(at_aiconsole, "Trace failed on revive\n");
				}
			}
			ClearMultiDamage();

			bool coilAttack = false;
#if FEATURE_ISLAVE_COIL
			// make coil attack on purpose to heal only if two wounded friends around
			if ( HasFreeEnergy() && IsValidHealTarget(m_hWounded) && IsValidHealTarget(m_hWounded2) &&
					(pev->origin - m_hWounded->pev->origin).Length() <= ISLAVE_COIL_ATTACK_RADIUS &&
					(pev->origin - m_hWounded2->pev->origin).Length() <= ISLAVE_COIL_ATTACK_RADIUS) {
				if (m_hWounded.Get() == m_hWounded2.Get()) {
					ALERT(at_console, "m_hWounded && m_hWounded2 are the same!\n");
				}

				coilAttack = true;
				ALERT(at_aiconsole, "Vort makes coil attack to heal friends\n");
			} else if ( m_hEnemy != 0 && (pev->origin - m_hEnemy->pev->origin).Length() <= ISLAVE_COIL_ATTACK_RADIUS && !m_lastAttackWasCoil ) {
				coilAttack = true;
			}
#endif

			if (coilAttack) {
				CoilBeam();
				m_lastAttackWasCoil = true;

				float flAdjustedDamage = gSkillData.slaveDmgZap*3;
				CBaseEntity *pEntity = NULL;
				while( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, ISLAVE_COIL_ATTACK_RADIUS ) ) != NULL )
				{
					if( pEntity != this && pEntity->pev->takedamage != DAMAGE_NO && pEntity->MyMonsterPointer() != NULL ) {
						if (IRelationship(pEntity) >= R_DL) {
							if( !FVisible( pEntity ) ) {
								flAdjustedDamage *= 0.5;
							}

							if( flAdjustedDamage > 0 ) {
								pEntity->TakeDamage( pev, pev, flAdjustedDamage, DMG_SHOCK );
							}
						} else {
							if (FClassnameIs(pEntity->pev, STRING(pev->classname))) {
#if FEATURE_ISLAVE_HEAL
								if (HealOther(pEntity)) {
									ALERT(at_aiconsole, "Vort healed friend with coil attack\n");
								}
#endif
							}
						}
					}
				}
				UTIL_EmitAmbientSound( ENT( pev ), pev->origin, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );
			} else {
				m_lastAttackWasCoil = false;
				UTIL_MakeAimVectors( pev->angles );

				ZapBeam( ISLAVE_LEFT_ARM );
				ZapBeam( ISLAVE_RIGHT_ARM );
				
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 130, 160 ) );
				// STOP_SOUND( ENT( pev ), CHAN_WEAPON, "debris/zap4.wav" );
				ApplyMultiDamage( pev, pev );
			}

			m_flNextAttack = gpGlobals->time + RANDOM_FLOAT( 0.5, 4.0 );
		}
			break;
		case ISLAVE_AE_ZAP_DONE:
		{
			ClearBeams();
		}
			break;
		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// CheckRangeAttack1 - normal beam attack 
//=========================================================
BOOL CISlave::CheckRangeAttack1( float flDot, float flDist )
{
	if( m_flNextAttack > gpGlobals->time )
	{
		return FALSE;
	}

#if FEATURE_ISLAVE_COIL
	if( flDist > 64 && flDist <= ISLAVE_COIL_ATTACK_RADIUS )
	{
		return TRUE;
	}
#endif

	return CSquadMonster::CheckRangeAttack1( flDot, flDist );
}

//=========================================================
// CheckRangeAttack2 - try to resurect dead comrades or heal wounded ones
//=========================================================
BOOL CISlave::CheckRangeAttack2( float flDot, float flDist )
{
	if( m_flNextAttack > gpGlobals->time )
	{
		return FALSE;
	}

	return HasFreeEnergy() && CheckHealOrReviveTargets(flDist, true);
}

BOOL CISlave::CheckHealOrReviveTargets(float flDist, bool mustSee)
{
	m_hDead = NULL;
	m_hWounded = NULL;
	m_hWounded2 = NULL;
	
	CBaseEntity *pEntity = NULL;
	while( ( pEntity = UTIL_FindEntityByClassname( pEntity, "monster_alien_slave" ) ) != NULL )
	{
		if (IRelationship(pEntity) >= R_DL)
			continue;
		TraceResult tr;

		UTIL_TraceLine( EyePosition(), pEntity->EyePosition(), ignore_monsters, ENT( pev ), &tr );
		if( (mustSee && (tr.flFraction == 1.0 || tr.pHit == pEntity->edict())) || (!mustSee && (pEntity->pev->origin -pev->origin).Length() < flDist ) )
		{
#if FEATURE_ISLAVE_REVIVE
			if( CanBeRevived(pEntity) )
			{
				float d = ( pev->origin - pEntity->pev->origin ).Length();
				if( d < flDist )
				{
					m_hDead = pEntity;
					flDist = d;
				}
			}
#endif
#if FEATURE_ISLAVE_HEAL
			if ( IsValidHealTarget(pEntity) ) 
			{
				float d = ( pev->origin - pEntity->pev->origin ).Length();
				if( d < flDist )
				{
					m_hWounded2 = m_hWounded;
					m_hWounded = pEntity;
					flDist = d;
				}
			}
#endif
		}
	}
	if( m_hDead != 0 || m_hWounded != 0 )
		return TRUE;
	else
		return FALSE;
}

bool CISlave::IsValidHealTarget(CBaseEntity *pEntity)
{
	return pEntity != NULL && pEntity != this && pEntity->pev->deadflag != DEAD_DYING && pEntity->IsAlive() && IsVortWounded(pEntity);
}

//=========================================================
// StartTask
//=========================================================
void CISlave::StartTask( Task_t *pTask )
{
	ClearBeams();
	
	switch(pTask->iTask)
	{
	case TASK_ISLAVE_SUMMON_FAMILIAR:
	{
		ALERT(at_aiconsole, "start TASK_ISLAVE_FAMILIAR\n");
		m_IdealActivity = ACT_CROUCH;
		EMIT_SOUND( edict(), CHAN_BODY, "debris/beamstart1.wav", 1, ATTN_NORM );
		UTIL_MakeAimVectors( pev->angles );
		Vector vecSrc = pev->origin + gpGlobals->v_forward * 8;
		MakeDynamicLight(vecSrc, 10, 15);
		HandsGlowOn();
		CreateSummonBeams();
		break;
	}
		
	case TASK_ISLAVE_HEAL_OR_REVIVE_ATTACK:
	{
		ALERT(at_aiconsole, "start TASK_ISLAVE_HEAL_OR_REVIVE_ATTACK\n");
		m_IdealActivity = ACT_RANGE_ATTACK1;
		break;
	}
	default:
		CSquadMonster::StartTask( pTask );
		break;
	}
}

void CISlave::RunTask(Task_t *pTask)
{
	switch(pTask->iTask)
	{
	case TASK_ISLAVE_SUMMON_FAMILIAR:
		if( m_fSequenceFinished )
		{
			UTIL_MakeVectors( pev->angles );
			if (pev->weapons & ISLAVE_HEADCRABS) {
				Vector headcrabOrigin = pev->origin + gpGlobals->v_forward * 48 + Vector(0,0,20);
				SpawnFamiliar("monster_headcrab", headcrabOrigin, head_hull);
			} else if (pev->weapons & ISLAVE_SNARKS) {
				Vector snarkOrigin = pev->origin + gpGlobals->v_forward * 36 + Vector(0,0,20);
				SpawnFamiliar("monster_snark", snarkOrigin, head_hull);
			}
			HandsGlowOff();
			TaskComplete();
			RemoveSummonBeams();
		}
		break;
	case TASK_ISLAVE_HEAL_OR_REVIVE_ATTACK:
		if( m_fSequenceFinished )
		{
			m_Activity = ACT_RESET;
			TaskComplete();
		}
		break;
	default:
		CSquadMonster::RunTask( pTask );
		break;
	}
}

void CISlave::PrescheduleThink()
{
	CSquadMonster::PrescheduleThink();
	if (m_Activity == ACT_MELEE_ATTACK1 && m_clawStrikeNum == 0) {
		if ( m_handGlow1 && (m_handGlow1->pev->effects & EF_NODRAW) && CanUseGlowArms() ) {
			StartMeleeAttackGlow(ISLAVE_RIGHT_ARM);
			EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, RANDOM_SOUND_ARRAY(pGlowArmSounds), RANDOM_FLOAT(0.6, 0.8), ATTN_NORM, 0, RANDOM_LONG(70,90));
		}
	}
}

void CISlave::SpawnFamiliar(const char *entityName, const Vector &origin, int hullType)
{	
	TraceResult tr;
	UTIL_TraceHull( origin, origin, dont_ignore_monsters, hullType, edict(), &tr );
	if (!tr.fStartSolid) {
		CBaseEntity *pNew = Create( entityName, origin, pev->angles );
		CBaseMonster *pNewMonster = pNew->MyMonsterPointer( );

		if(pNew) {
			CSprite *pSpr = CSprite::SpriteCreate( "sprites/bexplo.spr", origin, TRUE );
			pSpr->AnimateAndDie( 20 );
			pSpr->SetTransparency( kRenderTransAdd,  ISLAVE_ARMBEAM_RED, ISLAVE_ARMBEAM_GREEN, ISLAVE_ARMBEAM_BLUE,  255, kRenderFxNoDissipation );
			EMIT_SOUND( pNew->edict(), CHAN_BODY, "debris/beamstart7.wav", 0.9, ATTN_NORM );
			
			SetBits( pNew->pev->spawnflags, SF_MONSTER_FALL_TO_GROUND );
			if (pNewMonster) {
				if (m_hEnemy) {
					pNewMonster->m_hEnemy = m_hEnemy;
					pNewMonster->m_vecEnemyLKP = m_vecEnemyLKP;
					pNewMonster->SetConditions( bits_COND_NEW_ENEMY );
					pNewMonster->m_MonsterState = MONSTERSTATE_COMBAT;
					pNewMonster->m_IdealMonsterState = MONSTERSTATE_COMBAT;
					if (m_iClass) {
						pNewMonster->m_iClass = Classify();
					}
				}
			}
		}
	} else {
		ALERT(at_aiconsole, "Not enough room to create %s\n", entityName);
	}
	m_flSpawnFamiliarTime = gpGlobals->time + ISLAVE_SPAWNFAMILIAR_DELAY;
}

CSprite* CISlave::CreateHandGlow(int attachment)
{
	CSprite* handSprite = CSprite::SpriteCreate( ISLAVE_HAND_SPRITE_NAME, pev->origin, FALSE );
	handSprite->SetAttachment( edict(), attachment );
	handSprite->SetScale(0.25);
	return handSprite;
}

//=========================================================
// Spawn
//=========================================================
void CISlave::Spawn()
{
	Precache();

	SetMyModel( "models/islave.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid		= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_GREEN );
	pev->effects		= 0;
	SetMyHealth( gSkillData.slaveHealth );
	pev->view_ofs		= Vector( 0, 0, 64 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_RANGE_ATTACK2 | bits_CAP_DOORS_GROUP | bits_CAP_SQUAD;

	m_voicePitch		= RANDOM_LONG( 85, 110 );

#if FEATURE_ISLAVE_HANDGLOW
	m_handGlow1 = CreateHandGlow(1);
	m_handGlow2 = CreateHandGlow(2);
#endif

	HandsGlowOff();

#if FEATURE_ISLAVE_FAMILIAR
	if (!pev->weapons) {
		pev->weapons = ISLAVE_SNARKS;
	}
#endif

#if FEATURE_ISLAVE_ENERGY
	// leader starts with some energy pool
	if (pev->spawnflags & SF_SQUADMONSTER_LEADER)
		m_freeEnergy = pev->max_health;
#endif

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CISlave::Precache()
{
	m_iLightningTexture = PRECACHE_MODEL( "sprites/lgtning.spr" );
	m_iTrailTexture = PRECACHE_MODEL( "sprites/plasma.spr" );
	PRECACHE_MODEL( ISLAVE_HAND_SPRITE_NAME );
	PRECACHE_MODEL( "sprites/bexplo.spr" );
	PrecacheMyModel( "models/islave.mdl" );
	PRECACHE_SOUND( "debris/zap1.wav" );
	PRECACHE_SOUND( "debris/zap4.wav" );
	PRECACHE_SOUND( "debris/beamstart1.wav" );
	PRECACHE_SOUND( "debris/beamstart7.wav" );
	PRECACHE_SOUND( "weapons/electro4.wav" );
	PRECACHE_SOUND( "hassault/hw_shoot1.wav" );
	PRECACHE_SOUND( "zombie/zo_pain2.wav" );
	PRECACHE_SOUND( "headcrab/hc_headbite.wav" );
	PRECACHE_SOUND( "weapons/cbar_miss1.wav" );

	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);

	PRECACHE_SOUND_ARRAY(pGlowArmSounds);
	UTIL_PrecacheOther( "test_effect" );
	UTIL_PrecacheOther( "monster_snark" );
	UTIL_PrecacheOther( "monster_headcrab" );
}

void CISlave::UpdateOnRemove()
{
	CBaseEntity::UpdateOnRemove();

	ClearBeams();
}

//=========================================================
// TakeDamage - get provoked when injured
//=========================================================

int CISlave::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// don't slash one of your own
	if( ( bitsDamageType & DMG_SLASH ) && pevAttacker && IRelationship( Instance( pevAttacker ) ) < R_DL )
		return 0;

	m_afMemory |= bits_MEMORY_PROVOKED;
	return CSquadMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CISlave::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if( (bitsDamageType & DMG_SHOCK) && (!pevAttacker || IRelationship(Instance(pevAttacker)) < R_DL) )
		return;

	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t	tlSlaveAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slSlaveAttack1[] =
{
	{ 
		tlSlaveAttack1,
		ARRAYSIZE ( tlSlaveAttack1 ), 
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAR_SOUND |
		bits_COND_HEAVY_DAMAGE, 

		bits_SOUND_DANGER,
		"Slave Range Attack1"
	},
};

Task_t tlSlaveHealOrReviveAttack[] =
{
	{ TASK_STOP_MOVING,				0	},
	{ TASK_MOVE_TO_TARGET_RANGE,	128 },
	{ TASK_FACE_TARGET,				0	},
	{ TASK_ISLAVE_HEAL_OR_REVIVE_ATTACK,	0	}
};

Schedule_t	slSlaveHealOrReviveAttack[] =
{
	{ 
		tlSlaveHealOrReviveAttack,
		ARRAYSIZE ( tlSlaveHealOrReviveAttack ), 
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAR_SOUND |
		bits_COND_HEAVY_DAMAGE, 

		bits_SOUND_DANGER,
		"Slave Heal or Revive Range Attack"
	},
};

Task_t tlSlaveCoverAndSummon[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_WAIT, (float)0.1 },
	{ TASK_FIND_COVER_FROM_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_ISLAVE_SUMMON_FAMILIAR, (float)0 },
};

Schedule_t slSlaveCoverAndSummon[] =
{
	{
		tlSlaveCoverAndSummon,
		ARRAYSIZE( tlSlaveCoverAndSummon ),
		bits_COND_NEW_ENEMY,
		0,
		"Slave Cover and Summon"
	},
};

Task_t tlSlaveSummon[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_ISLAVE_SUMMON_FAMILIAR, (float)0 }
};

Schedule_t slSlaveSummon[] =
{
	{
		tlSlaveSummon,
		ARRAYSIZE( tlSlaveSummon ),
		bits_COND_NEW_ENEMY,
		0,
		"Slave Summon"
	}
};

DEFINE_CUSTOM_SCHEDULES( CISlave )
{
	slSlaveAttack1,
	slSlaveHealOrReviveAttack,
	slSlaveCoverAndSummon,
	slSlaveSummon
};

IMPLEMENT_CUSTOM_SCHEDULES( CISlave, CSquadMonster )

//=========================================================
//=========================================================
Schedule_t *CISlave::GetSchedule( void )
{
	ClearBeams();
	m_clawStrikeNum = 0;
/*
	if( pev->spawnflags )
	{
		pev->spawnflags = 0;
		return GetScheduleOfType( SCHED_RELOAD );
	}
*/
	if( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );

		if( pSound && ( pSound->m_iType & bits_SOUND_DANGER ) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
		if( pSound->m_iType & bits_SOUND_COMBAT )
			m_afMemory |= bits_MEMORY_PROVOKED;
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		// dead enemy
		if( HasConditions( bits_COND_ENEMY_DEAD ) )
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}

		if( IsVortWounded(this) )
		{
			if( !HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				m_failSchedule = SCHED_CHASE_ENEMY;
				int sched = SCHED_TAKE_COVER_FROM_ENEMY;
				if (CanSpawnFamiliar()) {
					sched = SCHED_ISLAVE_COVER_AND_SUMMON_FAMILIAR;
					m_failSchedule = SCHED_ISLAVE_SUMMON_FAMILIAR;
				}

				if( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
				{
					return GetScheduleOfType( sched );
				}
				if( HasConditions( bits_COND_SEE_ENEMY ) && HasConditions( bits_COND_ENEMY_FACING_ME )
						&& RANDOM_LONG(0,1) ) // give chance to use electro attack to restore health
				{
					return GetScheduleOfType( sched );
				}
			}
		}
		break;
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if ( HasFreeEnergy() && CheckHealOrReviveTargets()) {
			if (m_hDead) {
				m_hTargetEnt = m_hDead;
			} else if (m_hWounded) {
				m_hTargetEnt = m_hWounded;
			}
			ALERT(at_aiconsole, "Vort gonna heal or revive friend when idle. State is %s\n", m_MonsterState == MONSTERSTATE_ALERT ? "alert" : "idle");
			return GetScheduleOfType( SCHED_ISLAVE_HEAL_OR_REVIVE );
		}
		break;
	default:
		break;
	}
	return CSquadMonster::GetSchedule();
}

Schedule_t *CISlave::GetScheduleOfType( int Type ) 
{
	switch( Type )
	{
	case SCHED_FAIL:
		if( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
		{
			return CSquadMonster::GetScheduleOfType( SCHED_MELEE_ATTACK1 );
		}
		else if ( m_MonsterState == MONSTERSTATE_COMBAT && CanSpawnFamiliar() )
		{
			return GetScheduleOfType( SCHED_ISLAVE_SUMMON_FAMILIAR );
		}
	case SCHED_CHASE_ENEMY_FAILED:
		if ( HasFreeEnergy() && CheckHealOrReviveTargets() )
		{
			if (m_hDead) {
				m_hTargetEnt = m_hDead;
			} else if (m_hWounded) {
				m_hTargetEnt = m_hWounded;
			}
			ALERT(at_aiconsole, "Vort gonna heal or revive friends after chase enemy sched fail\n");
			return GetScheduleOfType( SCHED_ISLAVE_HEAL_OR_REVIVE );
		}
		break;
	case SCHED_RANGE_ATTACK1:
		return slSlaveAttack1;
	case SCHED_RANGE_ATTACK2:
		return slSlaveAttack1;
	case SCHED_ISLAVE_COVER_AND_SUMMON_FAMILIAR:
		return slSlaveCoverAndSummon;
	case SCHED_ISLAVE_SUMMON_FAMILIAR:
		if (m_failSchedule == SCHED_ISLAVE_SUMMON_FAMILIAR) {
			ALERT(at_aiconsole, "Vort gonna spawn familiar because it was set to failschedule\n");
		}
		return slSlaveSummon;
	case SCHED_ISLAVE_HEAL_OR_REVIVE:
		return slSlaveHealOrReviveAttack;
	}
	
	return CSquadMonster::GetScheduleOfType( Type );
}

//=========================================================
// ArmBeam - small beam from arm to nearby geometry
//=========================================================
void CISlave::ArmBeam( int side )
{
	TraceResult tr;
	float flDist = 1.0;

	if( m_iBeams >= ISLAVE_MAX_BEAMS )
		return;

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = HandPosition(side);

	for( int i = 0; i < 3; i++ )
	{
		Vector vecAim = gpGlobals->v_right * side * RANDOM_FLOAT( 0, 1 ) + gpGlobals->v_up * RANDOM_FLOAT( -1, 1 );
		TraceResult tr1;
		UTIL_TraceLine( vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT( pev ), &tr1 );
		if( flDist > tr1.flFraction )
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if( flDist == 1.0 )
		return;

	DecalGunshot( &tr, BULLET_PLAYER_CROWBAR );

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	if( !m_pBeam[m_iBeams] )
		return;

	int brightness;
	const Vector armBeamColor = GetArmBeamColor(brightness);
	m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex() );
	m_pBeam[m_iBeams]->SetEndAttachment( AttachmentFromSide(side) );
	// m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
	m_pBeam[m_iBeams]->SetColor( armBeamColor.x, armBeamColor.y, armBeamColor.z );
	m_pBeam[m_iBeams]->SetBrightness( brightness );
	m_pBeam[m_iBeams]->SetNoise( 80 );
	m_iBeams++;
}

void CISlave::ArmBeamMessage( int side )
{
	TraceResult tr;
	float flDist = 1.0;

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = HandPosition(side);

	for( int i = 0; i < 3; i++ )
	{
		Vector vecAim = gpGlobals->v_right * side * RANDOM_FLOAT( 0, 1 ) + gpGlobals->v_up * RANDOM_FLOAT( -1, 1 );
		TraceResult tr1;
		UTIL_TraceLine( vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT( pev ), &tr1 );
		if( flDist > tr1.flFraction )
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if( flDist == 1.0 )
		return;

	int brightness;
	const Vector armBeamColor = GetArmBeamColor(brightness);
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMENTPOINT );
		WRITE_SHORT( entindex() + 0x1000 * (AttachmentFromSide(side)) );
		WRITE_COORD( tr.vecEndPos.x );
		WRITE_COORD( tr.vecEndPos.y );
		WRITE_COORD( tr.vecEndPos.z );
		WRITE_SHORT( m_iLightningTexture );
		WRITE_BYTE( 0 ); // framestart
		WRITE_BYTE( 10 ); // framerate
		WRITE_BYTE( (int)(10*RANDOM_FLOAT( 0.8, 1.5 )) ); // life
		WRITE_BYTE( 30 );  // width
		WRITE_BYTE( 80 );   // noise
		WRITE_BYTE( armBeamColor.x );   // r, g, b
		WRITE_BYTE( armBeamColor.y );   // r, g, b
		WRITE_BYTE( armBeamColor.z );   // r, g, b
		WRITE_BYTE( brightness );	// brightness
		WRITE_BYTE( 10 );		// speed
	MESSAGE_END();
}

//=========================================================
// BeamGlow - brighten all beams
//=========================================================
void CISlave::BeamGlow()
{
	int b = m_iBeams * 32;
	if( b > 255 )
		b = 255;
	
	HandsGlowOn(b);

	for( int i = 0; i < m_iBeams; i++ )
	{
		if( m_pBeam[i]->GetBrightness() != 255 )
		{
			m_pBeam[i]->SetBrightness( b );
		}
	}
}

//=========================================================
// WackBeam - regenerate dead colleagues
//=========================================================
void CISlave::WackBeam( int side, CBaseEntity *pEntity )
{
	//Vector vecDest;
	//float flDist = 1.0;

	if( m_iBeams >= ISLAVE_MAX_BEAMS )
		return;

	if( pEntity == NULL )
		return;

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	if( !m_pBeam[m_iBeams] )
		return;

	Vector zapColor = GetZapColor();
	m_pBeam[m_iBeams]->PointEntInit( pEntity->Center(), entindex() );
	m_pBeam[m_iBeams]->SetEndAttachment( AttachmentFromSide(side) );
	m_pBeam[m_iBeams]->SetColor( zapColor.x, zapColor.y, zapColor.z );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 80 );
	m_iBeams++;
}

//=========================================================
// ZapBeam - heavy damage directly forward
//=========================================================
void CISlave::ZapBeam( int side )
{
	Vector vecSrc, vecAim;
	TraceResult tr;
	CBaseEntity *pEntity;

	if( m_iBeams >= ISLAVE_MAX_BEAMS )
		return;

	vecSrc = pev->origin + gpGlobals->v_up * 36;
	if (IsValidHealTarget(m_hWounded)) {
		vecAim = ( ( m_hWounded->BodyTarget( vecSrc ) ) - vecSrc ).Normalize();
		ALERT(at_aiconsole, "Vort shoot friend on purpose to heal\n");
	} else {
		vecAim = ShootAtEnemy( vecSrc );
	}

	float deflection = 0.01;
	vecAim = vecAim + side * gpGlobals->v_right * RANDOM_FLOAT( 0, deflection ) + gpGlobals->v_up * RANDOM_FLOAT( -deflection, deflection );
	UTIL_TraceLine( vecSrc, vecSrc + vecAim * 1024, dont_ignore_monsters, ENT( pev ), &tr );

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 50 );
	if( !m_pBeam[m_iBeams] )
		return;

	const Vector zapColor = GetZapColor();
	m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex() );
	m_pBeam[m_iBeams]->SetEndAttachment( AttachmentFromSide(side) );
	m_pBeam[m_iBeams]->SetColor( zapColor.x, zapColor.y, zapColor.z );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 20 );
	m_iBeams++;

	pEntity = CBaseEntity::Instance( tr.pHit );
	if( pEntity != NULL && pEntity->pev->takedamage )
	{
		if (IRelationship(pEntity) < R_DL && FClassnameIs(pEntity->pev, STRING(pev->classname))) {
#if FEATURE_ISLAVE_HEAL
			if (HealOther(pEntity)) {
				ALERT(at_aiconsole, "Vortigaunt healed friend with zap attack\n");
			}
#endif
		} else {
			pEntity->TraceAttack( pev, gSkillData.slaveDmgZap, vecAim, &tr, DMG_SHOCK );
#if FEATURE_ISLAVE_ENERGY
			if (pEntity->pev->flags & (FL_CLIENT | FL_MONSTER)) {
				//TODO: check that target is actually a living creature, not machine
				const float toHeal = gSkillData.slaveDmgZap;
				const int healed = TakeHealth(toHeal, DMG_GENERIC);
				if (healed) // give some health to vortigaunt like in Decay bonus mission
				{
					ALERT(at_aiconsole, "Vortigaunt gets health from enemy\n");
				}
				if (toHeal > healed)
				{
					m_freeEnergy += toHeal - healed;
					ALERT(at_aiconsole, "Vortigaunt gets energy from enemy. Energy level: %d\n", (int)m_freeEnergy);
				}
			}
#endif
		}
	}
	UTIL_EmitAmbientSound( ENT( pev ), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );
}

//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CISlave::ClearBeams()
{
	for( int i = 0; i < ISLAVE_MAX_BEAMS; i++ )
	{
		if( m_pBeam[i] )
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
	m_iBeams = 0;
	pev->skin = 0;
	
	HandsGlowOff();
	RemoveSummonBeams();

	STOP_SOUND( ENT( pev ), CHAN_WEAPON, "debris/zap4.wav" );
}

void CISlave::CoilBeam()
{
	Vector zapColor = GetZapColor();
	
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z + 16 );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z + 16 + ISLAVE_COIL_ATTACK_RADIUS*5 ); 
		WRITE_SHORT( m_iLightningTexture );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 10 ); // framerate
		WRITE_BYTE( 2 ); // life
		WRITE_BYTE( 128 );  // width
		WRITE_BYTE( 20 );   // noise

		WRITE_BYTE( zapColor.x );
		WRITE_BYTE( zapColor.y );
		WRITE_BYTE( zapColor.z );

		WRITE_BYTE( 255 ); //brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();
	
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z + 48 );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z + 48 + ISLAVE_COIL_ATTACK_RADIUS*2 ); 
		WRITE_SHORT( m_iLightningTexture );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 10 ); // framerate
		WRITE_BYTE( 2 ); // life
		WRITE_BYTE( 128 );  // width
		WRITE_BYTE( 25 );   // noise

		WRITE_BYTE( zapColor.x );
		WRITE_BYTE( zapColor.y );
		WRITE_BYTE( zapColor.z );

		WRITE_BYTE( 255 ); //brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();
}

void CISlave::MakeDynamicLight(const Vector &vecSrc, int radius, int t)
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
		WRITE_BYTE( TE_DLIGHT );
		WRITE_COORD( vecSrc.x );	// X
		WRITE_COORD( vecSrc.y );	// Y
		WRITE_COORD( vecSrc.z );	// Z
		WRITE_BYTE( radius );		// radius * 0.1
		WRITE_BYTE( ISLAVE_LIGHT_RED );		// r
		WRITE_BYTE( ISLAVE_LIGHT_GREEN);		// g
		WRITE_BYTE( ISLAVE_LIGHT_BLUE );		// b
		WRITE_BYTE( t );		// time * 10
		WRITE_BYTE( 0 );		// decay * 0.1
	MESSAGE_END();
}

void CISlave::HandGlowOff(CSprite *handGlow)
{
	if (handGlow) {
		handGlow->pev->effects |= EF_NODRAW;
	}
}

void CISlave::HandsGlowOff()
{
	HandGlowOff(m_handGlow1);
	HandGlowOff(m_handGlow2);
}

void CISlave::HandsGlowOn(int brightness)
{
	HandGlowOn(m_handGlow1, brightness);
	HandGlowOn(m_handGlow2, brightness);
}

void CISlave::HandGlowOn(CSprite *handGlow, int brightness)
{
	if (handGlow) {
		Vector zapColor = GetZapColor();
		handGlow->SetTransparency( kRenderTransAdd, zapColor.x, zapColor.y, zapColor.z, brightness, kRenderFxNoDissipation );
		UTIL_SetOrigin(handGlow->pev, pev->origin);
		handGlow->SetScale(brightness / (float)255 * 0.3);
		handGlow->pev->effects &= ~EF_NODRAW;
	}
}

void CISlave::StartMeleeAttackGlow(int side)
{
	CSprite* handGlow = side == ISLAVE_LEFT_ARM ? m_handGlow2 : m_handGlow1;
	HandGlowOn(handGlow);
	int brightness;
	const Vector armBeamColor = GetArmBeamColor(brightness);
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() + 0x1000 * (AttachmentFromSide(side)) );
		WRITE_SHORT( m_iTrailTexture );
		WRITE_BYTE( 5 ); // life
		WRITE_BYTE( 3 );  // width
		WRITE_BYTE( armBeamColor.x );   // r, g, b
		WRITE_BYTE( armBeamColor.y );   // r, g, b
		WRITE_BYTE( armBeamColor.z );   // r, g, b
		WRITE_BYTE( 128 );	// brightness
	MESSAGE_END();
}

bool CISlave::CanUseGlowArms()
{
#if FEATURE_ISLAVE_ARMBOOST
	return (pev->spawnflags & SF_SQUADMONSTER_LEADER) || HasFreeEnergy();
#else
	return false;
#endif
}

Vector CISlave::HandPosition(int side)
{
	UTIL_MakeAimVectors( pev->angles );
	return pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * side * 16 + gpGlobals->v_forward * 32;
}

void CISlave::CreateSummonBeams()
{
	UTIL_MakeVectors(pev->angles);
	Vector vecEnd = pev->origin + gpGlobals->v_forward * 36;
	if (m_handGlow1) {
		m_handsBeam1 = CreateSummonBeam(vecEnd, 1);
	}
	if (m_handGlow2) {
		m_handsBeam2 = CreateSummonBeam(vecEnd, 2);
	}
}

CBeam* CISlave::CreateSummonBeam(const Vector& vecEnd, int attachment)
{
	CBeam* beam = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	if( !beam )
		return beam;

	Vector zapColor = GetZapColor();
	beam->PointEntInit(vecEnd, entindex());
	beam->SetEndAttachment(attachment);
	beam->SetColor( zapColor.x, zapColor.y, zapColor.z );
	beam->SetBrightness( 192 );
	beam->SetNoise( 80 );
	return beam;
}

void CISlave::RemoveSummonBeams()
{
	UTIL_Remove(m_handsBeam1);
	m_handsBeam1 = NULL;
	UTIL_Remove(m_handsBeam2);
	m_handsBeam2 = NULL;
}

float CISlave::HealPower()
{
	return Q_min(gSkillData.slaveDmgZap, m_freeEnergy);
}

void CISlave::SpendEnergy(float energy)
{
	// It's ok to be negative. Vort must restore power to positive values to proceed with healing or reviving.

	if (pev->spawnflags & SF_SQUADMONSTER_LEADER) // leader spends less energy
		m_freeEnergy -= energy/2;
	else
		m_freeEnergy -= energy;
}

bool CISlave::HasFreeEnergy()
{
	return m_freeEnergy > 0;
}

bool CISlave::CanRevive()
{
#if FEATURE_ISLAVE_REVIVE
	return m_hDead != 0 && HasFreeEnergy();
#else
	return false;
#endif
}

int CISlave::HealOther(CBaseEntity *pEntity)
{
	int result = 0;
	if (pEntity->IsAlive()) {
		result = pEntity->TakeHealth(HealPower(), DMG_GENERIC);
		SpendEnergy(result);
	}
	return result;
}

bool CISlave::CanSpawnFamiliar()
{
#if FEATURE_ISLAVE_FAMILIAR
	if ((pev->weapons & (ISLAVE_SNARKS | ISLAVE_HEADCRABS)) != 0) {
		if (m_flSpawnFamiliarTime < gpGlobals->time) {
			return true;
		}
	}
#endif
	return false;
}

Vector CISlave::GetZapColor()
{
#if FEATURE_ISLAVE_LEADER_COLOR
	if (pev->spawnflags & SF_SQUADMONSTER_LEADER) {
		return Vector(ISLAVE_LEADER_ZAP_RED, ISLAVE_LEADER_ZAP_GREEN, ISLAVE_LEADER_ZAP_BLUE);
	}
#endif
	return Vector(ISLAVE_ZAP_RED, ISLAVE_ZAP_GREEN, ISLAVE_ZAP_BLUE);
}

Vector CISlave::GetArmBeamColor(int &brightness)
{
#if FEATURE_ISLAVE_LEADER_COLOR
	if (pev->spawnflags & SF_SQUADMONSTER_LEADER) {
		brightness = 128;
		return Vector(ISLAVE_LEADER_ARMBEAM_RED, ISLAVE_LEADER_ARMBEAM_GREEN, ISLAVE_LEADER_ARMBEAM_BLUE);
	}
#endif
	brightness = 64;
	return Vector(ISLAVE_ARMBEAM_RED, ISLAVE_ARMBEAM_GREEN, ISLAVE_ARMBEAM_BLUE);
}

#if FEATURE_ISLAVE_DEAD
class CDeadISlave : public CDeadMonster
{
public:
	void Spawn( void );
	int	DefaultClassify ( void ) { return	CLASS_ALIEN_MILITARY; }

	const char* getPos(int pos) const;
	static const char *m_szPoses[5];
};

const char *CDeadISlave::m_szPoses[] = { "dead_on_stomach", "dieheadshot", "diesimple", "diebackward", "dieforward" };

const char* CDeadISlave::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_alien_slave_dead, CDeadISlave )

void CDeadISlave::Spawn( )
{
	SpawnHelper("models/islave.mdl", BLOOD_COLOR_YELLOW);
	MonsterInitDead();
	if (m_iPose)
		pev->frame = 255;
}
#endif
