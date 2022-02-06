#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"scripted.h"
#include	"soundent.h"
#include	"mod_features.h"

#if FEATURE_RECRUIT

class CRecruit : public CTalkMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int DefaultISoundMask(void);
	int DefaultClassify(void);
	void DeathSound( void );
	void PainSound( void );

	Schedule_t *GetSchedule( void );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	void TalkInit();

	float m_painTime;
};

LINK_ENTITY_TO_CLASS( monster_recruit, CRecruit )

TYPEDESCRIPTION	CRecruit::m_SaveData[] =
{
	DEFINE_FIELD( CRecruit, m_painTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CRecruit, CTalkMonster )

void CRecruit::Precache()
{
	PrecacheMyModel("models/recruit.mdl");
	PRECACHE_SOUND("barney/ba_pain1.wav");
	PRECACHE_SOUND("barney/ba_pain2.wav");
	PRECACHE_SOUND("barney/ba_pain3.wav");
	PRECACHE_SOUND("barney/ba_die1.wav");
	PRECACHE_SOUND("barney/ba_die2.wav");
	PRECACHE_SOUND("barney/ba_die3.wav");
	TalkInit();
	CTalkMonster::Precache();
}

void CRecruit::Spawn()
{
	Precache();

	SetMyModel( "models/recruit.mdl" );
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_RED );
	SetMyHealth( gSkillData.barneyHealth );
	pev->view_ofs = Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	SetMyFieldOfView(VIEW_FIELD_WIDE); // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	TalkMonsterInit();
}

void CRecruit::SetYawSpeed( void )
{
	int ys = 0;
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

int CRecruit::DefaultISoundMask( void)
{
	return bits_SOUND_WORLD |
			bits_SOUND_COMBAT |
			bits_SOUND_CARCASS |
			bits_SOUND_MEAT |
			bits_SOUND_GARBAGE |
			bits_SOUND_DANGER |
			bits_SOUND_PLAYER;
}

int CRecruit::DefaultClassify(void)
{
	return CLASS_PLAYER_ALLY_MILITARY;
}

void CRecruit::PainSound( void )
{
	if( gpGlobals->time < m_painTime )
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT( 0.5, 0.75 );

	switch( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 2:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	}
}

//=========================================================
// DeathSound
//=========================================================
void CRecruit::DeathSound( void )
{
	switch( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 2:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	}
}

void CRecruit::TalkInit()
{
	CTalkMonster::TalkInit();

	m_szGrp[TLK_ANSWER] = "RC_ANSWER";
	m_szGrp[TLK_QUESTION] = "RC_QUESTION";
	m_szGrp[TLK_IDLE] = "RC_IDLE";
	m_szGrp[TLK_STARE] = "RC_STARE";
	m_szGrp[TLK_USE] = "RC_OK";
	m_szGrp[TLK_UNUSE] = "RC_WAIT";
	m_szGrp[TLK_DECLINE] = "RC_POK";
	m_szGrp[TLK_STOP] = "RC_STOP";

	m_szGrp[TLK_NOSHOOT] = "RC_SCARED";
	m_szGrp[TLK_HELLO] = "RC_HELLO";

	m_szGrp[TLK_PLHURT1] = "!RC_CUREA";
	m_szGrp[TLK_PLHURT2] = "!RC_CUREB";
	m_szGrp[TLK_PLHURT3] = "!RC_CUREC";

	m_szGrp[TLK_PHELLO] = NULL;// UNDONE
	m_szGrp[TLK_PIDLE] = NULL;// UNDONE
	m_szGrp[TLK_PQUESTION] = "RC_PQUEST";		// UNDONE

	m_szGrp[TLK_SMELL] = "RC_SMELL";

	m_szGrp[TLK_WOUND] = "RC_WOUND";
	m_szGrp[TLK_MORTAL] = "RC_MORTAL";

	m_szGrp[TLK_SHOT] = "RC_SHOT";
	m_szGrp[TLK_MAD] = "RC_MAD";
}

Schedule_t* CRecruit::GetSchedule()
{
	switch (m_MonsterState) {
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
	{
		Schedule_t* followingSchedule = GetFollowingSchedule();
		if (followingSchedule)
			return followingSchedule;
	}
		break;
	case MONSTERSTATE_COMBAT:
	{
		if( HasConditions( bits_COND_ENEMY_DEAD|bits_COND_ENEMY_LOST ) )
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}
		if( HasConditions( bits_COND_HEAR_SOUND ) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );	// Cower and panic from the scary sound!
		return GetScheduleOfType( SCHED_RETREAT_FROM_ENEMY );			// Run & Cower
	}
		break;
	default:
		break;
	}
	return CTalkMonster::GetSchedule();
}

#endif
