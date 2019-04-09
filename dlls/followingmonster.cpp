#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "nodes.h"
#include "monsters.h"
#include "animation.h"
#include "saverestore.h"
#include "squadmonster.h"
#include "followingmonster.h"
#include "scripted.h"
#include "soundent.h"

Task_t tlFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE, (float)128 },	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_FACE },
};

Schedule_t slFollow[] =
{
	{
		tlFollow,
		ARRAYSIZE( tlFollow ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

Task_t tlFaceTarget[] =
{
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_FACE_TARGET, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE },
};

Schedule_t slFaceTarget[] =
{
	{
		tlFaceTarget,
		ARRAYSIZE( tlFaceTarget ),
		bits_COND_CLIENT_PUSH |
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};

Task_t tlMoveAway[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_MOVE_AWAY_FAIL },
	{ TASK_STORE_LASTPOSITION, (float)0 },
	{ TASK_MOVE_AWAY_PATH, (float)100 },
	{ TASK_WALK_PATH_FOR_UNITS, (float)100 },
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_PLAYER, (float)0.5 },
};

Schedule_t slMoveAway[] =
{
	{
		tlMoveAway,
		ARRAYSIZE( tlMoveAway ),
		0,
		0,
		"MoveAway"
	},
};

Task_t tlMoveAwayFail[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_PLAYER, (float)0.5 },
};

Schedule_t slMoveAwayFail[] =
{
	{
		tlMoveAwayFail,
		ARRAYSIZE( tlMoveAwayFail ),
		0,
		0,
		"MoveAwayFail"
	},
};

Task_t tlMoveAwayFollow[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_TARGET_FACE },
	{ TASK_STORE_LASTPOSITION, (float)0 },
	{ TASK_MOVE_AWAY_PATH, (float)100 },
	{ TASK_WALK_PATH_FOR_UNITS, (float)100 },
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_FACE },
};

Schedule_t slMoveAwayFollow[] =
{
	{
		tlMoveAwayFollow,
		ARRAYSIZE( tlMoveAwayFollow ),
		0,
		0,
		"MoveAwayFollow"
	},
};

DEFINE_CUSTOM_SCHEDULES( CFollowingMonster )
{
	slFollow,
	slFaceTarget,
	slMoveAway,
	slMoveAwayFollow,
	slMoveAwayFail,
};

IMPLEMENT_CUSTOM_SCHEDULES( CFollowingMonster, CSquadMonster )

bool CFollowingMonster::CanBePushedByClient(CBaseEntity *pOther)
{
	// Ignore if pissed at player
	return IRelationship(pOther) == R_AL;
}

void CFollowingMonster::Touch( CBaseEntity *pOther )
{
	// Did the player touch me?
	if( pOther->IsPlayer() )
	{
		if( !CanBePushedByClient(pOther) )
			return;

		// Heuristic for determining if the player is pushing me away
		float speed = fabs( pOther->pev->velocity.x ) + fabs( pOther->pev->velocity.y );
		if( speed > 50 )
		{
			SetConditions( bits_COND_CLIENT_PUSH );
			MakeIdealYaw( pOther->pev->origin );
		}
	}
}

void CFollowingMonster::OnDying()
{
	m_hTargetEnt = 0;
	SetUse( NULL );
	CSquadMonster::OnDying();
}

int CFollowingMonster::ObjectCaps()
{
	int caps = CSquadMonster::ObjectCaps();
	if (m_afCapability & bits_CAP_USABLE)
	{
		caps |= FCAP_IMPULSE_USE | FCAP_ONLYDIRECT_USE;
	}
	return caps;
}

Schedule_t *CFollowingMonster::GetScheduleOfType( int Type )
{
	switch( Type )
	{
	case SCHED_MOVE_AWAY:
		return slMoveAway;
	case SCHED_MOVE_AWAY_FOLLOW:
		return slMoveAwayFollow;
	case SCHED_MOVE_AWAY_FAIL:
		return slMoveAwayFail;
	case SCHED_TARGET_FACE:
		return slFaceTarget;
	case SCHED_TARGET_CHASE:
	case SCHED_FOLLOW:
		return slFollow;
	default:
		return CSquadMonster::GetScheduleOfType(Type);
	}
}

void CFollowingMonster::StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_FACE_PLAYER:
		m_flWaitFinished = gpGlobals->time + pTask->flData;
		break;
	case TASK_WALK_PATH_FOR_UNITS:
		m_movementActivity = ACT_WALK;
		break;
	case TASK_MOVE_AWAY_PATH:
		{
			Vector dir = pev->angles;
			dir.y = pev->ideal_yaw + 180;
			Vector move;

			UTIL_MakeVectorsPrivate( dir, move, NULL, NULL );
			dir = pev->origin + move * pTask->flData;
			if( MoveToLocation( ACT_WALK, 2, dir ) )
			{
				TaskComplete();
			}
			else if( FindCover( pev->origin, pev->view_ofs, 0, CoverRadius() ) )
			{
				// then try for plain ole cover
				m_flMoveWaitFinished = gpGlobals->time + 2;
				TaskComplete();
			}
			else
			{
				// nowhere to go?
				TaskFail();
			}
		}
		break;
	default:
		CSquadMonster::StartTask( pTask );
		break;
	}
}

void CFollowingMonster::RunTask( Task_t *pTask )
{
	edict_t *pPlayer;
	switch( pTask->iTask )
	{
	case TASK_FACE_PLAYER:
		{
			// Get edict for one player
			pPlayer = g_engfuncs.pfnPEntityOfEntIndex( 1 );

			if( pPlayer )
			{
				MakeIdealYaw( pPlayer->v.origin );
				ChangeYaw( pev->yaw_speed );
				IdleHeadTurn( pPlayer->v.origin );
				if( gpGlobals->time > m_flWaitFinished && FlYawDiff() < 10 )
				{
					TaskComplete();
				}
			}
			else
			{
				TaskFail();
			}
		}
		break;
	case TASK_WALK_PATH_FOR_UNITS:
		{
			float distance = ( m_vecLastPosition - pev->origin ).Length2D();

			// Walk path until far enough away
			if( distance > pTask->flData || MovementIsComplete() )
			{
				TaskComplete();
				RouteClear();		// Stop moving
			}
		}
		break;
	default:
		CSquadMonster::RunTask( pTask );
		break;
	}
}

void CFollowingMonster::PrescheduleThink()
{
	if (IsFollowingPlayer() && IsLockedByMaster())
	{
		StopFollowing(TRUE, false);
	}
	CSquadMonster::PrescheduleThink();
}

void CFollowingMonster::FollowingMonsterInit()
{
	MonsterInit();
	if (IDefaultRelationship(CLASS_PLAYER) == R_AL) {
		m_afCapability |= bits_CAP_USABLE;
		SetUse( &CFollowingMonster::FollowerUse );
	}
}

// turn head towards supplied origin
void CFollowingMonster::IdleHeadTurn( Vector &vecFriend )
{
	// turn head in desired direction only if ent has a turnable head
	if( m_afCapability & bits_CAP_TURN_HEAD )
	{
		float yaw = VecToYaw( vecFriend - pev->origin ) - pev->angles.y;

		if( yaw > 180 )
			yaw -= 360;
		if( yaw < -180 )
			yaw += 360;

		// turn towards vector
		SetBoneController( 0, yaw );
	}
}

void CFollowingMonster::StopFollowing(BOOL clearSchedule , bool saySentence)
{
	if( IsFollowingPlayer() )
	{
		if( saySentence && !( m_afMemory & bits_MEMORY_PROVOKED ) )
		{
			PlayUnUseSentence();
		}

		if( m_movementGoal == MOVEGOAL_TARGETENT && m_hTargetEnt == FollowedPlayer() )
			RouteClear(); // Stop him from walking toward the player
		ClearFollowedPlayer();
		if( clearSchedule )
			ClearSchedule();
		if( m_hEnemy != 0 )
			m_IdealMonsterState = MONSTERSTATE_COMBAT;
	}
}

void CFollowingMonster::StartFollowing(CBaseEntity *pLeader , bool saySentence)
{
	if( m_pCine )
		m_pCine->CancelScript();

	if( m_hEnemy != 0 )
		m_IdealMonsterState = MONSTERSTATE_ALERT;

	m_hTargetEnt = pLeader;
	if (saySentence)
	{
		PlayUseSentence();
	}

	ClearConditions( bits_COND_CLIENT_PUSH );
	ClearSchedule();
}

void CFollowingMonster::LimitFollowers( CBaseEntity *pPlayer, int maxFollowers )
{
	return;
}

BOOL CFollowingMonster::CanFollow( void )
{
	return AbleToFollow() && !IsFollowingPlayer();
}

BOOL CFollowingMonster::AbleToFollow()
{
	if( m_MonsterState == MONSTERSTATE_SCRIPT )
	{
		if( !m_pCine )
			return FALSE;
		if( !m_pCine->CanInterrupt() )
			return FALSE;
	}

	if( !IsAlive() )
		return FALSE;
	return TRUE;
}

BOOL CFollowingMonster::IsFollowingPlayer(CBaseEntity *pLeader)
{
	return FollowedPlayer() == pLeader;
}

BOOL CFollowingMonster::IsFollowingPlayer()
{
	return FollowedPlayer() != 0;
}

CBaseEntity* CFollowingMonster::FollowedPlayer()
{
	if (m_hTargetEnt != 0 && m_hTargetEnt->IsPlayer())
		return m_hTargetEnt;
	return NULL;
}

void CFollowingMonster::ClearFollowedPlayer()
{
	m_hTargetEnt = 0;
}

bool CFollowingMonster::ReadyForUse()
{
	return true;
}

Schedule_t* CFollowingMonster::GetFollowingSchedule()
{
	if( m_hEnemy == 0 && IsFollowingPlayer() )
	{
		if( !FollowedPlayer()->IsAlive() )
		{
			// UNDONE: Comment about the recently dead player here?
			StopFollowing( FALSE, false );
			return NULL;
		}
		else
		{
			if( HasConditions( bits_COND_CLIENT_PUSH ) )
			{
				return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
			}
			return GetScheduleOfType( SCHED_TARGET_FACE );
		}
	}

	if( HasConditions( bits_COND_CLIENT_PUSH ) )
	{
		return GetScheduleOfType( SCHED_MOVE_AWAY );
	}
	return NULL;
}

void CFollowingMonster::FollowerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !ReadyForUse() )
		return;

	if( pCaller != NULL && pCaller->IsPlayer() && IRelationship(pCaller) < R_DL && IRelationship(pCaller) != R_FR )
	{
		// Pre-disaster followers can't be used unless they've got a master to override their behaviour...
		if (IsLockedByMaster() || (pev->spawnflags & SF_MONSTER_PREDISASTER && !m_sMaster))
		{
			DeclineFollowing();
		}
		else if( CanFollow() )
		{
			LimitFollowers( pCaller, MaxFollowers() );

			if( m_afMemory & bits_MEMORY_PROVOKED )
				ALERT( at_console, "I'm not following you, you evil person!\n" );
			else
			{
				StartFollowing( pCaller );
			}
		}
		else
		{
			StopFollowing( TRUE );
		}
	}
}

