/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
// Monster Maker - this is an entity that creates monsters
// in the game.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "saverestore.h"

#define MONSTERMAKER_START_ON_FIX 1

// Monstermaker spawnflags
#define	SF_MONSTERMAKER_START_ON	1 // start active ( if has targetname )
#define	SF_MONSTERMAKER_CYCLIC		4 // drop one monster every time fired.
#define SF_MONSTERMAKER_MONSTERCLIP	8 // Children are blocked by monsterclip
#define SF_MONSTERMAKER_PRISONER	16
#define SF_MONSTERMAKER_CYCLIC_BACKLOG 64
#define SF_MONSTERMAKER_PREDISASTER 256
#define SF_MONSTERMAKER_DONT_DROP_GUN 1024 // Spawn monster won't drop gun upon death
#define SF_MONSTERMAKER_NO_GROUND_CHECK 2048 // don't check if something on ground prevents a monster to fall on spawn

#define SF_MONSTERMAKER_WARP_AT_MONSTER_CENTER 8192 // When using warpball template, make it play at the center of monster's body, not origin
#define SF_MONSTERMAKER_PASS_MONSTER_AS_ACTIVATOR 16384 // Use the spawned monster as activator to fire target

enum
{
	MONSTERMAKER_LIMIT = 0,
	MONSTERMAKER_BLOCKED,
	MONSTERMAKER_NULLENTITY,
	MONSTERMAKER_SPAWNED,
};

//=========================================================
// MonsterMaker - this ent creates monsters during the game.
//=========================================================
class CMonsterMaker : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData* pkvd);
	void EXPORT ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT CyclicUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT MakerThink( void );
	void EXPORT CyclicBacklogThink( void );
	void DeathNotice( entvars_t *pevChild );// monster maker children use this to tell the monster maker that they have died.
	int MakeMonster( void );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	string_t m_iszMonsterClassname;// classname of the monster(s) that will be created.

	int m_cNumMonsters;// max number of monsters this ent can create

	int m_cLiveChildren;// how many monsters made by this monster maker that are currently alive
	int m_iMaxLiveChildren;// max number of monsters that this maker may have out at one time.

	float m_flGround; // z coord of the ground under me, used to make sure no monsters are under the maker when it drops a new child

	BOOL m_fActive;
	BOOL m_fFadeChildren;// should we make the children fadeout?
	string_t m_customModel;
	int m_classify;
	int m_iPose;
	BOOL m_notSolid;
	BOOL m_gag;
	int m_iHead;
	int m_cyclicBacklogSize;
};

LINK_ENTITY_TO_CLASS( monstermaker, CMonsterMaker )
LINK_ENTITY_TO_CLASS( squadmaker, CMonsterMaker )

TYPEDESCRIPTION	CMonsterMaker::m_SaveData[] =
{
	DEFINE_FIELD( CMonsterMaker, m_iszMonsterClassname, FIELD_STRING ),
	DEFINE_FIELD( CMonsterMaker, m_cNumMonsters, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_cLiveChildren, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_flGround, FIELD_FLOAT ),
	DEFINE_FIELD( CMonsterMaker, m_iMaxLiveChildren, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_fActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMonsterMaker, m_fFadeChildren, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMonsterMaker, m_customModel, FIELD_STRING ),
	DEFINE_FIELD( CMonsterMaker, m_classify, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_iPose, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_notSolid, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMonsterMaker, m_gag, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMonsterMaker, m_iHead, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_minHullSize, FIELD_VECTOR ),
	DEFINE_FIELD( CMonsterMaker, m_maxHullSize, FIELD_VECTOR ),
	DEFINE_FIELD( CMonsterMaker, m_cyclicBacklogSize, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CMonsterMaker, CBaseMonster )

void CMonsterMaker::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "monstercount" ) )
	{
		m_cNumMonsters = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "m_imaxlivechildren" ) )
	{
		m_iMaxLiveChildren = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "monstertype" ) )
	{
		m_iszMonsterClassname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "warpball" ) || FStrEq( pkvd->szKeyName, "xenmaker" ) )
	{
		pev->message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "new_model" ) )
	{
		m_customModel = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "classify" ) )
	{
		m_classify = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "pose" ) )
	{
		m_iPose = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "notsolid" ) )
	{
		m_notSolid = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "gag" ) )
	{
		m_gag = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "head" ) )
	{
		m_iHead = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "trigger_target" ) )
	{
		m_iszTriggerTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "trigger_condition" ) )
	{
		m_iTriggerCondition = (short)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "trigger_alt_condition" ) )
	{
		m_iTriggerAltCondition = (short)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "respawn_as_playerally" ) )
	{
		m_reverseRelationship = atoi( pkvd->szValue ) != 0;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

void CMonsterMaker::Spawn()
{
	pev->solid = SOLID_NOT;

	m_cLiveChildren = 0;
	Precache();
	if( !FStringNull( pev->targetname ) )
	{
		if( pev->spawnflags & SF_MONSTERMAKER_CYCLIC )
		{
			SetUse( &CMonsterMaker::CyclicUse );// drop one monster each time we fire
		}
		else
		{
			SetUse( &CMonsterMaker::ToggleUse );// so can be turned on/off
		}

		if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_START_ON ) )
		{
			// start making monsters as soon as monstermaker spawns
#if MONSTERMAKER_START_ON_FIX
			pev->nextthink = gpGlobals->time + m_flDelay;
#endif
			m_fActive = TRUE;
			SetThink( &CMonsterMaker::MakerThink );
		}
		else if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_CYCLIC ) && FBitSet( pev->spawnflags, SF_MONSTERMAKER_CYCLIC_BACKLOG ) )
		{
			SetThink( &CMonsterMaker::CyclicBacklogThink );
		}
		else
		{
			// wait to be activated.
			m_fActive = FALSE;
			SetThink( &CBaseEntity::SUB_DoNothing );
		}
	}
	else
	{
		// no targetname, just start.
		pev->nextthink = gpGlobals->time + m_flDelay;
		m_fActive = TRUE;
		SetThink( &CMonsterMaker::MakerThink );
	}

	if( m_cNumMonsters == 1 )
	{
		m_fFadeChildren = FALSE;
	}
	else
	{
		m_fFadeChildren = TRUE;
	}

	m_flGround = 0;
}

void CMonsterMaker::Precache( void )
{
	CBaseMonster::Precache();

	if (!FStringNull(m_customModel))
		PRECACHE_MODEL(STRING(m_customModel));
	if (!FStringNull(m_gibModel))
		PRECACHE_MODEL(STRING(m_gibModel));

	UTIL_PrecacheMonster( STRING(m_iszMonsterClassname), m_reverseRelationship );
}

//=========================================================
// MakeMonster-  this is the code that drops the monster
//=========================================================
int CMonsterMaker::MakeMonster( void )
{
	edict_t	*pent;
	entvars_t *pevCreate;

	if( m_iMaxLiveChildren > 0 && m_cLiveChildren >= m_iMaxLiveChildren )
	{
		// not allowed to make a new one yet. Too many live ones out right now.
		return MONSTERMAKER_LIMIT;
	}

	if (!FBitSet(pev->spawnflags, SF_MONSTERMAKER_NO_GROUND_CHECK))
	{
		if( !m_flGround )
		{
			// set altitude. Now that I'm activated, any breakables, etc should be out from under me.
			TraceResult tr;

			UTIL_TraceLine( pev->origin, pev->origin - Vector( 0, 0, 2048 ), ignore_monsters, ENT( pev ), &tr );
			m_flGround = tr.vecEndPos.z;
		}
	}

	Vector mins = pev->origin - Vector( 34, 34, 0 );
	Vector maxs = pev->origin + Vector( 34, 34, 0 );
	maxs.z = pev->origin.z;
	if (!FBitSet(pev->spawnflags, SF_MONSTERMAKER_NO_GROUND_CHECK))
		mins.z = m_flGround;

	CBaseEntity *pList[2];
	int count = UTIL_EntitiesInBox( pList, 2, mins, maxs, FL_CLIENT | FL_MONSTER );
	if( count )
	{
		// don't build a stack of monsters!
		return MONSTERMAKER_BLOCKED;
	}

	pent = CREATE_NAMED_ENTITY( m_iszMonsterClassname );

	if( FNullEnt( pent ) )
	{
		ALERT ( at_console, "NULL Ent in MonsterMaker!\n" );
		return MONSTERMAKER_NULLENTITY;
	}

	pevCreate = VARS( pent );
	pevCreate->origin = pev->origin;
	pevCreate->angles = pev->angles;
	SetBits( pevCreate->spawnflags, SF_MONSTER_FALL_TO_GROUND );
	pevCreate->body = pev->body;
	pevCreate->skin = pev->skin;
	pevCreate->health = pev->health;
	pevCreate->scale = pev->scale;
	if (!FStringNull(m_customModel))
		pevCreate->model = m_customModel;

	CBaseMonster* createdMonster = GetMonsterPointer(pent);
	if (createdMonster)
	{
		// Children hit monsterclip brushes
		if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_MONSTERCLIP ))
			SetBits( pevCreate->spawnflags, SF_MONSTER_HITMONSTERCLIP );

		if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_PRISONER ))
			SetBits( pevCreate->spawnflags, SF_MONSTER_PRISONER );
		if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_PREDISASTER ))
			SetBits( pevCreate->spawnflags, SF_MONSTER_PREDISASTER );
		if (FBitSet(pev->spawnflags, SF_MONSTERMAKER_DONT_DROP_GUN))
			SetBits(pevCreate->spawnflags, SF_MONSTER_DONT_DROP_GRUN);
		if (m_gag > 0)
			SetBits(pevCreate->spawnflags, SF_MONSTER_GAG);
		pevCreate->weapons = pev->weapons;

		if (m_classify)
			createdMonster->m_iClass = m_classify;
		createdMonster->m_reverseRelationship = m_reverseRelationship;
		createdMonster->m_displayName = m_displayName;
		createdMonster->SetMyBloodColor(m_bloodColor);
		if (!FStringNull(m_gibModel))
			createdMonster->m_gibModel = m_gibModel;

		if (!FStringNull(m_iszTriggerTarget))
		{
			createdMonster->m_iszTriggerTarget = m_iszTriggerTarget;
			createdMonster->m_iTriggerCondition = m_iTriggerCondition;
			createdMonster->m_iTriggerAltCondition = m_iTriggerAltCondition;
		}

		createdMonster->m_minHullSize = m_minHullSize;
		createdMonster->m_maxHullSize = m_maxHullSize;

		createdMonster->m_customSoundMask = m_customSoundMask;

		createdMonster->SetHead(m_iHead);

		CDeadMonster* deadMonster = createdMonster->MyDeadMonsterPointer();
		if (deadMonster)
		{
			deadMonster->m_iPose = m_iPose;
		}
	}

	DispatchSpawn( ENT( pevCreate ) );
	pevCreate->owner = edict();
	// Disable until proper investigation
#if 0
	if (m_notSolid > 0)
	{
		pevCreate->solid = SOLID_NOT;
	}
#endif

	if ( !FStringNull( pev->message ) && !FStringNull( pev->targetname ) )
	{
		CBaseEntity* foundEntity = UTIL_FindEntityByTargetname(NULL, STRING(pev->message));
		if ( foundEntity && (FClassnameIs(foundEntity->pev, "env_warpball")
							 || FClassnameIs(foundEntity->pev, "env_xenmaker")))
		{
			const bool warpAtCenter = FBitSet(pev->spawnflags, SF_MONSTERMAKER_WARP_AT_MONSTER_CENTER);

			foundEntity->pev->dmg_inflictor = edict();
			foundEntity->Use(warpAtCenter ? Instance(pent) : this, this, warpAtCenter ? USE_SET : USE_TOGGLE, 0.0f);
			foundEntity->pev->dmg_inflictor = NULL;
		}
	}

	if( !FStringNull( pev->netname ) )
	{
		// if I have a netname (overloaded), give the child monster that name as a targetname
		pevCreate->targetname = pev->netname;
	}

	m_cLiveChildren++;// count this monster
	if (m_cNumMonsters > 0)
		m_cNumMonsters--;

	if( m_cNumMonsters == 0 )
	{
		// Disable this forever.  Don't kill it because it still gets death notices
		SetThink( NULL );
		SetUse( NULL );
	}

	// If I have a target, fire!
	if( !FStringNull( pev->target ) )
	{
		CBaseEntity* pActivator = this;
		if (FBitSet(pev->spawnflags, SF_MONSTERMAKER_PASS_MONSTER_AS_ACTIVATOR))
		{
			pActivator = createdMonster;
		}
		// delay already overloaded for this entity, so can't call SUB_UseTargets()
		FireTargets( STRING( pev->target ), pActivator, this, USE_TOGGLE, 0 );
	}

	return MONSTERMAKER_SPAWNED;
}

//=========================================================
// CyclicUse - drops one monster from the monstermaker
// each time we call this.
//=========================================================
void CMonsterMaker::CyclicUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (MakeMonster() == MONSTERMAKER_BLOCKED)
	{
		if (FBitSet(pev->spawnflags, SF_MONSTERMAKER_CYCLIC_BACKLOG))
		{
			m_cyclicBacklogSize++;
			pev->nextthink = gpGlobals->time + m_flDelay;
		}
	}
}

//=========================================================
// ToggleUse - activates/deactivates the monster maker
//=========================================================
void CMonsterMaker::ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !ShouldToggle( useType, m_fActive ) )
		return;

	if( m_fActive )
	{
		m_fActive = FALSE;
		SetThink( NULL );
	}
	else
	{
		m_fActive = TRUE;
		SetThink( &CMonsterMaker::MakerThink );
	}

	pev->nextthink = gpGlobals->time;
}

//=========================================================
// MakerThink - creates a new monster every so often
//=========================================================
void CMonsterMaker::MakerThink( void )
{
	pev->nextthink = gpGlobals->time + m_flDelay;

	MakeMonster();
}

void CMonsterMaker::CyclicBacklogThink()
{
	if (MakeMonster() == MONSTERMAKER_SPAWNED)
	{
		m_cyclicBacklogSize--;
	}
	if (m_cyclicBacklogSize > 0)
		pev->nextthink = gpGlobals->time + m_flDelay;
}

//=========================================================
//=========================================================
void CMonsterMaker::DeathNotice( entvars_t *pevChild )
{
	// ok, we've gotten the deathnotice from our child, now clear out its owner if we don't want it to fade.
	m_cLiveChildren--;
	ALERT(at_aiconsole, "Monstermaker DeathNotice: %d live children left\n", m_cLiveChildren);

	if( !m_fFadeChildren )
	{
		pevChild->owner = NULL;
	}

	if (m_cNumMonsters == 0)
	{
		SetThink(&CMonsterMaker::SUB_Remove);
		pev->nextthink = gpGlobals->time;
	}
}
