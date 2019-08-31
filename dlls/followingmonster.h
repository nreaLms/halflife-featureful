#pragma once
#ifndef FOLLOWINGMOSNTER_H
#define FOLLOWINGMONSTER_H
#ifndef MONSTERS_H
#include "monsters.h"
#endif
#include "squadmonster.h"

enum
{
	SCHED_MOVE_AWAY = LAST_COMMON_SCHEDULE + 1,		// Try to get out of the player's way
	SCHED_MOVE_AWAY_FOLLOW,	// same, but follow afterward
	SCHED_MOVE_AWAY_FAIL,	// Turn back toward player

	LAST_FOLLOWINGMONSTER_SCHEDULE		// MUST be last
};

enum
{
	TASK_MOVE_AWAY_PATH = LAST_COMMON_TASK + 1,
	TASK_WALK_PATH_FOR_UNITS,
	TASK_FACE_PLAYER,		// Face the player

	LAST_FOLLOWINGMONSTER_TASK			// MUST be last
};

class CFollowingMonster : public CSquadMonster
{
public:
	// Base Monster functions
	virtual bool CanBePushedByClient(CBaseEntity* pOther);
	void Touch(	CBaseEntity *pOther );
	void OnDying();
	int ObjectCaps( void );

	// AI functions
	Schedule_t *GetScheduleOfType ( int Type );
	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );
	void PrescheduleThink( void );

	void FollowingMonsterInit();
	void IdleHeadTurn( Vector &vecFriend );

	// Following related
	BOOL	CanFollow( void );
	BOOL	AbleToFollow();
	BOOL	IsFollowingPlayer( CBaseEntity* pLeader );
	BOOL	IsFollowingPlayer( void );
	virtual	CBaseEntity* FollowedPlayer();
	virtual void ClearFollowedPlayer();
	virtual void StopFollowing(BOOL clearSchedule, bool saySentence = true );
	virtual void StartFollowing( CBaseEntity *pLeader, bool saySentence = true );
	virtual void DeclineFollowing( CBaseEntity* pCaller ) {}
	virtual void LimitFollowers( CBaseEntity *pPlayer, int maxFollowers );
	virtual int MaxFollowers() { return 3; }

	CFollowingMonster* MyFollowingMonsterPointer() { return this; }
	virtual bool ReadyForUse();
	Schedule_t* GetFollowingSchedule();
	void EXPORT FollowerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void DoFollowerUse(CBaseEntity* pCaller, bool saySentence);

	virtual void PlayUseSentence() {}
	virtual void PlayUnUseSentence() {}

	CBaseEntity* PlayerToFace();

	CUSTOM_SCHEDULES
};

#endif
