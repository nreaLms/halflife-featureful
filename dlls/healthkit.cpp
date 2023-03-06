/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "items.h"
#include "gamerules.h"
#include "wallcharger.h"
#include "game.h"

extern int gmsgItemPickup;

class CHealthKit : public CItem
{
public:
	void Spawn( void );
	void Precache( void );
	BOOL MyTouch( CBasePlayer *pPlayer );
/*
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
*/
protected:
	virtual int DefaultCapacity() { return gSkillData.healthkitCapacity; }
};

LINK_ENTITY_TO_CLASS( item_healthkit, CHealthKit )

/*
TYPEDESCRIPTION	CHealthKit::m_SaveData[] =
{

};

IMPLEMENT_SAVERESTORE( CHealthKit, CItem )
*/

void CHealthKit::Spawn( void )
{
	Precache();
	SetMyModel( "models/w_medkit.mdl" );

	CItem::Spawn();
}

void CHealthKit::Precache( void )
{
	PrecacheMyModel( "models/w_medkit.mdl" );
	PRECACHE_SOUND( "items/smallmedkit1.wav" );
}

BOOL CHealthKit::MyTouch( CBasePlayer *pPlayer )
{
	if( pPlayer->pev->deadflag != DEAD_NO )
	{
		return FALSE;
	}

	const bool healed = pPlayer->pev->health < pPlayer->pev->max_health;
	if( pPlayer->TakeHealth( this, pev->health > 0 ? pev->health : DefaultCapacity(), HEAL_CHARGE ) )
	{
		if (healed) {
			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( STRING( pev->classname ) );
			MESSAGE_END();

			EMIT_SOUND( ENT( pPlayer->pev ), CHAN_ITEM, "items/smallmedkit1.wav", 1, ATTN_NORM );
		}

		return TRUE;
	}

	return FALSE;
}

//-------------------------------------------------------------
// Base class for wall chargers
//-------------------------------------------------------------

#define SF_WALLCHARGER_STARTOFF 1
#define SF_WALLCHARGER_ONLYDIRECT 16

void CWallCharger::Spawn()
{
	Precache();

	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	UTIL_SetOrigin( pev, pev->origin );		// set size and link into world
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	if (FBitSet(pev->spawnflags, SF_WALLCHARGER_STARTOFF))
	{
		m_iJuice = 0;
	}
	else
	{
		m_iJuice = ChargerCapacity();
	}
	pev->frame = 0;
}

void CWallCharger::Precache()
{
	PRECACHE_SOUND( ChargeStartSound() );
	PRECACHE_SOUND( DenySound() );
	PRECACHE_SOUND( LoopingSound() );
	const char* rechargeSound = RechargeSound();
	if (rechargeSound)
		PRECACHE_SOUND(rechargeSound);
}

int CWallCharger::ObjectCaps( void )
{
	return ( CBaseEntity::ObjectCaps() | FCAP_CONTINUOUS_USE
			| (FBitSet(pev->spawnflags, SF_WALLCHARGER_ONLYDIRECT)?FCAP_ONLYDIRECT_USE:0) )
			& ~FCAP_ACROSS_TRANSITION;
}

void CWallCharger::Off()
{
	// Stop looping sound.
	if( m_iOn > 1 )
		STOP_SOUND( ENT( pev ), CHAN_STATIC, LoopingSound() );

	m_iOn = 0;

	SetThink( &CBaseEntity::SUB_DoNothing );
	if ( m_iJuice <= 0 )
	{
		if ( ( m_iReactivate = RechargeTime() ) > 0 )
		{
			pev->nextthink = pev->ltime + m_iReactivate;
			SetThink( &CWallCharger::Recharge );
		}
	}
}

void CWallCharger::Recharge( void )
{
	if (m_triggerOnRecharged)
	{
		FireTargets( STRING( m_triggerOnRecharged ), this, this, USE_TOGGLE, 0 );
	}
	const char* rechargeSound = RechargeSound();
	if (rechargeSound)
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, rechargeSound, 1.0, ATTN_NORM );
	m_iJuice = ChargerCapacity();
	pev->frame = OnStateFrame();
	SetThink( &CBaseEntity::SUB_DoNothing );
}

const char* CWallCharger::LoopingSound()
{
	return pev->noise ? STRING(pev->noise) : DefaultLoopingSound();
}
const char* CWallCharger::DenySound()
{
	return pev->noise1 ? STRING(pev->noise1) : DefaultDenySound();
}
const char* CWallCharger::ChargeStartSound()
{
	return pev->noise2 ? STRING(pev->noise2) : DefaultChargeStartSound();
}
const char* CWallCharger::RechargeSound()
{
	return pev->noise3 ? STRING(pev->noise3) : DefaultRechargeSound();
}

TYPEDESCRIPTION CWallCharger::m_SaveData[] =
{
	DEFINE_FIELD( CWallCharger, m_flNextCharge, FIELD_TIME ),
	DEFINE_FIELD( CWallCharger, m_iReactivate, FIELD_INTEGER ),
	DEFINE_FIELD( CWallCharger, m_iJuice, FIELD_INTEGER ),
	DEFINE_FIELD( CWallCharger, m_iOn, FIELD_INTEGER ),
	DEFINE_FIELD( CWallCharger, m_flSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CWallCharger, m_triggerOnFirstUse, FIELD_STRING ),
	DEFINE_FIELD( CWallCharger, m_triggerOnEmpty, FIELD_STRING ),
	DEFINE_FIELD( CWallCharger, m_triggerOnRecharged, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CWallCharger, CBaseEntity )

void CWallCharger::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq(pkvd->szKeyName, "style" ) ||
		FStrEq( pkvd->szKeyName, "height" ) ||
		FStrEq( pkvd->szKeyName, "value1" ) ||
		FStrEq( pkvd->szKeyName, "value2" ) ||
		FStrEq( pkvd->szKeyName, "value3" ) )
	{
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "dmdelay" ) )
	{
		m_iReactivate = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "TriggerOnEmpty" ) )
	{
		m_triggerOnEmpty = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "TriggerOnRecharged" ) )
	{
		m_triggerOnRecharged = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "TriggerOnFirstUse" ) )
	{
		m_triggerOnFirstUse = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "capacity" ) || FStrEq( pkvd->szKeyName, "CustomJuice" ) )
	{
		pev->health = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "CustomLoopSound" ) )
	{
		pev->noise = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "CustomDeniedSound" ) )
	{
		pev->noise1 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "CustomStartSound" ) )
	{
		pev->noise2 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "CustomRechargeSound" ) )
	{
		pev->noise3 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CWallCharger::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!pActivator || !pActivator->IsPlayer())
	{
		if (useType == USE_TOGGLE)
		{
			useType = m_iJuice > 0 ? USE_OFF : USE_ON;
		}
		switch (useType) {
		case USE_OFF:
			if (m_iJuice > 0)
			{
				EMIT_SOUND( ENT( pev ), CHAN_ITEM, DenySound(), SoundVolume(), ATTN_NORM );
				m_iJuice = 0;
				pev->frame = OffStateFrame();
				Off();
			}
			return;
		case USE_ON:
			if (m_iJuice <= 0)
			{
				Recharge();
			}
			return;
		default:
			return;
		}
	}

	// if there is no juice left, turn it off
	if( m_iJuice <= 0 )
	{
		if (m_triggerOnEmpty && pev->frame == OnStateFrame())
		{
			FireTargets( STRING( m_triggerOnEmpty ), this, this, USE_TOGGLE, 0 );
		}
		pev->frame = OffStateFrame();
		Off();
	}

	// if the player doesn't have the suit, or there is no juice left, make the deny noise
	if( ( m_iJuice <= 0 ) || !((static_cast<CBasePlayer*>(pActivator))->HasSuit() || AllowNoSuit()) )
	{
		if( m_flSoundTime <= gpGlobals->time )
		{
			m_flSoundTime = gpGlobals->time + 0.62f;
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, DenySound(), SoundVolume(), ATTN_NORM );
		}
		return;
	}

	pev->nextthink = pev->ltime + 0.25f;
	SetThink( &CWallCharger::Off );

	// Time to recharge yet?
	if( m_flNextCharge >= gpGlobals->time )
		return;

	// govern the rate of charge
	m_flNextCharge = gpGlobals->time + 0.1f;

	// charge the player
	if( GiveCharge(pActivator) )
	{
		if (m_triggerOnFirstUse)
		{
			FireTargets( STRING( m_triggerOnFirstUse ), this, this, USE_TOGGLE, 0 );
			m_triggerOnFirstUse = 0;
		}
		m_iJuice--;
	}
	else
	{
		if( m_flSoundTime <= gpGlobals->time )
		{
			m_flSoundTime = gpGlobals->time + 0.62f;
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, DenySound(), SoundVolume(), ATTN_NORM );
		}
		if( m_iOn > 1 )
			STOP_SOUND( ENT( pev ), CHAN_STATIC, LoopingSound() );
		m_iOn = 0;
		return;
	}

	// Play the on sound or the looping charging sound
	if( !m_iOn )
	{
		m_iOn++;
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, ChargeStartSound(), 1.0, ATTN_NORM );
		m_flSoundTime = 0.56f + gpGlobals->time;
	}
	if( ( m_iOn == 1 ) && ( m_flSoundTime <= gpGlobals->time ) )
	{
		m_iOn++;
		EMIT_SOUND( ENT( pev ), CHAN_STATIC, LoopingSound(), 1.0, ATTN_NORM );
	}
}

int CWallCharger::OnStateFrame()
{
	if (FBitSet(pev->spawnflags, SF_WALLCHARGER_STARTOFF))
		return 1;
	return 0;
}

int CWallCharger::OffStateFrame()
{
	if (FBitSet(pev->spawnflags, SF_WALLCHARGER_STARTOFF))
		return 0;
	return 1;
}

bool CWallCharger::CalcRatio( CBaseEntity *pLocus, float* outResult )
{
	*outResult = m_iJuice / static_cast<float>(ChargerCapacity());
	return true;
}


//-------------------------------------------------------------
// Wall mounted health kit
//-------------------------------------------------------------
class CWallHealth : public CWallCharger
{
public:
	const char* DefaultLoopingSound() { return "items/medcharge4.wav"; }
	int RechargeTime() { return (int)g_pGameRules->FlHealthChargerRechargeTime(); }
	const char* DefaultRechargeSound() { return "items/medshot4.wav"; }
	int ChargerCapacity() { return (int)(pev->health > 0 ? pev->health : gSkillData.healthchargerCapacity); }
	const char* DefaultDenySound() { return "items/medshotno1.wav"; }
	const char* DefaultChargeStartSound() { return "items/medshot4.wav"; }
	float SoundVolume() { return 1.0f; }
	bool GiveCharge(CBaseEntity* pActivator)
	{
		return pActivator->TakeHealth( this, 1, HEAL_CHARGE ) > 0;
	}
	bool AllowNoSuit() {
		return g_modFeatures.nosuit_allow_healthcharger;
	}
};

LINK_ENTITY_TO_CLASS( func_healthcharger, CWallHealth )

//-------------------------------------------------------------
// Wall mounted health kit (PS2 && Decay)
//-------------------------------------------------------------

class CWallHealthJarDecay : public CBaseAnimating
{
public:
	void Spawn();
	void Think();
	void Update(bool slosh, float value);
	void ToRest();
};

void CWallHealthJarDecay::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_FLY;

	SET_MODEL(ENT(pev), "models/health_charger_both.mdl");
	pev->renderamt = 180;
	pev->rendermode = kRenderTransTexture;
	InitBoneControllers();
}

void CWallHealthJarDecay::Think()
{
	if (pev->sequence > 0)
	{
		StudioFrameAdvance();
		if (pev->sequence == 2 && m_fSequenceFinished)
		{
			pev->sequence = 0;
		}
		else
		{
			pev->nextthink = gpGlobals->time + 0.1;
		}
	}
}

void CWallHealthJarDecay::Update(bool slosh, float value)
{
	if (slosh && pev->sequence != 1)
	{
		pev->sequence = 1;
		ResetSequenceInfo();
		pev->frame = 0;
		m_fSequenceLoops = TRUE;
		pev->nextthink = gpGlobals->time;
	}
	const float jarBoneControllerValue = value * 11 - 11;
	SetBoneController(0,  jarBoneControllerValue );
}

void CWallHealthJarDecay::ToRest()
{
	if (pev->sequence == 1)
	{
		pev->sequence = 2;
		ResetSequenceInfo();
		pev->frame = 0;
	}
}

LINK_ENTITY_TO_CLASS(item_healthcharger_jar, CWallHealthJarDecay)

class CWallHealthDecay : public CBaseAnimating
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn();
	void Precache(void);
	void EXPORT AnimateAndWork();
	void SearchForPlayer();
	void Off( void );
	void EXPORT Recharge( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int ObjectCaps( void ) { return ( CBaseAnimating::ObjectCaps() | FCAP_CONTINUOUS_USE | FCAP_ONLYDIRECT_USE ); }
	void TurnNeedleToPlayer(const Vector &player);
	void SetNeedleState(int state);
	void SetNeedleController(float yaw);
	void UpdateOnRemove();
	void UpdateJar();
	int ChargerCapacity() { return (int)(pev->health > 0 ? pev->health : gSkillData.healthchargerCapacity); }

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	enum {
		Still,
		Deploy,
		Idle,
		GiveShot,
		Healing,
		RetractShot,
		RetractArm,
		Inactive
	};

	float m_flNextCharge;
	int m_iJuice;
	int m_iState;
	float m_flSoundTime;
	float m_goToOffTime;
	BOOL m_goingToOff;
	CWallHealthJarDecay* m_jar;
	BOOL m_playingChargeSound;
	float m_currentYaw;
	float m_goalYaw;
	string_t m_triggerOnFirstUse;
	string_t m_triggerOnEmpty;

protected:
	void SetMySequence(const char* sequence);
};

TYPEDESCRIPTION CWallHealthDecay::m_SaveData[] =
{
	DEFINE_FIELD( CWallHealthDecay, m_flNextCharge, FIELD_TIME ),
	DEFINE_FIELD( CWallHealthDecay, m_iJuice, FIELD_INTEGER ),
	DEFINE_FIELD( CWallHealthDecay, m_iState, FIELD_INTEGER ),
	DEFINE_FIELD( CWallHealthDecay, m_flSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CWallHealthDecay, m_goToOffTime, FIELD_TIME ),
	DEFINE_FIELD( CWallHealthDecay, m_goingToOff, FIELD_BOOLEAN),
	DEFINE_FIELD( CWallHealthDecay, m_playingChargeSound, FIELD_BOOLEAN),
	DEFINE_FIELD( CWallHealthDecay, m_triggerOnFirstUse, FIELD_STRING),
	DEFINE_FIELD( CWallHealthDecay, m_triggerOnEmpty, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE( CWallHealthDecay, CBaseAnimating )

void CWallHealthDecay::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "capacity" ) || FStrEq( pkvd->szKeyName, "CustomJuice" ) )
	{
		pev->health = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "TriggerOnEmpty" ) )
	{
		m_triggerOnEmpty = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "TriggerOnFirstUse" ) )
	{
		m_triggerOnFirstUse = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseAnimating::KeyValue( pkvd );
}

void CWallHealthDecay::Spawn()
{
	m_iJuice = ChargerCapacity();
	Precache();

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_FLY;

	SET_MODEL(ENT(pev), "models/health_charger_body.mdl");
	UTIL_SetSize(pev, Vector(-12, -16, 0), Vector(12, 16, 48));
	UTIL_SetOrigin(pev, pev->origin);
	pev->skin = 0;

	InitBoneControllers();

	if (m_iJuice > 0)
	{
		m_iState = Still;
		SetThink(&CWallHealthDecay::AnimateAndWork);
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else
	{
		m_iState = Inactive;
	}
}

LINK_ENTITY_TO_CLASS(item_healthcharger, CWallHealthDecay)

void CWallHealthDecay::Precache(void)
{
	PRECACHE_MODEL("models/health_charger_body.mdl");
	PRECACHE_MODEL("models/health_charger_both.mdl");
	PRECACHE_SOUND( "items/medshot4.wav" );
	PRECACHE_SOUND( "items/medshotno1.wav" );
	PRECACHE_SOUND( "items/medcharge4.wav" );

	m_jar = GetClassPtr( (CWallHealthJarDecay *)NULL );
	if (m_jar)
	{
		m_jar->Spawn();
		UTIL_SetOrigin( m_jar->pev, pev->origin );
		m_jar->pev->angles = pev->angles;
		UpdateJar();
	}
}

void CWallHealthDecay::AnimateAndWork()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	if (m_goalYaw < 0)
		m_currentYaw = Q_max(m_currentYaw - 15, m_goalYaw);
	else
		m_currentYaw = Q_min(m_currentYaw + 15, m_goalYaw);
	SetBoneController(0, m_currentYaw);

	if (m_goingToOff)
	{
		if (m_goToOffTime <= gpGlobals->time)
			Off();
	}
	else
	{
		SearchForPlayer();
	}
}

void CWallHealthDecay::SearchForPlayer()
{
	CBaseEntity* pEntity = 0;
	UTIL_MakeVectors( pev->angles );
	while((pEntity = UTIL_FindEntityInSphere(pEntity, Center(), 64)) != 0) { // this must be in sync with PLAYER_SEARCH_RADIUS from player.cpp
		if (pEntity->IsPlayer() && pEntity->IsAlive() && ((static_cast<CBasePlayer*>(pEntity))->HasSuit() || g_modFeatures.nosuit_allow_healthcharger)) {
			if (DotProduct(pEntity->pev->origin - pev->origin, gpGlobals->v_forward) < 0) {
				continue;
			}
			TurnNeedleToPlayer(pEntity->pev->origin);
			switch (m_iState) {
			case RetractShot:
				if( m_fSequenceFinished )
					SetNeedleState(Idle);
				break;
			case RetractArm:
				SetNeedleState(Deploy);
				break;
			case Still:
				SetNeedleState(Deploy);
				break;
			case Deploy:
				if (m_fSequenceFinished)
				{
					SetNeedleState(Idle);
				}
				break;
			case Idle:
				break;
			default:
				break;
			}
		}
		break;
	}
	if (!pEntity || !pEntity->IsPlayer()) {
		switch (m_iState) {
		case Deploy:
		case Idle:
		case RetractShot:
			SetNeedleState(RetractArm);
			break;
		case RetractArm:
			if (m_fSequenceFinished)
			{
				SetNeedleState(Still);
				SetNeedleController(0);
			}
			else
			{
				SetNeedleController(m_currentYaw*0.75);
			}
			break;
		case Still:
			break;
		default:
			break;
		}
	}
}

void CWallHealthDecay::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	// Make sure that we have a caller
	if( !pCaller )
		return;
	// if it's not a player, ignore
	if( !pCaller->IsPlayer() )
		return;

	CBasePlayer* pPlayer = static_cast<CBasePlayer*>(pCaller);

	// if the player doesn't have the suit, or there is no juice left, make the deny noise
	if( ( m_iJuice <= 0 ) || !(pPlayer->HasSuit() || g_modFeatures.nosuit_allow_healthcharger) )
	{
		if( m_flSoundTime <= gpGlobals->time )
		{
			m_flSoundTime = gpGlobals->time + 0.62f;
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/medshotno1.wav", 1.0, ATTN_NORM );
		}
		return;
	}

	if (m_iState != Idle && m_iState != GiveShot && m_iState != Healing && m_iState != Inactive)
		return;

	m_goingToOff = TRUE;
	// if there is no juice left, turn it off
	if( (m_iState == Healing || m_iState == GiveShot) && m_iJuice <= 0 )
	{
		pev->skin = 1;
		pev->nextthink = m_goToOffTime = gpGlobals->time;
	}
	else
	{
		m_goToOffTime = gpGlobals->time + 0.25f;
	}

	// Time to recharge yet?
	if( m_flNextCharge >= gpGlobals->time )
		return;

	int soundType = 0;
	TurnNeedleToPlayer(pPlayer->pev->origin);
	switch (m_iState) {
	case Idle:
		m_flSoundTime = 0.56 + gpGlobals->time;
		SetNeedleState(GiveShot);
		soundType = 1;
		break;
	case GiveShot:
		if (m_fSequenceFinished)
		{
			SetNeedleState(Healing);
		}
		break;
	case Healing:
		if (!m_playingChargeSound && m_flSoundTime <= gpGlobals->time)
		{
			soundType = 2;
			m_playingChargeSound = TRUE;
		}
		break;
	default:
		ALERT(at_console, "Unexpected healthcharger state on use: %d\n", m_iState);
		break;
	}

	// charge the player
	if( pPlayer->TakeHealth( this, 1, HEAL_CHARGE ) )
	{
		if (m_triggerOnFirstUse)
		{
			FireTargets( STRING( m_triggerOnFirstUse ), pPlayer, this, USE_TOGGLE, 0 );
			m_triggerOnFirstUse = iStringNull;
		}
		m_iJuice--;
		if (m_iJuice <= 0)
		{
			pev->skin = 1;
			if (m_triggerOnEmpty)
			{
				FireTargets( STRING( m_triggerOnEmpty ), pPlayer, this, USE_TOGGLE, 0 );
			}
		}
		UpdateJar();

		if (soundType == 1)
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );
		}
		else if (soundType == 2)
		{
			EMIT_SOUND( ENT( pev ), CHAN_STATIC, "items/medcharge4.wav", 1.0, ATTN_NORM );
			m_playingChargeSound = TRUE;
		}
	}
	else
	{
		if (m_jar)
		{
			m_jar->ToRest();
		}
		if( m_flSoundTime <= gpGlobals->time )
		{
			m_flSoundTime = gpGlobals->time + 0.62;
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/medshotno1.wav", 1.0, ATTN_NORM );
		}
		if (m_playingChargeSound) {
			STOP_SOUND( ENT( pev ), CHAN_STATIC, "items/medcharge4.wav" );
			m_playingChargeSound = FALSE;
		}
	}

	// govern the rate of charge
	m_flNextCharge = gpGlobals->time + 0.1f;
}

void CWallHealthDecay::Recharge( void )
{
	EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );
	m_iJuice = ChargerCapacity();
	UpdateJar();
	pev->skin = 0;
	SetNeedleState(Still);
	SetThink( &CWallHealthDecay::AnimateAndWork );
	pev->nextthink = gpGlobals->time;
}

void CWallHealthDecay::Off( void )
{
	switch (m_iState) {
	case GiveShot:
	case Healing:
		if (m_playingChargeSound) {
			STOP_SOUND( ENT( pev ), CHAN_STATIC, "items/medcharge4.wav" );
			m_playingChargeSound = FALSE;
		}
		if (m_jar)
		{
			m_jar->ToRest();
		}
		SetNeedleState(RetractShot);
		break;
	case RetractShot:
		if( m_fSequenceFinished )
		{
			if (m_iJuice > 0) {
				SetNeedleState(Idle);
				m_goingToOff = FALSE;
				pev->nextthink = gpGlobals->time;
			} else {
				SetNeedleState(RetractArm);
			}
		}

		break;
	case RetractArm:
	{
		if( m_fSequenceFinished )
		{
			m_currentYaw = m_goalYaw = 0;
			SetBoneController(0, m_currentYaw);
			if( ( m_iJuice <= 0 ) )
			{
				SetNeedleState(Inactive);
				const float rechargeTime = g_pGameRules->FlHealthChargerRechargeTime();
				if (rechargeTime > 0 ) {
					pev->nextthink = gpGlobals->time + rechargeTime;
					SetThink( &CWallHealthDecay::Recharge );
				}
			}
		}
		else
		{
			SetNeedleController(m_currentYaw*0.75);
		}
		break;
	}
	default:
		break;
	}
}

void CWallHealthDecay::SetMySequence(const char *sequence)
{
	pev->sequence = LookupSequence( sequence );
	if (pev->sequence == -1) {
		ALERT(at_error, "unknown sequence in %s: %s\n", STRING(pev->model), sequence);
		pev->sequence = 0;
	}
	pev->frame = 0;
	ResetSequenceInfo( );
}

void CWallHealthDecay::SetNeedleState(int state)
{
	m_iState = state;
	switch (state) {
	case Still:
		SetMySequence("still");
		break;
	case Deploy:
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );
		SetMySequence("deploy");
		break;
	case Idle:
		SetMySequence("prep_shot");
		break;
	case GiveShot:
		SetMySequence("give_shot");
		break;
	case Healing:
		SetMySequence("shot_idle");
		break;
	case RetractShot:
		SetMySequence("retract_shot");
		break;
	case RetractArm:
		SetMySequence("retract_arm");
		break;
	case Inactive:
		SetMySequence("inactive");
	default:
		break;
	}
}

void CWallHealthDecay::TurnNeedleToPlayer(const Vector& player)
{
	float yaw = UTIL_VecToYaw( player - pev->origin ) - pev->angles.y;

	if( yaw > 180 )
		yaw -= 360;
	if( yaw < -180 )
		yaw += 360;

	SetNeedleController( yaw );
}

void CWallHealthDecay::SetNeedleController(float yaw)
{
	m_goalYaw = yaw;
}

void CWallHealthDecay::UpdateOnRemove()
{
	UTIL_Remove(m_jar);
	m_jar = NULL;
	CBaseAnimating::UpdateOnRemove();
}

void CWallHealthDecay::UpdateJar()
{
	if (m_jar)
	{
		m_jar->Update(m_iState == Healing || m_iState == GiveShot, m_iJuice / (float)ChargerCapacity());
	}
}
