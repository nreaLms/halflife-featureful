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
#pragma once
#if !defined(SCRIPTED_H)
#define SCRIPTED_H

#if !defined(SCRIPTEVENT_H)
#include "scriptevent.h"
#endif

#include "basemonster.h"

#define SF_SCRIPT_WAITTILLSEEN		1
#define SF_SCRIPT_EXITAGITATED		2
#define SF_SCRIPT_REPEATABLE		4
#define SF_SCRIPT_LEAVECORPSE		8
//#define SF_SCRIPT_INTERPOLATE		16 // don't use, old bug
#define SF_SCRIPT_NOINTERRUPT		32
#define SF_SCRIPT_OVERRIDESTATE		64
#define SF_SCRIPT_NOSCRIPTMOVEMENT	128
#define SF_SCRIPT_CONTINUOUS		256
#define SF_SCRIPT_APPLYNEWANGLES		512
#define SF_SCRIPT_FORCE_IDLE_LOOPING 2048
#define SF_SCRIPT_TRY_ONCE	4096

#define SCRIPT_BREAK_CONDITIONS		(bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE)

enum SCRIPT_MOVE_TYPE
{
	SCRIPT_MOVE_NO = 0,
	SCRIPT_MOVE_WALK = 1,
	SCRIPT_MOVE_RUN = 2,
	SCRIPT_MOVE_INSTANT = 4,
	SCRIPT_MOVE_FACE = 5,
	SCRIPT_MOVE_TELEPORT = 7,
};

enum SS_INTERRUPT
{
	SS_INTERRUPT_IDLE = 0,
	SS_INTERRUPT_BY_NAME,
	SS_INTERRUPT_AI
};

#define SCRIPT_REQUIRED_FOLLOWER_STATE_UNSPECIFIED 0
#define SCRIPT_REQUIRED_FOLLOWER_STATE_FOLLOWING 1
#define SCRIPT_REQUIRED_FOLLOWER_STATE_NOT_FOLLOWING 2

enum
{
	SCRIPT_APPLY_SEARCH_RADIUS_TO_CLASSNAME = 0,
	SCRIPT_APPLY_SEARCH_RADIUS_ALWAYS = 1
};

// when a monster finishes an AI scripted sequence, we can choose
// a schedule to place them in. These defines are the aliases to
// resolve worldcraft input to real schedules (sjb)
#define SCRIPT_FINISHSCHED_DEFAULT	0
#define SCRIPT_FINISHSCHED_AMBUSH	1

class CCineMonster : public CBaseMonster
{
public:
	void Spawn( void );
	virtual void KeyValue( KeyValueData *pkvd );
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void Blocked( CBaseEntity *pOther );
	virtual void Touch( CBaseEntity *pOther );
	virtual int	 ObjectCaps( void ) { return (CBaseMonster :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }
	virtual void Activate( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];

	// void EXPORT CineSpawnThink( void );
	void EXPORT CineThink( void );
	void Pain( void );
	void Die( void );
	void DelayStart( int state );
	CBaseMonster *FindEntity( void );
	bool TryFindAndPossessEntity();
	bool IsAppropriateTarget(CBaseMonster* pTarget, int interruptLevel, bool shouldCheckRadius);
	bool AcceptedFollowingState(CBaseMonster* pMonster);
	virtual void PossessEntity( void );

	void ReleaseEntity( CBaseMonster *pEntity );
	void CancelScript( void );
	virtual BOOL StartSequence( CBaseMonster *pTarget, int iszSeq, BOOL completeOnEmpty );
	virtual BOOL FCanOverrideState ( void );
	void SequenceDone ( CBaseMonster *pMonster );
	virtual void FixScriptMonsterSchedule( CBaseMonster *pMonster );
	BOOL	CanInterrupt( void );
	void	AllowInterrupt( BOOL fAllow );
	int		IgnoreConditions( void );
	virtual bool	ShouldResetOnGroundFlag();

	string_t m_iszIdle;		// string index for idle animation
	string_t m_iszPlay;		// string index for scripted animation
	string_t m_iszEntity;	// entity that is wanted for this script
	int m_fMoveTo;
	int m_iFinishSchedule;
	float m_flRadius;		// range to search
	float m_flRepeat;	// repeat rate

	int m_iDelay;
	float m_startTime;

	int	m_saved_movetype;
	int	m_saved_solid;
	int m_saved_effects;
	//Vector m_vecOrigOrigin;
	BOOL m_interruptable;
	string_t m_iszFireOnAnimStart;
	short m_targetActivator;
	short m_fTurnType;
	float m_flMoveToRadius;
	short m_requiredFollowerState;
	short m_applySearchRadius;

	bool m_cantFindReported; // no need to save
	bool m_cantPlayReported;
};

class CCineAI : public CCineMonster
{
	BOOL FCanOverrideState ( void );
	bool ShouldResetOnGroundFlag();
};
#endif //SCRIPTED_H
