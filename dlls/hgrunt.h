#pragma once
#ifndef HGRUNT_H
#define HGRUNT_H

#include	"cbase.h"
#include	"monsters.h"
#include	"followingmonster.h"

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_GRUNT_SUPPRESS = LAST_FOLLOWINGMONSTER_SCHEDULE+1,
	SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_GRUNT_COVER_AND_RELOAD,
	SCHED_GRUNT_SWEEP,
	SCHED_GRUNT_FOUND_ENEMY,
	SCHED_GRUNT_REPEL,
	SCHED_GRUNT_REPEL_ATTACK,
	SCHED_GRUNT_REPEL_LAND,
	SCHED_GRUNT_WAIT_FACE_ENEMY,
	SCHED_GRUNT_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_GRUNT_ELOF_FAIL
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_GRUNT_FACE_TOSS_DIR = LAST_FOLLOWINGMONSTER_TASK+1,
	TASK_GRUNT_SPEAK_SENTENCE,
};

typedef enum
{
	HGRUNT_SENT_NONE = -1,
	HGRUNT_SENT_GREN = 0,
	HGRUNT_SENT_ALERT,
	HGRUNT_SENT_MONSTER,
	HGRUNT_SENT_COVER,
	HGRUNT_SENT_THROW,
	HGRUNT_SENT_CHARGE,
	HGRUNT_SENT_TAUNT,
	HGRUNT_SENT_CHECK,
	HGRUNT_SENT_QUEST,
	HGRUNT_SENT_IDLE,
	HGRUNT_SENT_CLEAR,
	HGRUNT_SENT_ANSWER,
	HGRUNT_SENT_COUNT,
} HGRUNT_SENTENCE_TYPES;

class CHGrunt : public CFollowingMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int DefaultClassify( void );
	const char* DefaultDisplayName() { return "Human Grunt"; }
	const char* ReverseRelationshipModel();
	int ISoundMask( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL FCanCheckAttacks( void );
	BOOL CheckMeleeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack2( float flDot, float flDist );
	void CheckAmmo( void );
	void SetActivity( Activity NewActivity );
	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );
	void DeathSound( void );
	void PainSound( void );
	void IdleSound( void );
	Vector GetGunPosition( void );
	void Shoot( void );
	void Shotgun( void );
	void PrescheduleThink( void );
	void GibMonster( void );
	virtual void SpeakSentence( void );
	void PlayUseSentence();
	void PlayUnUseSentence();

	int Save( CSave &save );
	int Restore( CRestore &restore );

	CBaseEntity *Kick( void );
	void KickImpl(float damage, float zpunch = 0);
	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType( int Type );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	int IRelationship( CBaseEntity *pTarget );

	virtual BOOL FOkToSpeak( void );
	void JustSpoke( void );
	void DropMyItems(BOOL isGibbed);
	void DropMyItem(const char *entityName, const Vector &vecGunPos, const Vector &vecGunAngles, BOOL isGibbed);

	CUSTOM_SCHEDULES
	static TYPEDESCRIPTION m_SaveData[];

	virtual int SizeForGrapple() { return GRAPPLE_MEDIUM; }

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flLastEnemySightTime;

	Vector m_vecTossVelocity;

	BOOL m_fThrowGrenade;
	BOOL m_fStanding;
	BOOL m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.
	int m_cClipSize;

	int m_voicePitch;

	int m_iBrassShell;
	int m_iShotgunShell;

	int m_iSentence;


protected:
	static const char *pGruntSentences[HGRUNT_SENT_COUNT];

	void SpawnHelper(const char* modelName, int health, int bloodColor = BLOOD_COLOR_RED);
	void PrecacheHelper(const char* modelName);
	void PlayFirstBurstSounds();
	BOOL CheckRangeAttack2Impl( float grenadeSpeed, float flDot, float flDist );
	virtual int GetRangeAttack1Sequence();
	virtual int GetRangeAttack2Sequence();
	virtual Schedule_t* ScheduleOnRangeAttack1();
	virtual float LimpHealth();

	virtual float SentenceVolume();
	virtual float SentenceAttn();
	virtual const char* SentenceByNumber(int sentence);
	virtual int* GruntQuestionVar();
};

class CHGruntRepel : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT RepelUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int m_iSpriteTexture;	// Don't save, precache
	virtual int SizeForGrapple() { return GRAPPLE_LARGE; }
	virtual const char* TrooperName();
	virtual void PrepareBeforeSpawn(CBaseEntity* pEntity);
};

#endif
