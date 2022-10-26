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
// monster template
//=========================================================
// UNDONE: Holster weapon?

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"scripted.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"mod_features.h"
#include	"gamerules.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define		BARNEY_AE_DRAW		( 2 )
#define		BARNEY_AE_SHOOT		( 3 )
#define		BARNEY_AE_HOLSTER	( 4 )

#define	BARNEY_BODY_GUNHOLSTERED	0
#define	BARNEY_BODY_GUNDRAWN		1
#define BARNEY_BODY_GUNGONE		2

class CBarney : public CTalkMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue(KeyValueData* pkvd);
	void SetYawSpeed( void );
	int DefaultISoundMask( void );
	void BarneyFirePistol( const char* shotSound, Bullet bullet );
	void AlertSound( void );
	const char* DefaultDisplayName() { return "Barney"; }
	int DefaultClassify( void );
	const char* ReverseRelationshipModel() { return "models/barnabus.mdl"; }
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	int DefaultToleranceLevel() { return TOLERANCE_LOW; }
	BOOL CheckRangeAttack1( float flDot, float flDist );

	// Override these to set behavior
	Schedule_t *GetScheduleOfType( int Type );
	Schedule_t *GetSchedule( void );

	void DeathSound( void );
	void PainSound( void );

	void TalkInit( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void OnDying();

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	BOOL m_fGunDrawn;
	float m_painTime;
	float m_checkAttackTime;
	BOOL m_lastAttackCheck;

	int bodystate;
	CUSTOM_SCHEDULES

protected:
	void SpawnImpl(const char* modelName, float health);
	void PrecacheImpl( const char* modelName );
	void TraceAttackImpl( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType, bool hasHelmet);
	virtual bool PrioritizeMeleeAttack() { return false; }
};

LINK_ENTITY_TO_CLASS( monster_barney, CBarney )

TYPEDESCRIPTION	CBarney::m_SaveData[] =
{
	DEFINE_FIELD( CBarney, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBarney, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CBarney, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CBarney, m_lastAttackCheck, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CBarney, CTalkMonster )

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// BarneyDraw - much better looking draw schedule for when
// barney knows who he's gonna attack.
//=========================================================
Task_t tlBarneyEnemyDraw[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float) ACT_ARM },
};

Schedule_t slBarneyEnemyDraw[] =
{
	{
		tlBarneyEnemyDraw,
		ARRAYSIZE( tlBarneyEnemyDraw ),
		0,
		0,
		"Barney Enemy Draw"
	}
};

Task_t tlBaRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_CHECK_FIRE, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
};

Schedule_t slBaRangeAttack1[] =
{
	{
		tlBaRangeAttack1,
		ARRAYSIZE( tlBaRangeAttack1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED |
		bits_COND_NOFIRE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"Range Attack1"
	},
};

DEFINE_CUSTOM_SCHEDULES( CBarney )
{
	slBarneyEnemyDraw,
	slBaRangeAttack1,
};

IMPLEMENT_CUSTOM_SCHEDULES( CBarney, CTalkMonster )

void CBarney::StartTask( Task_t *pTask )
{
	CTalkMonster::StartTask( pTask );
}

void CBarney::RunTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if( m_hEnemy != 0 && ( m_hEnemy->IsPlayer() ) )
		{
			pev->framerate = 1.5;
		}
		CTalkMonster::RunTask( pTask );
		break;
	default:
		CTalkMonster::RunTask( pTask );
		break;
	}
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CBarney::DefaultISoundMask( void) 
{
	return bits_SOUND_WORLD |
			bits_SOUND_COMBAT |
			bits_SOUND_CARCASS |
			bits_SOUND_MEAT |
			bits_SOUND_GARBAGE |
			bits_SOUND_DANGER |
			bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CBarney::DefaultClassify( void )
{
	return CLASS_PLAYER_ALLY;
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CBarney::AlertSound( void )
{
	if( m_hEnemy != 0 )
	{
		if( FOkToSpeak(SPEAK_DISREGARD_ENEMY) && !m_hEnemy->IsPlayer() )
		{
			PlaySentence( m_szGrp[TLK_ATTACK], RANDOM_FLOAT( 2.8f, 3.2f ), VOL_NORM, ATTN_IDLE );
		}
	}
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBarney::SetYawSpeed( void )
{
	int ys;

	ys = 0;

	switch ( m_Activity )
	{
	case ACT_IDLE:		
		ys = 70;
		break;
	case ACT_WALK:
		ys = 70;
		break;
	case ACT_RUN:
		ys = 90;
		break;
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CBarney::CheckRangeAttack1( float flDot, float flDist )
{
	if( flDist <= 1024.0f && flDot >= 0.5f )
	{
		if( gpGlobals->time > m_checkAttackTime )
		{
			TraceResult tr;

			Vector shootOrigin = pev->origin + Vector( 0.0f, 0.0f, 55.0f );
			CBaseEntity *pEnemy = m_hEnemy;
			Vector shootTarget = ( ( pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin ) + m_vecEnemyLKP );
			UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ENT( pev ), &tr );
			m_checkAttackTime = gpGlobals->time + 1.0f;
			if( tr.flFraction == 1.0f || ( tr.pHit != NULL && CBaseEntity::Instance( tr.pHit ) == pEnemy ) )
				m_lastAttackCheck = TRUE;
			else
				m_lastAttackCheck = FALSE;
		}
		return m_lastAttackCheck;
	}
	return FALSE;
}

//=========================================================
// BarneyFirePistol - shoots one round from the pistol at
// the enemy barney is facing.
//=========================================================
void CBarney::BarneyFirePistol( const char* shotSound, Bullet bullet )
{
	Vector vecShootOrigin;

	UTIL_MakeVectors( pev->angles );
	vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects |= EF_MUZZLEFLASH;

	FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, bullet );

	int pitchShift = RANDOM_LONG( 0, 20 );
	
	// Only shift about half the time
	if( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, shotSound, 1.0f, ATTN_NORM, 0, 100 + pitchShift );

	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 384, 0.3f );

	// UNDONE: Reload?
	m_cAmmoLoaded--;// take away a bullet!
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CBarney::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BARNEY_AE_SHOOT:
		if (pev->frags)
		{
			if (RANDOM_LONG(0, 1))
				BarneyFirePistol("weapons/357_shot1.wav", BULLET_MONSTER_357);
			else
				BarneyFirePistol("weapons/357_shot2.wav", BULLET_MONSTER_357);
		}
		else
		{
			BarneyFirePistol("barney/ba_attack2.wav", BULLET_MONSTER_9MM);
		}

		break;
	case BARNEY_AE_DRAW:
		// barney's bodygroup switches here so he can pull gun from holster
		pev->body = BARNEY_BODY_GUNDRAWN;
		m_fGunDrawn = TRUE;
		break;
	case BARNEY_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		pev->body = BARNEY_BODY_GUNHOLSTERED;
		m_fGunDrawn = FALSE;
		break;
	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CBarney::SpawnImpl(const char* modelName, float health)
{
	SetMyModel( modelName );
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_RED );
	SetMyHealth( health );
	pev->view_ofs = Vector ( 0.0f, 0.0f, 50.0f );// position of the eyes relative to monster's origin.
	SetMyFieldOfView(VIEW_FIELD_WIDE); // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;
	m_HackedGunPos = Vector ( 0, 0, 55 );

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	TalkMonsterInit();
}

void CBarney::Spawn()
{
	Precache();
	SpawnImpl("models/barney.mdl", gSkillData.barneyHealth);
	if (bodystate == -1) {
		bodystate = RANDOM_LONG(0,1);
	}
	SetBodygroup(1, bodystate);
	m_fGunDrawn = FALSE;
	if (bodystate == BARNEY_BODY_GUNDRAWN) {
		m_fGunDrawn = TRUE;
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBarney::PrecacheImpl(const char* modelName)
{
	PrecacheMyModel( modelName );
	
	PRECACHE_SOUND("barney/ba_pain1.wav");
	PRECACHE_SOUND("barney/ba_pain2.wav");
	PRECACHE_SOUND("barney/ba_pain3.wav");
	
	PRECACHE_SOUND("barney/ba_die1.wav");
	PRECACHE_SOUND("barney/ba_die2.wav");
	PRECACHE_SOUND("barney/ba_die3.wav");
}

void CBarney::Precache()
{
	PrecacheImpl( "models/barney.mdl" );

	PRECACHE_SOUND( "barney/ba_attack1.wav" );
	PRECACHE_SOUND( "barney/ba_attack2.wav" );

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
	RegisterTalkMonster();
}	

// Init talk data
void CBarney::TalkInit()
{
	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)
	m_szGrp[TLK_ANSWER] = "BA_ANSWER";
	m_szGrp[TLK_QUESTION] =	"BA_QUESTION";
	m_szGrp[TLK_IDLE] = "BA_IDLE";
	m_szGrp[TLK_STARE] = "BA_STARE";
	m_szGrp[TLK_USE] = "BA_OK";
	m_szGrp[TLK_UNUSE] = "BA_WAIT";
	m_szGrp[TLK_DECLINE] = "BA_POK";
	m_szGrp[TLK_STOP] = "BA_STOP";

	m_szGrp[TLK_NOSHOOT] = "BA_SCARED";
	m_szGrp[TLK_HELLO] = "BA_HELLO";

	m_szGrp[TLK_PLHURT1] = "!BA_CUREA";
	m_szGrp[TLK_PLHURT2] = "!BA_CUREB"; 
	m_szGrp[TLK_PLHURT3] = "!BA_CUREC";

	m_szGrp[TLK_PHELLO] = "BA_PHELLO";
	m_szGrp[TLK_PIDLE] = "BA_PIDLE";
	m_szGrp[TLK_PQUESTION] = "BA_PQUEST";

	m_szGrp[TLK_SMELL] = "BA_SMELL";
	
	m_szGrp[TLK_WOUND] = "BA_WOUND";
	m_szGrp[TLK_MORTAL] = "BA_MORTAL";

	m_szGrp[TLK_SHOT] = "BA_SHOT";
	m_szGrp[TLK_MAD] = "BA_MAD";

	m_szGrp[TLK_KILL] = "BA_KILL";
	m_szGrp[TLK_ATTACK] = "BA_ATTACK";

	// get voice for head - just one barney voice for now
	m_voicePitch = 100;
}

void CBarney::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "bodystate"))
	{
		bodystate = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CTalkMonster::KeyValue( pkvd );
}

//=========================================================
// PainSound
//=========================================================
void CBarney::PainSound( void )
{
	if( gpGlobals->time < m_painTime )
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT( 0.5f, 0.75f );

	switch( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_pain1.wav", 1.0f, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_pain2.wav", 1.0f, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 2:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_pain3.wav", 1.0f, ATTN_NORM, 0, GetVoicePitch() );
		break;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CBarney::DeathSound( void )
{
	switch( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_die1.wav", 1.0f, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_die2.wav", 1.0f, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 2:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	}
}

void CBarney::TraceAttackImpl( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType, bool hasHelmet)
{
	switch( ptr->iHitgroup )
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (bitsDamageType & ( DMG_BULLET | DMG_SLASH | DMG_BLAST ) )
		{
			flDamage = flDamage * 0.5f;
		}
		break;
	case 10:
		if( (bitsDamageType & ( DMG_BULLET | DMG_SLASH | DMG_CLUB )) && hasHelmet )
		{
			flDamage -= 20.0f;
			if( flDamage <= 0.0f )
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0f );
				flDamage = 0.01f;
			}
		}

		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}

	CTalkMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

void CBarney::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	TraceAttackImpl( pevAttacker, flDamage, vecDir, ptr, bitsDamageType, true);
}

void CBarney::OnDying()
{
	if( g_pGameRules->FMonsterCanDropWeapons(this) && !FBitSet(pev->spawnflags, SF_MONSTER_DONT_DROP_GUN) && pev->body < BARNEY_BODY_GUNGONE )
	{
		// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		pev->body = BARNEY_BODY_GUNGONE;

		GetAttachment( 0, vecGunPos, vecGunAngles );

		if (pev->frags)
			DropItem( "weapon_357", vecGunPos, vecGunAngles );
		else
			DropItem( "weapon_9mmhandgun", vecGunPos, vecGunAngles );
	}
	CTalkMonster::OnDying();
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Schedule_t *CBarney::GetScheduleOfType( int Type )
{
	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if( m_hEnemy != 0 )
		{
			// face enemy, then draw.
			return slBarneyEnemyDraw;
		}
		break;
	case SCHED_RANGE_ATTACK1:
		return slBaRangeAttack1;
	}

	return CTalkMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CBarney::GetSchedule()
{
	if( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}
	if( HasConditions( bits_COND_ENEMY_DEAD ) && FOkToSpeak(SPEAK_DISREGARD_ENEMY) )
	{
		PlaySentence( m_szGrp[TLK_KILL], 4, VOL_NORM, ATTN_NORM );
	}

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

			// always act surprized with a new enemy
			if( HasConditions( bits_COND_NEW_ENEMY ) && HasConditions( bits_COND_LIGHT_DAMAGE ) )
				return GetScheduleOfType( SCHED_SMALL_FLINCH );

			// wait for one schedule to draw gun
			if( !m_fGunDrawn )
				return GetScheduleOfType( SCHED_ARM_WEAPON );

			if( HasConditions( bits_COND_HEAVY_DAMAGE ) )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );

			if( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) && PrioritizeMeleeAttack() )
				return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
		}
		break;
	case MONSTERSTATE_ALERT:	
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_HUNT:
	{
		if( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) && CanIdleFlinch() )
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		Schedule_t* followingSchedule = GetFollowingSchedule();
		if (followingSchedule)
			return followingSchedule;

		// try to say something about smells
		TrySmellTalk();
	}
		break;
	default:
		break;
	}

	return CTalkMonster::GetSchedule();
}

//=========================================================
// DEAD BARNEY PROP
//
// Designer selects a pose in worldcraft, 0 through num_poses-1
// this value is added to what is selected as the 'first dead pose'
// among the monster's normal animations. All dead poses must
// appear sequentially in the model file. Be sure and set
// the m_iFirstPose properly!
//
//=========================================================
class CDeadBarney : public CDeadMonster
{
public:
	void Spawn( void );
	int	DefaultClassify ( void ) { return	CLASS_PLAYER_ALLY; }

	const char* getPos(int pos) const;
	static const char *m_szPoses[3];
};

const char *CDeadBarney::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach" };

const char* CDeadBarney::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_barney_dead, CDeadBarney )

//=========================================================
// ********** DeadBarney SPAWN **********
//=========================================================
void CDeadBarney :: Spawn( )
{
	SpawnHelper("models/barney.mdl");
	MonsterInitDead();
}

#if FEATURE_OTIS
#define	OTIS_BODY_GUNHOLSTERED	0
#define	OTIS_BODY_GUNDRAWN		1
#define OTIS_BODY_DONUT			2

class COtis : public CBarney
{
public:
	void Spawn( void );
	void Precache( void );
	void TalkInit( void );
	const char* DefaultDisplayName() { return "Otis"; }
	const char* ReverseRelationshipModel() { return "models/otisf.mdl"; }

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void OnDying();
	
	void KeyValue( KeyValueData *pkvd );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void SetHead(int head);
	
	int m_iHead;
};

LINK_ENTITY_TO_CLASS( monster_otis, COtis )

void COtis::Spawn()
{
	Precache();
	SpawnImpl("models/otis.mdl", gSkillData.otisHealth);
	if ( m_iHead == -1 )
		SetBodygroup(2, RANDOM_LONG(0, 1));
	else
		SetBodygroup(2, m_iHead);
	if (bodystate == -1) {
		bodystate = RANDOM_LONG(0,1);
	}
	SetBodygroup(1, bodystate);
	m_fGunDrawn = FALSE;
	if (bodystate == OTIS_BODY_GUNDRAWN) {
		m_fGunDrawn = TRUE;
	}
}

void COtis::Precache()
{
	PrecacheImpl("models/otis.mdl");
	PRECACHE_SOUND("weapons/desert_eagle_fire.wav");
	TalkInit();
	CTalkMonster::Precache();
	RegisterTalkMonster();
}

void COtis::TalkInit()
{
	CTalkMonster::TalkInit();
	
	m_szGrp[TLK_ANSWER]  =	"OT_ANSWER";
	m_szGrp[TLK_QUESTION] =	"OT_QUESTION";
	m_szGrp[TLK_IDLE] =		"OT_IDLE";
	m_szGrp[TLK_STARE] =	"OT_STARE";
	m_szGrp[TLK_USE] =		"OT_OK";
	m_szGrp[TLK_UNUSE] =	"OT_WAIT";
	m_szGrp[TLK_DECLINE] =	"OT_POK";
	m_szGrp[TLK_STOP] =		"OT_STOP";
	
	m_szGrp[TLK_NOSHOOT] =	"OT_SCARED";
	m_szGrp[TLK_HELLO] =	"OT_HELLO";
	
	m_szGrp[TLK_PLHURT1] =	"!OT_CUREA";
	m_szGrp[TLK_PLHURT2] =	"!OT_CUREB"; 
	m_szGrp[TLK_PLHURT3] =	"!OT_CUREC";
	
	m_szGrp[TLK_PHELLO] =	"OT_PHELLO";
	m_szGrp[TLK_PIDLE] =	"OT_PIDLE";
	m_szGrp[TLK_PQUESTION] = "OT_PQUEST";
	
	m_szGrp[TLK_SMELL] =	"OT_SMELL";
	
	m_szGrp[TLK_WOUND] =	"OT_WOUND";
	m_szGrp[TLK_MORTAL] =	"OT_MORTAL";

	m_szGrp[TLK_SHOT] = "OT_SHOT";
	m_szGrp[TLK_MAD] = "OT_MAD";
	
	m_szGrp[TLK_KILL] = "OT_KILL";
	m_szGrp[TLK_ATTACK] = "OT_ATTACK";

	m_voicePitch = 100;
}

void COtis::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	TraceAttackImpl( pevAttacker, flDamage, vecDir, ptr, bitsDamageType, false);
}

void COtis::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "head"))
	{
		m_iHead = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBarney::KeyValue( pkvd );
}

void COtis::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case BARNEY_AE_SHOOT:
			BarneyFirePistol("weapons/desert_eagle_fire.wav", BULLET_MONSTER_357);
			break;
			
		case BARNEY_AE_DRAW:
			// barney's bodygroup switches here so he can pull gun from holster
			SetBodygroup(1, OTIS_BODY_GUNDRAWN);
			m_fGunDrawn = TRUE;
			break;
			
		case BARNEY_AE_HOLSTER:
			// change bodygroup to replace gun in holster
			SetBodygroup(1, OTIS_BODY_GUNHOLSTERED);
			m_fGunDrawn = FALSE;
			break;
			
		default:
			CTalkMonster::HandleAnimEvent( pEvent );
	}
}

void COtis::OnDying()
{
	if ( g_pGameRules->FMonsterCanDropWeapons(this) && !FBitSet(pev->spawnflags, SF_MONSTER_DONT_DROP_GUN) && GetBodygroup(1) != OTIS_BODY_GUNHOLSTERED )
	{
		Vector vecGunPos;
		Vector vecGunAngles;

		SetBodygroup(1, OTIS_BODY_GUNHOLSTERED);

		GetAttachment( 0, vecGunPos, vecGunAngles );

		DropItem( DESERT_EAGLE_DROP_NAME, vecGunPos, vecGunAngles );
	}
	CTalkMonster::OnDying();
}

void COtis::SetHead(int head)
{
	m_iHead = head;
}

class CDeadOtis : public CDeadBarney
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	const char* getPos(int pos) const;
	static const char *m_szPoses[5];

	int head;
};

const char *CDeadOtis::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach", "stuffed_in_vent", "dead_sitting" };

const char* CDeadOtis::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_otis_dead, CDeadOtis )

void CDeadOtis :: Spawn( )
{
	SpawnHelper("models/otis.mdl");
	if ( head == -1 )
		SetBodygroup(2, RANDOM_LONG(0, 1));
	else
		SetBodygroup(2, head);
	MonsterInitDead();
}

void CDeadOtis::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "head"))
	{
		head = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CDeadBarney::KeyValue( pkvd );
}
#endif

#if FEATURE_BARNIEL
class CBarniel : public CBarney
{
public:
	void Spawn( void );
	void Precache( void );
	void TalkInit( void );
	const char* DefaultDisplayName() { return "Barniel"; }
	const char* ReverseRelationshipModel() { return NULL; }
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void DeathSound( void );
	void PainSound( void );

	void OnDying();
};

LINK_ENTITY_TO_CLASS( monster_barniel, CBarniel )

void CBarniel::Spawn()
{
	Precache();
	SpawnImpl("models/barniel.mdl", gSkillData.barneyHealth);

	if (bodystate == -1) {
		bodystate = RANDOM_LONG(0,1);
	}
	SetBodygroup(1, bodystate);
	m_fGunDrawn = FALSE;
	if (bodystate == BARNEY_BODY_GUNDRAWN) {
		m_fGunDrawn = TRUE;
	}
}

void CBarniel::Precache()
{
	PrecacheImpl("models/barniel.mdl");
	PRECACHE_SOUND( "barniel/bn_attack1.wav" );
	PRECACHE_SOUND( "barniel/bn_attack2.wav" );

	PRECACHE_SOUND( "barniel/bn_pain1.wav" );
	PRECACHE_SOUND( "barniel/bn_die1.wav" );

	TalkInit();
	CTalkMonster::Precache();
	RegisterTalkMonster();
}

void CBarniel::TalkInit()
{
	CTalkMonster::TalkInit();

	m_szGrp[TLK_ANSWER] = "BN_ANSWER";
	m_szGrp[TLK_QUESTION] =	"BN_QUESTION";
	m_szGrp[TLK_IDLE] = "BN_IDLE";
	m_szGrp[TLK_STARE] = "BN_STARE";
	m_szGrp[TLK_USE] = "BN_OK";
	m_szGrp[TLK_UNUSE] = "BN_WAIT";
	m_szGrp[TLK_DECLINE] = "BN_POK";
	m_szGrp[TLK_STOP] = "BN_STOP";

	m_szGrp[TLK_NOSHOOT] = "BN_SCARED";
	m_szGrp[TLK_HELLO] = "BN_HELLO";

	m_szGrp[TLK_PLHURT1] = "!BN_CUREA";
	m_szGrp[TLK_PLHURT2] = "!BN_CUREB";
	m_szGrp[TLK_PLHURT3] = "!BN_CUREC";

	m_szGrp[TLK_PHELLO] = "BN_PHELLO";
	m_szGrp[TLK_PIDLE] = "BN_PIDLE";
	m_szGrp[TLK_PQUESTION] = "BN_PQUEST";

	m_szGrp[TLK_SMELL] = "BN_SMELL";

	m_szGrp[TLK_WOUND] = "BN_WOUND";
	m_szGrp[TLK_MORTAL] = "BN_MORTAL";

	m_szGrp[TLK_SHOT] = "BN_SHOT";
	m_szGrp[TLK_MAD] = "BN_MAD";

	m_szGrp[TLK_KILL] = "BN_KILL";
	m_szGrp[TLK_ATTACK] = "BN_ATTACK";

	m_voicePitch = 100;
}

void CBarniel::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BARNEY_AE_SHOOT:
		BarneyFirePistol("barniel/bn_attack2.wav", BULLET_MONSTER_9MM);
		break;
	default:
		CBarney::HandleAnimEvent( pEvent );
	}
}

void CBarniel::DeathSound( void )
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barniel/bn_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
}

void CBarniel::PainSound()
{
	if( gpGlobals->time < m_painTime )
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT( 0.5f, 0.75f );
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barniel/bn_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
}

void CBarniel::OnDying()
{
	if( g_pGameRules->FMonsterCanDropWeapons(this) && !FBitSet(pev->spawnflags, SF_MONSTER_DONT_DROP_GUN) && pev->body < BARNEY_BODY_GUNGONE )
	{
		Vector vecGunPos;
		Vector vecGunAngles;

		pev->body = BARNEY_BODY_GUNGONE;
		GetAttachment( 0, vecGunPos, vecGunAngles );

		DropItem( "weapon_9mmhandgun", vecGunPos, vecGunAngles );
	}
	CTalkMonster::OnDying();
}

class CDeadBarniel : public CDeadBarney
{
public:
	void Spawn( void );
	const char* getPos(int pos) const;
	static const char *m_szPoses[3];
};

const char *CDeadBarniel::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach" };

const char* CDeadBarniel::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_barniel_dead, CDeadBarniel )

void CDeadBarniel::Spawn( )
{
	SpawnHelper("models/barniel.mdl");
	MonsterInitDead();
}
#endif

#if FEATURE_KATE
#define		KATE_AE_KICK		( 6 )
#define KATE_LIMP_HEALTH 40

class CKate : public CBarney
{
public:
	void Spawn( void );
	void Precache( void );
	void TalkInit( void );
	const char* DefaultDisplayName() { return "Kate"; }
	const char* ReverseRelationshipModel() { return NULL; }
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int LookupActivity(int activity);
	int DefaultToleranceLevel() { return TOLERANCE_AVERAGE; }
	BOOL CheckMeleeAttack1( float flDot, float flDist );
	void DeathSound( void );
	void PainSound( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void OnDying();

protected:
	CBaseEntity* FindKickTarget();
	bool PrioritizeMeleeAttack() { return true; }
	float LimpHealth();

	int m_iCombatState;
};

LINK_ENTITY_TO_CLASS( monster_kate, CKate )

void CKate::Spawn()
{
	Precache();
	SpawnImpl("models/kate.mdl", gSkillData.kateHealth);
	m_iCombatState = -1;

	if (bodystate == -1) {
		bodystate = RANDOM_LONG(0,1);
	}
	SetBodygroup(1, bodystate);
	m_fGunDrawn = FALSE;
	if (bodystate == BARNEY_BODY_GUNDRAWN) {
		m_fGunDrawn = TRUE;
	}
}

void CKate::Precache()
{
	PrecacheImpl("models/kate.mdl");
	PRECACHE_SOUND( "kate/ka_attack1.wav" );
	PRECACHE_SOUND( "kate/ka_attack2.wav" );

	PRECACHE_SOUND( "kate/ka_pain1.wav" );
	PRECACHE_SOUND( "kate/ka_pain2.wav" );
	PRECACHE_SOUND( "kate/ka_die1.wav" );

	PRECACHE_SOUND( "zombie/claw_miss1.wav" );
	PRECACHE_SOUND( "zombie/claw_miss2.wav" );
	PRECACHE_SOUND( "common/kick.wav" );
	PRECACHE_SOUND( "common/punch.wav" );

	TalkInit();
	CTalkMonster::Precache();
}

void CKate::TalkInit()
{
	CTalkMonster::TalkInit();

	m_szGrp[TLK_ANSWER] = "KA_ANSWER";
	m_szGrp[TLK_QUESTION] =	"KA_QUESTION";
	m_szGrp[TLK_IDLE] = "KA_IDLE";
	m_szGrp[TLK_STARE] = "KA_STARE";
	m_szGrp[TLK_USE] = "KA_OK";
	m_szGrp[TLK_UNUSE] = "KA_WAIT";
	m_szGrp[TLK_DECLINE] = "KA_POK";
	m_szGrp[TLK_STOP] = "KA_STOP";

	m_szGrp[TLK_NOSHOOT] = "KA_SCARED";
	m_szGrp[TLK_HELLO] = "KA_HELLO";

	m_szGrp[TLK_PLHURT1] = "!KA_CUREA";
	m_szGrp[TLK_PLHURT2] = "!KA_CUREB";
	m_szGrp[TLK_PLHURT3] = "!KA_CUREC";

	m_szGrp[TLK_PHELLO] = "KA_PHELLO";
	m_szGrp[TLK_PIDLE] = "KA_PIDLE";
	m_szGrp[TLK_PQUESTION] = "KA_PQUEST";

	m_szGrp[TLK_SMELL] = "KA_SMELL";

	m_szGrp[TLK_WOUND] = "KA_WOUND";
	m_szGrp[TLK_MORTAL] = "KA_MORTAL";

	m_szGrp[TLK_SHOT] = "KA_SHOT";
	m_szGrp[TLK_MAD] = "KA_MAD";

	m_szGrp[TLK_KILL] = "KA_KILL";
	m_szGrp[TLK_ATTACK] = "KA_ATTACK";

	m_voicePitch = 100;
}

void CKate::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BARNEY_AE_SHOOT:
		BarneyFirePistol("kate/ka_attack2.wav", BULLET_MONSTER_9MM);
		break;
	case KATE_AE_KICK:
	{
		CBaseEntity *pHurt = FindKickTarget();
		if( pHurt )
		{
			const char *pszSound;
			if( m_iCombatState == -1 )
			{
				pszSound = "common/kick.wav";
			}
			else
			{
				++m_iCombatState;
				if( m_iCombatState == 3 )
					pszSound = "common/kick.wav";
				else
					pszSound = "common/punch.wav";
			}
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pszSound, 1, ATTN_NORM, 0, PITCH_NORM );
			UTIL_MakeVectors( pev->angles );

			pHurt->pev->punchangle.x = 5;
			pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
			pHurt->TakeDamage( pev, pev, gSkillData.hgruntDmgKick, DMG_CLUB );
		}
	}
		break;
	default:
		CBarney::HandleAnimEvent( pEvent );
	}
}

int CKate::LookupActivity(int activity)
{
	switch (activity)
	{
	case ACT_RUN:
		if( pev->health <= LimpHealth() )
		{
			// limp!
			return CBarney::LookupActivity( ACT_RUN_HURT );
		}
		else
		{
			return CBarney::LookupActivity( activity );
		}
	case ACT_WALK:
		if( pev->health <= LimpHealth() )
		{
			// limp!
			return CBarney::LookupActivity( ACT_WALK_HURT );
		}
		else
		{
			return CBarney::LookupActivity( activity );
		}
	case ACT_MELEE_ATTACK1:
		if( RANDOM_LONG( 0, 2 ) )
		{
			m_iCombatState = -1;
			return LookupSequence( "karate_hit" );
		}
		else
		{
			m_iCombatState = 0;
			return LookupSequence( "karate_bighit" );
		}
		break;
	default:
		return CBarney::LookupActivity( activity );
	}
}

BOOL CKate::CheckMeleeAttack1(float flDot, float flDist)
{
	CBaseMonster *pEnemy = 0;
	if( m_hEnemy != 0 )
		pEnemy = m_hEnemy->MyMonsterPointer();

	if( !pEnemy )
		return FALSE;

	const int enemyClassify = pEnemy->DefaultClassify();
	if( flDist <= 64.0f && flDot >= 0.7f &&
		 enemyClassify != CLASS_ALIEN_BIOWEAPON &&
		 enemyClassify != CLASS_PLAYER_BIOWEAPON )
	{
		return TRUE;
	}
	return FALSE;
}

void CKate::DeathSound( void )
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "kate/ka_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
}

void CKate::PainSound()
{
	if( gpGlobals->time < m_painTime )
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT( 0.5f, 0.75f );

	switch( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "kate/ka_pain1.wav", 1.0f, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "kate/ka_pain2.wav", 1.0f, ATTN_NORM, 0, GetVoicePitch() );
		break;
	}
}

void CKate::OnDying()
{
	if( g_pGameRules->FMonsterCanDropWeapons(this) && !FBitSet(pev->spawnflags, SF_MONSTER_DONT_DROP_GUN) && pev->body < BARNEY_BODY_GUNGONE )
	{
		Vector vecGunPos;
		Vector vecGunAngles;

		pev->body = BARNEY_BODY_GUNGONE;
		GetAttachment( 0, vecGunPos, vecGunAngles );

		DropItem( "weapon_9mmhandgun", vecGunPos, vecGunAngles );
	}
	CTalkMonster::OnDying();
}

void CKate::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	TraceAttackImpl( pevAttacker, flDamage, vecDir, ptr, bitsDamageType, false);
}

CBaseEntity* CKate::FindKickTarget()
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5f;
	Vector vecEnd = vecStart + ( gpGlobals->v_forward * 70 );

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT( pev ), &tr );

	if( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		if (pEntity && IRelationship(pEntity) != R_AL)
			return pEntity;
	}
	return NULL;
}

float CKate::LimpHealth()
{
	return Q_min(pev->max_health/2.0f, KATE_LIMP_HEALTH);
}

class CDeadKate : public CDeadBarney
{
public:
	void Spawn( void );
	const char* getPos(int pos) const;
	static const char *m_szPoses[3];
};

const char *CDeadKate::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach" };

const char* CDeadKate::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_kate_dead, CDeadKate )

void CDeadKate::Spawn( )
{
	SpawnHelper("models/kate.mdl");
	MonsterInitDead();
}
#endif
