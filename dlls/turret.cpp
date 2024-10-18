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
/*

===== turret.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "combat.h"
#include "global_models.h"
#include "effects.h"
#include "game.h"
#include "common_soundscripts.h"
#include "visuals_utils.h"

#define TURRET_SHOTS	2
#define TURRET_RANGE	(100 * 12)
#define TURRET_SPREAD	Vector( 0, 0, 0 )
#define TURRET_TURNRATE	30		//angles per 0.1 second
#define TURRET_MAXWAIT	15		// seconds turret will stay active w/o a target
#define TURRET_MAXSPIN	5		// seconds turret barrel will spin w/o a target
#define TURRET_MACHINE_VOLUME	0.5

typedef enum
{
	TURRET_ANIM_NONE = 0,
	TURRET_ANIM_FIRE,
	TURRET_ANIM_SPIN,
	TURRET_ANIM_DEPLOY,
	TURRET_ANIM_RETIRE,
	TURRET_ANIM_DIE
} TURRET_ANIM;

class CBaseTurret : public CBaseMonster
{
public:
	void Spawn( void );
	virtual void Precache( void );
	void UpdateOnRemove();
	void KeyValue( KeyValueData *pkvd );
	void EXPORT TurretUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual void TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	virtual int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	virtual int Classify( void );
	virtual int DefaultClassify();
	int RealClassify();

	int BloodColor( void ) { return DONT_BLEED; }
	void GibMonster( void ) {}	// UNDONE: Throw turret gibs?

	// Think functions
	void EXPORT ActiveThink(void);
	void EXPORT SearchThink(void);
	void EXPORT AutoSearchThink(void);
	void EXPORT TurretDeath(void);

	virtual void EXPORT SpinDownCall(void) { m_iSpin = 0; }
	virtual void EXPORT SpinUpCall(void) { m_iSpin = 1; }

	// void SpinDown( void );
	// float EXPORT SpinDownCall( void ) { return SpinDown(); }

	// virtual float SpinDown( void ) { return 0;}
	// virtual float Retire( void ) { return 0;}

	void EXPORT Deploy( void );
	void EXPORT Retire( void );

	void EXPORT Initialize( void );

	virtual void Ping( void );
	virtual void EyeOn( void );
	virtual void EyeOff( void );
	virtual int MaxEyeBrightness() { return 255; }

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	// other functions
	void SetTurretAnim( TURRET_ANIM anim );
	int MoveTurret( void );
	virtual void Shoot( Vector &vecSrc, Vector &vecDirToEnemy ) { }

	void SetEnemy(CBaseEntity* enemy);

	float m_flMaxSpin;		// Max time to spin the barrel w/o a target
	int m_iSpin;

	CSprite *m_pEyeGlow;
	int m_eyeBrightness;

	int m_iDeployHeight;
	int m_iRetractHeight;
	int m_iMinPitch;

	int m_iBaseTurnRate;	// angles per second
	float m_fTurnRate;		// actual turn rate
	int m_iOrientation;		// 0 = floor, 1 = Ceiling
	int m_iOn;
	int m_fBeserk;			// Sometimes this bitch will just freak out
	int m_iAutoStart;		// true if the turret auto deploys when a target
						// enters its range

	Vector m_vecLastSight;
	float m_flLastSight;	// Last time we saw a target
	float m_flMaxWait;		// Max time to seach w/o a target
	int m_iSearchSpeed;		// Not Used!

	// movement
	float m_flStartYaw;
	Vector m_vecCurAngles;
	Vector m_vecGoalAngles;

	float m_flPingTime;	// Time until the next ping, used when searching
	float m_flSpinUpTime;	// Amount of time until the barrel should spin down when searching

	static const NamedSoundScript alertSoundScript;
	static const NamedSoundScript dieSoundScript;
	static const NamedSoundScript deploySoundScript;
	static const NamedSoundScript undeploySoundScript;
	static const NamedSoundScript pingSoundScript;
	static const NamedSoundScript spinupSoundScript;
};

TYPEDESCRIPTION	CBaseTurret::m_SaveData[] =
{
	DEFINE_FIELD( CBaseTurret, m_flMaxSpin, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseTurret, m_iSpin, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseTurret, m_pEyeGlow, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBaseTurret, m_eyeBrightness, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_iDeployHeight, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_iRetractHeight, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_iMinPitch, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseTurret, m_iBaseTurnRate, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_fTurnRate, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseTurret, m_iOrientation, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_iOn, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_fBeserk, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret, m_iAutoStart, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseTurret, m_vecLastSight, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseTurret, m_flLastSight, FIELD_TIME ),
	DEFINE_FIELD( CBaseTurret, m_flMaxWait, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseTurret, m_iSearchSpeed, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseTurret, m_flStartYaw, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseTurret, m_vecCurAngles, FIELD_VECTOR ),
	DEFINE_FIELD( CBaseTurret, m_vecGoalAngles, FIELD_VECTOR ),

	DEFINE_FIELD( CBaseTurret, m_flPingTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseTurret, m_flSpinUpTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CBaseTurret, CBaseMonster )

const NamedSoundScript CBaseTurret::alertSoundScript = {
	CHAN_BODY,
	{"turret/tu_alert.wav"},
	TURRET_MACHINE_VOLUME,
	ATTN_NORM,
	"Turret.Alert"
};

const NamedSoundScript CBaseTurret::dieSoundScript = {
	CHAN_BODY,
	{"turret/tu_die.wav", "turret/tu_die2.wav", "turret/tu_die3.wav"},
	"Turret.Die"
};

const NamedSoundScript CBaseTurret::deploySoundScript = {
	CHAN_BODY,
	{"turret/tu_deploy.wav"},
	TURRET_MACHINE_VOLUME,
	ATTN_NORM,
	"Turret.Deploy"
};

const NamedSoundScript CBaseTurret::undeploySoundScript = {
	CHAN_BODY,
	{"turret/tu_deploy.wav"},
	TURRET_MACHINE_VOLUME,
	ATTN_NORM,
	IntRange(120),
	"Turret.Undeploy"
};

const NamedSoundScript CBaseTurret::pingSoundScript = {
	CHAN_ITEM,
	{"turret/tu_ping.wav"},
	"Turret.Ping"
};

const NamedSoundScript CBaseTurret::spinupSoundScript = {
	CHAN_STATIC,
	{"turret/tu_active2.wav"},
	TURRET_MACHINE_VOLUME,
	ATTN_NORM,
	"Turret.Spinup"
};

class CTurret : public CBaseTurret
{
public:
	void Spawn( void );
	void Precache( void );
	// Think functions
	const char* DefaultDisplayName() { return "Turret"; }
	void SpinUpCall( void );
	void SpinDownCall( void );
	virtual int MaxEyeBrightness();

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	
	static TYPEDESCRIPTION m_SaveData[];

	// other functions
	void Shoot( Vector &vecSrc, Vector &vecDirToEnemy );

	static const NamedSoundScript shootSoundScript;
	static const NamedSoundScript spinupCallSoundScript;
	static const NamedSoundScript spindownCallSoundScript;

	static const NamedVisual glowVisual;
private:
	int m_iStartSpin;
};

const NamedSoundScript CTurret::shootSoundScript = {
	CHAN_WEAPON,
	{"turret/tu_fire1.wav"},
	1.0f,
	0.6f,
	"Turret.Shoot"
};

const NamedSoundScript CTurret::spinupCallSoundScript = {
	CHAN_BODY,
	{"turret/tu_spinup.wav"},
	TURRET_MACHINE_VOLUME,
	ATTN_NORM,
	"Turret.SpinUpCall"
};

const NamedSoundScript CTurret::spindownCallSoundScript = {
	CHAN_ITEM,
	{"turret/tu_spindown.wav"},
	TURRET_MACHINE_VOLUME,
	ATTN_NORM,
	"Turret.SpinDownCall"
};

const NamedVisual CTurret::glowVisual = BuildVisual("Turret.Glow")
		.Model("sprites/flare3.spr")
		.RenderMode(kRenderGlow)
		.RenderColor(255, 0, 0)
		.Alpha(255)
		.RenderFx(kRenderFxNoDissipation);

TYPEDESCRIPTION	CTurret::m_SaveData[] =
{
	DEFINE_FIELD( CTurret, m_iStartSpin, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CTurret, CBaseTurret )

class CMiniTurret : public CBaseTurret
{
public:
	void Spawn();
	void Precache( void );
	// other functions
	const char* DefaultDisplayName() { return "Miniturret"; }
	void Shoot( Vector &vecSrc, Vector &vecDirToEnemy );

	static constexpr const char* shootSoundScript = "MiniTurret.Shoot";
};

LINK_ENTITY_TO_CLASS( monster_turret, CTurret )
LINK_ENTITY_TO_CLASS( monster_miniturret, CMiniTurret )

void CBaseTurret::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "maxsleep" ) )
	{
		m_flMaxWait = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "orientation" ) )
	{
		m_iOrientation = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "searchspeed" ) )
	{
		m_iSearchSpeed = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "turnrate" ) )
	{
		m_iBaseTurnRate = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "style" ) ||
			FStrEq( pkvd->szKeyName, "height" ) ||
			FStrEq( pkvd->szKeyName, "value1" ) ||
			FStrEq( pkvd->szKeyName, "value2" ) ||
			FStrEq( pkvd->szKeyName, "value3" ) )
		pkvd->fHandled = TRUE;
	else
		CBaseMonster::KeyValue( pkvd );
}

void CBaseTurret::Spawn()
{ 
	Precache();
	pev->max_health = pev->health;
	pev->nextthink		= gpGlobals->time + 1;
	pev->movetype		= MOVETYPE_FLY;
	pev->sequence		= 0;
	pev->frame		= 0;
	pev->solid		= SOLID_SLIDEBOX;
	pev->takedamage		= DAMAGE_AIM;

	SetBits( pev->flags, FL_MONSTER );
	SetUse( &CBaseTurret::TurretUse );

	if( ( pev->spawnflags & SF_MONSTER_TURRET_AUTOACTIVATE ) 
		 && !( pev->spawnflags & SF_MONSTER_TURRET_STARTINACTIVE ) )
	{
		m_iAutoStart = TRUE;
	}

	ResetSequenceInfo();
	SetBoneController( 0, 0 );
	SetBoneController( 1, 0 );
	SetMyFieldOfView(VIEW_FIELD_FULL);
	// m_flSightRange = TURRET_RANGE;
}

void CBaseTurret::Precache()
{
	RegisterAndPrecacheSoundScript(pingSoundScript);
	RegisterAndPrecacheSoundScript(spinupSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript);
	RegisterAndPrecacheSoundScript(deploySoundScript);
	RegisterAndPrecacheSoundScript(undeploySoundScript);
	RegisterAndPrecacheSoundScript(alertSoundScript);
	PRECACHE_SOUND( "turret/tu_search.wav" );
}

void CBaseTurret::UpdateOnRemove()
{
	if( m_pEyeGlow )
	{
		UTIL_Remove( m_pEyeGlow );
		m_pEyeGlow = 0;
	}
	CBaseMonster::UpdateOnRemove();
}

void CTurret::Spawn()
{
	Precache();
	SetMyModel( "models/turret.mdl" );
	SetMyHealth( gSkillData.turretHealth );
	m_HackedGunPos		= Vector( 0, 0, 12.75 );
	m_flMaxSpin		= TURRET_MAXSPIN;
	pev->view_ofs.z		= 12.75;

	CBaseTurret::Spawn();

	m_iRetractHeight = 16;
	m_iDeployHeight = 32;
	m_iMinPitch = -15;
	UTIL_SetSize( pev, Vector( -32, -32, -m_iRetractHeight ), Vector( 32, 32, m_iRetractHeight ) );

	SetThink( &CBaseTurret::Initialize );

	const Visual* visual = GetVisual(glowVisual);
	m_pEyeGlow = CreateSpriteFromVisual(visual, pev->origin);
	if (m_pEyeGlow)
	{
		m_pEyeGlow->SetBrightness(0);
		m_pEyeGlow->SetAttachment( edict(), 2 );
	}
	m_eyeBrightness = 0;

	pev->nextthink = gpGlobals->time + 0.3f; 
}

void CTurret::Precache()
{
	CBaseTurret::Precache();
	PrecacheMyModel( "models/turret.mdl" );	
	RegisterVisual(glowVisual);
	RegisterAndPrecacheSoundScript(shootSoundScript);
	RegisterAndPrecacheSoundScript(spinupCallSoundScript);
	RegisterAndPrecacheSoundScript(spindownCallSoundScript);
}

void CMiniTurret::Spawn()
{
	Precache();
	SetMyModel( "models/miniturret.mdl" );
	SetMyHealth( gSkillData.miniturretHealth );
	m_HackedGunPos = Vector( 0.0f, 0.0f, 12.75f );
	m_flMaxSpin = 0;
	pev->view_ofs.z = 12.75f;

	CBaseTurret::Spawn();
	m_iRetractHeight = 16;
	m_iDeployHeight = 32;
	m_iMinPitch = -15;
	UTIL_SetSize( pev, Vector( -16, -16, -m_iRetractHeight ), Vector( 16, 16, m_iRetractHeight ) );

	SetThink( &CBaseTurret::Initialize );	
	pev->nextthink = gpGlobals->time + 0.3f;
}

void CMiniTurret::Precache()
{
	CBaseTurret::Precache();
	PrecacheMyModel( "models/miniturret.mdl" );	
	RegisterAndPrecacheSoundScript(shootSoundScript, NPC::single9mmSoundScript);
}

void CBaseTurret::Initialize( void )
{
	m_iOn = 0;
	m_fBeserk = 0;
	m_iSpin = 0;

	SetBoneController( 0, 0 );
	SetBoneController( 1, 0 );

	if( m_iBaseTurnRate == 0 )
		m_iBaseTurnRate = TURRET_TURNRATE;
	if( m_flMaxWait == 0 )
		m_flMaxWait = TURRET_MAXWAIT;
	m_flStartYaw = pev->angles.y;
	if( m_iOrientation == 1 )
	{
		pev->idealpitch = 180;
		pev->angles.x = 180;
		pev->view_ofs.z = -pev->view_ofs.z;
		pev->effects |= EF_INVLIGHT;
		pev->angles.y = pev->angles.y + 180;
		if( pev->angles.y > 360 )
			pev->angles.y = pev->angles.y - 360;
	}

	m_vecGoalAngles.x = 0;

	if( m_iAutoStart )
	{
		m_flLastSight = gpGlobals->time + m_flMaxWait;
		SetThink( &CBaseTurret::AutoSearchThink );
		pev->nextthink = gpGlobals->time + 0.1f;
	}
	else
		SetThink( &CBaseEntity::SUB_DoNothing );
}

void CBaseTurret::TurretUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !ShouldToggle( useType, m_iOn ) )
		return;

	if( m_iOn )
	{
		m_hEnemy = NULL;
		pev->nextthink = gpGlobals->time + 0.1f;
		m_iAutoStart = FALSE;// switching off a turret disables autostart

		//!!!! this should spin down first!!BUGBUG
		SetThink( &CBaseTurret::Retire );
	}
	else 
	{
		pev->nextthink = gpGlobals->time + 0.1f; // turn on delay

		// if the turret is flagged as an autoactivate turret, re-enable it's ability open self.
		if( pev->spawnflags & SF_MONSTER_TURRET_AUTOACTIVATE )
		{
			m_iAutoStart = TRUE;
		}

		SetThink( &CBaseTurret::Deploy );
	}
}

void CBaseTurret::Ping( void )
{
	// make the pinging noise every second while searching
	if( m_flPingTime == 0 )
		m_flPingTime = gpGlobals->time + 1;
	else if( m_flPingTime <= gpGlobals->time )
	{
		m_flPingTime = gpGlobals->time + 1;
		EmitSoundScript(pingSoundScript);
		EyeOn();
	}
	else if( m_eyeBrightness > 0 )
	{
		EyeOff();
	}
}

void CBaseTurret::EyeOn()
{
	if( m_pEyeGlow )
	{
		const int maxBrightness = MaxEyeBrightness();
		if( m_eyeBrightness != maxBrightness )
		{
			m_eyeBrightness = maxBrightness;
		}
		m_pEyeGlow->SetBrightness( m_eyeBrightness );
	}
}

void CBaseTurret::EyeOff()
{
	if( m_pEyeGlow )
	{
		if( m_eyeBrightness > 0 )
		{
			m_eyeBrightness = Q_max( 0, m_eyeBrightness - 30 );
			m_pEyeGlow->SetBrightness( m_eyeBrightness );
		}
	}
}

void CBaseTurret::ActiveThink( void )
{
	int fAttack = 0;
	Vector vecDirToEnemy;

	pev->nextthink = gpGlobals->time + 0.1f;
	StudioFrameAdvance();
	GlowShellUpdate();

	if( ( !m_iOn ) || ( m_hEnemy == 0 ) )
	{
		m_hEnemy = NULL;
		m_flLastSight = gpGlobals->time + m_flMaxWait;
		SetThink( &CBaseTurret::SearchThink );
		return;
	}

	// if it's dead, look for something new
	if( !m_hEnemy->IsAlive() )
	{
		if( !m_flLastSight )
		{
			m_flLastSight = gpGlobals->time + 0.5f; // continue-shooting timeout
		}
		else
		{
			if( gpGlobals->time > m_flLastSight )
			{
				m_hEnemy = NULL;
				m_flLastSight = gpGlobals->time + m_flMaxWait;
				SetThink( &CBaseTurret::SearchThink );
				return;
			}
		}
	}

	Vector vecMid = pev->origin + pev->view_ofs;
	Vector vecMidEnemy = m_hEnemy->BodyTarget( vecMid );

	// Look for our current enemy
	int fEnemyVisible = FBoxVisible( pev, m_hEnemy->pev, vecMidEnemy );	

	vecDirToEnemy = vecMidEnemy - vecMid;	// calculate dir and dist to enemy
	float flDistToEnemy = vecDirToEnemy.Length();

	Vector vec = UTIL_VecToAngles( vecMidEnemy - vecMid );

	// Current enmey is not visible.
	if( !fEnemyVisible || ( flDistToEnemy > TURRET_RANGE ) )
	{
		if( !m_flLastSight )
			m_flLastSight = gpGlobals->time + 0.5f;
		else
		{
			// Should we look for a new target?
			if( gpGlobals->time > m_flLastSight )
			{
				m_hEnemy = NULL;
				m_flLastSight = gpGlobals->time + m_flMaxWait;
				SetThink( &CBaseTurret::SearchThink );
				return;
			}
		}
		fEnemyVisible = 0;
	}
	else
	{
		m_vecLastSight = vecMidEnemy;
	}

	UTIL_MakeAimVectors( m_vecCurAngles );

	/*
	ALERT( at_console, "%.0f %.0f : %.2f %.2f %.2f\n", 
		m_vecCurAngles.x, m_vecCurAngles.y,
		gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_forward.z );
	*/
	
	Vector vecLOS = vecDirToEnemy; //vecMid - m_vecLastSight;
	vecLOS = vecLOS.Normalize();

	// Is the Gun looking at the target
	if( DotProduct( vecLOS, gpGlobals->v_forward ) <= 0.866f ) // 30 degree slop
		fAttack = FALSE;
	else
		fAttack = TRUE;

	// fire the gun
	if( m_iSpin && ( ( fAttack ) || ( m_fBeserk ) ) )
	{
		Vector vecSrc, vecAng;
		GetAttachment( 0, vecSrc, vecAng );
		SetTurretAnim( TURRET_ANIM_FIRE );
		Shoot( vecSrc, gpGlobals->v_forward );
	} 
	else
	{
		SetTurretAnim( TURRET_ANIM_SPIN );
	}

	//move the gun
	if( m_fBeserk )
	{
		if( RANDOM_LONG( 0, 9 ) == 0 )
		{
			m_vecGoalAngles.y = RANDOM_FLOAT( 0, 360 );
			m_vecGoalAngles.x = RANDOM_FLOAT( 0, 90 ) - 90 * m_iOrientation;
			TakeDamage( pev, pev, 1, DMG_GENERIC ); // don't beserk forever
			return;
		}
	} 
	else if( fEnemyVisible )
	{
		if( vec.y > 360 )
			vec.y -= 360;

		if( vec.y < 0 )
			vec.y += 360;

		//ALERT( at_console, "[%.2f]", vec.x );

		if( vec.x < -180 )
			vec.x += 360;

		if( vec.x > 180 )
			vec.x -= 360;

		// now all numbers should be in [1...360]
		// pin to turret limitations to [-90...15]

		if( m_iOrientation == 0 )
		{
			if( vec.x > 90 )
				vec.x = 90;
			else if( vec.x < m_iMinPitch )
				vec.x = m_iMinPitch;
		}
		else
		{
			if( vec.x < -90 )
				vec.x = -90;
			else if( vec.x > -m_iMinPitch )
				vec.x = -m_iMinPitch;
		}

		// ALERT( at_console, "->[%.2f]\n", vec.x );

		m_vecGoalAngles.y = vec.y;
		m_vecGoalAngles.x = vec.x;
	}

	SpinUpCall();
	MoveTurret();
}

void CTurret::Shoot( Vector &vecSrc, Vector &vecDirToEnemy )
{
	FireBullets( 1, vecSrc, vecDirToEnemy, TURRET_SPREAD, TURRET_RANGE, BULLET_MONSTER_12MM, 1 );
	EmitSoundScript(shootSoundScript);
	pev->effects = pev->effects | EF_MUZZLEFLASH;
}

void CMiniTurret::Shoot( Vector &vecSrc, Vector &vecDirToEnemy )
{
	FireBullets( 1, vecSrc, vecDirToEnemy, TURRET_SPREAD, TURRET_RANGE, BULLET_MONSTER_9MM, 1 );
	EmitSoundScript(shootSoundScript);
	pev->effects = pev->effects | EF_MUZZLEFLASH;
}

void CBaseTurret::Deploy( void )
{
	pev->nextthink = gpGlobals->time + 0.1f;
	StudioFrameAdvance();
	GlowShellUpdate();

	if( pev->sequence != TURRET_ANIM_DEPLOY )
	{
		m_iOn = 1;
		SetTurretAnim( TURRET_ANIM_DEPLOY );
		EmitSoundScript(deploySoundScript);
		SUB_UseTargets( this, USE_ON );
	}

	if( m_fSequenceFinished )
	{
		pev->maxs.z = m_iDeployHeight;
		pev->mins.z = -m_iDeployHeight;
		UTIL_SetSize( pev, pev->mins, pev->maxs );

		m_vecCurAngles.x = 0;

		if( m_iOrientation == 1 )
		{
			m_vecCurAngles.y = UTIL_AngleMod( pev->angles.y + 180 );
		}
		else
		{
			m_vecCurAngles.y = UTIL_AngleMod( pev->angles.y );
		}

		SetTurretAnim( TURRET_ANIM_SPIN );
		pev->framerate = 0;
		SetThink( &CBaseTurret::SearchThink );
	}

	m_flLastSight = gpGlobals->time + m_flMaxWait;
}

void CBaseTurret::Retire( void )
{
	// make the turret level
	m_vecGoalAngles.x = 0;
	m_vecGoalAngles.y = m_flStartYaw;

	pev->nextthink = gpGlobals->time + 0.1f;

	StudioFrameAdvance();
	GlowShellUpdate();

	EyeOff();

	if( !MoveTurret() )
	{
		if( m_iSpin )
		{
			SpinDownCall();
		}
		else if( pev->sequence != TURRET_ANIM_RETIRE )
		{
			SetTurretAnim( TURRET_ANIM_RETIRE );
			EmitSoundScript(undeploySoundScript);
			SUB_UseTargets( this, USE_OFF );
		}
		else if( m_fSequenceFinished )
		{
			m_iOn = 0;
			m_flLastSight = 0;
			SetTurretAnim( TURRET_ANIM_NONE );
			pev->maxs.z = m_iRetractHeight;
			pev->mins.z = -m_iRetractHeight;
			UTIL_SetSize( pev, pev->mins, pev->maxs );
			if( m_iAutoStart )
			{
				SetThink( &CBaseTurret::AutoSearchThink );		
				pev->nextthink = gpGlobals->time + 0.1f;
			}
			else
				SetThink( &CBaseEntity::SUB_DoNothing );
		}
	}
	else
	{
		SetTurretAnim( TURRET_ANIM_SPIN );
	}
}

void CTurret::SpinUpCall( void )
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;

	// Are we already spun up? If not start the two stage process.
	if( !m_iSpin )
	{
		SetTurretAnim( TURRET_ANIM_SPIN );

		// for the first pass, spin up the the barrel
		if( !m_iStartSpin )
		{
			pev->nextthink = gpGlobals->time + 1.0f; // spinup delay
			EmitSoundScript(spinupCallSoundScript);
			m_iStartSpin = 1;
			pev->framerate = 0.1f;
		}
		// after the barrel is spun up, turn on the hum
		else if( pev->framerate >= 1.0f )
		{
			pev->nextthink = gpGlobals->time + 0.1f; // retarget delay
			EmitSoundScript(spinupSoundScript);
			SetThink( &CBaseTurret::ActiveThink );
			m_iStartSpin = 0;
			m_iSpin = 1;
		} 
		else
		{
			pev->framerate += 0.075f;
		}
	}

	if( m_iSpin )
	{
		SetThink( &CBaseTurret::ActiveThink );
	}
}

void CTurret::SpinDownCall( void )
{
	if( m_iSpin )
	{
		SetTurretAnim( TURRET_ANIM_SPIN );
		if( pev->framerate == 1.0f )
		{
			StopSoundScript(spinupSoundScript);
			EmitSoundScript(spindownCallSoundScript);
		}
		pev->framerate -= 0.02f;
		if( pev->framerate <= 0 )
		{
			pev->framerate = 0;
			m_iSpin = 0;
		}
	}
}

int CTurret::MaxEyeBrightness()
{
	const Visual* visual = GetVisual(glowVisual);
	return visual->renderamt;
}

void CBaseTurret::SetTurretAnim( TURRET_ANIM anim )
{
	if( pev->sequence != anim )
	{
		switch( anim )
		{
		case TURRET_ANIM_FIRE:
		case TURRET_ANIM_SPIN:
			if( pev->sequence != TURRET_ANIM_FIRE && pev->sequence != TURRET_ANIM_SPIN )
			{
				pev->frame = 0;
			}
			break;
		default:
			pev->frame = 0;
			break;
		}

		pev->sequence = anim;
		ResetSequenceInfo();

		switch( anim )
		{
		case TURRET_ANIM_RETIRE:
			pev->frame = 255;
			pev->framerate = -1.0;
			break;
		case TURRET_ANIM_DIE:
			pev->framerate = 1.0;
			break;
		default:
			break;
		}
		//ALERT( at_console, "Turret anim #%d\n", anim );
	}
}

//
// This search function will sit with the turret deployed and look for a new target. 
// After a set amount of time, the barrel will spin down. After m_flMaxWait, the turret will
// retact.
//
void CBaseTurret::SearchThink( void )
{
	// ensure rethink
	SetTurretAnim( TURRET_ANIM_SPIN );
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;
	GlowShellUpdate();

	if( m_flSpinUpTime == 0 && m_flMaxSpin )
		m_flSpinUpTime = gpGlobals->time + m_flMaxSpin;

	Ping();

	// If we have a target and we're still healthy
	if( m_hEnemy != 0 )
	{
		if( !m_hEnemy->IsAlive() )
			m_hEnemy = NULL;// Dead enemy forces a search for new one
	}

	// Acquire Target
	if( m_hEnemy == 0 )
	{
		Look( TURRET_RANGE );
		SetEnemy(BestVisibleEnemy());
	}

	// If we've found a target, spin up the barrel and start to attack
	if( m_hEnemy != 0 )
	{
		m_flLastSight = 0;
		m_flSpinUpTime = 0;
		SetThink( &CBaseTurret::ActiveThink );
	}
	else
	{
		// Are we out of time, do we need to retract?
 		if( gpGlobals->time > m_flLastSight )
		{
			//Before we retrace, make sure that we are spun down.
			m_flLastSight = 0;
			m_flSpinUpTime = 0;
			SetThink( &CBaseTurret::Retire );
		}
		// should we stop the spin?
		else if( ( m_flSpinUpTime ) && ( gpGlobals->time > m_flSpinUpTime ) )
		{
			SpinDownCall();
		}
		
		// generic hunt for new victims
		m_vecGoalAngles.y = ( m_vecGoalAngles.y + 0.1f * m_fTurnRate );
		if( m_vecGoalAngles.y >= 360 )
			m_vecGoalAngles.y -= 360;
		MoveTurret();
	}
}

//
// This think function will deploy the turret when something comes into range. This is for
// automatically activated turrets.
//
void CBaseTurret::AutoSearchThink( void )
{
	// ensure rethink
	StudioFrameAdvance();

	pev->nextthink = gpGlobals->time + 0.3f;
	GlowShellUpdate();

	// If we have a target and we're still healthy
	if( m_hEnemy != 0 )
	{
		if( !m_hEnemy->IsAlive() )
			m_hEnemy = NULL;// Dead enemy forces a search for new one
	}

	// Acquire Target
	if( m_hEnemy == 0 )
	{
		Look( TURRET_RANGE );
		SetEnemy(BestVisibleEnemy());
	}

	if( m_hEnemy != 0 )
	{
		SetThink( &CBaseTurret::Deploy );
		EmitSoundScript(alertSoundScript);
	}
}

void CBaseTurret::TurretDeath( void )
{
	//BOOL iActive = FALSE;

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;
	GlowShellUpdate();

	if( pev->deadflag != DEAD_DEAD )
	{
		pev->deadflag = DEAD_DEAD;
		FCheckAITrigger();

		EmitSoundScript(dieSoundScript);

		StopSoundScript(spinupSoundScript);

		if( m_iOrientation == 0 )
			m_vecGoalAngles.x = -15;
		else
			m_vecGoalAngles.x = -90;

		SetTurretAnim( TURRET_ANIM_DIE ); 

		EyeOn();
	}

	EyeOff();

	if( pev->dmgtime + RANDOM_FLOAT( 0, 2 ) > gpGlobals->time )
	{
		// lots of smoke
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( RANDOM_FLOAT( pev->absmin.x, pev->absmax.x ) );
			WRITE_COORD( RANDOM_FLOAT( pev->absmin.y, pev->absmax.y ) );
			WRITE_COORD( pev->origin.z - m_iOrientation * 64 );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 25 ); // scale * 10
			WRITE_BYTE( 10 - m_iOrientation * 5 ); // framerate
		MESSAGE_END();
	}

	if( pev->dmgtime + RANDOM_FLOAT( 0, 5 ) > gpGlobals->time )
	{
		Vector vecSrc = Vector( RANDOM_FLOAT( pev->absmin.x, pev->absmax.x ), RANDOM_FLOAT( pev->absmin.y, pev->absmax.y ), 0 );
		if( m_iOrientation == 0 )
			vecSrc = vecSrc + Vector( 0, 0, RANDOM_FLOAT( pev->origin.z, pev->absmax.z ) );
		else
			vecSrc = vecSrc + Vector( 0, 0, RANDOM_FLOAT( pev->absmin.z, pev->origin.z ) );

		UTIL_Sparks( vecSrc );
	}

	if( m_fSequenceFinished && !MoveTurret() && pev->dmgtime + 5 < gpGlobals->time )
	{
		pev->framerate = 0;
		SetThink( NULL );
	}
}

void CBaseTurret::TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if( ptr->iHitgroup == 10 )
	{
		// hit armor
		if( pev->dmgtime != gpGlobals->time || (RANDOM_LONG( 0, 10 ) < 1 ) )
		{
			UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT( 1, 2 ) );
			pev->dmgtime = gpGlobals->time;
		}

		flDamage = 0.1;// don't hurt the monster much, but allow bits_COND_LIGHT_DAMAGE to be generated
	}

	if( !pev->takedamage )
		return;

	AddMultiDamage( pevInflictor, pevAttacker, this, flDamage, bitsDamageType );
}

// take damage. bitsDamageType indicates type of damage sustained, ie: DMG_BULLET
int CBaseTurret::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if( !pev->takedamage )
		return 0;

	if( !m_iOn )
		flDamage *= 0.1f;

	AddScoreForDamage(pevAttacker, this, flDamage);

	pev->health -= flDamage;
	if( pev->health <= 0 )
	{
		//HACK to trigger on death condition
		const int deadflag = pev->deadflag;
		pev->deadflag = DEAD_DEAD;
		FCheckAITrigger();
		pev->deadflag = deadflag;

		pev->health = 0;
		pev->takedamage = DAMAGE_NO;
		pev->dmgtime = gpGlobals->time;

		ClearBits( pev->flags, FL_MONSTER ); // why are they set in the first place???

		SetUse( NULL );
		SetThink( &CBaseTurret::TurretDeath );
		SUB_UseTargets( this, USE_ON ); // wake up others
		pev->nextthink = gpGlobals->time + 0.1f;

		return 0;
	} else {
		SetConditions(bits_COND_LIGHT_DAMAGE);
		FCheckAITrigger();
		ClearConditions(bits_COND_LIGHT_DAMAGE);
	}

	if( pev->health <= 10 )
	{
		if( m_iOn && ( 1 || RANDOM_LONG( 0, 0x7FFF ) > 800 ) )
		{
			m_fBeserk = 1;
			SetThink( &CBaseTurret::SearchThink );
		}
	}

	return 1;
}

int CBaseTurret::MoveTurret( void )
{
	int state = 0;
	// any x movement?

	if( m_vecCurAngles.x != m_vecGoalAngles.x )
	{
		float flDir = m_vecGoalAngles.x > m_vecCurAngles.x ? 1 : -1 ;

		m_vecCurAngles.x += 0.1f * m_fTurnRate * flDir;

		// if we started below the goal, and now we're past, peg to goal
		if( flDir == 1 )
		{
			if( m_vecCurAngles.x > m_vecGoalAngles.x )
				m_vecCurAngles.x = m_vecGoalAngles.x;
		} 
		else
		{
			if( m_vecCurAngles.x < m_vecGoalAngles.x )
				m_vecCurAngles.x = m_vecGoalAngles.x;
		}

		if( m_iOrientation == 0 )
			SetBoneController( 1, -m_vecCurAngles.x );
		else
			SetBoneController( 1, m_vecCurAngles.x );
		state = 1;
	}

	if( m_vecCurAngles.y != m_vecGoalAngles.y )
	{
		float flDir = m_vecGoalAngles.y > m_vecCurAngles.y ? 1 : -1 ;
		float flDist = fabs( m_vecGoalAngles.y - m_vecCurAngles.y );

		if( flDist > 180 )
		{
			flDist = 360 - flDist;
			flDir = -flDir;
		}
		if( flDist > 30 )
		{
			if( m_fTurnRate < m_iBaseTurnRate * 10 )
			{
				m_fTurnRate += m_iBaseTurnRate;
			}
		}
		else if( m_fTurnRate > 45 )
		{
			m_fTurnRate -= m_iBaseTurnRate;
		}
		else
		{
			m_fTurnRate += m_iBaseTurnRate;
		}

		m_vecCurAngles.y += 0.1f * m_fTurnRate * flDir;

		if( m_vecCurAngles.y < 0 )
			m_vecCurAngles.y += 360;
		else if( m_vecCurAngles.y >= 360 )
			m_vecCurAngles.y -= 360;

		if( flDist < ( 0.05f * m_iBaseTurnRate ) )
			m_vecCurAngles.y = m_vecGoalAngles.y;

		//ALERT( at_console, "%.2f -> %.2f\n", m_vecCurAngles.y, y );
		if( m_iOrientation == 0 )
			SetBoneController( 0, m_vecCurAngles.y - pev->angles.y );
		else 
			SetBoneController( 0, pev->angles.y - 180 - m_vecCurAngles.y );
		state = 1;
	}

	if( !state )
		m_fTurnRate = m_iBaseTurnRate;

	//ALERT( at_console, "(%.2f, %.2f)->(%.2f, %.2f)\n", m_vecCurAngles.x,
	//	m_vecCurAngles.y, m_vecGoalAngles.x, m_vecGoalAngles.y );
	return state;
}

//
// ID as a machine
//
int CBaseTurret::Classify( void )
{
	if( m_iOn || m_iAutoStart )
		return	CBaseMonster::Classify();
	return CLASS_NONE;
}

int CBaseTurret::RealClassify()
{
	return CBaseMonster::Classify();
}

void CBaseTurret::SetEnemy(CBaseEntity *enemy)
{
	m_hEnemy = enemy;
	if (m_hEnemy) {
		SetConditions(bits_COND_SEE_ENEMY);
		FCheckAITrigger();
		ClearConditions(bits_COND_SEE_ENEMY);
	}
}

int CBaseTurret::DefaultClassify()
{
	return CLASS_MACHINE;
}

//=========================================================
// Sentry gun - smallest turret, placed near grunt entrenchments
//=========================================================
class CSentry : public CBaseTurret
{
public:
	void Spawn();
	void Precache( void );
	// other functions
	const char* DefaultDisplayName() { return "Sentry Turret"; }
	void Shoot( Vector &vecSrc, Vector &vecDirToEnemy );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void EXPORT SentryTouch( CBaseEntity *pOther );
	void EXPORT SentryDeath( void );

	static constexpr const char* shootSoundScript = "Sentry.Shoot";
};

LINK_ENTITY_TO_CLASS( monster_sentry, CSentry )

void CSentry::Precache()
{
	CBaseTurret::Precache();
	PrecacheMyModel( "models/sentry.mdl" );
	RegisterAndPrecacheSoundScript(shootSoundScript, NPC::single9mmSoundScript);
}

void CSentry::Spawn()
{
	Precache();
	SetMyModel( "models/sentry.mdl" );
	SetMyHealth( gSkillData.sentryHealth );
	m_HackedGunPos = Vector( 0, 0, 48 );
	pev->view_ofs.z = 48;
	if (!g_modFeatures.sentry_retract)
		m_flMaxWait = 1E6;
	m_flMaxSpin = 1E6;

	CBaseTurret::Spawn();
	m_iRetractHeight = 64;
	m_iDeployHeight = 64;
	m_iMinPitch = -60;
	UTIL_SetSize( pev, Vector( -16, -16, -m_iRetractHeight ), Vector( 16, 16, m_iRetractHeight ) );

	SetTouch( &CSentry::SentryTouch );
	SetThink( &CBaseTurret::Initialize );	
	pev->nextthink = gpGlobals->time + 0.3f;
}

void CSentry::Shoot( Vector &vecSrc, Vector &vecDirToEnemy )
{
	FireBullets( 1, vecSrc, vecDirToEnemy, TURRET_SPREAD, TURRET_RANGE, BULLET_MONSTER_MP5, 1 );
	EmitSoundScript(shootSoundScript);
	pev->effects = pev->effects | EF_MUZZLEFLASH;
}

int CSentry::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if( !pev->takedamage )
		return 0;

	if( !m_iOn )
	{
		SetThink( &CBaseTurret::Deploy );
		SetUse( NULL );
		pev->nextthink = gpGlobals->time + 0.1f;
	}

	AddScoreForDamage(pevAttacker, this, flDamage);

	pev->health -= flDamage;
	if( pev->health <= 0 )
	{
		pev->health = 0;
		pev->takedamage = DAMAGE_NO;
		pev->dmgtime = gpGlobals->time;

		ClearBits( pev->flags, FL_MONSTER ); // why are they set in the first place???

		SetUse( NULL );
		SetThink( &CSentry::SentryDeath );
		SUB_UseTargets( this, USE_ON ); // wake up others
		pev->nextthink = gpGlobals->time + 0.1f;

		return 0;
	} else {
		SetConditions(bits_COND_LIGHT_DAMAGE);
		FCheckAITrigger();
		ClearConditions(bits_COND_LIGHT_DAMAGE);
	}

	return 1;
}

void CSentry::SentryTouch( CBaseEntity *pOther )
{
	if( pOther && ( pOther->IsPlayer() || ( pOther->pev->flags & FL_MONSTER ) ) && IDefaultRelationship(RealClassify(), pOther->Classify()) >= R_DL )
	{
		TakeDamage( pOther->pev, pOther->pev, 0, 0 );
	}
}

void CSentry::SentryDeath( void )
{
	//BOOL iActive = FALSE;

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;
	GlowShellUpdate();

	if( pev->deadflag != DEAD_DEAD )
	{
		pev->deadflag = DEAD_DEAD;
		FCheckAITrigger();

		EmitSoundScript(dieSoundScript);

		StopSoundScript(spinupSoundScript);

		SetBoneController( 0, 0 );
		SetBoneController( 1, 0 );

		SetTurretAnim( TURRET_ANIM_DIE );

		pev->solid = SOLID_NOT;
		pev->angles.y = UTIL_AngleMod( pev->angles.y + RANDOM_LONG( 0, 2 ) * 120 );

		EyeOn();
	}

	EyeOff();

	Vector vecSrc, vecAng;
	GetAttachment( 1, vecSrc, vecAng );

	if( pev->dmgtime + RANDOM_FLOAT( 0, 2 ) > gpGlobals->time )
	{
		// lots of smoke
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( vecSrc.x + RANDOM_FLOAT( -16, 16 ) );
			WRITE_COORD( vecSrc.y + RANDOM_FLOAT( -16, 16 ) );
			WRITE_COORD( vecSrc.z - 32 );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 15 ); // scale * 10
			WRITE_BYTE( 8 ); // framerate
		MESSAGE_END();
	}

	if( pev->dmgtime + RANDOM_FLOAT( 0, 8 ) > gpGlobals->time )
	{
		UTIL_Sparks( vecSrc );
	}

	if( m_fSequenceFinished && pev->dmgtime + 5 < gpGlobals->time )
	{
		pev->framerate = 0;
		SetThink( NULL );
	}
}

class CBaseDeadTurret : public CBaseAnimating
{
public:
	void Spawn();
	void Precache();
	void KeyValue( KeyValueData *pkvd );
	void TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );

protected:
	virtual void SetMyModel();
	virtual const char* DefaultModel() = 0;

	int m_iOrientation; // no need to save
};

void CBaseDeadTurret::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "orientation" ) )
	{
		m_iOrientation = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseAnimating::KeyValue(pkvd);
}

void CBaseDeadTurret::Precache()
{
	if (FStringNull(pev->model)) {
		PRECACHE_MODEL( DefaultModel() );
	} else {
		PRECACHE_MODEL( STRING(pev->model) );
	}
}

void CBaseDeadTurret::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid		= SOLID_SLIDEBOX;
	pev->sequence = TURRET_ANIM_DIE;
	pev->frame = 255.0f;
	pev->takedamage	= DAMAGE_NO;

	ResetSequenceInfo();

	SetMyModel();

	SetBoneController( 0, 0 );
	if (m_iOrientation == 0)
		SetBoneController( 1, 0 );
	else
		SetBoneController( 1, -90 );

	if( m_iOrientation == 1 )
	{
		pev->idealpitch = 180;
		pev->angles.x = 180;
		pev->effects |= EF_INVLIGHT;
		pev->angles.y = pev->angles.y + 180;
		if( pev->angles.y > 360 )
			pev->angles.y = pev->angles.y - 360;
	}
}

void CBaseDeadTurret::SetMyModel()
{
	if (FStringNull(pev->model)) {
		SET_MODEL( ENT( pev ), DefaultModel() );
	} else {
		SET_MODEL( ENT( pev ), STRING(pev->model) );
	}
}

void CBaseDeadTurret::TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if( ptr->iHitgroup == 10 )
	{
		// hit armor
		if( pev->dmgtime != gpGlobals->time || (RANDOM_LONG( 0, 10 ) < 1 ) )
		{
			UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT( 1, 2 ) );
			pev->dmgtime = gpGlobals->time;
		}
	}
}

class CDeadTurret : public CBaseDeadTurret
{
public:
	void Spawn();
protected:
	const char* DefaultModel() {
		return "models/turret.mdl";
	}
};

LINK_ENTITY_TO_CLASS( monster_turret_dead, CDeadTurret )

void CDeadTurret::Spawn()
{
	CBaseDeadTurret::Spawn();
	UTIL_SetSize( pev, Vector( -32, -32, -16 ), Vector( 32, 32, 16 ) );
}

class CDeadMiniTurret : public CBaseDeadTurret
{
public:
	void Spawn();
protected:
	const char* DefaultModel() {
		return "models/miniturret.mdl";
	}
};

LINK_ENTITY_TO_CLASS( monster_miniturret_dead, CDeadMiniTurret )

void CDeadMiniTurret::Spawn()
{
	CBaseDeadTurret::Spawn();
	UTIL_SetSize( pev, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ) );
}

class CDeadSentry : public CBaseDeadTurret
{
public:
	void Spawn();
protected:
	const char* DefaultModel() {
		return "models/sentry.mdl";
	}
};

LINK_ENTITY_TO_CLASS( monster_sentry_dead, CDeadSentry )

void CDeadSentry::Spawn()
{
	CBaseDeadTurret::Spawn();
	pev->solid = SOLID_NOT;
	SetBoneController( 1, 0 );
	UTIL_SetSize( pev, Vector( -16, -16, -64 ), Vector( 16, 16, 64 ) );
}
