#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"soundent.h"
#include	"mod_features.h"

#if FEATURE_DRILLSERGEANT

class CDrillSergeant : public CTalkMonster
{
public:
	void Spawn(void);
	void Precache(void);
	const char* DefaultDisplayName() { return "Drill Sergeant"; }
	void SetYawSpeed(void);
	int ISoundMask(void);
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

LINK_ENTITY_TO_CLASS( monster_drillsergeant, CDrillSergeant )

TYPEDESCRIPTION	CDrillSergeant::m_SaveData[] =
{
	DEFINE_FIELD( CDrillSergeant, m_painTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CDrillSergeant, CTalkMonster )

void CDrillSergeant::Precache()
{
	PrecacheMyModel("models/drill.mdl");
	PRECACHE_SOUND("barney/ba_pain1.wav");
	PRECACHE_SOUND("barney/ba_pain2.wav");
	PRECACHE_SOUND("barney/ba_pain3.wav");
	PRECACHE_SOUND("barney/ba_die1.wav");
	PRECACHE_SOUND("barney/ba_die2.wav");
	PRECACHE_SOUND("barney/ba_die3.wav");
	TalkInit();
	CTalkMonster::Precache();
}

void CDrillSergeant::Spawn()
{
	Precache();

	SetMyModel( "models/drill.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_RED );
	SetMyHealth( gSkillData.barneyHealth );
	pev->view_ofs = Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	TalkMonsterInit();
}

void CDrillSergeant::SetYawSpeed( void )
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

int CDrillSergeant::ISoundMask( void)
{
	return bits_SOUND_WORLD |
			bits_SOUND_COMBAT |
			bits_SOUND_CARCASS |
			bits_SOUND_MEAT |
			bits_SOUND_GARBAGE |
			bits_SOUND_DANGER |
			bits_SOUND_PLAYER;
}

int CDrillSergeant::DefaultClassify(void)
{
	return CLASS_PLAYER_ALLY_MILITARY;
}

void CDrillSergeant::PainSound( void )
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
void CDrillSergeant::DeathSound( void )
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

void CDrillSergeant::TalkInit()
{
	CTalkMonster::TalkInit();

	m_szGrp[TLK_ANSWER] = "DR_ANSWER";
	m_szGrp[TLK_QUESTION] = "DR_QUESTION";
	m_szGrp[TLK_IDLE] = "DR_IDLE";
	m_szGrp[TLK_STARE] = "DR_STARE";
	m_szGrp[TLK_USE] = "DR_OK";
	m_szGrp[TLK_UNUSE] = "DR_WAIT";
	m_szGrp[TLK_DECLINE] = "DR_POK";
	m_szGrp[TLK_STOP] = "DR_STOP";

	m_szGrp[TLK_NOSHOOT] = "DR_SCARED";
	m_szGrp[TLK_HELLO] = "DR_HELLO";

	m_szGrp[TLK_PLHURT1] = "!DR_CUREA";
	m_szGrp[TLK_PLHURT2] = "!DR_CUREB";
	m_szGrp[TLK_PLHURT3] = "!DR_CUREC";

	m_szGrp[TLK_PHELLO] = NULL;// UNDONE
	m_szGrp[TLK_PIDLE] = NULL;// UNDONE
	m_szGrp[TLK_PQUESTION] = "DR_PQUEST";		// UNDONE

	m_szGrp[TLK_SMELL] = "DR_SMELL";

	m_szGrp[TLK_WOUND] = "DR_WOUND";
	m_szGrp[TLK_MORTAL] = "DR_MORTAL";

	m_szGrp[TLK_SHOT] = "DR_SHOT";
	m_szGrp[TLK_MAD] = "DR_MAD";
}

Schedule_t* CDrillSergeant::GetSchedule()
{
	switch (m_MonsterState) {
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
		Schedule_t* followingSchedule = GetFollowingSchedule();
		if (followingSchedule)
			return followingSchedule;
		break;
	}
	return CTalkMonster::GetSchedule();
}

#endif
