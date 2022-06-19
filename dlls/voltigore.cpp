/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
// voltigore
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"nodes.h"
#include	"effects.h"
#include	"decals.h"
#include	"soundent.h"
#include	"game.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"mod_features.h"

#define FEATURE_VOLGITOGRE_LESSER_SIZE 0

#if FEATURE_VOLTIFORE
#define		VOLTIGORE_SPRINT_DIST	256 // how close the voltigore has to get before starting to sprint and refusing to swerve

#define		VOLTIGORE_MAX_BEAMS		8

#define VOLTIGORE_ZAP_RED 180
#define VOLTIGORE_ZAP_GREEN 16
#define VOLTIGORE_ZAP_BLUE 255
#define VOLTIGORE_ZAP_BEAM "sprites/lgtning.spr"
#define VOLTIGORE_ZAP_NOISE 80
#define VOLTIGORE_ZAP_WIDTH 30
#define VOLTIGORE_ZAP_BRIGHTNESS 255
#define VOLTIGORE_ZAP_DISTANCE 512
#define VOLTIGORE_GLOW_SCALE 0.75f
#define VOLTIGORE_GIB_COUNT 10
#define VOLTIGORE_GLOW_SPRITE "sprites/blueflare2.spr"

//=========================================================
// monster-specific schedule types
//=========================================================

//=========================================================
// monster-specific tasks
//=========================================================

//=========================================================
// Voltigore's energy ball projectile
//=========================================================
class CVoltigoreEnergyBall : public CBaseEntity
{
public:
	void Spawn(void);

	static void Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity);
	void EXPORT BallTouch(CBaseEntity *pOther);
	void EXPORT FlyThink(void);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	void CreateBeams();
	void ClearBeams();
	void UpdateBeams();

	CBeam* m_pBeam[VOLTIGORE_MAX_BEAMS];
	int m_iBeams;
	float m_timeToDie;

protected:

	void CreateBeam(int nIndex, const Vector& vecPos, int width, int brightness);
	void UpdateBeam(int nIndex, const Vector& vecPos, bool show);
	void ClearBeam(int nIndex);
};


LINK_ENTITY_TO_CLASS(voltigore_energy_ball, CVoltigoreEnergyBall)

TYPEDESCRIPTION	CVoltigoreEnergyBall::m_SaveData[] =
{
	DEFINE_ARRAY(CVoltigoreEnergyBall, m_pBeam, FIELD_CLASSPTR, VOLTIGORE_MAX_BEAMS),
	DEFINE_FIELD(CVoltigoreEnergyBall, m_iBeams, FIELD_INTEGER),
	DEFINE_FIELD(CVoltigoreEnergyBall, m_timeToDie, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CVoltigoreEnergyBall, CBaseEntity)

//=========================================================
// Purpose:
//=========================================================
void CVoltigoreEnergyBall::Spawn(void)
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING("voltigore_energy_ball");

	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), VOLTIGORE_GLOW_SPRITE);
	pev->frame = 0;
	pev->scale = VOLTIGORE_GLOW_SCALE;

	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	m_iBeams = 0;
}

//=========================================================
// Purpose:
//=========================================================
void CVoltigoreEnergyBall::Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity)
{
	CVoltigoreEnergyBall *pEnergyBall = GetClassPtr((CVoltigoreEnergyBall *)NULL);
	pEnergyBall->Spawn();

	UTIL_SetOrigin(pEnergyBall->pev, vecStart);
	pEnergyBall->pev->velocity = vecVelocity;
	pEnergyBall->pev->owner = ENT(pevOwner);
	pEnergyBall->pev->angles = pevOwner->angles;

	pEnergyBall->SetTouch(&CVoltigoreEnergyBall::BallTouch);
	pEnergyBall->SetThink(&CVoltigoreEnergyBall::FlyThink);
	pEnergyBall->pev->nextthink = gpGlobals->time + 0.1;
}

//=========================================================
// Purpose:
//=========================================================
void CVoltigoreEnergyBall::BallTouch(CBaseEntity *pOther)
{
	TraceResult tr;
	if (!pOther->pev->takedamage)
	{
		// make a splat on the wall
		UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT(pev), &tr);
		UTIL_DecalTrace(&tr, DECAL_SCORCH1 + RANDOM_LONG(0, 1));
	}
	else
	{
		tr = UTIL_GetGlobalTrace();
		ClearMultiDamage();
		entvars_t* attacker = VARS( pev->owner );
		if (!attacker)
			attacker = pev;
		pOther->TraceAttack(attacker, gSkillData.voltigoreDmgBeam, pev->velocity, &tr, DMG_SHOCK|DMG_ALWAYSGIB);
		ApplyMultiDamage(pev, attacker);
	}
	pev->velocity = Vector(0,0,0);

	m_timeToDie = gpGlobals->time + 0.3;
	SetTouch(NULL);
}

//=========================================================
// Purpose:
//=========================================================
void CVoltigoreEnergyBall::FlyThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1;
	if (m_timeToDie)
	{
		CBaseEntity* pEntity = NULL;
		while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, 32)) != NULL) {
			if ( pEntity->pev->takedamage && pEntity->MyMonsterPointer()) {
				bool shouldDamage = true;
				if (pev->owner) {
					CBaseMonster* owner = GetMonsterPointer(pev->owner);
					if (owner) {
						const int relationship = owner->IRelationship(pEntity);
						if (relationship == R_AL) {
							shouldDamage = false;
						}
					}
				}
				if (shouldDamage)
					pEntity->TakeDamage(pev, pev, gSkillData.voltigoreDmgBeam/5, DMG_SHOCK);
			}
		}
		
		if (m_timeToDie <= gpGlobals->time) {
			ClearBeams();
			SetThink(&CVoltigoreEnergyBall::SUB_Remove);
			pev->nextthink = gpGlobals->time;
		}
	}
	else
	{
		if (m_iBeams) {
			UpdateBeams();
		} else {
			CreateBeams();
		}
	}
}

//=========================================================
// Purpose:
//=========================================================
void CVoltigoreEnergyBall::CreateBeam(int nIndex, const Vector& vecPos, int width, int brightness)
{
	m_pBeam[nIndex] = CBeam::BeamCreate(VOLTIGORE_ZAP_BEAM, width);
	if (!m_pBeam[nIndex])
		return;

	m_pBeam[nIndex]->PointEntInit(vecPos, entindex());
	m_pBeam[nIndex]->SetColor(VOLTIGORE_ZAP_RED, VOLTIGORE_ZAP_GREEN, VOLTIGORE_ZAP_BLUE);
	m_pBeam[nIndex]->SetBrightness(brightness);
	m_pBeam[nIndex]->SetNoise(VOLTIGORE_ZAP_NOISE);
}

//=========================================================
// Purpose:
//=========================================================
void CVoltigoreEnergyBall::UpdateBeam(int nIndex, const Vector& vecPos, bool show)
{
	if (!m_pBeam[nIndex])
		return;
	m_pBeam[nIndex]->SetBrightness(show ? VOLTIGORE_ZAP_BRIGHTNESS : 0);
	m_pBeam[nIndex]->SetStartPos(vecPos);
	m_pBeam[nIndex]->SetEndEntity(entindex());
	m_pBeam[nIndex]->RelinkBeam();
}

//=========================================================
// Purpose:
//=========================================================
void CVoltigoreEnergyBall::ClearBeam(int nIndex)
{
	if (m_pBeam[nIndex])
	{
		UTIL_Remove(m_pBeam[nIndex]);
		m_pBeam[nIndex] = NULL;
	}
}

//=========================================================
// CreateBeams - create all beams
//=========================================================
void CVoltigoreEnergyBall::CreateBeams()
{
	for (int i = 0; i < VOLTIGORE_MAX_BEAMS; ++i)
	{
		CreateBeam(i, pev->origin, VOLTIGORE_ZAP_WIDTH, VOLTIGORE_ZAP_BRIGHTNESS );
	}
	m_iBeams = VOLTIGORE_MAX_BEAMS;
}

//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CVoltigoreEnergyBall::ClearBeams()
{
	for (int i = 0; i < VOLTIGORE_MAX_BEAMS; ++i)
	{
		ClearBeam( i );
	}
	m_iBeams = 0;
}


void CVoltigoreEnergyBall::UpdateBeams()
{
	int i, j;

	TraceResult tr;
	const Vector vecSrc = pev->origin;
	const int baseDistance = VOLTIGORE_ZAP_DISTANCE;
	UTIL_MakeVectors(pev->angles);
	for (i = 0; i < m_iBeams; ++i)
	{
		for (j = 0; j < 3; ++j)
		{
			//ALERT(at_console, "Randomize: %f %f\n", randomX, randomY);
			Vector vecTarget = vecSrc + (gpGlobals->v_right * RANDOM_FLOAT(-1,1) + gpGlobals->v_up * RANDOM_FLOAT(-1,1)) * baseDistance;
			TraceResult tr1;
			UTIL_TraceLine(vecSrc, vecTarget, ignore_monsters, ENT(pev), &tr1);
			if (tr1.flFraction != 1.0f) {
				tr = tr1;
				break;
			}
		}

		// Update the target position of the beam.
		UpdateBeam(i, tr.vecEndPos, tr.flFraction != 1.0f);
	}
}

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		VOLTIGORE_AE_THROW			( 1 )
#define		VOLTIGORE_AE_PUNCH_BOTH		( 12 )
#define		VOLTIGORE_AE_PUNCH_SINGLE	( 13 )
#define		VOLTIGORE_AE_GIB			( 2002 )


//=========================================================
// CVoltigore
//=========================================================
class CVoltigore : public CSquadMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	void SetYawSpeed(void);
	virtual int  DefaultClassify(void);
	const char* DefaultDisplayName() { return "Voltigore"; }
	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);
	virtual void IdleSound(void);
	virtual void PainSound(void);
	virtual void DeathSound(void);
	virtual void AlertSound(void);
	void AttackSound(void);
	virtual void StartTask(Task_t *pTask);
	virtual BOOL CheckMeleeAttack1(float flDot, float flDist);
	virtual BOOL CheckRangeAttack1(float flDot, float flDist);
	virtual void RunAI(void);
	virtual void GibMonster();
	Schedule_t *GetSchedule(void);
	Schedule_t *GetScheduleOfType(int Type);
	virtual void Killed(entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib);
	void UpdateOnRemove();
	const char* DefaultGibModel() {
		return "models/vgibs.mdl";
	}
	int DefaultGibCount() {
		return VOLTIGORE_GIB_COUNT;
	}
	
	int	Save(CSave &save);
	int Restore(CRestore &restore);

	CUSTOM_SCHEDULES
	static TYPEDESCRIPTION m_SaveData[];

	virtual int DefaultSizeForGrapple() { return GRAPPLE_LARGE; }
	Vector DefaultMinHullSize() {
#if FEATURE_VOLGITOGRE_LESSER_SIZE
		return Vector( -40.0f, -40.0f, 0.0f );
#else
		return Vector( -80.0f, -80.0f, 0.0f );
#endif
	}
	Vector DefaultMaxHullSize() {
#if FEATURE_VOLGITOGRE_LESSER_SIZE
		return Vector( 40.0f, 40.0f, 85.0f );
#else
		return Vector( 80.0f, 80.0f, 90.0f );
#endif
	}

	float m_flNextBeamAttackCheck; // next time the voltigore can use the spit attack.
	BOOL m_fShouldUpdateBeam;
	CBeam* m_pBeam[3];
	CSprite* m_pBeamGlow;
	int m_glowBrightness;

	static const char* pAlertSounds[];
	static const char* pAttackMeleeSounds[];
	static const char* pMeleeHitSounds[];
	static const char* pMeleeMissSounds[];
	static const char* pComSounds[];
	static const char* pDeathSounds[];
	static const char* pFootstepSounds[];
	static const char* pIdleSounds[];
	static const char* pPainSounds[];
	static const char* pGruntSounds[];

	void CreateBeams();
	void DestroyBeams();
	void UpdateBeams();

	void CreateGlow();
	void DestroyGlow();
	void GlowUpdate();
	void GlowOff(void);
	void GlowOn(int level);
protected:
	void GibBeamDamage();
	void PrecacheImpl(const char* modelName);
	int m_beamTexture;
};

LINK_ENTITY_TO_CLASS(monster_alien_voltigore, CVoltigore)


TYPEDESCRIPTION	CVoltigore::m_SaveData[] =
{
	DEFINE_FIELD(CVoltigore, m_flNextBeamAttackCheck, FIELD_TIME),
	DEFINE_FIELD(CVoltigore, m_fShouldUpdateBeam, FIELD_BOOLEAN),
	DEFINE_ARRAY(CVoltigore, m_pBeam, FIELD_CLASSPTR, 3),
	DEFINE_FIELD(CVoltigore, m_glowBrightness, FIELD_INTEGER),
	DEFINE_FIELD(CVoltigore, m_pBeamGlow, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE(CVoltigore, CSquadMonster)

const char* CVoltigore::pAlertSounds[] =
{
	"voltigore/voltigore_alert1.wav",
	"voltigore/voltigore_alert2.wav",
	"voltigore/voltigore_alert3.wav",
};

const char* CVoltigore::pAttackMeleeSounds[] =
{
	"voltigore/voltigore_attack_melee1.wav",
	"voltigore/voltigore_attack_melee2.wav",
};

const char* CVoltigore::pMeleeHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char* CVoltigore::pMeleeMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char* CVoltigore::pComSounds[] =
{
	"voltigore/voltigore_communicate1.wav",
	"voltigore/voltigore_communicate2.wav",
	"voltigore/voltigore_communicate3.wav",
};


const char* CVoltigore::pDeathSounds[] =
{
	"voltigore/voltigore_die1.wav",
	"voltigore/voltigore_die2.wav",
	"voltigore/voltigore_die3.wav",
};

const char* CVoltigore::pFootstepSounds[] =
{
	"voltigore/voltigore_footstep1.wav",
	"voltigore/voltigore_footstep2.wav",
	"voltigore/voltigore_footstep3.wav",
};

const char* CVoltigore::pIdleSounds[] =
{
	"voltigore/voltigore_idle1.wav",
	"voltigore/voltigore_idle2.wav",
	"voltigore/voltigore_idle3.wav",
};

const char* CVoltigore::pPainSounds[] =
{
	"voltigore/voltigore_pain1.wav",
	"voltigore/voltigore_pain2.wav",
	"voltigore/voltigore_pain3.wav",
	"voltigore/voltigore_pain4.wav",
};

const char* CVoltigore::pGruntSounds[] =
{
	"voltigore/voltigore_run_grunt1.wav",
	"voltigore/voltigore_run_grunt2.wav",
};

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CVoltigore::CheckRangeAttack1(float flDot, float flDist)
{
	if (IsMoving() && flDist >= 512)
	{
		// voltigore will far too far behind if he stops running to spit at this distance from the enemy.
		return FALSE;
	}

	if (flDist > 128.0f && flDist <= 1024.0f && flDot >= 0.5f && gpGlobals->time >= m_flNextBeamAttackCheck)
	{
		Vector vecStart, vecDir;
		TraceResult tr;

		UTIL_MakeVectors( pev->angles );
		GetAttachment( 3, vecStart, vecDir );
		UTIL_TraceLine( vecStart, m_hEnemy->BodyTarget( vecStart ), dont_ignore_monsters, ENT( pev ), &tr );

		if( tr.flFraction == 1.0f || tr.pHit == m_hEnemy->edict() )
		{
			m_flNextBeamAttackCheck = gpGlobals->time + RANDOM_FLOAT( 5.0f, 10.0f );
			return TRUE;
		}
		else
		{
			m_flNextBeamAttackCheck = gpGlobals->time + 0.2;
		}
	}

	return FALSE;
}

//=========================================================
//=========================================================
void CVoltigore::RunAI(void)
{
	CSquadMonster::RunAI();

	if (m_fShouldUpdateBeam)
	{
		UpdateBeams();
	}

	GlowUpdate();
}

void CVoltigore::GibMonster()
{
	GibBeamDamage();
	CSquadMonster::GibMonster();
}

//=========================================================
// CheckMeleeAttack1 - voltigore is a big guy, so has a longer
// melee range than most monsters. This is the tailwhip attack
//=========================================================
BOOL CVoltigore::CheckMeleeAttack1(float flDot, float flDist)
{
	if (HasConditions(bits_COND_SEE_ENEMY) && flDist <= 128.0f && flDot >= 0.6 && m_hEnemy != 0)
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CVoltigore::DefaultClassify(void)
{
	return	CLASS_RACEX_SHOCK;
}

//=========================================================
// IdleSound 
//=========================================================
void CVoltigore::IdleSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), 1, ATTN_NORM);
}

//=========================================================
// PainSound 
//=========================================================
void CVoltigore::PainSound(void)
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1, ATTN_NORM, 0, RANDOM_LONG(85, 120));
}

//=========================================================
// AlertSound
//=========================================================
void CVoltigore::AlertSound(void)
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), 1, ATTN_NORM, 0, RANDOM_LONG(140, 160));
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CVoltigore::SetYawSpeed(void)
{
	int ys;

	ys = 0;

	switch (m_Activity)
	{
	case	ACT_WALK:			ys = 90;	break;
	case	ACT_RUN:			ys = 90;	break;
	case	ACT_IDLE:			ys = 90;	break;
	case	ACT_RANGE_ATTACK1:	ys = 90;	break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CVoltigore::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case VOLTIGORE_AE_THROW:
	{
		// SOUND HERE!
		Vector	vecShootDir;

		UTIL_MakeVectors(pev->angles);

		Vector vecBoltOrigin, vecAngles;
		GetAttachment(3, vecBoltOrigin, vecAngles);

		Vector vecEnemyPosition;
		if (m_hEnemy != 0)
		{
			if (HasConditions(bits_COND_ENEMY_OCCLUDED))
			{
				vecEnemyPosition = m_vecEnemyLKP + (m_hEnemy->BodyTarget(pev->origin) - m_hEnemy->pev->origin);
			}
			else
			{
				vecEnemyPosition = m_hEnemy->BodyTarget(pev->origin);
			}
		}
		else
			vecEnemyPosition = m_vecEnemyLKP;
		vecShootDir = (vecEnemyPosition - vecBoltOrigin).Normalize();

		// do stuff for this event.
		//AttackSound();

		CVoltigoreEnergyBall::Shoot(pev, vecBoltOrigin, vecShootDir * 1000);

		// turn the beam glow off.
		DestroyBeams();

		GlowOff();

		m_fShouldUpdateBeam = FALSE;
	}
	break;


	case VOLTIGORE_AE_PUNCH_SINGLE:
	{
		// SOUND HERE!
		CBaseEntity *pHurt = CheckTraceHullAttack(128.0f, gSkillData.voltigoreDmgPunch, DMG_CLUB);
		if (pHurt)
		{
			if (FBitSet(pHurt->pev->flags, FL_MONSTER|FL_CLIENT))
			{
				pHurt->pev->punchangle.z = -15;
				pHurt->pev->punchangle.x = 15;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * -150;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
			}

			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeHitSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);

			Vector vecArmPos, vecArmAng;
			GetAttachment( 0, vecArmPos, vecArmAng );
			SpawnBlood( vecArmPos, pHurt->BloodColor(), 25 );// a little surface blood.
		}
		else
		{
			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeMissSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		}
	}
	break;

	case VOLTIGORE_AE_PUNCH_BOTH:
	{
		// SOUND HERE!
		CBaseEntity *pHurt = CheckTraceHullAttack(128.0f, gSkillData.voltigoreDmgPunch, DMG_CLUB);
		if (pHurt)
		{
			if (FBitSet(pHurt->pev->flags, FL_MONSTER|FL_CLIENT))
			{
				pHurt->pev->punchangle.x = 20;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 150;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
			}

			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeHitSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);

			Vector vecArmPos, vecArmAng;
			GetAttachment( 0, vecArmPos, vecArmAng );
			SpawnBlood( vecArmPos, pHurt->BloodColor(), 25 );// a little surface blood.
		}
		else
		{
			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeMissSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		}
	}
	break;

	case VOLTIGORE_AE_GIB:
	{
		pev->health = 0;
		GibMonster();
	}
	break;

	default:
		CSquadMonster::HandleAnimEvent(pEvent);
	}
}

//=========================================================
// Spawn
//=========================================================
void CVoltigore::Spawn()
{
	Precache();

	SetMyModel("models/voltigore.mdl");
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	SetMyBloodColor(BLOOD_COLOR_GREEN);
	pev->effects		= 0;
	SetMyHealth(gSkillData.voltigoreHealth);
	SetMyFieldOfView(0.2f);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_TURN_HEAD | bits_CAP_SQUAD;

	m_flNextBeamAttackCheck	= gpGlobals->time;

	m_fShouldUpdateBeam = FALSE;
	m_pBeamGlow = NULL;

	GlowOff();

	// Create glow.
	CreateGlow();

	MonsterInit();
	pev->view_ofs		= Vector(0, 0, 84);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CVoltigore::Precache()
{
	PrecacheImpl("models/voltigore.mdl");
	PRECACHE_MODEL("models/vgibs.mdl");
}

void CVoltigore::PrecacheImpl(const char *modelName)
{
	PrecacheMyModel(modelName);
	
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pAttackMeleeSounds);
	PRECACHE_SOUND_ARRAY(pMeleeHitSounds);
	PRECACHE_SOUND_ARRAY(pMeleeMissSounds);
	PRECACHE_SOUND_ARRAY(pComSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pFootstepSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pGruntSounds);

	PRECACHE_SOUND("voltigore/voltigore_attack_shock.wav");
	PRECACHE_SOUND("voltigore/voltigore_eat.wav");

	PRECACHE_SOUND("debris/beamstart1.wav");

	m_beamTexture = PRECACHE_MODEL(VOLTIGORE_ZAP_BEAM);
	PRECACHE_MODEL(VOLTIGORE_GLOW_SPRITE);
	UTIL_PrecacheOther("voltigore_energy_ball");
}

//=========================================================
// DeathSound
//=========================================================
void CVoltigore::DeathSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), 1, ATTN_NORM);
}

//=========================================================
// AttackSound
//=========================================================
void CVoltigore::AttackSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "voltigore/voltigore_attack_shock.wav", 1, ATTN_NORM);
}

//========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t	tlVoltigoreRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t	slVoltigoreRangeAttack1[] =
{
	{
		tlVoltigoreRangeAttack1,
		ARRAYSIZE(tlVoltigoreRangeAttack1),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_NO_AMMO_LOADED,
		0,
		"Voltigore Range Attack1"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t tlVoltigoreVictoryDance[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_WAIT, 0.2f },
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,	50.0f },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_STAND },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.2f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.2f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.2f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.2f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_STAND },
};

Schedule_t slVoltigoreVictoryDance[] =
{
	{
		tlVoltigoreVictoryDance,
		ARRAYSIZE( tlVoltigoreVictoryDance ),
		bits_COND_NEW_ENEMY |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		0,
		"VoltigoreVictoryDance"
	},
};

DEFINE_CUSTOM_SCHEDULES(CVoltigore)
{
	slVoltigoreRangeAttack1,
	slVoltigoreVictoryDance
};

IMPLEMENT_CUSTOM_SCHEDULES(CVoltigore, CSquadMonster)

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CVoltigore::GetSchedule(void)
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
	{
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD|bits_COND_ENEMY_LOST))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CSquadMonster::GetSchedule();
		}

		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			return GetScheduleOfType(SCHED_WAKE_ANGRY);
		}

		if( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
		{
			return GetScheduleOfType(SCHED_CHASE_ENEMY);
		}

		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_RANGE_ATTACK1);
		}

		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}

		return GetScheduleOfType(SCHED_CHASE_ENEMY);

		break;
	}
	default:
		break;
	}

	return CSquadMonster::GetSchedule();
}

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t* CVoltigore::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_RANGE_ATTACK1:
		return &slVoltigoreRangeAttack1[0];
		break;
	case SCHED_VICTORY_DANCE:
		return &slVoltigoreVictoryDance[0];
		break;
	}

	return CSquadMonster::GetScheduleOfType(Type);
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for voltigore because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CVoltigore::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;
	GlowOff();
	DestroyBeams();
	m_fShouldUpdateBeam = FALSE;

	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
		{
			CreateBeams();

			GlowOn( 255 );
			m_fShouldUpdateBeam = TRUE;

			// Play the beam 'glow' sound.
			EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "debris/beamstart1.wav", 1, ATTN_NORM, 0, PITCH_HIGH);

			CSquadMonster::StartTask(pTask);
		}
		break;
	case TASK_GET_PATH_TO_ENEMY:
		{
			CBaseEntity *pEnemy = m_hEnemy;

			if( pEnemy == NULL )
			{
				TaskFail("no enemy");
				return;
			}

			if( BuildRoute( pEnemy->pev->origin, bits_MF_TO_ENEMY, pEnemy ) )
			{
				TaskComplete();
			}
			else
			{
				TaskFail("can't build path to enemy");
			}
			break;
		}
		break;
	default:
		CSquadMonster::StartTask(pTask);
		break;
	}
}

void CVoltigore::Killed(entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib)
{
	DestroyBeams();
	DestroyGlow();
	
	int iTimes = 0;
	int iDrawn = 0;
	const int iBeams = VOLTIGORE_MAX_BEAMS;
	while( iDrawn < iBeams && iTimes < ( iBeams * 3 ) )
	{
		TraceResult tr;
		const Vector vecOrigin = Center();
		const Vector vecDest = VOLTIGORE_ZAP_DISTANCE * ( Vector( RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ) ).Normalize() );
		UTIL_TraceLine( vecOrigin, vecOrigin + vecDest, ignore_monsters, ENT( pev ), &tr );
		if( tr.flFraction != 1.0 )
		{
			// we hit something.
			iDrawn++;
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_BEAMPOINTS );
				WRITE_COORD( vecOrigin.x );
				WRITE_COORD( vecOrigin.y );
				WRITE_COORD( vecOrigin.z );
				WRITE_COORD( tr.vecEndPos.x );
				WRITE_COORD( tr.vecEndPos.y );
				WRITE_COORD( tr.vecEndPos.z );
				WRITE_SHORT( m_beamTexture );
				WRITE_BYTE( 0 ); // framestart
				WRITE_BYTE( 10 ); // framerate
				WRITE_BYTE( RANDOM_LONG( 8, 10 ) ); // life
				WRITE_BYTE( VOLTIGORE_ZAP_WIDTH );  // width
				WRITE_BYTE( VOLTIGORE_ZAP_NOISE );   // noise
				WRITE_BYTE( VOLTIGORE_ZAP_RED );   // r, g, b
				WRITE_BYTE( VOLTIGORE_ZAP_GREEN);   // r, g, b
				WRITE_BYTE( VOLTIGORE_ZAP_BLUE );   // r, g, b
				WRITE_BYTE( VOLTIGORE_ZAP_BRIGHTNESS );	// brightness
				WRITE_BYTE( 35 );		// speed
			MESSAGE_END();
		}
		iTimes++;
	}

	CSquadMonster::Killed(pevInflictor, pevAttacker, iGib);
}

void CVoltigore::UpdateOnRemove()
{
	DestroyBeams();
	DestroyGlow();
	CSquadMonster::UpdateOnRemove();
}

void CVoltigore::GibBeamDamage()
{
	CBaseEntity *pEntity = NULL;
	// iterate on all entities in the vicinity.
	const float attackRadius = Q_max( Q_min(gSkillData.voltigoreDmgExplode * 2, 200.0f), 160.0f);
	while( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, attackRadius ) ) != NULL )
	{
		if( pEntity->pev->takedamage != DAMAGE_NO )
		{
			if( pEntity->pev->classname != pev->classname && !FClassnameIs( pEntity->pev, "monster_alien_babyvoltigore" ) )
			{
				// voltigores don't hurt other voltigores on death
				const float flDist = ( pEntity->Center() - pev->origin ).Length();

				float flAdjustedDamage = gSkillData.voltigoreDmgExplode;
				flAdjustedDamage -= ( flDist / attackRadius ) * flAdjustedDamage;

				if( !FVisible( pEntity ) )
				{
					if( pEntity->IsPlayer() )
					{
						// if this entity is a client, and is not in full view, inflict half damage. We do this so that players still 
						// take the residual damage if they don't totally leave the houndeye's effective radius. We restrict it to clients
						// so that monsters in other parts of the level don't take the damage and get pissed.
						flAdjustedDamage *= 0.5;
					}
					else if( !FClassnameIs( pEntity->pev, "func_breakable" ) && !FClassnameIs( pEntity->pev, "func_pushable" ) ) 
					{
						// do not hurt nonclients through walls, but allow damage to be done to breakables
						flAdjustedDamage = 0;
					}
				}

				if( flAdjustedDamage > 0 )
				{
					pEntity->TakeDamage( pev, pev, flAdjustedDamage, DMG_SHOCK );
				}
			}
		}
	}
}

void CVoltigore::CreateBeams()
{
	Vector vecStart, vecEnd, vecAngles;
	GetAttachment(3, vecStart, vecAngles);

	for (int i = 0; i < 3; i++)
	{
		m_pBeam[i] = CBeam::BeamCreate(VOLTIGORE_ZAP_BEAM, VOLTIGORE_ZAP_WIDTH);
		if (!m_pBeam[i])
			return;

		GetAttachment(i, vecEnd, vecAngles);

		m_pBeam[i]->PointsInit(vecStart, vecEnd);
		m_pBeam[i]->SetColor(VOLTIGORE_ZAP_RED, VOLTIGORE_ZAP_GREEN, VOLTIGORE_ZAP_BLUE);
		m_pBeam[i]->SetBrightness(VOLTIGORE_ZAP_BRIGHTNESS);
		m_pBeam[i]->SetNoise(VOLTIGORE_ZAP_NOISE);
	}
}

void CVoltigore::DestroyBeams()
{
	for (int i = 0; i < 3; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove(m_pBeam[i]);
			m_pBeam[i] = NULL;
		}
	}
}

void CVoltigore::UpdateBeams()
{
	Vector vecStart, vecEnd, vecAngles;
	GetAttachment(3, vecStart, vecAngles);

	for (int i = 0; i < 3; i++)
	{
		if (!m_pBeam[i]) {
			continue;
		}
		GetAttachment(i, vecEnd, vecAngles);
		m_pBeam[i]->SetStartPos(vecStart);
		m_pBeam[i]->SetEndPos(vecEnd);
		m_pBeam[i]->RelinkBeam();
	}
}

void CVoltigore::CreateGlow()
{
	m_pBeamGlow = CSprite::SpriteCreate(VOLTIGORE_GLOW_SPRITE, pev->origin, FALSE);
	m_pBeamGlow->SetTransparency(kRenderTransAdd, 255, 255, 255, 0, kRenderFxNoDissipation);
	m_pBeamGlow->SetScale(VOLTIGORE_GLOW_SCALE);
	m_pBeamGlow->SetAttachment(edict(), 4);
}

void CVoltigore::DestroyGlow()
{
	if (m_pBeamGlow)
	{
		UTIL_Remove(m_pBeamGlow);
		m_pBeamGlow = NULL;
	}
}

void CVoltigore::GlowUpdate()
{
	if (m_pBeamGlow)
	{
		m_pBeamGlow->pev->renderamt = UTIL_Approach(m_glowBrightness, m_pBeamGlow->pev->renderamt, 100);
		if (m_pBeamGlow->pev->renderamt == 0)
			m_pBeamGlow->pev->effects |= EF_NODRAW;
		else
			m_pBeamGlow->pev->effects &= ~EF_NODRAW;
		UTIL_SetOrigin(m_pBeamGlow->pev, pev->origin);
	}
}

void CVoltigore::GlowOff(void)
{
	m_glowBrightness = 0;
}

void CVoltigore::GlowOn(int level)
{
	m_glowBrightness = level;
}


//=========================================================
// CBabyAlienVoltigore
//=========================================================

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		BABY_VOLTIGORE_AE_RUN			( 14 )

class CBabyVoltigore : public CVoltigore
{
public:
	void	Spawn(void);
	void	Precache(void);
	const char* DefaultDisplayName() { return "Baby Voltigore"; }
	void	HandleAnimEvent(MonsterEvent_t* pEvent);
	BOOL	CheckMeleeAttack1(float flDot, float flDist);
	BOOL	CheckRangeAttack1(float flDot, float flDist);
	void	StartTask(Task_t *pTask);
	void	Killed(entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib);
	void	GibMonster();
	Schedule_t* GetSchedule();
	Schedule_t* GetScheduleOfType(int Type);

	virtual int DefaultSizeForGrapple() { return GRAPPLE_SMALL; }
	Vector DefaultMinHullSize() { return Vector( -16.0f, -16.0f, 0.0f ); }
	Vector DefaultMaxHullSize() { return Vector( 16.0f, 16.0f, 32.0f ); }
};

LINK_ENTITY_TO_CLASS(monster_alien_babyvoltigore, CBabyVoltigore)



//=========================================================
// Spawn
//=========================================================
void CBabyVoltigore::Spawn()
{
	Precache();

	SetMyModel("models/baby_voltigore.mdl");
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	SetMyBloodColor(BLOOD_COLOR_GREEN);
	pev->effects		= 0;
	SetMyHealth(gSkillData.babyVoltigoreHealth);
	SetMyFieldOfView(0.2f);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_TURN_HEAD | bits_CAP_SQUAD;

	m_flNextBeamAttackCheck	= gpGlobals->time;

	MonsterInit();
	pev->view_ofs		= Vector(0, 0, 32);
}

//=========================================================
//=========================================================
void CBabyVoltigore::Precache(void)
{
	PrecacheImpl("models/baby_voltigore.mdl");
}

void CBabyVoltigore::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case BABY_VOLTIGORE_AE_RUN:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pFootstepSounds), RANDOM_FLOAT(0.5, 0.6), ATTN_NORM, 0, RANDOM_LONG(85, 120));
		break;

	case VOLTIGORE_AE_PUNCH_SINGLE:
	{
		CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.babyVoltigoreDmgPunch, DMG_CLUB | DMG_ALWAYSGIB);
		if (pHurt)
		{
			if (FBitSet(pHurt->pev->flags, FL_MONSTER|FL_CLIENT))
			{
				pHurt->pev->punchangle.z = -10;
				pHurt->pev->punchangle.x = 10;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * -100;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 50;
			}

			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeHitSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);

			Vector vecArmPos, vecArmAng;
			GetAttachment( 0, vecArmPos, vecArmAng );
			SpawnBlood( vecArmPos, pHurt->BloodColor(), 25 );// a little surface blood.
		}
		else
		{
			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeMissSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		}
	}
	break;

	case VOLTIGORE_AE_PUNCH_BOTH:
	{
		CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.babyVoltigoreDmgPunch, DMG_CLUB | DMG_ALWAYSGIB);
		if (pHurt)
		{
			if (FBitSet(pHurt->pev->flags, FL_MONSTER|FL_CLIENT))
			{
				pHurt->pev->punchangle.x = 15;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 50;

				Vector vecArmPos, vecArmAng;
				GetAttachment( 0, vecArmPos, vecArmAng );
				SpawnBlood( vecArmPos, pHurt->BloodColor(), 25 );// a little surface blood.
			}

			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeHitSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		}
		else
		{
			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeMissSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		}
	}
	break;
	default:
		CVoltigore::HandleAnimEvent(pEvent);
		break;
	}
}

BOOL CBabyVoltigore::CheckMeleeAttack1(float flDot, float flDist)
{
	if (HasConditions(bits_COND_SEE_ENEMY) && flDist <= 64.0f && flDot >= 0.6f && m_hEnemy != 0)
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for voltigore because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CBabyVoltigore::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_MELEE_ATTACK1:
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAttackMeleeSounds), RANDOM_FLOAT(0.5, 0.6), ATTN_NONE, 0, RANDOM_LONG(110, 120));
		CSquadMonster::StartTask(pTask);
	}
	break;
	default:
		CSquadMonster::StartTask(pTask);
		break;
	}
}

void CBabyVoltigore::Killed(entvars_t *pevInflictor, entvars_t* pevAttacker, int iGib)
{
	DestroyBeams();
	CSquadMonster::Killed(pevInflictor, pevAttacker, iGib);
}

void CBabyVoltigore::GibMonster()
{
	CSquadMonster::GibMonster();
}

BOOL CBabyVoltigore::CheckRangeAttack1(float flDot, float flDist)
{
	return FALSE;
}

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CBabyVoltigore::GetSchedule(void)
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
	{
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD|bits_COND_ENEMY_LOST))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CSquadMonster::GetSchedule();
		}

		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			return GetScheduleOfType(SCHED_WAKE_ANGRY);
		}

		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}

		return GetScheduleOfType(SCHED_CHASE_ENEMY);

		break;
	}
	default:
		break;
	}

	return CSquadMonster::GetSchedule();
}

Schedule_t *CBabyVoltigore::GetScheduleOfType(int Type)
{
	switch (Type) {
	// For some cryptic reason baby voltigore tries to start the range attack even though its model does not have sequence with range attack activity. 
	// This hack is for preventing baby voltigore to do this.
	case SCHED_RANGE_ATTACK1:
		return GetScheduleOfType(SCHED_CHASE_ENEMY);
		break;
	default:
		return CVoltigore::GetScheduleOfType(Type);
		break;
	}
}
#endif
