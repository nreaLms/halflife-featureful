/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

//=========================================================
// Gonome.cpp
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"player.h"
#include	"bullsquid.h"
#include	"decals.h"
#include	"animation.h"
#include	"studio.h"

#define		GONOME_SPRINT_DIST	256 // how close the squid has to get before starting to sprint and refusing to swerve

#define		GONOME_TOLERANCE_MELEE1_RANGE	85
#define		GONOME_TOLERANCE_MELEE2_RANGE	85

#define		GONOME_TOLERANCE_MELEE1_DOT		0.7
#define		GONOME_TOLERANCE_MELEE2_DOT		0.7

#define		GONOME_MELEE_ATTACK_RADIUS		70

enum 
{
	TASK_GONOME_GET_PATH_TO_ENEMY_CORPSE = LAST_COMMON_TASK + 1
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

#define GONOME_AE_SLASH_RIGHT	( 1 )
#define GONOME_AE_SLASH_LEFT	( 2 )
#define GONOME_AE_THROW			( 4 )

#define GONOME_AE_BITE1			( 19 )
#define GONOME_AE_BITE2			( 20 )
#define GONOME_AE_BITE3			( 21 )
#define GONOME_AE_BITE4			( 22 )

//=========================================================
// Gonome's guts projectile
//=========================================================
class CGonomeGuts : public CSquidSpit
{
public:
	void Spawn(void);
	static void Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity);
	void Touch(CBaseEntity *pOther);
};

void CGonomeGuts::Spawn()
{
	SpawnHelper("sprites/blood_chnk.spr", "gonomeguts");
}

void CGonomeGuts::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CGonomeGuts *pSpit = GetClassPtr( (CGonomeGuts *)NULL );
	pSpit->Spawn();

	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT( pevOwner );

	pSpit->SetThink( &CSquidSpit::Animate );
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void CGonomeGuts::Touch( CBaseEntity *pOther )
{
	TraceResult tr;
	int iPitch;

	// splat sound
	iPitch = RANDOM_FLOAT( 90, 110 );

	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "bullchicken/bc_acid1.wav", 1, ATTN_NORM, 0, iPitch );

	switch( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "bullchicken/bc_spithit1.wav", 1, ATTN_NORM, 0, iPitch );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "bullchicken/bc_spithit2.wav", 1, ATTN_NORM, 0, iPitch );
		break;
	}

	if( !pOther->pev->takedamage )
	{
		// make a splat on the wall
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_DecalTrace( &tr, DECAL_BLOOD1 + RANDOM_LONG( 0, 5 ) );
	}
	else
	{
		pOther->TakeDamage( pev, pev, gSkillData.gonomeDmgGuts, DMG_GENERIC );
	}

	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

//=========================================================
// CGonome
//=========================================================
class CGonome : public CBullsquid
{
public:

	void Spawn(void);
	void Precache(void);

	int  DefaultClassify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	void IdleSound(void);
	void PainSound(void);
	void DeathSound(void);
	void AlertSound(void);
	void AttackSound(void);
	void StartTask(Task_t *pTask);

	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckMeleeAttack2(float flDot, float flDist);
	BOOL CheckRangeAttack1(float flDot, float flDist);
	void RunAI(void);
	Schedule_t *GetSchedule();
	Schedule_t *GetScheduleOfType( int Type );

	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	int IRelationship(CBaseEntity *pTarget);
	int IgnoreConditions(void);
	
	void SetActivity( Activity NewActivity );
	
protected:
	int GonomeLookupActivity( void *pmodel, int activity );
	bool gonnaAttack1;
};

LINK_ENTITY_TO_CLASS(monster_gonome, CGonome)

/*
 * Hack to ignore activity weights when choosing melee attack animation
 */
int CGonome::GonomeLookupActivity( void *pmodel, int activity )
{
	studiohdr_t *pstudiohdr;

	pstudiohdr = (studiohdr_t *)pmodel;
	if( !pstudiohdr )
		return 0;

	mstudioseqdesc_t *pseqdesc;

	pseqdesc = (mstudioseqdesc_t *)( (byte *)pstudiohdr + pstudiohdr->seqindex );

	int sameActivityNum = 0;
	for( int i = 0; i < pstudiohdr->numseq && sameActivityNum < 2; i++ )
	{
		if( pseqdesc[i].activity == activity )
		{
			sameActivityNum++;
			if (sameActivityNum == 1 && gonnaAttack1) {
				return i;
			} else if (sameActivityNum == 2) {
				return i;
			}
		}
	}

	return ACTIVITY_NOT_AVAILABLE;
}

void CGonome::SetActivity( Activity NewActivity )
{
	if (NewActivity != ACT_MELEE_ATTACK1) {
		CBaseMonster::SetActivity(NewActivity);
	} else {
		ASSERT( NewActivity != 0 );
		void *pmodel = GET_MODEL_PTR( ENT( pev ) );
		int iSequence = GonomeLookupActivity( pmodel, NewActivity );
	
		// Set to the desired anim, or default anim if the desired is not present
		if( iSequence > ACTIVITY_NOT_AVAILABLE )
		{
			if( pev->sequence != iSequence || !m_fSequenceLoops )
			{
				// don't reset frame between walk and run
				if( !( m_Activity == ACT_WALK || m_Activity == ACT_RUN ) || !( NewActivity == ACT_WALK || NewActivity == ACT_RUN ) )
					pev->frame = 0;
			}
	
			pev->sequence = iSequence;	// Set to the reset anim (if it's there)
			ResetSequenceInfo();
			SetYawSpeed();
		}
		else
		{
			// Not available try to get default anim
			ALERT( at_aiconsole, "%s has no sequence for act:%d\n", STRING( pev->classname ), NewActivity );
			pev->sequence = 0;	// Set to the reset anim (if it's there)
		}
	
		m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present
	
		// In case someone calls this with something other than the ideal activity
		m_IdealActivity = m_Activity;
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CGonome::DefaultClassify(void)
{
	return	CLASS_ALIEN_MONSTER;
}


//=========================================================
// IgnoreConditions 
//=========================================================
int CGonome::IgnoreConditions(void)
{
	return CBaseMonster::IgnoreConditions();
}

//=========================================================
// IRelationship - overridden for gonome
//=========================================================
int CGonome::IRelationship(CBaseEntity *pTarget)
{
	return CBaseMonster::IRelationship(pTarget);
}

//=========================================================
// TakeDamage - overridden for gonome so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CGonome::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	float flDist;
	Vector vecApex;

	// if the squid is running, has an enemy, was hurt by the enemy, hasn't been hurt in the last 3 seconds, and isn't too close to the enemy,
	// it will swerve. (whew).
	if (m_hEnemy != NULL && IsMoving() && pevAttacker == m_hEnemy->pev && gpGlobals->time - m_flLastHurtTime > 3)
	{
		flDist = (pev->origin - m_hEnemy->pev->origin).Length2D();

		if (flDist > GONOME_SPRINT_DIST)
		{
			flDist = (pev->origin - m_Route[m_iRouteIndex].vecLocation).Length2D();// reusing flDist. 

			if (FTriangulate(pev->origin, m_Route[m_iRouteIndex].vecLocation, flDist * 0.5, m_hEnemy, &vecApex))
			{
				InsertWaypoint(vecApex, bits_MF_TO_DETOUR | bits_MF_DONT_SIMPLIFY);
			}
		}
	}

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}


//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CGonome::CheckRangeAttack1(float flDot, float flDist)
{
	if (IsMoving() && flDist >= 512)
	{
		// squid will far too far behind if he stops running to spit at this distance from the enemy.
		return FALSE;
	}

	if (flDist > 64 && flDist <= 784 && flDot >= 0.5 && gpGlobals->time >= m_flNextSpitTime)
	{
		if (m_hEnemy != NULL)
		{
			if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 256)
			{
				// don't try to spit at someone up really high or down really low.
				return FALSE;
			}
		}

		if (IsMoving())
		{
			// don't spit again for a long time, resume chasing enemy.
			m_flNextSpitTime = gpGlobals->time + 5;
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextSpitTime = gpGlobals->time + 3;
		}

		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckMeleeAttack1 - gonome is a big guy, so has a longer
// melee range than most monsters.
//=========================================================
BOOL CGonome::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDist <= GONOME_TOLERANCE_MELEE1_RANGE && 
		flDot  >= GONOME_TOLERANCE_MELEE1_DOT)
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckMeleeAttack2 - both gonome's melee attacks are ACT_MELEE_ATTACK1
//=========================================================
BOOL CGonome::CheckMeleeAttack2(float flDot, float flDist)
{
	return FALSE;
}


//=========================================================
// IdleSound 
//=========================================================
#define GONOME_ATTN_IDLE	(float)1.5
void CGonome::IdleSound(void)
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "gonome/gonome_idle1.wav", 1, GONOME_ATTN_IDLE);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "gonome/gonome_idle2.wav", 1, GONOME_ATTN_IDLE);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "gonome/gonome_idle3.wav", 1, GONOME_ATTN_IDLE);
		break;
	}
}

//=========================================================
// PainSound 
//=========================================================
void CGonome::PainSound(void)
{
	int iPitch = RANDOM_LONG(85, 120);

	switch (RANDOM_LONG(0, 3))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "gonome/gonome_pain1.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "gonome/gonome_pain2.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "gonome/gonome_pain3.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 3:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "gonome/gonome_pain4.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	}
}

//=========================================================
// AlertSound
//=========================================================
void CGonome::AlertSound(void)
{
	int iPitch = RANDOM_LONG(140, 160);

	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "gonome/gonome_idle1.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "gonome/gonome_idle2.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "gonome/gonome_idle3.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	}
}



//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGonome::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{

	case GONOME_AE_THROW:
	{
		Vector	vecSpitOffset;
		Vector	vecSpitDir;

		UTIL_MakeVectors(pev->angles);

		// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
		// we should be able to read the position of bones at runtime for this info.


		Vector vecArmPos, vecArmAng;
		GetAttachment(0, vecArmPos, vecArmAng);

		//vecSpitOffset = (gpGlobals->v_right * 8 + gpGlobals->v_forward * 37 + gpGlobals->v_up * 23);
		//vecSpitOffset = (pev->origin + vecSpitOffset);
		vecSpitOffset = vecArmPos;
		vecSpitDir = ((m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs) - vecSpitOffset).Normalize();

		vecSpitDir.x += RANDOM_FLOAT(-0.05, 0.05);
		vecSpitDir.y += RANDOM_FLOAT(-0.05, 0.05);
		vecSpitDir.z += RANDOM_FLOAT(-0.05, 0);

#if 0
		// do stuff for this event.
		AttackSound();
#endif

		CGonomeGuts::Shoot(pev, vecSpitOffset, vecSpitDir * 1200); // Default: 900
	}
	break;

	case GONOME_AE_SLASH_LEFT:
	{
		CBaseEntity *pHurt = CheckTraceHullAttack(GONOME_MELEE_ATTACK_RADIUS, gSkillData.gonomeDmgOneSlash, DMG_SLASH);
		if (pHurt)
		{
			pHurt->pev->punchangle.z = 20;
			pHurt->pev->punchangle.x = 20;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 200;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
		}
	}
	break;

	case GONOME_AE_SLASH_RIGHT:
	{
		CBaseEntity *pHurt = CheckTraceHullAttack(GONOME_MELEE_ATTACK_RADIUS, gSkillData.gonomeDmgOneSlash, DMG_SLASH);
		if (pHurt)
		{
			pHurt->pev->punchangle.z = -20;
			pHurt->pev->punchangle.x = 20;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * -200;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
		}
	}
	break;

	case GONOME_AE_BITE1:
	case GONOME_AE_BITE2:
	case GONOME_AE_BITE3:
	case GONOME_AE_BITE4:
		{
			int iPitch;

			// SOUND HERE!
			CBaseEntity *pHurt = CheckTraceHullAttack(GONOME_TOLERANCE_MELEE2_RANGE, gSkillData.gonomeDmgOneBite, DMG_SLASH);

			if (pHurt)
			{
				// croonchy bite sound
				iPitch = RANDOM_FLOAT(90, 110);
				switch (RANDOM_LONG(0, 1))
				{
				case 0:
					EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "bullchicken/bc_bite2.wav", 1, ATTN_NORM, 0, iPitch);
					break;
				case 1:
					EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "bullchicken/bc_bite3.wav", 1, ATTN_NORM, 0, iPitch);
					break;
				}

				pHurt->pev->punchangle.z = RANDOM_LONG(-10, 10);
				pHurt->pev->punchangle.x = RANDOM_LONG(-35, -45);
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 50;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 50;
			}
		}
		break;



	default:
		CBaseMonster::HandleAnimEvent(pEvent);
	}
}

//=========================================================
// Spawn
//=========================================================
void CGonome::Spawn()
{
	Precache();

	SetMyModel("models/gonome.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_GREEN );
	pev->effects = 0;
	SetMyHealth( gSkillData.gonomeHealth );
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	m_fCanThreatDisplay = TRUE;
	m_flNextSpitTime = gpGlobals->time;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGonome::Precache()
{
	PrecacheMyModel("models/gonome.mdl");

	PRECACHE_MODEL("sprites/blood_chnk.spr");// spit projectile.

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	PRECACHE_SOUND("gonome/gonome_death2.wav");
	PRECACHE_SOUND("gonome/gonome_death3.wav");
	PRECACHE_SOUND("gonome/gonome_death4.wav");

	PRECACHE_SOUND("gonome/gonome_eat.wav");
	PRECACHE_SOUND("gonome/gonome_idle1.wav");
	PRECACHE_SOUND("gonome/gonome_idle2.wav");
	PRECACHE_SOUND("gonome/gonome_idle3.wav");

	PRECACHE_SOUND("gonome/gonome_jumpattack.wav");

	PRECACHE_SOUND("gonome/gonome_melee1.wav");
	PRECACHE_SOUND("gonome/gonome_melee2.wav");

	PRECACHE_SOUND("gonome/gonome_pain1.wav");
	PRECACHE_SOUND("gonome/gonome_pain2.wav");
	PRECACHE_SOUND("gonome/gonome_pain3.wav");
	PRECACHE_SOUND("gonome/gonome_pain4.wav");

	PRECACHE_SOUND("gonome/gonome_run.wav");

	PRECACHE_SOUND("bullchicken/bc_acid1.wav");

	PRECACHE_SOUND("bullchicken/bc_bite2.wav");
	PRECACHE_SOUND("bullchicken/bc_bite3.wav");

	PRECACHE_SOUND("bullchicken/bc_spithit1.wav");
	PRECACHE_SOUND("bullchicken/bc_spithit2.wav");
}

//=========================================================
// DeathSound
//=========================================================
void CGonome::DeathSound(void)
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "gonome/gonome_death2.wav", 1, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "gonome/gonome_death3.wav", 1, ATTN_NORM);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "gonome/gonome_death4.wav", 1, ATTN_NORM);
		break;
	}
}

//=========================================================
// AttackSound
//=========================================================
void CGonome::AttackSound(void)
{
}

//========================================================
// RunAI - overridden for gonome because there are things
// that need to be checked every think.
//========================================================
void CGonome::RunAI(void)
{
	// first, do base class stuff
	CBaseMonster::RunAI();

	if (m_hEnemy != NULL && m_Activity == ACT_RUN)
	{
		// chasing enemy. Sprint for last bit
		if ((pev->origin - m_hEnemy->pev->origin).Length2D() < GONOME_SPRINT_DIST)
		{
			pev->framerate = 1.25;
		}
	}
}

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CGonome::GetSchedule( void )
{
	switch( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
		{
			break;
		}
	case MONSTERSTATE_COMBAT:
		{
			// dead enemy
			if( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster::GetSchedule();
			}

			if( HasConditions( bits_COND_NEW_ENEMY ) )
			{
				return GetScheduleOfType( SCHED_WAKE_ANGRY );
			}

			if( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			if( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
			}

			if( HasConditions( bits_COND_CAN_MELEE_ATTACK2 ) )
			{
				return GetScheduleOfType( SCHED_MELEE_ATTACK2 );
			}

			return GetScheduleOfType( SCHED_CHASE_ENEMY );
			break;
		}
	default:
			break;
	}

	return CBaseMonster::GetSchedule();
}

Task_t tlGonomeVictoryDance[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_WAIT, (float)0.1 },
	{ TASK_GONOME_GET_PATH_TO_ENEMY_CORPSE,	(float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_WAIT, (float)0 }
};

Schedule_t slGonomeVictoryDance[] =
{
	{
		tlGonomeVictoryDance,
		ARRAYSIZE( tlGonomeVictoryDance ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		0,
		"GonomeVictoryDance"
	},
};

Schedule_t* CGonome::GetScheduleOfType(int Type)
{
	switch ( Type ) {
	case SCHED_VICTORY_DANCE:
		return &slGonomeVictoryDance[0];
		break;
	default:
		break;
	}
	return CBullsquid::GetScheduleOfType(Type);
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for bullsquid because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CGonome::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_MELEE_ATTACK1:
		{
			gonnaAttack1 = RANDOM_LONG(0,1) ? true : false;
			if (gonnaAttack1)
				EMIT_SOUND(ENT(pev), CHAN_VOICE, "gonome/gonome_melee1.wav", 1, ATTN_NORM);
			else
				EMIT_SOUND(ENT(pev), CHAN_VOICE, "gonome/gonome_melee2.wav", 1, ATTN_NORM);
			CBaseMonster::StartTask(pTask);
		}
		break;

	case TASK_MELEE_ATTACK2:
		{
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "gonome/gonome_melee2.wav", 1, ATTN_NORM);
			CBaseMonster::StartTask(pTask);
		}
		break;

	case TASK_GONOME_GET_PATH_TO_ENEMY_CORPSE:
		{
			UTIL_MakeVectors( pev->angles );
			if( BuildRoute( m_vecEnemyLKP - gpGlobals->v_forward * 50, bits_MF_TO_LOCATION, NULL ) )
			{
				TaskComplete();
			}
			else
			{
				ALERT( at_aiconsole, "GonomeGetPathToEnemyCorpse failed!!\n" );
				TaskFail();
			}
		}
		break;
	default:
		CBullsquid::StartTask(pTask);
		break;

	}
}

//=========================================================
// DEAD GONOME PROP
//=========================================================
class CDeadGonome : public CDeadMonster
{
public:
	void Spawn(void);
	int	DefaultClassify(void) { return	CLASS_ALIEN_MONSTER; }
	const char* getPos(int pos) const;
	static const char *m_szPoses[3];
};

const char *CDeadGonome::m_szPoses[] = { "dead_on_stomach1", "dead_on_back", "dead_on_side" };

const char* CDeadGonome::getPos(int pos) const
{
	return m_szPoses[pos % (sizeof(m_szPoses)/sizeof(const char*))];
}

LINK_ENTITY_TO_CLASS(monster_gonome_dead, CDeadGonome)

//=========================================================
// ********** DeadGonome SPAWN **********
//=========================================================
void CDeadGonome::Spawn(void)
{
	SpawnHelper("models/gonome.mdl", "Dead gonome with bad pose\n", BLOOD_COLOR_YELLOW);
	MonsterInitDead();
}
