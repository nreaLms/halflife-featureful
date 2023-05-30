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
#define SF_SCRIPT_AUTOSEARCH		1024
#define SF_SCRIPT_TRY_ONCE	4096
#define SF_SCRIPT_DONT_RESET_HEAD	8192
#define SF_SCRIPT_FORCE_IDLE_LOOPING 16384
#define SF_REMOVE_ON_INTERRUPTION	32768

#define SCRIPT_BREAK_CONDITIONS		(bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE)

//LRC - rearranged into flags
#define SS_INTERRUPT_IDLE		0x0
#define SS_INTERRUPT_ALERT		0x1
#define SS_INTERRUPT_ANYSTATE	0x2
#define SS_INTERRUPT_SCRIPTS	0x4

enum SCRIPT_MOVE_TYPE
{
	SCRIPT_MOVE_NO = 0,
	SCRIPT_MOVE_WALK = 1,
	SCRIPT_MOVE_RUN = 2,
	SCRIPT_MOVE_INSTANT = 4,
	SCRIPT_MOVE_FACE = 5,
	SCRIPT_MOVE_TELEPORT = 7,
};

enum SCRIPT_TURN_TYPE
{
	SCRIPT_TURN_MATCH_ANGLE = 0,
	SCRIPT_TURN_FACE,
	SCRIPT_TURN_NO
};

enum SCRIPT_ACTION
{
	SCRIPT_ACT_RANGE_ATTACK = 0,
	SCRIPT_ACT_RANGE_ATTACK2,
	SCRIPT_ACT_MELEE_ATTACK,
	SCRIPT_ACT_MELEE_ATTACK2,
	SCRIPT_ACT_SPECIAL_ATTACK,
	SCRIPT_ACT_SPECIAL_ATTACK2,
	SCRIPT_ACT_RELOAD,
	SCRIPT_ACT_JUMP,
	SCRIPT_ACT_NO_ACTION,
};

#define SCRIPT_REQUIRED_FOLLOWER_STATE_UNSPECIFIED 0
#define SCRIPT_REQUIRED_FOLLOWER_STATE_FOLLOWING 1
#define SCRIPT_REQUIRED_FOLLOWER_STATE_NOT_FOLLOWING 2

enum
{
	SCRIPT_APPLY_SEARCH_RADIUS_TO_CLASSNAME = 0,
	SCRIPT_APPLY_SEARCH_RADIUS_ALWAYS = 1
};

enum
{
	SCRIPT_INTERRUPTION_POLICY_ANY_DAMAGE = 0,
	SCRIPT_INTERRUPTION_POLICY_NO_INTERRUPTIONS = 1,
	SCRIPT_INTERRUPTION_POLICY_ONLY_DEATH = 2,
};

enum
{
	SCRIPT_SEARCH_POLICY_ALL = 0,
	SCRIPT_SEARCH_POLICY_TARGETNAME_ONLY = 1,
	SCRIPT_SEARCH_POLICY_CLASSNAME_ONLY = 2,
};

enum
{
	SCRIPT_TAKE_DAMAGE_POLICY_DEFAULT = 0,
	SCRIPT_TAKE_DAMAGE_POLICY_INVULNERABLE = 1,
	SCRIPT_TAKE_DAMAGE_POLICY_NONLETHAL = 2,
};

enum
{
	SCRIPT_CANCELLATION_REASON_GENERIC = 0,
	SCRIPT_CANCELLATION_REASON_INTERRUPTED = 1,
	SCRIPT_CANCELLATION_REASON_STARTED_FOLLOWING = 2,
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
	virtual void UpdateOnRemove();

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
	bool MayReportInappropriateTarget(int checkFail);
	bool IsAppropriateTarget(CBaseMonster* pTarget, int interruptFlags, bool shouldCheckRadius, int* pCheckFail = 0);
	bool AcceptedFollowingState(CBaseMonster* pMonster);
	virtual void PossessEntity( void );

	inline bool IsAction( void ) { return FClassnameIs(pev, "scripted_action"); }; //LRC

	//LRC: Should the monster do a precise attack for this scripted_action?
	// (Do a precise attack if we'll be turning to face the target, but we haven't just walked to the target.)
	bool PreciseAttack( void )
	{
	//	if (m_fTurnType != 1) { ALERT(at_console,"preciseattack fails check 1\n"); return FALSE; }
	//	if (m_fMoveTo == 0) { ALERT(at_console,"preciseattack fails check 2\n"); return FALSE; }
	//	if (m_fMoveTo != 5 && m_iszAttack == 0) { ALERT(at_console,"preciseattack fails check 3\n"); return FALSE; }
	//	ALERT(at_console,"preciseattack passes!\n");
	//	return TRUE;
		return m_fTurnType == SCRIPT_TURN_FACE && ( m_fMoveTo == SCRIPT_MOVE_FACE || (m_fMoveTo != SCRIPT_MOVE_NO && !FStrEq(STRING(m_iszAttack), STRING(m_iszMoveTarget)) ));
	};


	void ReleaseEntity( CBaseMonster *pEntity );
	void CancelScript( int cancellationReason = SCRIPT_CANCELLATION_REASON_GENERIC );
	virtual BOOL StartSequence( CBaseMonster *pTarget, int iszSeq, BOOL completeOnEmpty );
	virtual BOOL FCanOverrideState ( void );
	void SequenceDone ( CBaseMonster *pMonster );
	virtual void FixScriptMonsterSchedule( CBaseMonster *pMonster );
	bool ForcedNoInterruptions();
	BOOL	CanInterrupt( void );
	bool	CanInterruptByPlayerCall();
	void	AllowInterrupt( BOOL fAllow );
	int		IgnoreConditions( void );
	virtual bool	ShouldResetOnGroundFlag();
	void OnMoveFail();
	bool MoveFailAttemptsExceeded() const;
	bool IsAutoSearch() const;
	CBaseEntity* GetActivator(CBaseEntity *pMonster);

	string_t m_iszIdle;		// string index for idle animation
	string_t m_iszPlay;		// string index for scripted animation
	string_t m_iszEntity;	// entity that is wanted for this script
	string_t m_iszAttack;	// entity to attack
	string_t m_iszMoveTarget; // entity to move to
	string_t m_iszFireOnBegin; // entity to fire when the sequence _starts_.
	int m_fMoveTo;
	int m_iFinishSchedule;
	float m_flRadius;		// range to search
	//float m_flRepeat;	// repeat rate
	int m_iRepeats; //LRC - number of times to repeat the animation
	int m_iRepeatsLeft; //LRC
	float m_fRepeatFrame; //LRC
	int m_iPriority;

	int m_iDelay;
	float m_startTime;

	int	m_saved_movetype;
	int	m_saved_solid;
	int m_saved_effects;
	//Vector m_vecOrigOrigin;
	BOOL m_interruptable;
	BOOL m_firedOnAnimStart;
	string_t m_iszFireOnAnimStart;
	string_t m_iszFireOnPossessed;
	short m_targetActivator;
	short m_fTurnType;
	short m_fAction;
	float m_flMoveToRadius;
	short m_requiredFollowerState;
	short m_applySearchRadius;
	short m_maxMoveFailAttempts;
	short m_moveFailCount;

	short m_interruptionPolicy;
	short m_searchPolicy;
	short m_requiredState;
	short m_takeDamagePolicy;

	bool m_cantFindReported; // no need to save
	bool m_cantPlayReported;
};

class CCineAI : public CCineMonster
{
	BOOL FCanOverrideState ( void );
	bool ShouldResetOnGroundFlag();
};
#endif //SCRIPTED_H
