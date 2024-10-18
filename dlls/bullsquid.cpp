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
// bullsquid - big, spotty tentacle-mouthed meanie.
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
#include	"scripted.h"
#include	"game.h"
#include	"bullsquid.h"
#include	"common_soundscripts.h"
#include	"visuals_utils.h"

const NamedVisual sharedTinySpitVisual = BuildVisual("Bullsquid.TinySpitBase")
		.Model("sprites/tinyspit.spr");

// Slow big poisonous ball as alternative range attack for bullsquid
#define FEATURE_BULLSQUID_TOXICSPIT 1

#define		SQUID_SPRINT_DIST	256.0f // how close the squid has to get before starting to sprint and refusing to swerve

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_SQUID_HURTHOP = LAST_COMMON_SCHEDULE + 1,
	SCHED_SQUID_SMELLFOOD,
	SCHED_SQUID_SEECRAB,
	SCHED_SQUID_EAT,
	SCHED_SQUID_SNIFF_AND_EAT,
	SCHED_SQUID_WALLOW
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_SQUID_HOPTURN = LAST_COMMON_TASK + 1
};

//=========================================================
// Bullsquid's spit projectile
//=========================================================

LINK_ENTITY_TO_CLASS( squidspit, CSquidSpit )

TYPEDESCRIPTION	CSquidSpit::m_SaveData[] =
{
	DEFINE_FIELD( CSquidSpit, m_maxFrame, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CSquidSpit, CBaseEntity )


const NamedVisual CSquidSpit::spitVisual = BuildVisual::Animated("Bullsquid.Spit")
		.Model("sprites/bigspit.spr")
		.RenderMode(kRenderTransAlpha)
		.Alpha(255)
		.Scale(0.5f);

const NamedVisual CSquidSpit::fleckVisual = BuildVisual::Spray("Bullsquid.Fleck").Mixin(&sharedTinySpitVisual);

void CSquidSpit::Spawn( void )
{
	SpawnHelper("squidspit", spitVisual);
}

void CSquidSpit::Precache()
{
	RegisterVisual(spitVisual);
	RegisterAndPrecacheSoundScript(spitTouchSoundScript, NPC::spitTouchSoundScript);
	RegisterAndPrecacheSoundScript(spitHitSoundScript, NPC::spitHitSoundScript);
	RegisterVisual(fleckVisual);// client side spittle.
}

void CSquidSpit::SpawnHelper(const char *className, const char* spitVisualName)
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( className );
	pev->solid = SOLID_BBOX;

	ApplyVisual(GetVisual(spitVisualName));
	pev->frame = 0;

	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	m_maxFrame = MODEL_FRAMES( pev->modelindex ) - 1;
}

void CSquidSpit::Animate( void )
{
	pev->nextthink = gpGlobals->time + 0.1f;
	pev->frame = AnimateWithFramerate(pev->frame, m_maxFrame, pev->framerate);
}

void CSquidSpit::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, EntityOverrides entityOverrides )
{
	CSquidSpit *pSpit = GetClassPtr( (CSquidSpit *)NULL );
	pSpit->AssignEntityOverrides(entityOverrides);
	pSpit->Spawn();

	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->angles = UTIL_VecToAngles(vecVelocity);
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT( pevOwner );

	pSpit->SetThink( &CSquidSpit::Animate );
	pSpit->pev->nextthink = gpGlobals->time + 0.1f;
}

void CSquidSpit::Touch( CBaseEntity *pOther )
{
	TraceResult tr;

	EmitSoundScript(spitTouchSoundScript);
	EmitSoundScript(spitHitSoundScript);

	if( !pOther->pev->takedamage )
	{
		// make a splat on the wall
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_DecalTrace( &tr, DECAL_SPIT1 + RANDOM_LONG( 0, 1 ) );

		SendSpray(tr.vecEndPos, tr.vecPlaneNormal, GetVisual(fleckVisual), 5, 30, 80);
	}
	else
	{
		CBaseMonster* owner = GetMonsterPointer( pev->owner );
		entvars_t* pevAttacker = owner ? owner->pev : pev;
		pOther->TakeDamage( pev, pevAttacker, gSkillData.bullsquidDmgSpit, DMG_GENERIC );
	}

	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

// Bullsquid big slow poisonous spit

LINK_ENTITY_TO_CLASS( squidtoxicspit, CSquidToxicSpit )

TYPEDESCRIPTION	CSquidToxicSpit::m_SaveData[] =
{
	DEFINE_FIELD( CSquidToxicSpit, m_maxFrame, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CSquidToxicSpit, CBaseEntity )

const NamedSoundScript CSquidToxicSpit::acidSoundScript = {
	CHAN_VOICE,
	{"bullchicken/bc_acid2.wav"},
	IntRange(90, 110),
	"Bullsquid.ToxicSpitTouch"
};

const NamedSoundScript CSquidToxicSpit::spithitSoundScript = {
	CHAN_WEAPON,
	{"bullchicken/bc_spithit2.wav", "bullchicken/bc_spithit3.wav"},
	IntRange(90, 110),
	"Bullsquid.ToxicSpitHit"
};

const NamedVisual CSquidToxicSpit::toxicSpitVisual = BuildVisual::Animated("Bullsquid.ToxicSpit")
		.Model("sprites/cnt1.spr")
		.RenderProps(kRenderTransAdd, Color(110, 120, 0), 228)
		.Scale(0.8f);

const NamedVisual CSquidToxicSpit::fleckVisual = BuildVisual::Spray("Bullsquid.ToxicFleck").Mixin(&sharedTinySpitVisual);

const NamedVisual CSquidToxicSpit::particleVisual = BuildVisual("Bullsquid.ToxicParticle")
		.Model("sprites/glow01.spr")
		.RenderProps(kRenderGlow, Color(80, 160, 0), 255, kRenderFxNoDissipation)
		.Scale(0.3f)
		.Life(0.1f);

void CSquidToxicSpit::Spawn( void )
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( "bigsquidspit" );
	pev->solid = SOLID_BBOX;

	ApplyVisual(GetVisual(toxicSpitVisual));
	pev->frame = 0;

	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	m_maxFrame = MODEL_FRAMES( pev->modelindex ) - 1;
}

void CSquidToxicSpit::Precache()
{
	RegisterVisual(toxicSpitVisual);

	RegisterAndPrecacheSoundScript(acidSoundScript);
	RegisterAndPrecacheSoundScript(spithitSoundScript);

	RegisterVisual(fleckVisual);
	RegisterVisual(particleVisual);
}

extern int gmsgSpriteTrail;

void CSquidToxicSpit::Animate( void )
{
	CBaseEntity* pEntity = NULL;
	CBaseMonster* spitOwner = GetSpitOwner();
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, 32)) != NULL) {
		if ( pEntity != spitOwner && pEntity->MyMonsterPointer() && !FClassnameIs(pEntity->pev, "monster_bullchicken")) {
			if (!spitOwner || spitOwner->IRelationship(pEntity) >= R_DL) {
				pEntity->TakeDamage(pev, spitOwner ? spitOwner->pev : pev, gSkillData.bullsquidDmgToxicPoison, DMG_POISON | DMG_TIMEDNONLETHAL | DMG_IGNORE_ARMOR);
			}
		}
	}

	if (pev->dmgtime < gpGlobals->time)
	{
		Vector end = pev->origin + pev->velocity.Normalize() * 16.0f;
		end.z += 16.0f;

		const Visual* visual = GetVisual(particleVisual);
		if (visual->modelIndex)
		{
			MESSAGE_BEGIN( MSG_PVS, gmsgSpriteTrail, pev->origin );
				WRITE_VECTOR( pev->origin );	// start
				WRITE_VECTOR( end );	// end
				WRITE_SHORT( visual->modelIndex );	// model
				WRITE_BYTE( 3 );			// count
				WRITE_BYTE( RandomizeNumberFromRange(visual->life)*10 );			// life in 0.1s
				WRITE_BYTE( (int)(RandomizeNumberFromRange(visual->scale) * 10) );			// scale in 0.1
				WRITE_BYTE( 20 );			// velocity along vector in 10's
				WRITE_BYTE( 20 );			// randomness of velocity in 10's
				WRITE_BYTE( visual->rendermode );
				WRITE_COLOR( visual->rendercolor );
				WRITE_BYTE( visual->renderamt );
				WRITE_BYTE( visual->renderfx );
				WRITE_BYTE( 10 ); // random extra life in 0.1s
			MESSAGE_END();
		}

		pev->dmgtime = gpGlobals->time + 0.2f;
	}

	pev->nextthink = gpGlobals->time + 0.1;
	pev->frame = AnimateWithFramerate(pev->frame, m_maxFrame, pev->framerate);
}

void CSquidToxicSpit::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, EntityOverrides entityOverrides )
{
	CSquidToxicSpit *pSpit = GetClassPtr( (CSquidToxicSpit *)NULL );
	pSpit->AssignEntityOverrides(entityOverrides);
	pSpit->Spawn();

	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT( pevOwner );

	pSpit->SetThink( &CSquidToxicSpit::Animate );
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void CSquidToxicSpit::Touch( CBaseEntity *pOther )
{
	TraceResult tr;

	EmitSoundScript(acidSoundScript);
	EmitSoundScript(spithitSoundScript);

	if( !pOther->pev->takedamage )
	{
		// make a splat on the wall
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_DecalTrace( &tr, DECAL_SPIT1 + RANDOM_LONG( 0, 1 ) );

		SendSpray(tr.vecEndPos, tr.vecPlaneNormal, GetVisual(fleckVisual), 8, 15, 100);
	}
	else if (pev->owner == pOther->edict())
	{
		ALERT(at_aiconsole, "%s caught himself in big spit\n", STRING(pev->classname));
		return;
	}
	else
	{
		CBaseMonster* spitOwner = GetSpitOwner();
		if (!spitOwner || spitOwner->IRelationship(pOther) >= R_DL) {
			entvars_t* pevAttacker = spitOwner ? spitOwner->pev : pev;
			pOther->TakeDamage( pev, pevAttacker, gSkillData.bullsquidDmgToxicImpact, DMG_ACID );
			pOther->TakeDamage( pev, pevAttacker, gSkillData.bullsquidDmgToxicPoison, DMG_POISON | DMG_TIMEDNONLETHAL | DMG_IGNORE_ARMOR);
		}
	}

	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

CBaseMonster* CSquidToxicSpit::GetSpitOwner() {
	if (!FNullEnt(pev->owner))
		return GetMonsterPointer(pev->owner);
	return 0;
}

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		BSQUID_AE_SPIT		( 1 )
#define		BSQUID_AE_BITE		( 2 )
#define		BSQUID_AE_BLINK		( 3 )
#define		BSQUID_AE_TAILWHIP	( 4 )
#define		BSQUID_AE_HOP		( 5 )
#define		BSQUID_AE_THROW		( 6 )

//=========================================================
// CBullsquid
//=========================================================
class CBullsquid : public CBaseMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	void SetYawSpeed(void);
	int  DefaultISoundMask(void);
	virtual int  DefaultClassify(void);
	const char* DefaultDisplayName() { return "Bullsquid"; }
	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);
	virtual void IdleSound(void);
	virtual void PainSound(void);
	virtual void DeathSound(void);
	virtual void AlertSound(void);
	virtual void AttackSound(bool bigSpit);
	virtual void StartTask(Task_t *pTask);
	void RunTask(Task_t *pTask);
	virtual BOOL CheckMeleeAttack1(float flDot, float flDist);
	virtual BOOL CheckMeleeAttack2(float flDot, float flDist);
	virtual BOOL CheckRangeAttack1(float flDot, float flDist);
	virtual void RunAI(void);
	BOOL FValidateHintType(short sHint);
	Schedule_t *GetSchedule(void);
	Schedule_t *GetScheduleOfType(int Type);
	virtual int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	virtual int IRelationship(CBaseEntity *pTarget);
	virtual int IgnoreConditions(void);
	MONSTERSTATE GetIdealState(void);

	int	Save(CSave &save);
	int Restore(CRestore &restore);

	CUSTOM_SCHEDULES
	static TYPEDESCRIPTION m_SaveData[];

	virtual int DefaultSizeForGrapple() { return GRAPPLE_MEDIUM; }
	bool IsDisplaceable() { return true; }
	Vector DefaultMinHullSize() { return Vector( -32.0f, -32.0f, 0.0f ); }
	Vector DefaultMaxHullSize() { return Vector( 32.0f, 32.0f, 64.0f ); }

	BOOL m_fCanThreatDisplay;// this is so the squid only does the "I see a headcrab!" dance one time. 

	float m_flLastHurtTime;// we keep track of this, because if something hurts a squid, it will forget about its love of headcrabs for a while.
	float m_flNextSpitTime;// last time the bullsquid used the spit attack.
	float m_flNextHopTime;

	static const NamedSoundScript idleSoundScript;
	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript painSoundScript;
	static const NamedSoundScript dieSoundScript;
	static const NamedSoundScript attackGrowlSoundScript;
	static const NamedSoundScript attackSoundScript;
	static const NamedSoundScript attackToxicSoundScript;
	static const NamedSoundScript biteSoundScript;

	static const NamedVisual tinySpitVisual;
	static const NamedVisual toxicTinySpitVisual;
};

LINK_ENTITY_TO_CLASS( monster_bullchicken, CBullsquid )

TYPEDESCRIPTION	CBullsquid::m_SaveData[] =
{
	DEFINE_FIELD( CBullsquid, m_fCanThreatDisplay, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBullsquid, m_flLastHurtTime, FIELD_TIME ),
	DEFINE_FIELD( CBullsquid, m_flNextSpitTime, FIELD_TIME ),
	DEFINE_FIELD( CBullsquid, m_flNextHopTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CBullsquid, CBaseMonster )

#define SQUID_ATTN_IDLE	1.5f

const NamedSoundScript CBullsquid::idleSoundScript = {
	CHAN_VOICE,
	{"bullchicken/bc_idle1.wav", "bullchicken/bc_idle2.wav", "bullchicken/bc_idle3.wav", "bullchicken/bc_idle4.wav", "bullchicken/bc_idle5.wav"},
	1.0f,
	SQUID_ATTN_IDLE,
	"Bullsquid.Idle"
};

const NamedSoundScript CBullsquid::alertSoundScript = {
	CHAN_VOICE,
	{ "bullchicken/bc_idle1.wav", "bullchicken/bc_idle2.wav" },
	IntRange(140, 160),
	"Bullsquid.Alert"
};

const NamedSoundScript CBullsquid::painSoundScript = {
	CHAN_VOICE,
	{"bullchicken/bc_pain1.wav", "bullchicken/bc_pain2.wav", "bullchicken/bc_pain3.wav", "bullchicken/bc_pain4.wav"},
	IntRange(85, 120),
	"Bullsquid.Pain"
};

const NamedSoundScript CBullsquid::dieSoundScript = {
	CHAN_VOICE,
	{"bullchicken/bc_die1.wav", "bullchicken/bc_die2.wav", "bullchicken/bc_die3.wav"},
	"Bullsquid.Die"
};

const NamedSoundScript CBullsquid::attackGrowlSoundScript = {
	CHAN_VOICE,
	{"bullchicken/bc_attackgrowl.wav", "bullchicken/bc_attackgrowl2.wav", "bullchicken/bc_attackgrowl3.wav"},
	"Bullsquid.Growl"
};

const NamedSoundScript CBullsquid::attackSoundScript = {
	CHAN_WEAPON,
	{"bullchicken/bc_attack2.wav", "bullchicken/bc_attack3.wav"},
	"Bullsquid.Attack"
};

const NamedSoundScript CBullsquid::attackToxicSoundScript = {
	CHAN_WEAPON,
	{"bullchicken/bc_attack1.wav"},
	"Bullsquid.AttackToxic"
};

const NamedSoundScript CBullsquid::biteSoundScript = {
	CHAN_WEAPON,
	{"bullchicken/bc_bite2.wav", "bullchicken/bc_bite3.wav"},
	IntRange(90, 110),
	"Bullsquid.Bite"
};

const NamedVisual CBullsquid::tinySpitVisual = BuildVisual::Spray("Bullsquid.TinySpit").Mixin(&sharedTinySpitVisual);

const NamedVisual CBullsquid::toxicTinySpitVisual = BuildVisual::Spray("Bullsquid.ToxicTinySpit").Mixin(&sharedTinySpitVisual);

//=========================================================
// IgnoreConditions 
//=========================================================
int CBullsquid::IgnoreConditions( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if( gpGlobals->time - m_flLastHurtTime <= 20.0f )
	{
		// haven't been hurt in 20 seconds, so let the squid care about stink. 
		iIgnore = bits_COND_SMELL | bits_COND_SMELL_FOOD;
	}

	if( m_hEnemy != 0 )
	{
		if( FClassnameIs( m_hEnemy->pev, "monster_headcrab" ) )
		{
			// (Unless after a tasty headcrab)
			iIgnore = bits_COND_SMELL | bits_COND_SMELL_FOOD;
		}
	}

	return iIgnore;
}

//=========================================================
// IRelationship - overridden for bullsquid so that it can
// be made to ignore its love of headcrabs for a while.
//=========================================================
int CBullsquid::IRelationship( CBaseEntity *pTarget )
{
	if( gpGlobals->time - m_flLastHurtTime < 5.0f && IDefaultRelationship(pTarget) >= R_DL && FClassnameIs( pTarget->pev, "monster_headcrab" ) )
	{
		// if squid has been hurt in the last 5 seconds, and is getting relationship for a headcrab, 
		// tell squid to disregard crab. 
		return R_NO;
	}

	return CBaseMonster::IRelationship( pTarget );
}

//=========================================================
// TakeDamage - overridden for bullsquid so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CBullsquid::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	float flDist;
	Vector vecApex;

	// if the squid is running, has an enemy, was hurt by the enemy, hasn't been hurt in the last 3 seconds, and isn't too close to the enemy,
	// it will swerve. (whew).
	if( m_hEnemy != 0 && IsMoving() && pevAttacker == m_hEnemy->pev && gpGlobals->time - m_flLastHurtTime > 3.0f )
	{
		flDist = ( pev->origin - m_hEnemy->pev->origin ).Length2D();

		if( flDist > SQUID_SPRINT_DIST )
		{
			flDist = ( pev->origin - m_Route[m_iRouteIndex].vecLocation ).Length2D();// reusing flDist.

			if( FTriangulate( pev->origin, m_Route[m_iRouteIndex].vecLocation, flDist * 0.5f, m_hEnemy, &vecApex ) )
			{
				InsertWaypoint( vecApex, bits_MF_TO_DETOUR | bits_MF_DONT_SIMPLIFY );
			}
		}
	}

	if( pevAttacker && !FClassnameIs( pevAttacker, "monster_headcrab" ) )
	{
		// don't forget about headcrabs if it was a headcrab that hurt the squid.
		m_flLastHurtTime = gpGlobals->time;
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CBullsquid::CheckRangeAttack1( float flDot, float flDist )
{
	if( IsMoving() && flDist >= 512.0f )
	{
		// squid will far too far behind if he stops running to spit at this distance from the enemy.
		return FALSE;
	}

	if( flDist > 64.0f && flDist <= 784.0f && flDot >= 0.5f && gpGlobals->time >= m_flNextSpitTime )
	{
		if( m_hEnemy != 0 )
		{
			if( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 256.0f )
			{
				// don't try to spit at someone up really high or down really low.
				return FALSE;
			}
		}

		if( IsMoving() )
		{
			// don't spit again for a long time, resume chasing enemy.
			m_flNextSpitTime = gpGlobals->time + 5.0f;
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextSpitTime = gpGlobals->time + 0.5f;
		}

		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckMeleeAttack1 - bullsquid is a big guy, so has a longer
// melee range than most monsters. This is the tailwhip attack
//=========================================================
BOOL CBullsquid::CheckMeleeAttack1( float flDot, float flDist )
{
	if( m_hEnemy->pev->health <= gSkillData.bullsquidDmgWhip && flDist <= 85.0f && flDot >= 0.7f )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckMeleeAttack2 - bullsquid is a big guy, so has a longer
// melee range than most monsters. This is the bite attack.
// this attack will not be performed if the tailwhip attack
// is valid.
//=========================================================
BOOL CBullsquid::CheckMeleeAttack2( float flDot, float flDist )
{
	if( flDist <= 85.0f && flDot >= 0.7f && !HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )		// The player & bullsquid can be as much as their bboxes 
	{										// apart (48 * sqrt(3)) and he can still attack (85 is a little more than 48*sqrt(3))
		return TRUE;
	}
	return FALSE;
}

//=========================================================
//  FValidateHintType 
//=========================================================
BOOL CBullsquid::FValidateHintType( short sHint )
{
	size_t i;

	static short sSquidHints[] =
	{
		HINT_WORLD_HUMAN_BLOOD,
	};

	for( i = 0; i < ARRAYSIZE( sSquidHints ); i++ )
	{
		if( sSquidHints[i] == sHint )
		{
			return TRUE;
		}
	}

	ALERT( at_aiconsole, "%s couldn't validate hint type\n", STRING(pev->classname) );
	return FALSE;
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CBullsquid::DefaultISoundMask( void )
{
	return	bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_CARCASS |
		bits_SOUND_MEAT |
		bits_SOUND_GARBAGE |
		bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CBullsquid::DefaultClassify( void )
{
	return CLASS_ALIEN_PREDATOR;
}

//=========================================================
// IdleSound 
//=========================================================

void CBullsquid::IdleSound( void )
{
	EmitSoundScript(idleSoundScript);
}

//=========================================================
// PainSound 
//=========================================================
void CBullsquid::PainSound( void )
{
	EmitSoundScript(painSoundScript);
}

//=========================================================
// AlertSound
//=========================================================
void CBullsquid::AlertSound( void )
{
	EmitSoundScript(alertSoundScript);
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBullsquid::SetYawSpeed( void )
{
	int ys = 0;

	switch( m_Activity )
	{
	case ACT_WALK:
		ys = 90;
		break;
	case ACT_RUN:
		ys = 90;
		break;
	case ACT_IDLE:
		ys = 90;
		break;
	case ACT_RANGE_ATTACK1:
		ys = 90;
		break;
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
void CBullsquid::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case BSQUID_AE_SPIT:
			{
				UTIL_MakeVectors( pev->angles );

				// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
				// we should be able to read the position of bones at runtime for this info.
				const Vector vecSpitOffset = ( gpGlobals->v_right * 8.0f + gpGlobals->v_forward * 37.0f + gpGlobals->v_up * 23.0f );
				const Vector vecSpitOrigin = ( pev->origin + vecSpitOffset );

				float dirRandomDeviation = 0.05f;
				if (g_iSkillLevel == SKILL_HARD)
					dirRandomDeviation = 0.01f;
				else if (g_iSkillLevel == SKILL_MEDIUM)
					dirRandomDeviation = 0.03f;
				float distanceToEnemy;

				const Vector vecSpitDir = SpitAtEnemy(vecSpitOrigin, dirRandomDeviation, &distanceToEnemy);

				bool toxicSpit = false;
#if FEATURE_BULLSQUID_TOXICSPIT
				if (gSkillData.bullsquidToxicity > 0.0f && RANDOM_LONG(0,1))
				{
					if (distanceToEnemy < 400) {
						toxicSpit = true;
					}
				}
#endif

				// do stuff for this event.
				AttackSound(toxicSpit);

				const Visual* visual = toxicSpit ? GetVisual(toxicTinySpitVisual) : GetVisual(tinySpitVisual);
				// spew the spittle temporary ents.
				SendSpray(vecSpitOrigin, vecSpitDir, visual, 15, 210, 25);

				if (toxicSpit) {
					CSquidToxicSpit::Shoot(pev, vecSpitOrigin, vecSpitDir * CSquidToxicSpit::SpitSpeed(), GetProjectileOverrides());
				} else {
					CSquidSpit::Shoot( pev, vecSpitOrigin, vecSpitDir * CSquidSpit::SpitSpeed(), GetProjectileOverrides() );
				}
			}
			break;
		case BSQUID_AE_BITE:
			{
				// SOUND HERE!
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.bullsquidDmgBite, DMG_SLASH );

				if( pHurt )
				{
					if (FBitSet(pHurt->pev->flags, FL_MONSTER|FL_CLIENT))
					{
						//pHurt->pev->punchangle.z = -15;
						//pHurt->pev->punchangle.x = -45;
						pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 100.0f;
						pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100.0f;
					}
				}
			}
			break;
		case BSQUID_AE_TAILWHIP:
			{
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.bullsquidDmgWhip, DMG_CLUB | DMG_ALWAYSGIB );

				if( pHurt ) 
				{
					if (FBitSet(pHurt->pev->flags, FL_MONSTER|FL_CLIENT))
					{
						pHurt->pev->punchangle.z = -20.0f;
						pHurt->pev->punchangle.x = 20.0f;
						pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 200.0f;
						pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100.0f;
					}
				}
			}
			break;
		case BSQUID_AE_BLINK:
			{
				// close eye. 
				pev->skin = 1;
			}
			break;
		case BSQUID_AE_HOP:
			{
				float flGravity = g_psv_gravity->value;

				// throw the squid up into the air on this frame.
				if( FBitSet( pev->flags, FL_ONGROUND ) )
				{
					pev->flags -= FL_ONGROUND;
				}

				// jump into air for 0.8 (24/30) seconds
				//pev->velocity.z += ( 0.875f * flGravity ) * 0.5f;
				pev->velocity.z += ( 0.625f * flGravity ) * 0.5f;
			}
			break;
		case BSQUID_AE_THROW:
			{
				// squid throws its prey IF the prey is a client. 
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, 0, 0 );

				if( pHurt )
				{
					// croonchy bite sound
					EmitSoundScript(biteSoundScript);

					//pHurt->pev->punchangle.x = RANDOM_LONG( 0, 34 ) - 5;
					//pHurt->pev->punchangle.z = RANDOM_LONG( 0, 49 ) - 25;
					//pHurt->pev->punchangle.y = RANDOM_LONG( 0, 89 ) - 45;
		
					// screeshake transforms the viewmodel as well as the viewangle. No problems with seeing the ends of the viewmodels.
					UTIL_ScreenShake( pHurt->pev->origin, 25.0f, 1.5f, 0.7f, 2.0f );

					if( pHurt->IsPlayer() )
					{
						UTIL_MakeVectors( pev->angles );
						pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 300.0f + gpGlobals->v_up * 300.0f;
					}
				}
			}
			break;
		default:
			CBaseMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CBullsquid::Spawn()
{
	Precache();

	SetMyModel( "models/bullsquid.mdl" );
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_GREEN );
	pev->effects = 0;
	SetMyHealth( gSkillData.bullsquidHealth );
	SetMyFieldOfView(0.2f);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	m_fCanThreatDisplay = TRUE;
	m_flNextSpitTime = gpGlobals->time;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBullsquid::Precache()
{
	PrecacheMyModel( "models/bullsquid.mdl" );
	PrecacheMyGibModel();

	UTIL_PrecacheOther("squidspit", GetProjectileOverrides());
#if FEATURE_BULLSQUID_TOXICSPIT
	UTIL_PrecacheOther("squidtoxicspit", GetProjectileOverrides()); // toxic spit projectile
#endif

	RegisterVisual(tinySpitVisual);
	RegisterVisual(toxicTinySpitVisual);

	RegisterAndPrecacheSoundScript(NPC::swishSoundScript);// because we use the basemonster SWIPE animation event

	RegisterAndPrecacheSoundScript(idleSoundScript);
	RegisterAndPrecacheSoundScript(alertSoundScript);
	RegisterAndPrecacheSoundScript(painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(attackGrowlSoundScript);

	RegisterAndPrecacheSoundScript(attackSoundScript);
	RegisterAndPrecacheSoundScript(attackToxicSoundScript);
	RegisterAndPrecacheSoundScript(biteSoundScript);
}

//=========================================================
// DeathSound
//=========================================================
void CBullsquid::DeathSound( void )
{
	EmitSoundScript(dieSoundScript);
}

//=========================================================
// AttackSound
//=========================================================
void CBullsquid::AttackSound( bool bigSpit )
{
	if (bigSpit) {
		EmitSoundScript(attackToxicSoundScript);
	} else {
		EmitSoundScript(attackSoundScript);
	}
}

//========================================================
// RunAI - overridden for bullsquid because there are things
// that need to be checked every think.
//========================================================
void CBullsquid::RunAI( void )
{
	// first, do base class stuff
	CBaseMonster::RunAI();

	if( pev->skin != 0 )
	{
		// close eye if it was open.
		pev->skin = 0; 
	}

	if( RANDOM_LONG( 0, 39 ) == 0 )
	{
		pev->skin = 1;
	}

	if( m_hEnemy != 0 && m_Activity == ACT_RUN )
	{
		// chasing enemy. Sprint for last bit
		if( ( pev->origin - m_hEnemy->pev->origin).Length2D() < SQUID_SPRINT_DIST )
		{
			pev->framerate = 1.25f;
		}
	}
}

//========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t tlSquidRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, 0.0f },
	{ TASK_RANGE_ATTACK1, 0.0f },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t slSquidRangeAttack1[] =
{
	{
		tlSquidRangeAttack1,
		ARRAYSIZE( tlSquidRangeAttack1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED,
		0,
		"Squid Range Attack1"
	},
};

// Chase enemy schedule
Task_t tlSquidChaseEnemy1[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_RANGE_ATTACK1 },// !!!OEM - this will stop nasty squid oscillation.
	{ TASK_GET_PATH_TO_ENEMY, 0.0f },
	{ TASK_RUN_PATH, 0.0f },
	{ TASK_WAIT_FOR_MOVEMENT, 0.0f },
};

Schedule_t slSquidChaseEnemy[] =
{
	{
		tlSquidChaseEnemy1,
		ARRAYSIZE( tlSquidChaseEnemy1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_SMELL_FOOD |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2 |
		bits_COND_TASK_FAILED |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER |
		bits_SOUND_MEAT,
		"Squid Chase Enemy"
	},
};

Task_t tlSquidHurtHop[] =
{
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_SOUND_WAKE, 0.0f },
	{ TASK_SQUID_HOPTURN, 0.0f },
	{ TASK_FACE_ENEMY, 0.0f },// in case squid didn't turn all the way in the air.
};

Schedule_t slSquidHurtHop[] =
{
	{
		tlSquidHurtHop,
		ARRAYSIZE( tlSquidHurtHop ),
		0,
		0,
		"SquidHurtHop"
	}
};

Task_t tlSquidSeeCrab[] =
{
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_SOUND_WAKE, 0.0f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EXCITED },
	{ TASK_FACE_ENEMY, 0.0f },
};

Schedule_t slSquidSeeCrab[] =
{
	{
		tlSquidSeeCrab,
		ARRAYSIZE( tlSquidSeeCrab ),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		0,
		"SquidSeeCrab"
	}
};

// squid walks to something tasty and eats it.
Task_t tlSquidEat[] =
{
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_EAT, 10.0f },// this is in case the squid can't get to the food
	{ TASK_STORE_LASTPOSITION, 0.0f },
	{ TASK_GET_PATH_TO_BESTSCENT, 0.0f },
	{ TASK_WALK_PATH, 0.0f },
	{ TASK_WAIT_FOR_MOVEMENT, 0.0f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.25f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.25f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.5f },
	{ TASK_EAT, 50.0f },
	{ TASK_GET_PATH_TO_LASTPOSITION, 0.0f },
	{ TASK_WALK_PATH, 0.0f },
	{ TASK_WAIT_FOR_MOVEMENT, 0.0f },
	{ TASK_CLEAR_LASTPOSITION, 0.0f },
};

Schedule_t slSquidEat[] =
{
	{
		tlSquidEat,
		ARRAYSIZE( tlSquidEat ),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_NEW_ENEMY,
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT |
		bits_SOUND_CARCASS,
		"SquidEat"
	}
};

// this is a bit different than just Eat. We use this schedule when the food is far away, occluded, or behind
// the squid. This schedule plays a sniff animation before going to the source of food.
Task_t tlSquidSniffAndEat[] =
{
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_EAT, 10.0f },// this is in case the squid can't get to the food
	{ TASK_PLAY_SEQUENCE, (float)ACT_DETECT_SCENT },
	{ TASK_STORE_LASTPOSITION, 0.0f },
	{ TASK_GET_PATH_TO_BESTSCENT, 0.0f },
	{ TASK_WALK_PATH, 0.0f },
	{ TASK_WAIT_FOR_MOVEMENT, 0.0f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.25f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.25f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.5f },
	{ TASK_EAT, 50.0f },
	{ TASK_GET_PATH_TO_LASTPOSITION, 0.0f },
	{ TASK_WALK_PATH, 0.0f },
	{ TASK_WAIT_FOR_MOVEMENT, 0.0f },
	{ TASK_CLEAR_LASTPOSITION, 0.0f },
};

Schedule_t slSquidSniffAndEat[] =
{
	{
		tlSquidSniffAndEat,
		ARRAYSIZE( tlSquidSniffAndEat ),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_NEW_ENEMY,
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT |
		bits_SOUND_CARCASS,
		"SquidSniffAndEat"
	}
};

// squid does this to stinky things. 
Task_t tlSquidWallow[] =
{
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_EAT, 10.0f },// this is in case the squid can't get to the stinkiness
	{ TASK_STORE_LASTPOSITION, 0.0f },
	{ TASK_GET_PATH_TO_BESTSCENT, 0.0f },
	{ TASK_WALK_PATH, 0.0f },
	{ TASK_WAIT_FOR_MOVEMENT, 0.0f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_INSPECT_FLOOR },
	{ TASK_EAT, 50.0f },// keeps squid from eating or sniffing anything else for a while.
	{ TASK_GET_PATH_TO_LASTPOSITION, 0.0f },
	{ TASK_WALK_PATH, 0.0f },
	{ TASK_WAIT_FOR_MOVEMENT, 0.0f },
	{ TASK_CLEAR_LASTPOSITION, 0.0f },
};

Schedule_t slSquidWallow[] =
{
	{
		tlSquidWallow,
		ARRAYSIZE( tlSquidWallow ),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_SCHEDULE_SUGGESTED |
		bits_COND_NEW_ENEMY,
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_GARBAGE,
		"SquidWallow"
	}
};

Task_t tlSquidVictoryDance[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_EAT, (float)10 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_WAIT, 0.2f },
	{ TASK_STORE_LASTPOSITION, (float)0 },
	{ TASK_GET_PATH_TO_ENEMY_CORPSE, 50.0f },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.25f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.25f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_GET_HEALTH_FROM_FOOD, 0.5f },
	{ TASK_EAT, (float)50 },
	{ TASK_GET_PATH_TO_LASTPOSITION, (float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_CLEAR_LASTPOSITION, (float)0 },
};

Schedule_t slSquidVictoryDance[] =
{
	{
		tlSquidVictoryDance,
		ARRAYSIZE( tlSquidVictoryDance ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		0,
		"SquidVictoryDance"
	},
};

DEFINE_CUSTOM_SCHEDULES( CBullsquid ) 
{
	slSquidRangeAttack1,
	slSquidChaseEnemy,
	slSquidHurtHop,
	slSquidSeeCrab,
	slSquidEat,
	slSquidSniffAndEat,
	slSquidWallow,
	slSquidVictoryDance
};

IMPLEMENT_CUSTOM_SCHEDULES( CBullsquid, CBaseMonster )

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CBullsquid::GetSchedule( void )
{
	switch( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
		{
			if( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) && gpGlobals->time >= m_flNextHopTime )
			{
				return GetScheduleOfType( SCHED_SQUID_HURTHOP );
			}

			if( HasConditions( bits_COND_ENEMY_DEAD ) && pev->health < pev->max_health )
			{
				return GetScheduleOfType( SCHED_VICTORY_DANCE );
			}

			if( HasConditions( bits_COND_SMELL_FOOD ) )
			{
				CSound *pSound;

				pSound = PBestScent();
				
				if( pSound && ( !FInViewCone( &pSound->m_vecOrigin ) || !FVisible( pSound->m_vecOrigin ) ) )
				{
					// scent is behind or occluded
					return GetScheduleOfType( SCHED_SQUID_SNIFF_AND_EAT );
				}

				// food is right out in the open. Just go get it.
				return GetScheduleOfType( SCHED_SQUID_EAT );
			}

			if( HasConditions( bits_COND_SMELL ) )
			{
				// there's something stinky. 
				CSound *pSound;

				pSound = PBestScent();
				if( pSound )
					return GetScheduleOfType( SCHED_SQUID_WALLOW );
			}
			break;
		}
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
				if( m_fCanThreatDisplay && IRelationship( m_hEnemy ) == R_HT )
				{
					// this means squid sees a headcrab!
					m_fCanThreatDisplay = FALSE;// only do the headcrab dance once per lifetime.
					return GetScheduleOfType( SCHED_SQUID_SEECRAB );
				}
				else
				{
					return GetScheduleOfType( SCHED_WAKE_ANGRY );
				}
			}

			if( HasConditions( bits_COND_SMELL_FOOD ) )
			{
				CSound *pSound;

				pSound = PBestScent();

				if( pSound && ( !FInViewCone( &pSound->m_vecOrigin ) || !FVisible( pSound->m_vecOrigin ) ) )
				{
					// scent is behind or occluded
					return GetScheduleOfType( SCHED_SQUID_SNIFF_AND_EAT );
				}

				// food is right out in the open. Just go get it.
				return GetScheduleOfType( SCHED_SQUID_EAT );
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

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t *CBullsquid::GetScheduleOfType( int Type ) 
{
	switch( Type )
	{
	case SCHED_RANGE_ATTACK1:
		return &slSquidRangeAttack1[0];
		break;
	case SCHED_SQUID_HURTHOP:
		return &slSquidHurtHop[0];
		break;
	case SCHED_SQUID_SEECRAB:
		return &slSquidSeeCrab[0];
		break;
	case SCHED_SQUID_EAT:
		return &slSquidEat[0];
		break;
	case SCHED_SQUID_SNIFF_AND_EAT:
		return &slSquidSniffAndEat[0];
		break;
	case SCHED_SQUID_WALLOW:
		return &slSquidWallow[0];
		break;
	case SCHED_CHASE_ENEMY:
		return &slSquidChaseEnemy[0];
		break;
	case SCHED_VICTORY_DANCE:
		return slSquidVictoryDance;
		break;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for bullsquid because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CBullsquid::StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_MELEE_ATTACK2:
		{
			EmitSoundScript(attackGrowlSoundScript);
			CBaseMonster::StartTask( pTask );
			break;
		}
	case TASK_SQUID_HOPTURN:
		{
			m_flNextHopTime = gpGlobals->time + 5.0f;
			SetActivity( ACT_HOP );
			MakeIdealYaw( m_vecEnemyLKP );
			break;
		}
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
	default:
		{
			CBaseMonster::StartTask( pTask );
			break;
		}
	}
}

//=========================================================
// RunTask
//=========================================================
void CBullsquid::RunTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_SQUID_HOPTURN:
		{
			MakeIdealYaw( m_vecEnemyLKP );
			ChangeYaw( pev->yaw_speed );

			if( m_fSequenceFinished )
			{
				TaskComplete();
			}
			break;
		}
	default:
		{
			CBaseMonster::RunTask( pTask );
			break;
		}
	}
}

//=========================================================
// GetIdealState - Overridden for Bullsquid to deal with
// the feature that makes it lose interest in headcrabs for 
// a while if something injures it. 
//=========================================================
MONSTERSTATE CBullsquid::GetIdealState( void )
{
	int iConditions;

	iConditions = IScheduleFlags();

	// If no schedule conditions, the new ideal state is probably the reason we're in here.
	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		/*
		COMBAT goes to ALERT upon death of enemy
		*/
		{
			if( m_hEnemy != 0 && ( iConditions & bits_COND_LIGHT_DAMAGE || iConditions & bits_COND_HEAVY_DAMAGE ) && FClassnameIs( m_hEnemy->pev, "monster_headcrab" ) )
			{
				// if the squid has a headcrab enemy and something hurts it, it's going to forget about the crab for a while.
				m_hEnemy = NULL;
				m_IdealMonsterState = MONSTERSTATE_ALERT;
			}
			break;
		}
	default:
			break;
	}

	m_IdealMonsterState = CBaseMonster::GetIdealState();

	return m_IdealMonsterState;
}

class CDeadBullsquid : public CDeadMonster
{
public:
	void Spawn( void );
	const char* DefaultModel() { return "models/bullsquid.mdl"; }
	int	DefaultClassify ( void ) { return	CLASS_ALIEN_MONSTER; }

	const char* getPos(int pos) const;
};

const char* CDeadBullsquid::getPos(int pos) const
{
	return "die1";
}

LINK_ENTITY_TO_CLASS( monster_bullchicken_dead, CDeadBullsquid )

void CDeadBullsquid :: Spawn( )
{
	SpawnHelper(BLOOD_COLOR_YELLOW, gSkillData.bullsquidHealth/2);
	MonsterInitDead();
	pev->frame = 255;
}
