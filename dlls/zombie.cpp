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
// Zombie
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"mod_features.h"
#include	"game.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	ZOMBIE_AE_ATTACK_RIGHT		0x01
#define	ZOMBIE_AE_ATTACK_LEFT		0x02
#define	ZOMBIE_AE_ATTACK_BOTH		0x03

#define ZOMBIE_FLINCH_DELAY		2		// at most one flinch every n secs

class CZombie : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int DefaultClassify( void );
	const char* DefaultDisplayName() { return "Zombie"; }
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int IgnoreConditions( void );

	float m_flNextFlinch;

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	// No range attacks
	BOOL CheckRangeAttack1( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	virtual int DefaultSizeForGrapple() { return GRAPPLE_MEDIUM; }
	bool IsDisplaceable() { return true; }
	Vector DefaultMinHullSize() { return VEC_HUMAN_HULL_MIN; }
	Vector DefaultMaxHullSize() { return VEC_HUMAN_HULL_MAX; }
	virtual float OneSlashDamage() { return gSkillData.zombieDmgOneSlash; }
	virtual float BothSlashDamage() { return gSkillData.zombieDmgBothSlash; }
protected:
	void SlashAttack( float dmg, float rightScalar, float forwardScalar, float punchz );
	void ZombieSpawnHelper(const char* modelName, float health);
	void PrecacheSounds();
};

LINK_ENTITY_TO_CLASS( monster_zombie, CZombie )

const char *CZombie::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CZombie::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CZombie::pAttackSounds[] =
{
	"zombie/zo_attack1.wav",
	"zombie/zo_attack2.wav",
};

const char *CZombie::pIdleSounds[] =
{
	"zombie/zo_idle1.wav",
	"zombie/zo_idle2.wav",
	"zombie/zo_idle3.wav",
	"zombie/zo_idle4.wav",
};

const char *CZombie::pAlertSounds[] =
{
	"zombie/zo_alert10.wav",
	"zombie/zo_alert20.wav",
	"zombie/zo_alert30.wav",
};

const char *CZombie::pPainSounds[] =
{
	"zombie/zo_pain1.wav",
	"zombie/zo_pain2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CZombie::DefaultClassify( void )
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CZombie::SetYawSpeed( void )
{
	pev->yaw_speed = 120;
}

int CZombie::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Take 30% damage from bullets
	if( bitsDamageType == DMG_BULLET )
	{
		Vector vecDir = pev->origin - ( pevInflictor->absmin + pevInflictor->absmax ) * 0.5f;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce( flDamage );
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.3f;
	}

	// HACK HACK -- until we fix this.
	if( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CZombie::PainSound( void )
{
	int pitch = 95 + RANDOM_LONG( 0, 9 );

	if( RANDOM_LONG( 0, 5 ) < 2 )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pPainSounds ), 1.0, ATTN_NORM, 0, pitch );
}

void CZombie::AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG( 0, 9 );

	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pAlertSounds ), 1.0, ATTN_NORM, 0, pitch );
}

void CZombie::IdleSound( void )
{
	int pitch = 95 + RANDOM_LONG( 0, 9 );

	// Play a random idle sound
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pIdleSounds ), 1.0, ATTN_NORM, 0, pitch );
}

void CZombie::AttackSound( void )
{
	int pitch = 95 + RANDOM_LONG( 0, 9 );

	// Play a random attack sound
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pAttackSounds ), 1.0, ATTN_NORM, 0, pitch );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CZombie::SlashAttack(float dmg, float rightScalar, float forwardScalar, float punchz )
{
	CBaseEntity *pHurt = CheckTraceHullAttack( 70, dmg, DMG_SLASH );
	if ( pHurt )
	{
		if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
		{
			if (punchz) {
				pHurt->pev->punchangle.z = punchz;
			}
			pHurt->pev->punchangle.x = 5;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * rightScalar + gpGlobals->v_forward * forwardScalar;
		}
		// Play a random attack hit sound
		EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
	}
	else // Play a random attack miss sound
		EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackMissSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

	if (RANDOM_LONG(0,1))
		AttackSound();
}

void CZombie::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case ZOMBIE_AE_ATTACK_RIGHT:
			SlashAttack(OneSlashDamage(), -100, 0, -18 );
		break;

		case ZOMBIE_AE_ATTACK_LEFT:
			SlashAttack(OneSlashDamage(), 100, 0, 18 );
		break;

		case ZOMBIE_AE_ATTACK_BOTH:
			SlashAttack(BothSlashDamage(), 0, -100, 0 );
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CZombie::ZombieSpawnHelper(const char* modelName, float health)
{
	SetMyModel( modelName );
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_GREEN );
	SetMyHealth( health );
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	SetMyFieldOfView(0.5f);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();
}

void CZombie::Spawn()
{
	Precache();
	ZombieSpawnHelper("models/zombie.mdl", gSkillData.zombieHealth);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CZombie::Precache()
{
	PrecacheMyModel( "models/zombie.mdl" );
	PrecacheSounds();
}

void CZombie::PrecacheSounds()
{
	PRECACHE_SOUND_ARRAY( pAttackHitSounds );
	PRECACHE_SOUND_ARRAY( pAttackMissSounds );
	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

int CZombie::IgnoreConditions( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if( ( m_Activity == ACT_MELEE_ATTACK1 ) || ( m_Activity == ACT_MELEE_ATTACK1 ) )
	{
#if 0
		if( pev->health < 20 )
			iIgnore |= ( bits_COND_LIGHT_DAMAGE| bits_COND_HEAVY_DAMAGE );
		else
#endif
		if( m_flNextFlinch >= gpGlobals->time )
			iIgnore |= ( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE );
	}

	if( ( m_Activity == ACT_SMALL_FLINCH ) || ( m_Activity == ACT_BIG_FLINCH ) )
	{
		if( m_flNextFlinch < gpGlobals->time )
			m_flNextFlinch = gpGlobals->time + ZOMBIE_FLINCH_DELAY;
	}

	return iIgnore;
}

class CDeadZombie : public CDeadMonster
{
public:
	void Spawn( void );
	int	DefaultClassify ( void ) { return	CLASS_ALIEN_MONSTER; }

	const char* getPos(int pos) const;
	static const char *m_szPoses[2];
};

const char *CDeadZombie::m_szPoses[] = { "dieheadshot", "dieforward" };

const char* CDeadZombie::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_zombie_dead, CDeadZombie )

void CDeadZombie::Spawn( )
{
	SpawnHelper("models/zombie.mdl", BLOOD_COLOR_YELLOW);
	MonsterInitDead();
	pev->frame = 255;
}

#if FEATURE_ZOMBIE_BARNEY
class CZombieBarney : public CZombie
{
	void Spawn( void );
	void Precache( void );
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("zombie_barney"); }
	const char* DefaultDisplayName() { return "Zombie Barney"; }
	float OneSlashDamage() { return gSkillData.zombieBarneyDmgOneSlash; }
	float BothSlashDamage() { return gSkillData.zombieBarneyDmgBothSlash; }
};

LINK_ENTITY_TO_CLASS( monster_zombie_barney, CZombieBarney )

void CZombieBarney::Spawn()
{
	Precache( );
	ZombieSpawnHelper("models/zombie_barney.mdl", gSkillData.zombieBarneyHealth);
}

void CZombieBarney::Precache()
{
	PrecacheMyModel("models/zombie_barney.mdl");
	PrecacheSounds();
}

class CDeadZombieBarney : public CDeadZombie
{
public:
	void Spawn( void );
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("zombie_barney"); }
};

LINK_ENTITY_TO_CLASS( monster_zombie_barney_dead, CDeadZombieBarney )

void CDeadZombieBarney::Spawn( )
{
	SpawnHelper("models/zombie_barney.mdl", BLOOD_COLOR_YELLOW);
	MonsterInitDead();
	pev->frame = 255;
}
#endif

#if FEATURE_ZOMBIE_SOLDIER
class CZombieSoldier : public CZombie
{
	void Spawn( void );
	void Precache( void );
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("zombie_soldier"); }
	const char* DefaultDisplayName() { return "Zombie Soldier"; }
	float OneSlashDamage() { return gSkillData.zombieSoldierDmgOneSlash; }
	float BothSlashDamage() { return gSkillData.zombieSoldierDmgBothSlash; }
};

LINK_ENTITY_TO_CLASS( monster_zombie_soldier, CZombieSoldier )

void CZombieSoldier::Spawn()
{
	Precache( );
	ZombieSpawnHelper("models/zombie_soldier.mdl", gSkillData.zombieSoldierHealth);
}

void CZombieSoldier::Precache()
{
	PrecacheMyModel("models/zombie_soldier.mdl");
	PrecacheSounds();
}

class CDeadZombieSoldier : public CDeadMonster
{
public:
	void Spawn( void );
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("zombie_soldier"); }
	int	DefaultClassify ( void ) { return	CLASS_ALIEN_MONSTER; }

	const char* getPos(int pos) const;
	static const char *m_szPoses[2];
};

const char *CDeadZombieSoldier::m_szPoses[] = { "dead_on_back", "dead_on_stomach" };

const char* CDeadZombieSoldier::getPos(int pos) const
{
	return m_szPoses[pos % (sizeof(m_szPoses)/sizeof(const char*))];
}

LINK_ENTITY_TO_CLASS( monster_zombie_soldier_dead, CDeadZombieSoldier )

void CDeadZombieSoldier::Spawn( )
{
	SpawnHelper("models/zombie_soldier.mdl", BLOOD_COLOR_YELLOW);
	MonsterInitDead();
}
#endif
