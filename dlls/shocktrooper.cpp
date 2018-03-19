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

#include "shockbeam.h"
#include "spore.h"

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
	void MonsterThink();
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
	void GibMonster(void);

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	int	Save(CSave &save);
	int Restore(CRestore &restore);

	void SpeakSentence();

	static TYPEDESCRIPTION m_SaveData[];

	virtual int SizeForGrapple() { return GRAPPLE_LARGE; }

	BOOL m_bRightClaw;
	float m_rechargeTime;
	float m_blinkTime;
	float m_eyeChangeTime;

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
	DEFINE_FIELD(CStrooper, m_blinkTime, FIELD_TIME),
	DEFINE_FIELD(CStrooper, m_eyeChangeTime, FIELD_TIME),
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

typedef enum
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
			CBaseEntity* pRoach = CBaseEntity::Create( "monster_shockroach", vecGunPos, vecDropAngles, edict() );
			if (pRoach)
			{
				// Remove any pitch.
				pRoach->pev->angles.x = 0;
			}
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
		CSporeGrenade::ShootTimed(pev, GetGunPosition(), m_vecTossVelocity, true);

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
		if (m_hEnemy)
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
				WRITE_BYTE( 128 );			// brightness
			MESSAGE_END();

			Vector vecShootOrigin = GetGunPosition();
			Vector vecShootDir = ShootAtEnemy( vecShootOrigin );
			vecGunAngles = UTIL_VecToAngles(vecShootDir);

			CShock::Shoot(pev, vecGunAngles, vecShootOrigin, vecShootDir * 2000);
			m_cAmmoLoaded--;
			SetBlending( 0, vecGunAngles.x );

			// Play fire sound.
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/shock_fire.wav", 1, ATTN_NORM);
			CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
		}
		else
		{
			ALERT(at_aiconsole, "Shooting with no enemy! Schedule: %s\n", m_pSchedule->pName);
		}
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

	SpawnHelper("models/strooper.mdl", gSkillData.strooperHealth * 2.5, BLOOD_COLOR_GREEN);
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
	m_blinkTime = gpGlobals->time + RANDOM_FLOAT(3.0f, 7.0f);

	MonsterInit();
}

void CStrooper::MonsterThink()
{
	if (m_cAmmoLoaded < m_cClipSize)
	{
		if (m_rechargeTime < gpGlobals->time)
		{
			m_cAmmoLoaded++;
			m_rechargeTime = gpGlobals->time + gSkillData.strooperRchgSpeed;
		}
	}
	if (m_blinkTime <= gpGlobals->time && pev->skin == 0) {
		pev->skin = 1;
		m_blinkTime = gpGlobals->time + RANDOM_FLOAT(3.0f, 7.0f);
		m_eyeChangeTime = gpGlobals->time + 0.1;
	}
	if (pev->skin != 0) {
		if (m_eyeChangeTime <= gpGlobals->time) {
			m_eyeChangeTime = gpGlobals->time + 0.1;
			pev->skin++;
			if (pev->skin > 3) {
				pev->skin = 0;
			}
		}
	}
	CHGrunt::MonsterThink();
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
