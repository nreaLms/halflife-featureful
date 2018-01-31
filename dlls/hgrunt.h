#pragma once
#ifndef HGRUNT_H
#define HGRUNT_H

#include	"cbase.h"
#include	"monsters.h"
#include	"squadmonster.h"

class CHGrunt : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int DefaultClassify( void );
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

	int Save( CSave &save );
	int Restore( CRestore &restore );

	CBaseEntity *Kick( void );
	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType( int Type );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	int IRelationship( CBaseEntity *pTarget );

	BOOL FOkToSpeak( void );
	void JustSpoke( void );
	void DropMyItems(BOOL isGibbed);
	void DropMyItem(const char *entityName, const Vector &vecGunPos, const Vector &vecGunAngles, BOOL isGibbed);

	CUSTOM_SCHEDULES
	static TYPEDESCRIPTION m_SaveData[];

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

	static const char *pGruntSentences[];

protected:
	void SpawnHelper(const char* modelName, int health, int bloodColor = BLOOD_COLOR_RED);
	void PrecacheHelper(const char* modelName);
	void PlayFirstBurstSounds();
	BOOL CheckRangeAttack2Impl( float grenadeSpeed, float flDot, float flDist );
	virtual float SentenceVolume();
	virtual float SentenceAttn();
	virtual const char* SentenceByNumber(int sentence);
	virtual Schedule_t* ScheduleOnRangeAttack1();
};

class CHGruntRepel : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT RepelUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int m_iSpriteTexture;	// Don't save, precache
protected:
	void RepelUseHelper( const char* monsterName, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

#endif
