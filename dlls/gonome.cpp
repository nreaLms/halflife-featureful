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
#include	"scripted.h"
#include	"animation.h"
#include	"studio.h"
#include	"mod_features.h"
#include	"game.h"
#include	"common_soundscripts.h"

#if FEATURE_GONOME

/* Gonome's model references step sounds but there're none in files.
 * If your mod has suitable sounds for gonome's foot steps, enable this feature to precache step sounds.
 */
#define FEATURE_GONOME_STEP_SOUNDS 0

#define		GONOME_MELEE_ATTACK_RADIUS		70

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

#define GONOME_AE_SLASH_RIGHT	( 1 )
#define GONOME_AE_SLASH_LEFT	( 2 )
#define GONOME_AE_SPIT			( 3 )
#define GONOME_AE_THROW			( 4 )

#define GONOME_AE_BITE1			( 19 )
#define GONOME_AE_BITE2			( 20 )
#define GONOME_AE_BITE3			( 21 )
#define GONOME_AE_BITE4			( 22 )

#define GONOME_SCRIPT_EVENT_SOUND ( 1011 )

//=========================================================
// Gonome's guts projectile
//=========================================================
class CGonomeGuts : public CSquidSpit
{
public:
	void Spawn();
	void Precache();
	void Touch(CBaseEntity *pOther);

	static constexpr const char* spitTouchSoundScript = "Gonome.SpitTouch";
	static constexpr const char* spitHitSoundScript = "Gonome.SpitHit";

	static const NamedVisual gutsVisual;
};

LINK_ENTITY_TO_CLASS( gonomeguts, CGonomeGuts )

const NamedVisual CGonomeGuts::gutsVisual = BuildVisual::Animated("Gonome.Guts")
		.Model("sprites/bigspit.spr")
		.RenderProps(kRenderTransAlpha, Color(255, 0, 0))
		.Scale(0.5f);

void CGonomeGuts::Spawn()
{
	SpawnHelper("gonomeguts", gutsVisual);
}

void CGonomeGuts::Precache()
{
	RegisterVisual(gutsVisual);
	RegisterAndPrecacheSoundScript(spitTouchSoundScript, NPC::spitTouchSoundScript);
	RegisterAndPrecacheSoundScript(spitHitSoundScript, NPC::spitHitSoundScript);
}

void CGonomeGuts::Touch( CBaseEntity *pOther )
{
	TraceResult tr;

	EmitSoundScript(spitTouchSoundScript);
	EmitSoundScript(spitHitSoundScript);

	if( !pOther->pev->takedamage )
	{
		// make a splat on the wall
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_BloodDecalTrace( &tr, BLOOD_COLOR_RED );
		UTIL_BloodDrips( tr.vecEndPos, UTIL_RandomBloodVector(), BLOOD_COLOR_RED, 35 );
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
class CGonome : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("gonome"); }

	int  DefaultClassify(void);
	const char* DefaultDisplayName() { return "Gonome"; }
	void SetYawSpeed();
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	int IgnoreConditions();
	void IdleSound(void);
	void PainSound(void);
	void DeathSound(void);
	void AlertSound(void);

	BOOL CheckMeleeAttack2(float flDot, float flDist);
	BOOL CheckRangeAttack1(float flDot, float flDist);
	int LookupActivity(int activity);
	void SetActivity( Activity NewActivity );

	Schedule_t *GetSchedule();
	Schedule_t *GetScheduleOfType( int Type );
	void RunTask(Task_t* pTask);

	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	void OnDying();
	void UpdateOnRemove();

	void UnlockPlayer();
	CGonomeGuts* GetGonomeGuts(const Vector& pos);
	void ClearGuts();

	int	Save(CSave &save);
	int Restore(CRestore &restore);
	
	CUSTOM_SCHEDULES
	static TYPEDESCRIPTION m_SaveData[];

	static const NamedSoundScript idleSoundScript;
	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
	static constexpr const char* attackHitSoundScript = "Gonome.AttackHit";
	static constexpr const char* attackMissSoundScript = "Gonome.AttackMiss";
	static const NamedSoundScript biteSoundScript;
	static const NamedSoundScript melee1SoundScript;
	static const NamedSoundScript melee2SoundScript;

	virtual int DefaultSizeForGrapple() { return GRAPPLE_LARGE; }
	bool IsDisplaceable() { return true; }
	Vector DefaultMinHullSize() { return VEC_HUMAN_HULL_MIN; }
	Vector DefaultMaxHullSize() { return VEC_HUMAN_HULL_MAX; }
protected:
	float m_flNextFlinch;
	float m_flNextThrowTime;// last time the gonome used the guts attack.
	CGonomeGuts* m_pGonomeGuts;
	BOOL m_fPlayerLocked;
	EHANDLE m_lockedPlayer;
	bool m_meleeAttack2;
	bool m_playedAttackSound;
};

LINK_ENTITY_TO_CLASS(monster_gonome, CGonome)

const NamedSoundScript CGonome::idleSoundScript = {
	CHAN_VOICE,
	{"gonome/gonome_idle1.wav", "gonome/gonome_idle2.wav", "gonome/gonome_idle3.wav"},
	IntRange(95, 105),
	"Gonome.Idle"
};

const NamedSoundScript CGonome::alertSoundScript = {
	CHAN_VOICE,
	{"zombie/zo_alert10.wav", "zombie/zo_alert20.wav", "zombie/zo_alert30.wav"},
	IntRange(95, 104),
	"Gonome.Alert"
};

const NamedSoundScript CGonome::painSoundScript = {
	CHAN_VOICE,
	{"gonome/gonome_pain1.wav", "gonome/gonome_pain2.wav", "gonome/gonome_pain3.wav", "gonome/gonome_pain4.wav"},
	IntRange(95, 104),
	"Gonome.Pain"
};

const NamedSoundScript CGonome::dieSoundScript = {
	CHAN_VOICE,
	{"gonome/gonome_death2.wav", "gonome/gonome_death3.wav", "gonome/gonome_death4.wav"},
	"Gonome.Die"
};

const NamedSoundScript CGonome::biteSoundScript = {
	CHAN_WEAPON,
	{"bullchicken/bc_bite2.wav", "bullchicken/bc_bite3.wav"},
	IntRange(90, 110),
	"Gonome.Bite"
};

const NamedSoundScript CGonome::melee1SoundScript = {
	CHAN_WEAPON,
	{"gonome/gonome_melee1.wav"},
	"Gonome.Melee1"
};

const NamedSoundScript CGonome::melee2SoundScript = {
	CHAN_WEAPON,
	{"gonome/gonome_melee2.wav"},
	"Gonome.Melee2"
};

TYPEDESCRIPTION	CGonome::m_SaveData[] =
{
	DEFINE_FIELD( CGonome, m_flNextFlinch, FIELD_TIME ),
	DEFINE_FIELD( CGonome, m_flNextThrowTime, FIELD_TIME ),
	DEFINE_FIELD( CGonome, m_fPlayerLocked, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CGonome, CBaseMonster )

void CGonome::OnDying()
{
	ClearGuts();
	UnlockPlayer();
	CBaseMonster::OnDying();
}

void CGonome::UpdateOnRemove()
{
	ClearGuts();
	UnlockPlayer();
	CBaseMonster::UpdateOnRemove();
}

void CGonome::UnlockPlayer()
{
	if (g_modFeatures.gonome_lock_player)
	{
		if (m_fPlayerLocked)
		{
			CBasePlayer* player = 0;
			if (m_lockedPlayer != 0 && m_lockedPlayer->IsPlayer())
				player = (CBasePlayer*)((CBaseEntity*)m_lockedPlayer);
			else // if ehandle is empty for some reason just unlock the first player
				player = (CBasePlayer*)UTIL_FindEntityByClassname(0, "player");

			if (player)
				player->EnableControl(TRUE);

			m_lockedPlayer = 0;
			m_fPlayerLocked = FALSE;
		}
	}
}

CGonomeGuts* CGonome::GetGonomeGuts(const Vector &pos)
{
	if (m_pGonomeGuts)
		return m_pGonomeGuts;
	CGonomeGuts *pGuts = GetClassPtr( (CGonomeGuts *)NULL );
	pGuts->AssignEntityOverrides(GetProjectileOverrides());
	pGuts->Spawn();

	UTIL_SetOrigin( pGuts->pev, pos );

	m_pGonomeGuts = pGuts;
	return m_pGonomeGuts;
}

void CGonome::ClearGuts()
{
	if (m_pGonomeGuts)
	{
		UTIL_Remove(m_pGonomeGuts);
		m_pGonomeGuts = NULL;
	}
}

int CGonome::LookupActivity(int activity)
{
	if (activity == ACT_MELEE_ATTACK1 && m_hEnemy != 0)
	{
		// special melee animations
		if ((pev->origin - m_hEnemy->pev->origin).Length2D() >= 48 )
		{
			m_meleeAttack2 = false;
			return LookupSequence("attack1");
		}
		else
		{
			m_meleeAttack2 = true;
			return LookupSequence("attack2");
		}
	}
	else
	{
		if (activity == ACT_RUN && m_hEnemy != 0)
		{
			// special run animations
			if ((pev->origin - m_hEnemy->pev->origin).Length2D() <= 512 )
			{
				return LookupSequence("runshort");
			}
			else
			{
				return LookupSequence("runlong");
			}
		}
		else
		{
			return CBaseMonster::LookupActivity(activity);
		}
	}
}

void CGonome::SetActivity( Activity NewActivity )
{
	if (NewActivity != ACT_RANGE_ATTACK1)
	{
		ClearGuts();
	}
	if (NewActivity != ACT_MELEE_ATTACK1 || m_hEnemy == 0)
	{
		UnlockPlayer();
	}
	CBaseMonster::SetActivity(NewActivity);
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
// TakeDamage - overridden for gonome so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CGonome::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	if( bitsDamageType == DMG_BULLET )
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce( flDamage );
		pev->velocity = pev->velocity + vecDir * flForce;
#if 0
		// Take 15% damage from bullets
		flDamage *= 0.15;
#endif
	}

	// HACK HACK -- until we fix this.
	if( IsAlive() )
		PainSound();

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}


//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CGonome::CheckRangeAttack1(float flDot, float flDist)
{
	// gonome won't try to shoot in short range
	if (flDist < 256)
		return FALSE;

	if (IsMoving() && flDist >= 512)
	{
		// gonome will far too far behind if he stops running to spit at this distance from the enemy.
		return FALSE;
	}

	if (flDist > 64 && flDist <= 784 && flDot >= 0.5 && gpGlobals->time >= m_flNextThrowTime)
	{
		if (m_hEnemy != 0)
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
			m_flNextThrowTime = gpGlobals->time + 5;
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextThrowTime = gpGlobals->time + 0.5;
		}

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
void CGonome::IdleSound(void)
{
	EmitSoundScript(idleSoundScript);
}

//=========================================================
// PainSound 
//=========================================================
void CGonome::PainSound(void)
{
	if (RANDOM_LONG(0, 5) < 2)
		EmitSoundScript(painSoundScript);
}

//=========================================================
// AlertSound
//=========================================================
void CGonome::AlertSound(void)
{
	EmitSoundScript(alertSoundScript);
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGonome::SetYawSpeed( void )
{
	pev->yaw_speed = 120;
}
//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGonome::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case GONOME_SCRIPT_EVENT_SOUND:
		if (m_Activity != ACT_MELEE_ATTACK1)
			EmitSound( CHAN_BODY, pEvent->options, 1, ATTN_NORM);
		break;
	case GONOME_AE_SPIT:
	{
		Vector vecArmPos, vecArmAng;
		GetAttachment(0, vecArmPos, vecArmAng);

		if (GetGonomeGuts(vecArmPos))
		{
			m_pGonomeGuts->pev->skin = entindex();
			m_pGonomeGuts->pev->body = 1;
			m_pGonomeGuts->pev->aiment = edict();
			m_pGonomeGuts->pev->movetype = MOVETYPE_FOLLOW;
		}
		UTIL_BloodDrips( vecArmPos, UTIL_RandomBloodVector(), BLOOD_COLOR_RED, 35 );
	}
	break;
	case GONOME_AE_THROW:
	{
		UTIL_MakeVectors(pev->angles);
		Vector vecArmPos, vecArmAng;
		GetAttachment(0, vecArmPos, vecArmAng);

		if (GetGonomeGuts(vecArmPos))
		{
			const Vector vecSpitDir = SpitAtEnemy(vecArmPos);

			m_pGonomeGuts->pev->body = 0;
			m_pGonomeGuts->pev->skin = 0;
			m_pGonomeGuts->pev->owner = ENT( pev );
			m_pGonomeGuts->pev->aiment = 0;
			m_pGonomeGuts->pev->movetype = MOVETYPE_FLY;
			m_pGonomeGuts->pev->velocity = vecSpitDir * 900;
			m_pGonomeGuts->SetThink( &CGonomeGuts::Animate );
			m_pGonomeGuts->pev->nextthink = gpGlobals->time + 0.1;
			UTIL_SetOrigin(m_pGonomeGuts->pev, vecArmPos);

			m_pGonomeGuts = 0;
		}
		UTIL_BloodDrips( vecArmPos, UTIL_RandomBloodVector(), BLOOD_COLOR_RED, 35 );
	}
	break;

	case GONOME_AE_SLASH_LEFT:
	{
		CBaseEntity *pHurt = CheckTraceHullAttack(GONOME_MELEE_ATTACK_RADIUS, gSkillData.gonomeDmgOneSlash, DMG_SLASH);
		if (pHurt)
		{
			if (FBitSet(pHurt->pev->flags, FL_MONSTER|FL_CLIENT))
			{
				pHurt->pev->punchangle.z = 9;
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 25;
			}
			EmitSoundScript(attackHitSoundScript);
		}
		else
		{
			EmitSoundScript(attackMissSoundScript);
		}
	}
	break;

	case GONOME_AE_SLASH_RIGHT:
	{
		CBaseEntity *pHurt = CheckTraceHullAttack(GONOME_MELEE_ATTACK_RADIUS, gSkillData.gonomeDmgOneSlash, DMG_SLASH);
		if (pHurt)
		{
			if (FBitSet(pHurt->pev->flags, FL_MONSTER|FL_CLIENT))
			{
				pHurt->pev->punchangle.z = -9;
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * -25;
			}
			EmitSoundScript(attackHitSoundScript);
		}
		else
		{
			EmitSoundScript(attackMissSoundScript);
		}
	}
	break;

	case GONOME_AE_BITE1:
	case GONOME_AE_BITE2:
	case GONOME_AE_BITE3:
	case GONOME_AE_BITE4:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack(GONOME_MELEE_ATTACK_RADIUS, gSkillData.gonomeDmgOneBite, DMG_SLASH);

			if (pHurt)
			{
				// croonchy bite sound
				EmitSoundScript(biteSoundScript);

				if (FBitSet(pHurt->pev->flags, FL_MONSTER|FL_CLIENT))
				{
					if (pEvent->event == GONOME_AE_BITE4)
					{
						pHurt->pev->punchangle.x = 15;
						pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 75;
					}
					else
					{
						pHurt->pev->punchangle.x = 9;
						pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 25;
					}
				}
				if (g_modFeatures.gonome_lock_player)
				{
					if (pEvent->event == GONOME_AE_BITE4)
					{
						UnlockPlayer();
					}
					else if (pHurt->IsPlayer() && pHurt->IsAlive())
					{
						if (!m_fPlayerLocked)
						{
							CBasePlayer* player = (CBasePlayer*)pHurt;
							player->EnableControl(FALSE);
							m_lockedPlayer = player;
							m_fPlayerLocked = TRUE;
						}
					}
				}
			}
		}
		break;



	default:
		CBaseMonster::HandleAnimEvent(pEvent);
	}
}

#define GONOME_FLINCH_DELAY 2

int CGonome::IgnoreConditions( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if (m_Activity == ACT_RANGE_ATTACK1)
	{
		iIgnore |= bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_ENEMY_TOOFAR | bits_COND_ENEMY_OCCLUDED;
	}
	else if( m_Activity == ACT_MELEE_ATTACK1 )
	{
		if( m_flNextFlinch >= gpGlobals->time )
			iIgnore |= ( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE );
	}

	if( ( m_Activity == ACT_SMALL_FLINCH ) || ( m_Activity == ACT_BIG_FLINCH ) )
	{
		if( m_flNextFlinch < gpGlobals->time )
			m_flNextFlinch = gpGlobals->time + GONOME_FLINCH_DELAY;
	}

	return iIgnore;
}

//=========================================================
// Spawn
//=========================================================
void CGonome::Spawn()
{
	Precache();

	SetMyModel("models/gonome.mdl");
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_GREEN );
	pev->effects = 0;
	SetMyHealth( gSkillData.gonomeHealth );
	SetMyFieldOfView(0.2f);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	m_flNextThrowTime = gpGlobals->time;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGonome::Precache()
{
	PrecacheMyModel("models/gonome.mdl");
	PrecacheMyGibModel();

	UTIL_PrecacheOther("gonomeguts", GetProjectileOverrides());

	RegisterAndPrecacheSoundScript(NPC::swishSoundScript);// because we use the basemonster SWIPE animation event

	PRECACHE_SOUND("gonome/gonome_eat.wav");
	PRECACHE_SOUND("gonome/gonome_jumpattack.wav");
#if FEATURE_GONOME_STEP_SOUNDS
	PRECACHE_SOUND("gonome/gonome_step1.wav");
	PRECACHE_SOUND("gonome/gonome_step2.wav");
#endif

	RegisterAndPrecacheSoundScript(idleSoundScript);
	RegisterAndPrecacheSoundScript(alertSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(attackHitSoundScript, NPC::attackHitSoundScript);
	RegisterAndPrecacheSoundScript(attackMissSoundScript, NPC::attackMissSoundScript);
	RegisterAndPrecacheSoundScript(biteSoundScript);
	RegisterAndPrecacheSoundScript(melee1SoundScript);
	RegisterAndPrecacheSoundScript(melee2SoundScript);

	PRECACHE_SOUND("gonome/gonome_run.wav");
}

//=========================================================
// DeathSound
//=========================================================
void CGonome::DeathSound(void)
{
	EmitSoundScript(dieSoundScript);
}

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CGonome::GetSchedule( void )
{
	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			// dead enemy
			if( HasConditions( bits_COND_ENEMY_DEAD|bits_COND_ENEMY_LOST ) )
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

// primary range attack
Task_t tlGonomeRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t slGonomeRangeAttack1[] =
{
	{
		tlGonomeRangeAttack1,
		ARRAYSIZE( tlGonomeRangeAttack1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED,
		0,
		"Gonome Range Attack1"
	},
};

// Chase enemy schedule
Task_t tlGonomeChaseEnemy1[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_RANGE_ATTACK1 },
	{ TASK_GET_PATH_TO_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
};

Schedule_t slGonomeChaseEnemy[] =
{
	{
		tlGonomeChaseEnemy1,
		ARRAYSIZE( tlGonomeChaseEnemy1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_SMELL_FOOD |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2 |
		bits_COND_TASK_FAILED,
		0,
		"Gonome Chase Enemy"
	},
};

// victory dance (eating body)
Task_t tlGonomeVictoryDance[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_WAIT, 0.1f },
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,	40.0f },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.25f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.25f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.25f },
};

Schedule_t slGonomeVictoryDance[] =
{
	{
		tlGonomeVictoryDance,
		ARRAYSIZE( tlGonomeVictoryDance ),
		bits_COND_NEW_ENEMY |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		0,
		"GonomeVictoryDance"
	},
};

DEFINE_CUSTOM_SCHEDULES( CGonome )
{
	slGonomeRangeAttack1,
	slGonomeChaseEnemy,
	slGonomeVictoryDance,
};

IMPLEMENT_CUSTOM_SCHEDULES( CGonome, CBaseMonster )

Schedule_t* CGonome::GetScheduleOfType(int Type)
{
	switch ( Type )
	{
	case SCHED_RANGE_ATTACK1:
		return &slGonomeRangeAttack1[0];
		break;
	case SCHED_CHASE_ENEMY:
		return &slGonomeChaseEnemy[0];
		break;
	case SCHED_VICTORY_DANCE:
		return &slGonomeVictoryDance[0];
		break;
	default:
		break;
	}
	return CBaseMonster::GetScheduleOfType(Type);
}

void CGonome::RunTask(Task_t *pTask)
{
	// HACK to stop Gonome from playing attack sound twice
	if (pTask->iTask == TASK_MELEE_ATTACK1)
	{
		if (!m_playedAttackSound)
		{
			if (m_meleeAttack2)
			{
				EmitSoundScript(melee2SoundScript);
			}
			else
			{
				EmitSoundScript(melee1SoundScript);
			}
			m_playedAttackSound = true;
		}
	}
	else
	{
		m_playedAttackSound = false;
	}
	CBaseMonster::RunTask(pTask);
}

//=========================================================
// DEAD GONOME PROP
//=========================================================
class CDeadGonome : public CDeadMonster
{
public:
	void Spawn();
	const char* DefaultModel() { return "models/gonome.mdl"; }
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("gonome"); }
	int	DefaultClassify() { return	CLASS_ALIEN_MONSTER; }
	const char* getPos(int pos) const;
	static const char *m_szPoses[3];
};

const char *CDeadGonome::m_szPoses[] = { "dead_on_stomach1", "dead_on_back", "dead_on_side" };

const char* CDeadGonome::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS(monster_gonome_dead, CDeadGonome)

//=========================================================
// ********** DeadGonome SPAWN **********
//=========================================================
void CDeadGonome::Spawn(void)
{
	SpawnHelper(BLOOD_COLOR_YELLOW);
	MonsterInitDead();
}
#endif
