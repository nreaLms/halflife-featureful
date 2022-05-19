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

#if FEATURE_SHOCKTROOPER

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

#define STRIIPER_GIB_COUNT 6

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
// shocktrooper
//=========================================================
class CShockTrooper : public CHGrunt
{
public:
	void Spawn(void);
	void MonsterThink();
	void Precache(void);
	int  DefaultClassify(void);
	const char* ReverseRelationshipModel() { return NULL; }
	const char* DefaultDisplayName() { return "Shock Trooper"; }
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist);
	void HandleAnimEvent(MonsterEvent_t *pEvent);

	void DeathSound(void);
	void PainSound(void);
	void GibMonster(void);
	void PlayUseSentence();
	void PlayUnUseSentence();

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	int	Save(CSave &save);
	int Restore(CRestore &restore);

	const char* DefaultGibModel() {
		return "models/strooper_gibs.mdl";
	}
	int DefaultGibCount() {
		return STRIIPER_GIB_COUNT;
	}
	virtual bool CanDropGrenade() const;
	void DropShockRoach(bool gibbed);

	static TYPEDESCRIPTION m_SaveData[];

	virtual int SizeForGrapple() { return GRAPPLE_LARGE; }
	Vector DefaultMinHullSize() { return Vector( -24.0f, -24.0f, 0.0f ); }
	Vector DefaultMaxHullSize() { return Vector( 24.0f, 24.0f, 72.0f ); }

	BOOL m_bRightClaw;
	float m_rechargeTime;
	float m_blinkTime;
	float m_eyeChangeTime;

	static int iStrooperMuzzleFlash;
protected:
	static const char *pTrooperSentences[HGRUNT_SENT_COUNT];

	int GetRangeAttack1Sequence();
	int GetRangeAttack2Sequence();
	Schedule_t* ScheduleOnRangeAttack1();

	float SentenceVolume();
	float SentenceAttn();
	const char* SentenceByNumber(int sentence);
	virtual int* GruntQuestionVar();
};

LINK_ENTITY_TO_CLASS(monster_shocktrooper, CShockTrooper)

int CShockTrooper::iStrooperMuzzleFlash = 0;

TYPEDESCRIPTION	CShockTrooper::m_SaveData[] =
{
	DEFINE_FIELD(CShockTrooper, m_bRightClaw, FIELD_BOOLEAN),
	DEFINE_FIELD(CShockTrooper, m_rechargeTime, FIELD_TIME),
	DEFINE_FIELD(CShockTrooper, m_blinkTime, FIELD_TIME),
	DEFINE_FIELD(CShockTrooper, m_eyeChangeTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CShockTrooper, CHGrunt)

const char *CShockTrooper::pTrooperSentences[] =
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

const char* CShockTrooper::SentenceByNumber(int sentence)
{
	return pTrooperSentences[sentence];
}

int* CShockTrooper::GruntQuestionVar()
{
	static int g_fStrooperQuestion = 0;
	return &g_fStrooperQuestion;
}

Schedule_t* CShockTrooper::ScheduleOnRangeAttack1()
{
	if( InSquad() )
	{
		// if the enemy has eluded the squad and a squad member has just located the enemy
		// and the enemy does not see the squad member, issue a call to the squad to waste a
		// little time and give the player a chance to turn.
		if( MySquadLeader()->m_fEnemyEluded && !HasConditions( bits_COND_ENEMY_FACING_ME ) )
		{
			MySquadLeader()->m_fEnemyEluded = FALSE;
			return GetScheduleOfType( SCHED_GRUNT_FOUND_ENEMY );
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

int CShockTrooper::GetRangeAttack1Sequence()
{
	if (m_fStanding)
	{
		// get aimable sequence
		return LookupSequence("standing_mp5");
	}
	else
	{
		// get crouching shoot
		return LookupSequence("crouching_mp5");
	}
}

int CShockTrooper::GetRangeAttack2Sequence()
{
	return LookupSequence("throwgrenade");
}

float CShockTrooper::SentenceVolume()
{
	return STROOPER_SENTENCE_VOLUME;
}

float CShockTrooper::SentenceAttn()
{
	return STROOPER_ATTN;
}

#define STROOPER_GIB_COUNT 8
//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CShockTrooper::GibMonster(void)
{
	if (GetBodygroup(STROOPER_GUN_GROUP) != STROOPER_GUN_NONE)
	{
		DropShockRoach(true);
	}

	CBaseMonster::GibMonster();
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CShockTrooper::DefaultClassify(void)
{
	return CLASS_RACEX_SHOCK;
}

BOOL CShockTrooper::CheckRangeAttack1(float flDot, float flDist)
{
	return m_cAmmoLoaded >= 1 && CHGrunt::CheckRangeAttack1(flDot, flDist);
}

BOOL CShockTrooper::CheckRangeAttack2( float flDot, float flDist )
{
	if( !FBitSet( pev->weapons, STROOPER_HANDGRENADE ) )
	{
		return FALSE;
	}
	return CheckRangeAttack2Impl(gSkillData.strooperGrenadeSpeed, flDot, flDist);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CShockTrooper::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case STROOPER_AE_DROP_GUN:
	{
		if (GetBodygroup(STROOPER_GUN_GROUP) != STROOPER_GUN_NONE)
		{
			DropShockRoach(false);
		}
	}
	break;

	case STROOPER_AE_RELOAD:
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	case STROOPER_AE_GREN_TOSS:
	{
		CSpore::CreateSpore(pev->origin + Vector(0, 0, 98), pev->angles, m_vecTossVelocity, this, CSpore::GRENADE, true, false);

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

			UTIL_MakeVectors(pev->angles);
			Vector vecShootOrigin = vecGunPos + gpGlobals->v_forward * 32;
			Vector vecShootDir = ShootAtEnemy( vecShootOrigin );
			vecGunAngles = UTIL_VecToAngles(vecShootDir);

			CShock::Shoot(pev, vecGunAngles, vecShootOrigin, vecShootDir * CShock::ShockSpeed());
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
		KickImpl(gSkillData.strooperDmgKick, (m_bRightClaw) ? -10 : 10);

		m_bRightClaw = !m_bRightClaw;
	}
	break;

	case STROOPER_AE_CAUGHT_ENEMY:
	{
		if (FOkToSpeak())
		{
			SENTENCEG_PlayRndSz(ENT(pev), SentenceByNumber(HGRUNT_SENT_ALERT), STROOPER_SENTENCE_VOLUME, STROOPER_ATTN, 0, m_voicePitch);
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
void CShockTrooper::Spawn()
{
	Precache();

	SpawnHelper("models/strooper.mdl", gSkillData.strooperHealth * 2.5, BLOOD_COLOR_GREEN);
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

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

	FollowingMonsterInit();
}

void CShockTrooper::MonsterThink()
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
void CShockTrooper::Precache()
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
	UTIL_PrecacheOther("spore");
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
void CShockTrooper::PainSound(void)
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
void CShockTrooper::DeathSound(void)
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
// TraceAttack - reimplemented in shock trooper because they never have helmets
//=========================================================
void CShockTrooper::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CFollowingMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

bool CShockTrooper::CanDropGrenade() const
{
	return false;
}

void CShockTrooper::DropShockRoach(bool gibbed)
{
	if (!FBitSet(pev->spawnflags, SF_MONSTER_DONT_DROP_GUN))
	{
		Vector	vecGunPos;
		Vector	vecGunAngles;

		GetAttachment(0, vecGunPos, vecGunAngles);
		SetBodygroup(STROOPER_GUN_GROUP, STROOPER_GUN_NONE);

		Vector vecDropGunAngles = pev->angles;
		vecDropGunAngles.x = vecDropGunAngles.z = 0;

		// now spawn a shockroach.
		CBaseEntity* pRoach = CBaseEntity::Create( "monster_shockroach", vecGunPos, pev->angles );
		if (pRoach)
		{
			if (gibbed)
			{
				pRoach->pev->velocity = Vector(RANDOM_FLOAT(-100.0f, 100.0f), RANDOM_FLOAT(-100.0f, 100.0f), RANDOM_FLOAT(200.0f, 300.0f));
				pRoach->pev->avelocity = Vector(0, RANDOM_FLOAT(200.0f, 300.0f), 0);
			}
			else
			{
				pRoach->pev->velocity = Vector(RANDOM_FLOAT(-20.0f, 20.0f) , RANDOM_FLOAT(-20.0f, 20.0f), RANDOM_FLOAT(20.0f, 30.0f));
				pRoach->pev->avelocity = Vector(0, RANDOM_FLOAT(20.0f, 40.0f), 0);
			}
			if (ShouldFadeOnDeath())
				pRoach->pev->spawnflags |= SF_MONSTER_FADECORPSE;
			CBaseMonster *pNewMonster = pRoach->MyMonsterPointer();
			if (pNewMonster) {
				pNewMonster->m_iClass = m_iClass;
				pNewMonster->m_reverseRelationship = m_reverseRelationship;
			}
		}
	}
}

void CShockTrooper::PlayUseSentence()
{
	SENTENCEG_PlayRndSz( ENT( pev ), "ST_IDLE", SentenceVolume(), SentenceAttn(), 0, m_voicePitch );
	JustSpoke();
}

void CShockTrooper::PlayUnUseSentence()
{
	SENTENCEG_PlayRndSz( ENT( pev ), "ST_ALERT", SentenceVolume(), SentenceAttn(), 0, m_voicePitch );
	JustSpoke();
}

class CDeadStrooper : public CDeadMonster
{
public:
	void Spawn( void );
	void Precache();
	int	DefaultClassify ( void ) { return	CLASS_RACEX_SHOCK; }
	const char* DefaultGibModel() {
		return "models/strooper_gibs.mdl";
	}
	int DefaultGibCount() {
		return STRIIPER_GIB_COUNT;
	}

	const char* getPos(int pos) const;
	static const char *m_szPoses[2];
};

const char *CDeadStrooper::m_szPoses[] = { "diesimple", "diebackwards" };

const char* CDeadStrooper::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_shocktrooper_dead, CDeadStrooper )

void CDeadStrooper::Precache()
{
	CDeadMonster::Precache();
	PRECACHE_MODEL("models/strooper_gibs.mdl");
}

void CDeadStrooper::Spawn( )
{
	Precache();
	SpawnHelper("models/strooper.mdl", BLOOD_COLOR_YELLOW, gSkillData.strooperHealth/2);
	MonsterInitDead();
	pev->frame = 255;
}

#endif
