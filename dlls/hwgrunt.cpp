#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"hgrunt.h"
#include	"animation.h"
#include	"mod_features.h"
#include	"game.h"
#include	"combat.h"

#if FEATURE_HWGRUNT

#define HWGRUNT_AE_DROP_GUN 11

enum
{
	SCHED_HWGRUNT_SHOOT = LAST_FOLLOWINGMONSTER_SCHEDULE + 1,
	SCHED_HWGRUNT_SPINDOWN,
	SCHED_HWGRUNT_REPEL,
	SCHED_HWGRUNT_REPEL_ATTACK,
	SCHED_HWGRUNT_REPEL_LAND,
};

enum
{
	TASK_HWGRUNT_PLAY_SPINDOWN = LAST_FOLLOWINGMONSTER_TASK + 1,
};

class CHWGrunt : public CFollowingMonster
{
public:
	void Spawn( void );
	void Precache( void );
	bool IsEnabledInMod() { return g_modFeatures.IsMonsterEnabled("hwgrunt"); }
	void SetYawSpeed( void );
	int DefaultClassify( void ) { return CLASS_HUMAN_MILITARY; }
	const char* DefaultDisplayName() { return "Heavy Weapons Grunt"; }
	const char* ReverseRelationshipModel() { return "models/hwgruntf.mdl"; }
	int DefaultISoundMask( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void SetActivity( Activity NewActivity );
	BOOL CheckMeleeAttack1( float flDot, float flDist ) {
		return FALSE;
	}
	BOOL CheckMeleeAttack2( float flDot, float flDist ) {
		return FALSE;
	}
	BOOL CheckRangeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack2( float flDot, float flDist ) {
		return FALSE;
	}
	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );

	void PlayUseSentence();
	void PlayUnUseSentence();

	void DeathSound( void );
	void PainSound( void );
	void Shoot();

	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType( int Type );

	virtual int SizeForGrapple() { return GRAPPLE_MEDIUM; }
	bool IsDisplaceable() { return true; }
	Vector DefaultMinHullSize() { return VEC_HUMAN_HULL_MIN; }
	Vector DefaultMaxHullSize() { return VEC_HUMAN_HULL_MAX; }

	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES

	float m_flNextPainTime;

	int		m_iM249Shell;
	int		m_iM249Link;

	static constexpr const char* painSoundScript = "HWGrunt.Pain";
	static constexpr const char* dieSoundScript = "HWGrunt.Die";

	static const NamedSoundScript shootSoundScript;
	static const NamedSoundScript spinDownSoundScript;
	static const NamedSoundScript spinUpSoundScript;

	static constexpr const char* useSoundScript = "HWGrunt.Use";
	static constexpr const char* unuseSoundScript = "HWGrunt.UnUse";
};

LINK_ENTITY_TO_CLASS( monster_hwgrunt, CHWGrunt )

TYPEDESCRIPTION	CHWGrunt::m_SaveData[] =
{
	DEFINE_FIELD( CHGrunt, m_flNextPainTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CHWGrunt, CFollowingMonster )

const NamedSoundScript CHWGrunt::shootSoundScript = {
	CHAN_WEAPON,
	{"hassault/hw_shoot2.wav", "hassault/hw_shoot3.wav"},
	"HWGrunt.Shoot"
};

const NamedSoundScript CHWGrunt::spinDownSoundScript = {
	CHAN_WEAPON,
	{"hassault/hw_spindown.wav"},
	"HWGrunt.Spindown"
};

const NamedSoundScript CHWGrunt::spinUpSoundScript = {
	CHAN_WEAPON,
	{"hassault/hw_spinup.wav"},
	"HWGrunt.Spinup"
};

void CHWGrunt::Spawn()
{
	Precache();

	SetMyModel( "models/hwgrunt.mdl" );
	SetMySize( DefaultMinHullSize(), DefaultMaxHullSize() );

	pev->solid		= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	SetMyBloodColor( BLOOD_COLOR_RED );
	pev->effects		= 0;
	SetMyHealth( gSkillData.hwgruntHealth );
	SetMyFieldOfView(0.2);// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextPainTime	= gpGlobals->time;

	m_afCapability		= bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;

	m_HackedGunPos = Vector( 0, 0, 55 );

	FollowingMonsterInit();
}

void CHWGrunt::Precache()
{
	PrecacheMyModel("models/hwgrunt.mdl");
	PrecacheMyGibModel();

	RegisterAndPrecacheSoundScript(painSoundScript, CHGrunt::painSoundScript);
	RegisterAndPrecacheSoundScript(dieSoundScript, CHGrunt::dieSoundScript);

	RegisterAndPrecacheSoundScript(shootSoundScript);
	RegisterAndPrecacheSoundScript(spinDownSoundScript);
	RegisterAndPrecacheSoundScript(spinUpSoundScript);

	RegisterAndPrecacheSoundScript(useSoundScript, CHGrunt::useSoundScript);
	RegisterAndPrecacheSoundScript(unuseSoundScript, CHGrunt::unuseSoundScript);

	m_iM249Shell = PRECACHE_MODEL ("models/saw_shell.mdl");// saw shell
	m_iM249Link = PRECACHE_MODEL ("models/saw_link.mdl");// saw link
}

void CHWGrunt::SetYawSpeed( void )
{
	int ys;

	switch( m_Activity )
	{
	case ACT_IDLE:
		ys = 150;
		break;
	case ACT_RUN:
		ys = 150;
		break;
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RANGE_ATTACK1:
		ys = 120;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	case ACT_GLIDE:
	case ACT_FLY:
		ys = 30;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

int CHWGrunt::DefaultISoundMask( void )
{
	return	bits_SOUND_WORLD |
			bits_SOUND_COMBAT |
			bits_SOUND_PLAYER |
			bits_SOUND_DANGER;
}

void CHWGrunt::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch(pEvent->event)
	{
	case HWGRUNT_AE_DROP_GUN:
		break;
	default:
		CFollowingMonster::HandleAnimEvent(pEvent);
		break;
	}
}

void CHWGrunt::SetActivity( Activity NewActivity )
{
	if (NewActivity == ACT_THREAT_DISPLAY)
	{
		EmitSoundScript(spinUpSoundScript);
	}
	CFollowingMonster::SetActivity(NewActivity);
}

BOOL CHWGrunt::CheckRangeAttack1( float flDot, float flDist )
{
	if( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5 && NoFriendlyFire() )
	{
		TraceResult tr;
		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget( vecSrc ), ignore_monsters, ignore_glass, ENT( pev ), &tr );

		if( tr.flFraction == 1.0 )
		{
			return TRUE;
		}
	}

	return FALSE;
}

void CHWGrunt::StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CFollowingMonster::StartTask( pTask );
		if( pev->movetype == MOVETYPE_FLY )
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;
	case TASK_HWGRUNT_PLAY_SPINDOWN:
	{
		EmitSoundScript(spinDownSoundScript);
		int iSequence = LookupSequence("spindown");
		if( iSequence > ACTIVITY_NOT_AVAILABLE )
		{
			pev->frame = 0;
			pev->sequence = iSequence;	// Set to the reset anim (if it's there)
			ResetSequenceInfo();
		}
		else
		{
			TaskComplete();
		}
	}
		break;
	default:
		CFollowingMonster::StartTask( pTask );
		break;
	}
}

void CHWGrunt::RunTask( Task_t *pTask )
{
	switch(pTask->iTask)
	{
	case TASK_HWGRUNT_PLAY_SPINDOWN:
		if (m_fSequenceFinished)
		{
			TaskComplete();
		}
		break;
	case TASK_PLAY_SEQUENCE_FACE_ENEMY:
	{
		if (m_Activity == ACT_THREAT_DISPLAY)
		{
			Vector vecShootOrigin = GetGunPosition();
			Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

			Vector angDir = UTIL_VecToAngles( vecShootDir );
			SetBlending( 0, angDir.x );
		}
		CFollowingMonster::RunTask(pTask);
	}
		break;
	case TASK_RANGE_ATTACK1:
		// The fire rate depends on Think rate which is 0.1 for monsters.
		Shoot();
		CFollowingMonster::RunTask(pTask);
		break;
	default:
		CFollowingMonster::RunTask(pTask);
		break;
	}
}

#define HWGRUNT_VOLUME 0.4

void CHWGrunt::PlayUseSentence()
{
	EmitSoundScript(useSoundScript);
}

void CHWGrunt::PlayUnUseSentence()
{
	EmitSoundScript(unuseSoundScript);
}

void CHWGrunt::DeathSound()
{
	EmitSoundScript(dieSoundScript);
}

void CHWGrunt::PainSound( void )
{
	if( gpGlobals->time > m_flNextPainTime )
	{
		EmitSoundScript(painSoundScript);

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

void CHWGrunt::Shoot()
{
	if( m_hEnemy == 0 )
	{
		return;
	}

	EmitSoundScript(shootSoundScript);

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	UTIL_MakeVectors ( pev->angles );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);

	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iM249Link, TE_BOUNCE_SHELL);

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_6DEGREES, 2048, BULLET_MONSTER_556 ); // shoot +-5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

Task_t tlHWGruntStartRangeAttack[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_THREAT_DISPLAY },
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_HWGRUNT_SPINDOWN },
	{ TASK_SET_SCHEDULE, (float)SCHED_HWGRUNT_SHOOT }
};

Schedule_t slHWGruntStartRangeAttack[] =
{
	{
		tlHWGruntStartRangeAttack,
		ARRAYSIZE( tlHWGruntStartRangeAttack ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED |
		bits_COND_NOFIRE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"HWGrunt Start Range Attack"
	},
};

Task_t tlHWGruntContinueRangeAttack[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_HWGRUNT_SPINDOWN },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_CHECK_FIRE, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_SET_SCHEDULE, (float)SCHED_HWGRUNT_SHOOT }
};

Schedule_t slHWGruntContinueRangeAttack[] =
{
	{
		tlHWGruntContinueRangeAttack,
		ARRAYSIZE( tlHWGruntContinueRangeAttack ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_ENEMY_LOST |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED |
		bits_COND_NOFIRE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"HWGrunt Continue Range Attack"
	},
};

Task_t tlHWGruntSpindown[] =
{
	{ TASK_HWGRUNT_PLAY_SPINDOWN, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t slHWGruntSpindown[] =
{
	{
		tlHWGruntSpindown,
		ARRAYSIZE( tlHWGruntSpindown ),
		0,
		0,
		"HWGrunt Spindown"
	},
};

Task_t tlHWGruntRepel[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_GLIDE },
};

Schedule_t	slHWGruntRepel[] =
{
	{
		tlHWGruntRepel,
		ARRAYSIZE( tlHWGruntRepel ),
		bits_COND_SEE_ENEMY |
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER |
		bits_SOUND_COMBAT |
		bits_SOUND_PLAYER,
		"Repel"
	},
};

//=========================================================
// repel land
//=========================================================
Task_t tlHWGruntRepelLand[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_LAND },
	{ TASK_GET_PATH_TO_LASTPOSITION, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_CLEAR_LASTPOSITION, (float)0 },
};

Schedule_t slHWGruntRepelLand[] =
{
	{
		tlHWGruntRepelLand,
		ARRAYSIZE( tlHWGruntRepelLand ),
		bits_COND_SEE_ENEMY |
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER |
		bits_SOUND_COMBAT |
		bits_SOUND_PLAYER,
		"Repel Land"
	},
};

DEFINE_CUSTOM_SCHEDULES( CHWGrunt )
{
	slHWGruntStartRangeAttack,
	slHWGruntContinueRangeAttack,
	slHWGruntSpindown,
	slHWGruntRepel,
	slHWGruntRepelLand,
};

IMPLEMENT_CUSTOM_SCHEDULES( CHWGrunt, CFollowingMonster )

Schedule_t *CHWGrunt::GetSchedule( void )
{
	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling.
	if( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if( pev->flags & FL_ONGROUND )
		{
			// just landed
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType( SCHED_HWGRUNT_REPEL_LAND );
		}
		else
		{
			return GetScheduleOfType( SCHED_HWGRUNT_REPEL );
		}
	}

	switch (m_MonsterState)
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_HUNT:
	{
		Schedule_t* followingSchedule = GetFollowingSchedule();
		if (followingSchedule)
			return followingSchedule;
		break;
	}
	default:
		break;
	}
	return CFollowingMonster::GetSchedule();
}

Schedule_t* CHWGrunt::GetScheduleOfType(int Type)
{
	switch(Type)
	{
	case SCHED_RANGE_ATTACK1:
		{
			return slHWGruntStartRangeAttack;
		}
	case SCHED_HWGRUNT_SHOOT:
		{
			return slHWGruntContinueRangeAttack;
		}
	case SCHED_HWGRUNT_SPINDOWN:
		{
			return slHWGruntSpindown;
		}
	case SCHED_HWGRUNT_REPEL:
		{
			if( pev->velocity.z > -128 )
				pev->velocity.z -= 32;
			return &slHWGruntRepel[0];
		}
	case SCHED_HWGRUNT_REPEL_LAND:
		{
			return &slHWGruntRepelLand[0];
		}
	default:
		{
			return CFollowingMonster::GetScheduleOfType(Type);
		}
	}
}

class CHWGruntRepel : public CHGruntRepel
{
public:
	const char* TrooperName() {
		return "monster_hwgrunt";
	}
};

LINK_ENTITY_TO_CLASS(monster_hwgrunt_repel, CHWGruntRepel)

#endif
