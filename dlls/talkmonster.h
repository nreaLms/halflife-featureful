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
#ifndef TALKMONSTER_H
#define TALKMONSTER_H

#include "followingmonster.h"

//=========================================================
// Talking monster base class
// Used for scientists and barneys
//=========================================================

#define TALKRANGE_MIN 500.0				// don't talk to anyone farther away than this

#define TLK_STARE_DIST	128				// anyone closer than this and looking at me is probably staring at me.

#define bit_saidDamageLight		(1<<0)	// bits so we don't repeat key sentences
#define bit_saidDamageMedium	(1<<1)
#define bit_saidDamageHeavy		(1<<2)
#define bit_saidHelloPlayer		(1<<3)
#define bit_saidWoundLight		(1<<4)
#define bit_saidWoundHeavy		(1<<5)
#define bit_saidHeard			(1<<6)
#define bit_saidSmelled			(1<<7)

#define TLK_CFRIENDS		10

enum
{
	TALK_FRIEND_UNKNOWN = 0,
	TALK_FRIEND_PERSONNEL = 1,
	TALK_FRIEND_SOLDIER = 2,
};

typedef enum
{
	TLK_ANSWER = 0,
	TLK_QUESTION,
	TLK_IDLE,
	TLK_STARE,
	TLK_USE,
	TLK_UNUSE,
	TLK_DECLINE,
	TLK_STOP,
	TLK_NOSHOOT,
	TLK_HELLO,
	TLK_PHELLO,
	TLK_PIDLE,
	TLK_PQUESTION,
	TLK_PLHURT1,
	TLK_PLHURT2,
	TLK_PLHURT3,
	TLK_SMELL,
	TLK_WOUND,
	TLK_MORTAL,
	TLK_SHOT,
	TLK_MAD,

	TLK_CGROUPS					// MUST be last entry
} TALKGROUPNAMES;

enum
{
	SCHED_CANT_FOLLOW = LAST_FOLLOWINGMONSTER_SCHEDULE+1,
	SCHED_FOLLOW_FALLIBLE,
	SCHED_FIND_MEDIC,

	LAST_TALKMONSTER_SCHEDULE		// MUST be last
};

enum
{
	TASK_CANT_FOLLOW = LAST_FOLLOWINGMONSTER_TASK+1,

	TASK_TLK_RESPOND,		// say my response
	TASK_TLK_SPEAK,			// question or remark
	TASK_TLK_HELLO,			// Try to say hello to player
	TASK_TLK_HEADRESET,		// reset head position
	TASK_TLK_STOPSHOOTING,	// tell player to stop shooting friend
	TASK_TLK_STARE,			// let the player know I know he's staring at me.
	TASK_TLK_LOOK_AT_CLIENT,// faces player if not moving and not talking and in idle.
	TASK_TLK_CLIENT_STARE,	// same as look at client, but says something if the player stares.
	TASK_TLK_EYECONTACT,	// maintain eyecontact with person who I'm talking to
	TASK_TLK_IDEALYAW,		// set ideal yaw to face who I'm talking to
	TASK_FIND_MEDIC,		// Try to find and call someone who can heal me

	LAST_TALKMONSTER_TASK			// MUST be last
};

enum
{
	TOLERANCE_DEFAULT,
	TOLERANCE_ZERO,
	TOLERANCE_LOW,
	TOLERANCE_AVERAGE,
	TOLERANCE_HIGH,
	TOLERANCE_ABSOLUTE,
	TOLERANCE_ABSOLUTE_NO_ALERTS,
};

class CTalkMonster : public CFollowingMonster
{
public:
	void			TalkInit( void );				
	CBaseEntity		*FindNearestFriend(BOOL fPlayer);
	float			TargetDistance( void );
	void			StopTalking( void ) { SentenceStop(); }
	
	// Base Monster functions
	void			Precache( void );
	int 			TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	int 			TakeHealth(float flHealth, int bitsDamageType);
	bool			CanBePushedByClient(CBaseEntity *pOther);
	void			Killed( entvars_t *pevAttacker, int iGib );
	void			OnDying();
	void			StartMonster( void );
	int				IRelationship ( CBaseEntity *pTarget );
	bool			IsFriendWithPlayerBeforeProvoked();
	virtual int		CanPlaySentence( BOOL fDisregardState );
	virtual bool PlaySentence( const char *pszSentence, float duration, float volume, float attenuation );
	void			PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener );
	void			KeyValue( KeyValueData *pkvd );

	// AI functions
	void			SetActivity ( Activity newActivity );
	Schedule_t		*GetScheduleOfType ( int Type );
	void			StartTask( Task_t *pTask );
	void			RunTask( Task_t *pTask );
	void			HandleAnimEvent( MonsterEvent_t *pEvent );
	void			PrescheduleThink( void );
	void			ReactToPlayerHit(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	void			TalkMonsterInit();

	// Conversations / communication
	int				GetVoicePitch( void );
	virtual void	IdleRespond( void );
	virtual bool AskQuestion( float duration );
	virtual void	MakeIdleStatement( void );
	float			RandomSentenceDuraion( void );
	int				FIdleSpeak( void );
	int				FIdleStare( void );
	int				FIdleHello( void );
	int				FOkToSpeak( void );
	void			TrySmellTalk( void );
	CBaseEntity		*EnumFriends( CBaseEntity *pentPrevious, int listNumber, BOOL bTrace );
	CBaseEntity		*EnumFriends(CBaseEntity *pentPrevious, const char* pszFriend, BOOL bTrace );
	void			AlertFriends( void );
	void			ShutUpFriends( void );
	BOOL			IsTalking( void );
	void			Talk( float flDuration );	

	// Following related
	virtual void	StartFollowing( CBaseEntity *pLeader, bool saySentence = true );
	void			LimitFollowers( CBaseEntity *pPlayer, int maxFollowers );
	virtual int		TalkFriendCategory() { return TALK_FRIEND_PERSONNEL; }
	bool	ReadyForUse();
	virtual void PlayUseSentence();
	virtual void PlayUnUseSentence();
	virtual void DeclineFollowing(CBaseEntity* pCaller);

	// Medic related
	bool			WantsToCallMedic();
	bool			TryCallForMedic(CBaseEntity* pOther);
	virtual void	PlayCallForMedic();
	bool			IsWounded();
	bool			IsHeavilyWounded();
	
	virtual bool	SetAnswerQuestion( CTalkMonster *pSpeaker );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	virtual int SizeForGrapple() { return GRAPPLE_MEDIUM; }
	virtual int DefaultToleranceLevel() { return TOLERANCE_LOW; }
	int MyToleranceLevel() { return m_iTolerance ? m_iTolerance : DefaultToleranceLevel(); }
	static const char* GetRedefinedSentence(string_t sentence);

	void ReportAIState(ALERT_TYPE level);

	struct TalkFriend
	{
		const char* name;
		bool canFollow;
		bool canHeal;
		short category;
	};

	static TalkFriend m_szFriends[TLK_CFRIENDS];		// array of friend names
	static float g_talkWaitTime;
	
	int			m_bitsSaid;						// set bits for sentences we don't want repeated
	int			m_nSpeak;						// number of times initiated talking
	int			m_voicePitch;					// pitch of voice for this head
	const char	*m_szGrp[TLK_CGROUPS];			// sentence group names
	float		m_useTime;						// Don't allow +USE until this time
	string_t			m_iszUse;						// Custom +USE sentence group (follow)
	string_t			m_iszUnUse;						// Custom +USE sentence group (stop following)
	string_t			m_iszDecline;					// Custom +USE sentence group (decline following)

	float		m_flLastSaidSmelled;// last time we talked about something that stinks
	float		m_flStopTalkTime;// when in the future that I'll be done saying this sentence.
	float		m_flMedicWaitTime;

	EHANDLE		m_hTalkTarget;	// who to look at while talking
	BOOL m_fStartSuspicious;
	short m_iTolerance;
	float m_flLastHitByPlayer;
	int m_iPlayerHits;

	CUSTOM_SCHEDULES
};

// Don't see a client right now.
#define		bits_COND_CLIENT_UNSEEN		( bits_COND_SPECIAL2 )

#endif //TALKMONSTER_H
