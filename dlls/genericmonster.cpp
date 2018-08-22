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
// Generic Monster - purely for scripted sequence work.
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"

// For holograms, make them not solid so the player can walk through them
#define	SF_GENERICMONSTER_NOTSOLID					4 
#define SF_HEAD_CONTROLLER					8

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CGenericMonster : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int DefaultClassify( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int ISoundMask( void );
	void PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener );
	void IdleHeadTurn( Vector &vecFriend );
	void MonsterThink();
	void Killed( entvars_t *pevAttacker, int iGib );

	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

private:
	float m_talkTime;
	EHANDLE m_hTalkTarget;
	float m_flIdealYaw;
	float m_flCurrentYaw;

};

LINK_ENTITY_TO_CLASS( monster_generic, CGenericMonster )

TYPEDESCRIPTION CGenericMonster::m_SaveData[] =
{
	DEFINE_FIELD( CGenericMonster, m_talkTime, FIELD_FLOAT ),
	DEFINE_FIELD( CGenericMonster, m_hTalkTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CGenericMonster, m_flIdealYaw, FIELD_FLOAT ),
	DEFINE_FIELD( CGenericMonster, m_flCurrentYaw, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CGenericMonster, CBaseMonster )

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CGenericMonster::DefaultClassify( void )
{
	return pev->takedamage ? CLASS_PLAYER_ALLY : CLASS_NONE;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGenericMonster::SetYawSpeed( void )
{
	int ys;

	switch( m_Activity )
	{
	case ACT_IDLE:
	default:
		ys = 90;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGenericMonster::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 0:
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// ISoundMask - generic monster can't hear.
//=========================================================
int CGenericMonster::ISoundMask( void )
{
	return 0;
}

//=========================================================
// Spawn
//=========================================================
void CGenericMonster::Spawn()
{
	Precache();

	if (FStringNull(pev->model))
	{
		ALERT(at_console, "Spawning monster_generic without model!\n");
	}
	else
	{
		SET_MODEL( ENT( pev ), STRING( pev->model ) );
	}
/*
	if( FStrEq( STRING( pev->model ), "models/player.mdl" ) )
		UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );
	else
		UTIL_SetSize( pev, VEC_HULL_MIN, VEC_HULL_MAX);
*/
	if( FStrEq( STRING( pev->model ), "models/player.mdl" ) || FStrEq( STRING( pev->model ), "models/holo.mdl" ) )
		UTIL_SetSize( pev, VEC_HULL_MIN, VEC_HULL_MAX );
	else
		UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_RED );
	SetMyHealth( 8 );
	m_flFieldOfView = 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();

	if( pev->spawnflags & SF_HEAD_CONTROLLER )
	{
		m_afCapability = bits_CAP_TURN_HEAD;
	}

	m_flIdealYaw = m_flCurrentYaw = 0;

	if( pev->spawnflags & SF_GENERICMONSTER_NOTSOLID )
	{
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGenericMonster::Precache()
{
	if (!FStringNull(pev->model))
		PRECACHE_MODEL( STRING( pev->model ) );
	if (!FStringNull(m_gibModel))
		PRECACHE_MODEL( STRING(m_gibModel) );
}

void CGenericMonster::PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener )
{
	m_talkTime = gpGlobals->time + duration;
	PlaySentence( pszSentence, duration, volume, attenuation );

	m_hTalkTarget = pListener;
}

void CGenericMonster::IdleHeadTurn( Vector &vecFriend )
{
	// turn head in desired direction only if ent has a turnable head
	if( m_afCapability & bits_CAP_TURN_HEAD )
	{
		float yaw = VecToYaw( vecFriend - pev->origin ) - pev->angles.y;

		if( yaw > 180 )
			yaw -= 360;
		if( yaw < -180 )
			yaw += 360;

		m_flIdealYaw = yaw;
	}
}

void CGenericMonster::MonsterThink()
{
	if( m_afCapability & bits_CAP_TURN_HEAD )
		{
		if( m_hTalkTarget != 0 )
		{
			if( gpGlobals->time > m_talkTime )
			{
				m_flIdealYaw = 0;
				m_hTalkTarget = 0;
			}
			else
			{
				IdleHeadTurn( m_hTalkTarget->pev->origin );
			}
		}

		if( m_flCurrentYaw != m_flIdealYaw )
		{
			if( m_flCurrentYaw <= m_flIdealYaw )
			{
				m_flCurrentYaw += Q_min( m_flIdealYaw - m_flCurrentYaw, 20.0f );
			}
			else
			{
				m_flCurrentYaw -= Q_min( m_flCurrentYaw - m_flIdealYaw, 20.0f );
			}
			SetBoneController( 0, m_flCurrentYaw );
		}
	}

	CBaseMonster::MonsterThink();
}

void CGenericMonster::Killed(entvars_t *pevAttacker, int iGib)
{
	SentenceStop();
	CBaseMonster::Killed(pevAttacker, iGib);
}

class CDeadGenericMonster : public CBaseMonster
{
public:
	void Precache();
	void Spawn();
	void KeyValue( KeyValueData *pkvd );
	int DefaultClassify() { return CLASS_HUMAN_PASSIVE; }

	string_t pose; // no need to save
};

LINK_ENTITY_TO_CLASS( monster_generic_dead, CDeadGenericMonster )

void CDeadGenericMonster::Precache()
{
	if (!FStringNull(pev->model))
		PRECACHE_MODEL( STRING( pev->model ) );
	if (!FStringNull(m_gibModel))
		PRECACHE_MODEL( STRING(m_gibModel) );
}

void CDeadGenericMonster::Spawn()
{
	Precache();
	if (FStringNull(pev->model))
	{
		ALERT(at_console, "Spawning monster_generic_dead without model!\n");
	}
	else
	{
		SET_MODEL( ENT( pev ), STRING( pev->model ) );
	}

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	SetMyBloodColor( BLOOD_COLOR_RED );

	if (FStringNull(pose))
	{
		ALERT(at_console, "Spawning monster_generic_dead without pose!\n");
	}
	else
	{
		pev->sequence = LookupSequence( STRING(pose) );
		if (pev->sequence == -1)
		{
			ALERT ( at_console, "%s with bad pose (no %s animation in %s)\n", STRING(pev->classname), STRING(pose), STRING(pev->model) );
		}
	}

	SetMyHealth( 8 );
	MonsterInitDead();
	if (pev->spawnflags & SF_GENERICMONSTER_NOTSOLID)
		pev->solid = SOLID_NOT;
}

void CDeadGenericMonster::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		pose = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

//=========================================================
// Op4 Loader
//=========================================================

class CLoader : public CGenericMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int DefaultClassify() {return CLASS_NONE;}
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
};

LINK_ENTITY_TO_CLASS(monster_op4loader, CLoader)

void CLoader::Spawn()
{
	Precache();

	SetMyModel("models/loader.mdl");

	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	SetMyBloodColor(DONT_BLEED);
	SetMyHealth(100);
	m_flFieldOfView = 0.5;
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();
	pev->takedamage = DAMAGE_NO;

	if (pev->spawnflags & SF_GENERICMONSTER_NOTSOLID)
	{
		pev->solid = SOLID_NOT;
	}
}

void CLoader::Precache()
{
	PrecacheMyModel("models/loader.mdl");
}

void CLoader::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT(1.0,2.0) );
}
