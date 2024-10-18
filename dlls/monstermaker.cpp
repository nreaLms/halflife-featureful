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
#include "game.h"
#include "locus.h"
#include "talkmonster.h"
#include "warpball.h"

#define MONSTERMAKER_START_ON_FIX 1

// Monstermaker spawnflags
#define	SF_MONSTERMAKER_START_ON	1 // start active ( if has targetname )
#define	SF_MONSTERMAKER_CYCLIC		4 // drop one monster every time fired.
#define SF_MONSTERMAKER_MONSTERCLIP	8 // Children are blocked by monsterclip
#define SF_MONSTERMAKER_PRISONER	16
#define SF_MONSTERMAKER_AUTOSIZEBBOX 32
#define SF_MONSTERMAKER_CYCLIC_BACKLOG 64
#define SF_MONSTERMAKER_WAIT_FOR_SCRIPT 128 // TODO: implement
#define SF_MONSTERMAKER_PREDISASTER 256
#define SF_MONSTERMAKER_DONT_DROP_GUN 1024 // Spawn monster won't drop gun upon death
#define SF_MONSTERMAKER_ALIGN_TO_PLAYER 4096 // Align to closest player on spawn
#define SF_MONSTERMAKER_WAIT_UNTIL_PROVOKED 8192
#define SF_MONSTERMAKER_NO_GROUND_CHECK		( 1 << 14 ) // don't check if something on ground prevents a monster to fall on spawn
#define SF_MONSTERMAKER_SPECIAL_FLAG		( 1 << 15 )
#define SF_MONSTERMAKER_NONSOLID_CORPSE		( 1 << 16 )
#define SF_MONSTERMAKER_IGNORE_PLAYER_PUSHING ( 1 << 19 )
#define SF_MONSTERMAKER_ACT_IN_NON_PVS		( 1 << 20 )

enum
{
	MONSTERMAKER_LIMIT = 0,
	MONSTERMAKER_BLOCKED,
	MONSTERMAKER_NULLENTITY,
	MONSTERMAKER_SPAWNED,
	MONSTERMAKER_BADPLACE,
};

typedef enum
{
	MMA_NO = -1,
	MMA_DEFAULT = 0,
	MMA_MAKER = 1,
	MMA_MONSTER = 2,
	MMA_FORWARD = 3,
} MONSTERMAKER_TARGET_ACTIVATOR;

#define MAX_CHILD_KEYS 16

//=========================================================
// MonsterMaker - this ent creates monsters during the game.
//=========================================================
class CMonsterMaker : public CBaseMonster
{
public:
	void Spawn( void );
	bool CheckMonsterClassname();
	void Precache( void );
	void KeyValue( KeyValueData* pkvd);
	void EXPORT ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT CyclicUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT MakerThink( void );
	void EXPORT CyclicBacklogThink( void );
	void ReportNullEntity();
	void DeathNotice( entvars_t *pevChild );// monster maker children use this to tell the monster maker that they have died.
	int MakeMonster( void );

	void GetRealHullSizes(Vector& minHullSize, Vector& maxHullSize);
	int CalculateSpot(const Vector& testMinHullSize, const Vector& testMaxHullSize, Vector& placePosition, Vector& placeAngles, edict_t*& warpballSoundEnt, float spawnDelay);
	CBaseEntity* SpawnMonster(const Vector& placePosition, const Vector& placeAngles);
	void StartWarpballEffect(const Vector& vecPosition, edict_t* warpballSoundEnt);
	string_t WarpballName() {
		return pev->message;
	}

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
	int m_iPose;
	BOOL m_notSolid;
	BOOL m_gag;
	int m_iHead;
	int m_cyclicBacklogSize;
	string_t m_iszPlacePosition;
	short m_targetActivator;
	short m_iMaxYawDeviation;
	Vector m_defaultMinHullSize;
	Vector m_defaultMaxHullSize;

	short m_followFailPolicy;
	string_t m_iszUse;
	string_t m_iszUnUse;
	string_t m_iszDecline;

	short m_followagePolicy;

	float m_spawnDelay;
	int m_delayedCount;

	float m_delayAfterBlocked;

	string_t m_childKeys[MAX_CHILD_KEYS];
	string_t m_childValues[MAX_CHILD_KEYS];
	int m_childKeyCount;

	bool m_childIsValid;
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
	DEFINE_FIELD( CMonsterMaker, m_iPose, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_notSolid, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMonsterMaker, m_gag, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMonsterMaker, m_iHead, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_minHullSize, FIELD_VECTOR ),
	DEFINE_FIELD( CMonsterMaker, m_maxHullSize, FIELD_VECTOR ),
	DEFINE_FIELD( CMonsterMaker, m_cyclicBacklogSize, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_iszPlacePosition, FIELD_STRING ),
	DEFINE_FIELD( CMonsterMaker, m_targetActivator, FIELD_SHORT ),
	DEFINE_FIELD( CMonsterMaker, m_iMaxYawDeviation, FIELD_SHORT ),
	DEFINE_FIELD( CMonsterMaker, m_defaultMinHullSize, FIELD_VECTOR ),
	DEFINE_FIELD( CMonsterMaker, m_defaultMaxHullSize, FIELD_VECTOR ),
	DEFINE_FIELD( CMonsterMaker, m_followFailPolicy, FIELD_SHORT ),
	DEFINE_FIELD( CMonsterMaker, m_followagePolicy, FIELD_SHORT ),
	DEFINE_FIELD( CMonsterMaker, m_iszUse, FIELD_STRING ),
	DEFINE_FIELD( CMonsterMaker, m_iszUnUse, FIELD_STRING ),
	DEFINE_FIELD( CMonsterMaker, m_iszDecline, FIELD_STRING ),
	DEFINE_FIELD( CMonsterMaker, m_spawnDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CMonsterMaker, m_delayedCount, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_delayAfterBlocked, FIELD_FLOAT ),
	DEFINE_ARRAY( CMonsterMaker, m_childKeys, FIELD_STRING, MAX_CHILD_KEYS ),
	DEFINE_ARRAY( CMonsterMaker, m_childValues, FIELD_STRING, MAX_CHILD_KEYS ),
	DEFINE_FIELD( CMonsterMaker, m_childKeyCount, FIELD_INTEGER ),
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
	else if( FStrEq( pkvd->szKeyName, "target_activator" ) )
	{
		m_targetActivator = (short)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "spawnorigin" ) )
	{
		m_iszPlacePosition = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "yawdeviation" ) )
	{
		m_iMaxYawDeviation = (short)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "followfailpolicy" ) )
	{
		m_followFailPolicy = (short)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "UseSentence" ) )
	{
		m_iszUse = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "UnUseSentence" ) )
	{
		m_iszUnUse = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq( pkvd->szKeyName, "RefusalSentence" ))
	{
		m_iszDecline = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "followage_policy" ) )
	{
		m_followagePolicy = (short)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "spawndelay" ) )
	{
		m_spawnDelay = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "delay_after_blocked" ) )
	{
		m_delayAfterBlocked = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( pkvd->szKeyName[0] == '#' )
	{
		if (m_childKeyCount < MAX_CHILD_KEYS)
		{
			m_childKeys[m_childKeyCount] = ALLOC_STRING(pkvd->szKeyName + 1);
			m_childValues[m_childKeyCount] = ALLOC_STRING(pkvd->szValue);
			++m_childKeyCount;
		}
		else
		{
			ALERT(at_warning, "%s: Too many child keys", STRING(pev->classname));
		}
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

void CMonsterMaker::Spawn()
{
	pev->solid = SOLID_NOT;

	if (FStringNull(m_iszPlacePosition))
	{
		// Spirit compat
		m_iszPlacePosition = pev->noise;
		pev->noise = iStringNull;
	}

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

bool CMonsterMaker::CheckMonsterClassname()
{
	if (FStringNull(m_iszMonsterClassname))
	{
		ALERT(at_error, "%s at (%g, %g, %g) has an empty monster classname!\n", STRING(pev->classname), pev->origin.x, pev->origin.y, pev->origin.z);
		return false;
	}
	return true;
}

void CMonsterMaker::Precache( void )
{
	CBaseMonster::Precache();

	if (!FStringNull(m_customModel))
		PRECACHE_MODEL(STRING(m_customModel));
	if (!FStringNull(m_gibModel))
		PRECACHE_MODEL(STRING(m_gibModel));

	EntityOverrides entityOverrides;
	entityOverrides.model = m_customModel;
	entityOverrides.entTemplate = m_entTemplate;
	entityOverrides.soundList = m_soundList;

	if (CheckMonsterClassname())
		m_childIsValid = UTIL_PrecacheMonster( STRING(m_iszMonsterClassname), m_reverseRelationship, &m_defaultMinHullSize, &m_defaultMaxHullSize, entityOverrides );

	if (!FStringNull(WarpballName()))
		PrecacheWarpballTemplate(STRING(WarpballName()), STRING(m_iszMonsterClassname));

	UTIL_PrecacheOther("monstermaker_hull");
}

class CMonsterMakerHull : public CBaseEntity
{
public:
	void Spawn();
	void Precache();
	static CMonsterMakerHull* SelfCreate(CMonsterMaker* pMonsterMaker, const Vector& position, const Vector& angles, const Vector& minHullSize, const Vector& maxHullSize, float delay);
	void Think();
};

LINK_ENTITY_TO_CLASS( monstermaker_hull, CMonsterMakerHull )

void CMonsterMakerHull::Precache()
{
	PRECACHE_MODEL("sprites/iunknown.spr");
}

void CMonsterMakerHull::Spawn()
{
	Precache();
}

CMonsterMakerHull* CMonsterMakerHull::SelfCreate(CMonsterMaker *pMonsterMaker, const Vector &position, const Vector &angles, const Vector &minHullSize, const Vector &maxHullSize, float delay)
{
	CMonsterMakerHull *pHull = GetClassPtr((CMonsterMakerHull *)NULL);
	UTIL_SetOrigin( pHull->pev, position );
	SET_MODEL( pHull->edict(), "sprites/iunknown.spr" );
	pHull->pev->angles = angles;
	pHull->pev->solid = SOLID_BBOX;
	pHull->pev->flags = FL_MONSTER;
	pHull->pev->classname = MAKE_STRING("monstermaker_hull");
	pHull->pev->movetype = MOVETYPE_NONE;
	pHull->pev->owner = pMonsterMaker->edict();
	UTIL_SetSize( pHull->pev, minHullSize, maxHullSize );
	pHull->pev->renderamt = 0;
	pHull->pev->rendermode = kRenderTransTexture;
	pHull->pev->nextthink = gpGlobals->time + delay;
	return pHull;
}

void CMonsterMakerHull::Think()
{
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;

	if (!FNullEnt(pev->owner))
	{
		CBaseEntity* pOwner = CBaseEntity::Instance(pev->owner);
		if (pOwner)
		{
			CMonsterMaker* pMonsterMaker = static_cast<CMonsterMaker*>(pOwner);
			pMonsterMaker->SpawnMonster(pev->origin, pev->angles);
			pMonsterMaker->m_delayedCount--;
		}
	}

	UTIL_Remove(this);
}

static CBaseEntity* MakerBlocker(const Vector& mins, const Vector& maxs)
{
	CBaseEntity *pList[2];
	int count = UTIL_EntitiesInBox( pList, 2, mins, maxs, FL_CLIENT | FL_MONSTER );
	for (int i=0; i<count; ++i)
	{
		// Dead bodies don't block spawn
		if (pList[i]->pev->deadflag != DEAD_DEAD)
		{
			return pList[i];
		}
	}
	return NULL;
}

static float MakerGroundLevel(const Vector& position, entvars_t* pev)
{
	TraceResult tr;
	UTIL_TraceLine( position, position - Vector( 0, 0, 2048 ), ignore_monsters, ENT( pev ), &tr );
	return tr.vecEndPos.z;
}

void CMonsterMaker::GetRealHullSizes(Vector &minHullSize, Vector &maxHullSize)
{
	if (m_minHullSize != g_vecZero)
	{
		minHullSize = Vector( m_minHullSize.x, m_minHullSize.y, 0 );
	}
	else if (m_defaultMinHullSize != g_vecZero)
	{
		minHullSize = Vector( m_defaultMinHullSize.x, m_defaultMinHullSize.y, 0 );
	}

	if (m_maxHullSize != g_vecZero)
	{
		maxHullSize = m_maxHullSize;
	}
	else if (m_defaultMaxHullSize != g_vecZero)
	{
		maxHullSize = m_defaultMaxHullSize;
	}
}

int CMonsterMaker::CalculateSpot(const Vector &testMinHullSize, const Vector &testMaxHullSize, Vector &placePosition, Vector &placeAngles, edict_t *&warpballSoundEnt, float spawnDelay)
{
	Vector mins, maxs;

	const char* placeIdentifier = STRING(m_iszPlacePosition);
	if (!FStringNull(m_iszPlacePosition) && *placeIdentifier == '@' && placeIdentifier[1] != '\0')
	{
		// Random spot out of several ones

		const char* candidateName = placeIdentifier+1;

		int total = 0;
		CBaseEntity* pCandidate = NULL;
		CBaseEntity* pLastValidCandidate = NULL;
		CBaseEntity* pChosenSpot = NULL;
		bool foundAnything = false;

		while( ( pCandidate = UTIL_FindEntityByTargetname( pCandidate, candidateName ) ) != NULL )
		{
			foundAnything = true;

			mins = pCandidate->pev->origin + testMinHullSize;
			maxs = pCandidate->pev->origin + testMaxHullSize;

			if (!FBitSet(pev->spawnflags, SF_MONSTERMAKER_AUTOSIZEBBOX))
				maxs.z = pCandidate->pev->origin.z;

			if (!FBitSet(pev->spawnflags, SF_MONSTERMAKER_NO_GROUND_CHECK ))
			{
				mins.z = MakerGroundLevel(pCandidate->pev->origin, pev);
			}

			if (MakerBlocker(mins, maxs) == 0)
			{
				pLastValidCandidate = pCandidate;
				total++;
				if (RANDOM_LONG(0, total - 1) < 1)
					pChosenSpot = pLastValidCandidate;
			}
		}

		if (pChosenSpot)
		{
			placePosition = pChosenSpot->pev->origin;
			placeAngles = pChosenSpot->pev->angles;
			warpballSoundEnt = pChosenSpot->edict();
		}
		else
		{
			if (foundAnything)
				return MONSTERMAKER_BLOCKED;
			else
				return MONSTERMAKER_BADPLACE;
		}
	}
	else
	{
		// Single spot
		placeAngles = pev->angles;

		if (FStringNull(m_iszPlacePosition))
		{
			placePosition = pev->origin;
			warpballSoundEnt = edict();
		}
		else
		{
			if (!TryCalcLocus_Position(this, m_hActivator, placeIdentifier, placePosition))
				return MONSTERMAKER_BADPLACE;
			CBaseEntity* tempPosEnt = CBaseEntity::Create("info_target", placePosition, pev->angles);
			if (tempPosEnt)
			{
				tempPosEnt->SetThink(&CBaseEntity::SUB_Remove);
				tempPosEnt->pev->nextthink = gpGlobals->time + spawnDelay + 0.5f;
				warpballSoundEnt = tempPosEnt->edict();
			}
		}

		if (!FBitSet(pev->spawnflags, SF_MONSTERMAKER_NO_GROUND_CHECK ))
		{
			if( !m_flGround
					|| !FStringNull(m_iszPlacePosition) ) // The position could change, so we need to calculate the new ground level.
			{
				// set altitude. Now that I'm activated, any breakables, etc should be out from under me.
				m_flGround = MakerGroundLevel(placePosition, pev);
			}
		}

		mins = placePosition + testMinHullSize;
		maxs = placePosition + testMaxHullSize;

		if (!FBitSet(pev->spawnflags, SF_MONSTERMAKER_AUTOSIZEBBOX))
			maxs.z = placePosition.z;
		if (!FBitSet(pev->spawnflags, SF_MONSTERMAKER_NO_GROUND_CHECK ))
			mins.z = m_flGround;

		CBaseEntity *pBlocker = MakerBlocker(mins, maxs);
		if (pBlocker)
		{
			const char* blockerName = FStringNull(pBlocker->pev->classname) ? "" : STRING(pBlocker->pev->classname);
			ALERT( at_aiconsole, "Spawning of %s is blocked by %s\n", STRING(m_iszMonsterClassname), blockerName );
			return MONSTERMAKER_BLOCKED;
		}
	}

	if (FBitSet(pev->spawnflags, SF_MONSTERMAKER_ALIGN_TO_PLAYER))
	{
		float minDist = 10000.0f;
		CBaseEntity* foundPlayer = NULL;
		for (int i = 1; i <= gpGlobals->maxClients; ++i) {
			CBaseEntity* player = UTIL_PlayerByIndex(i);
			if (player && player->IsPlayer() && player->IsAlive()) {
				const float dist = (pev->origin - player->pev->origin).Length();
				if (dist < minDist) {
					minDist = dist;
					foundPlayer = player;
				}
			}
		}
		if (foundPlayer) {
			placeAngles = Vector(0, UTIL_VecToYaw(foundPlayer->pev->origin - placePosition), 0);
		}
	}

	if (m_iMaxYawDeviation)
	{
		const int deviation = RANDOM_LONG(0, m_iMaxYawDeviation);
		if (RANDOM_LONG(0,1)) {
			placeAngles.y += deviation;
		} else {
			placeAngles.y -= deviation;
		}
		placeAngles.y = UTIL_AngleMod(placeAngles.y);
	}

	return 0;
}

CBaseEntity* CMonsterMaker::SpawnMonster(const Vector &placePosition, const Vector &placeAngles)
{
	if (!CheckMonsterClassname())
		return 0;

	edict_t *pent = CREATE_NAMED_ENTITY( m_iszMonsterClassname );
	if( FNullEnt( pent ) )
	{
		return 0;
	}

	entvars_t *pevCreate = VARS( pent );
	pevCreate->origin = placePosition;
	pevCreate->angles = placeAngles;
	SetBits( pevCreate->spawnflags, SF_MONSTER_FALL_TO_GROUND );

	if (m_childKeyCount > 0)
	{
		const char* classname = STRING(pevCreate->classname);
		KeyValueData kvd;
		kvd.szClassName = classname;
		for (int i=0; i<m_childKeyCount; ++i)
		{
			kvd.szKeyName = STRING(m_childKeys[i]);
			kvd.szValue = STRING(m_childValues[i]);
			kvd.fHandled = FALSE;

			// don't change classname
			if (FStrEq(kvd.szKeyName, "classname"))
			{
				continue;
			}

			DispatchKeyValue(pent, &kvd);
		}
	}

	pevCreate->body = pev->body;
	pevCreate->skin = pev->skin;
	pevCreate->health = pev->health;
	pevCreate->scale = pev->scale;
	if (!FStringNull(m_customModel))
		pevCreate->model = m_customModel;

	CBaseEntity* pEntity = CBaseEntity::Instance(pent);
	if (pEntity)
	{
		pEntity->m_entTemplate = m_entTemplate;
		pEntity->m_soundList = m_soundList;
	}
	CBaseMonster* createdMonster = pEntity ? pEntity->MyMonsterPointer() : NULL;
	if (createdMonster)
	{
		// Children hit monsterclip brushes
		if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_MONSTERCLIP ))
			SetBits( pevCreate->spawnflags, SF_MONSTER_HITMONSTERCLIP );

		if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_PRISONER ) )
			SetBits( pevCreate->spawnflags, SF_MONSTER_PRISONER );
		if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_PREDISASTER ) )
			SetBits( pevCreate->spawnflags, SF_MONSTER_PREDISASTER );
		if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_DONT_DROP_GUN ) )
			SetBits( pevCreate->spawnflags, SF_MONSTER_DONT_DROP_GUN);
		if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_WAIT_UNTIL_PROVOKED ) )
			SetBits( pevCreate->spawnflags, SF_MONSTER_WAIT_UNTIL_PROVOKED );
		if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_SPECIAL_FLAG ) )
			SetBits( pevCreate->spawnflags, SF_MONSTER_SPECIAL_FLAG );
		if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_NONSOLID_CORPSE ) )
			SetBits( pevCreate->spawnflags, SF_MONSTER_NONSOLID_CORPSE );
		if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_ACT_IN_NON_PVS ) )
			SetBits( pevCreate->spawnflags, SF_MONSTER_ACT_OUT_OF_PVS );
		if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_IGNORE_PLAYER_PUSHING ) )
			SetBits( pevCreate->spawnflags, SF_MONSTER_IGNORE_PLAYER_PUSH );
		if (m_gag > 0)
			SetBits(pevCreate->spawnflags, SF_MONSTER_GAG);
		pevCreate->weapons = pev->weapons;

		if (m_iClass)
			createdMonster->m_iClass = m_iClass;
		createdMonster->m_reverseRelationship = m_reverseRelationship;
		createdMonster->m_displayName = m_displayName;
		createdMonster->SetMyBloodColor(m_bloodColor);
		createdMonster->SetMyFieldOfView(m_flFieldOfView);
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
		createdMonster->m_prisonerTo = m_prisonerTo;
		createdMonster->m_ignoredBy = m_ignoredBy;
		createdMonster->m_freeRoam = m_freeRoam;
		createdMonster->m_activeAfterCombat = m_activeAfterCombat;
		createdMonster->m_sizeForGrapple = m_sizeForGrapple;
		createdMonster->m_gibPolicy = m_gibPolicy;

		createdMonster->SetHead(m_iHead);

		CDeadMonster* deadMonster = createdMonster->MyDeadMonsterPointer();
		if (deadMonster)
		{
			deadMonster->m_iPose = m_iPose;
		}

		CFollowingMonster* pFollowingMonster = createdMonster->MyFollowingMonsterPointer();
		if (pFollowingMonster)
		{
			pFollowingMonster->m_followFailPolicy = m_followFailPolicy;
			pFollowingMonster->m_followagePolicy = m_followagePolicy;
		}

		CTalkMonster* pTalkMonster = createdMonster->MyTalkMonsterPointer();
		if (pTalkMonster)
		{
			if (!FStringNull(m_iszUse))
			{
				pTalkMonster->m_iszUse = m_iszUse;
			}

			if (!FStringNull(m_iszUnUse))
			{
				pTalkMonster->m_iszUnUse = m_iszUnUse;
			}

			if (!FStringNull(m_iszDecline))
			{
				pTalkMonster->m_iszDecline = m_iszDecline;
			}
		}
	}

	if (DispatchSpawn( ENT( pevCreate ) ) == -1)
	{
		ALERT( at_console, "Game rejected to spawn '%s' (probably not enabled)\n", STRING(m_iszMonsterClassname) );
		REMOVE_ENTITY(ENT(pevCreate));
		return 0;
	}

	pevCreate->owner = edict();
	// Disable until proper investigation
#if 0
	if (m_notSolid > 0)
	{
		pevCreate->solid = SOLID_NOT;
	}
#endif

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
		// NOTE: in Spirit activator is monster.
		// We keep original Half-Life behavior by default, but it can be configured.
		CBaseEntity* pActivator = this;
		switch (m_targetActivator) {
		case MMA_NO:
			pActivator = NULL;
			break;
		case MMA_MAKER:
			pActivator = this;
			break;
		case MMA_MONSTER:
			pActivator = createdMonster;
			break;
		case MMA_FORWARD:
			pActivator = m_hActivator;
			break;
		case MMA_DEFAULT:
		default:
			break;
		}
		// delay already overloaded for this entity, so can't call SUB_UseTargets()
		FireTargets( STRING( pev->target ), pActivator, this );
	}

	return CBaseEntity::Instance(pevCreate);
}

void CMonsterMaker::StartWarpballEffect(const Vector &vecPosition, edict_t* warpballSoundEnt)
{
	const char* warpballName = STRING(WarpballName());
	const WarpballTemplate* warpballTemplate = FindWarpballTemplate(warpballName, STRING(m_iszMonsterClassname));
	if (warpballTemplate)
	{
		PlayWarpballEffect(*warpballTemplate, vecPosition, warpballSoundEnt);
		return;
	}

	CBaseEntity* foundEntity = UTIL_FindEntityByTargetname(0, warpballName);
	if ( foundEntity && (FClassnameIs(foundEntity->pev, "env_warpball") || FClassnameIs(foundEntity->pev, "env_xenmaker")))
	{
		edict_t* prevInflictor = foundEntity->pev->dmg_inflictor;
		foundEntity->pev->vuser1 = vecPosition;
		foundEntity->pev->dmg_inflictor = warpballSoundEnt;
		foundEntity->Use(this, this, USE_SET, 0.0f);
		foundEntity->pev->vuser1 = g_vecZero;
		foundEntity->pev->dmg_inflictor = prevInflictor;
	}
	else
	{
		ALERT(at_error, "template %s for %s is not env_warpball or does not exist\n", STRING(pev->message), STRING(pev->classname));
	}
}

//=========================================================
// MakeMonster-  this is the code that drops the monster
//=========================================================
int CMonsterMaker::MakeMonster( void )
{
	if( m_iMaxLiveChildren > 0 && m_cLiveChildren + m_delayedCount >= m_iMaxLiveChildren )
	{
		// not allowed to make a new one yet. Too many live ones out right now.
		return MONSTERMAKER_LIMIT;
	}

	Vector minHullSize = Vector( -34, -34, 0 );
	Vector maxHullSize = Vector( 34, 34, 0 );

	Vector realMinHullSize;
	Vector realMaxHullSize;
	GetRealHullSizes(realMinHullSize, realMaxHullSize);

	if (FBitSet(pev->spawnflags, SF_MONSTERMAKER_AUTOSIZEBBOX))
	{
		minHullSize = realMinHullSize;
		maxHullSize = realMaxHullSize;
	}

	Vector placePosition, placeAngles;
	edict_t *warpballSoundEnt = NULL;

	string_t warpballName = WarpballName();
	float spawnDelay = m_spawnDelay;
	float verticalShift = 0.0f;
	bool templateShiftDefined = false;

	if (!FStringNull(warpballName))
	{
		const WarpballTemplate* w = FindWarpballTemplate(STRING(warpballName), STRING(m_iszMonsterClassname));
		if (w)
		{
			if (spawnDelay == 0.0f)
				spawnDelay = w->spawnDelay;
			templateShiftDefined = w->position.IsDefined();
			if (templateShiftDefined)
			{
				verticalShift = w->position.verticalShift;
			}
		}
	}

	const int spotResult = CalculateSpot(minHullSize, maxHullSize, placePosition, placeAngles, warpballSoundEnt, spawnDelay);
	if (spotResult != 0)
		return spotResult;

	if (spawnDelay <= 0.0f)
	{
		CBaseEntity* createdEntity = SpawnMonster(placePosition, placeAngles);
		if (!createdEntity)
			return MONSTERMAKER_NULLENTITY;

		if (!FStringNull(warpballName))
		{
			Vector vecWarpPosition;
			if (pev->impulse == 0)
			{
				if (templateShiftDefined)
				{
					vecWarpPosition = placePosition + Vector(0.0f, 0.0f, verticalShift);
				}
				else
				{
					vecWarpPosition = g_modFeatures.warpball_at_monster_center ? createdEntity->Center() : createdEntity->pev->origin;
				}
			}
			else if (pev->impulse < 0)
			{
				vecWarpPosition = createdEntity->pev->origin;
			}
			else
			{
				vecWarpPosition = createdEntity->Center();
			}
			StartWarpballEffect(vecWarpPosition, warpballSoundEnt);
		}
	}
	else
	{
		if (!m_childIsValid)
			return MONSTERMAKER_NULLENTITY;
		CMonsterMakerHull* pHull = CMonsterMakerHull::SelfCreate(this, placePosition, placeAngles, minHullSize, maxHullSize, spawnDelay);
		if (!pHull)
			return MONSTERMAKER_NULLENTITY;
		m_delayedCount++;

		if (!FStringNull(warpballName))
		{
			Vector vecWarpPosition;
			if (pev->impulse == 0)
			{
				if (templateShiftDefined)
				{
					vecWarpPosition = placePosition + Vector(0.0f, 0.0f, verticalShift);
				}
				else
				{
					vecWarpPosition = g_modFeatures.warpball_at_monster_center ? placePosition + (realMinHullSize + realMaxHullSize) * 0.5f : placePosition;
				}
			}
			else if (pev->impulse < 0)
			{
				vecWarpPosition = placePosition;
			}
			else
			{
				vecWarpPosition = placePosition + (realMinHullSize + realMaxHullSize) * 0.5f;
			}
			StartWarpballEffect(vecWarpPosition, warpballSoundEnt);
		}
	}

	return MONSTERMAKER_SPAWNED;
}

//=========================================================
// CyclicUse - drops one monster from the monstermaker
// each time we call this.
//=========================================================
void CMonsterMaker::CyclicUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_hActivator = pActivator;

	if (MakeMonster() == MONSTERMAKER_BLOCKED)
	{
		if (FBitSet(pev->spawnflags, SF_MONSTERMAKER_CYCLIC_BACKLOG))
		{
			m_cyclicBacklogSize++;
			if (m_delayAfterBlocked > 0)
				pev->nextthink = gpGlobals->time + m_delayAfterBlocked;
			else
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

	m_hActivator = pActivator;

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

	const int result = MakeMonster();
	if (result == MONSTERMAKER_BLOCKED)
	{
		if (m_delayAfterBlocked > 0)
			pev->nextthink = gpGlobals->time + m_delayAfterBlocked;
	}
	else if (result == MONSTERMAKER_NULLENTITY)
	{
		ReportNullEntity();
		SetThink(NULL); // I can't spawn it anyway, so prevent further spamming to console
	}
}

void CMonsterMaker::CyclicBacklogThink()
{
	const int result = MakeMonster();
	if (result == MONSTERMAKER_SPAWNED)
	{
		m_cyclicBacklogSize--;
	}
	else if (result == MONSTERMAKER_NULLENTITY)
	{
		ReportNullEntity();
	}
	if (m_cyclicBacklogSize > 0)
		pev->nextthink = gpGlobals->time + m_flDelay;
}

void CMonsterMaker::ReportNullEntity()
{
	ALERT ( at_console, "NULL Ent %s in %s!\n", STRING(m_iszMonsterClassname), STRING(pev->classname) );
}

//=========================================================
//=========================================================
void CMonsterMaker::DeathNotice( entvars_t *pevChild )
{
	// ok, we've gotten the deathnotice from our child, now clear out its owner if we don't want it to fade.
	if (m_cLiveChildren > 0)
	{
		m_cLiveChildren--;
		ALERT(at_aiconsole, "%s DeathNotice: %d live children left\n", STRING(pev->classname), m_cLiveChildren);
	}
	else
	{
		ALERT(at_aiconsole, "Impossible situation: %s got DeathNotice from %s when live children count is 0!\n", STRING(pev->classname), STRING(pevChild->classname));
	}

	if( !m_fFadeChildren )
	{
		pevChild->owner = NULL;
	}

	if (m_cNumMonsters == 0 && m_cLiveChildren == 0)
	{
		SetThink(&CMonsterMaker::SUB_Remove);
		pev->nextthink = gpGlobals->time;
	}
}
