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
/*

===== h_battery.cpp ========================================================

  battery-related code

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "skill.h"
#include "gamerules.h"
#include "effects.h"
#include "customentity.h"
#include "wallcharger.h"
#include "weapons.h"
#include "player.h"

class CRecharge : public CWallCharger
{
public:
	const char* DefaultLoopingSound() { return "items/suitcharge1.wav"; }
	int RechargeTime() { return (int)g_pGameRules->FlHEVChargerRechargeTime(); }
	const char* DefaultRechargeSound() { return NULL; }
	int ChargerCapacity() { return (int)(pev->health > 0 ? pev->health : gSkillData.suitchargerCapacity); }
	const char* DefaultDenySound() { return "items/suitchargeno1.wav"; }
	const char* DefaultChargeStartSound() { return "items/suitchargeok1.wav"; }
	float SoundVolume() { return 0.85f; }
	bool GiveCharge(CBaseEntity* pActivator)
	{
		return pActivator->TakeArmor(this, 1);
	}
};

LINK_ENTITY_TO_CLASS( func_recharge, CRecharge )

//-------------------------------------------------------------
// Wall mounted suit charger (PS2 && Decay)
//-------------------------------------------------------------

class CRechargeGlassDecay : public CBaseAnimating
{
public:
	void Spawn();
};

void CRechargeGlassDecay::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_FLY;

	SET_MODEL(ENT(pev), "models/hev_glass.mdl");
	pev->renderamt = 150;
	pev->rendermode = kRenderTransTexture;
}

LINK_ENTITY_TO_CLASS(item_recharge_glass, CRechargeGlassDecay)

#define RECHARGER_COIL_CONTROLLER 1
#define RECHARGER_COIL_CONTROLLER2 2
#define RECHARGER_ARM_CONTROLLER 3
class CRechargeDecay : public CBaseAnimating
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
	void TurnChargeToPlayer(const Vector &player);
	void SetChargeState(int state);
	void SetChargeController(float yaw);
	void UpdateOnRemove();
	void TurnBeamOn()
	{
		if (m_beam)
			ClearBits(m_beam->pev->effects, EF_NODRAW);
	}
	void TurnBeamOff()
	{
		if (m_beam)
			SetBits(m_beam->pev->effects, EF_NODRAW);
	}

	int ChargerCapacity() { return (int)(pev->health > 0 ? pev->health : gSkillData.suitchargerCapacity); }
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
	CRechargeGlassDecay* m_glass;
	BOOL m_playingChargeSound;
	CBeam* m_beam;
	float m_currentYaw;
	float m_goalYaw;
	string_t m_triggerOnFirstUse;
	string_t m_triggerOnEmpty;

protected:
	void SetMySequence(const char* sequence);
	void CreateBeam();
};

TYPEDESCRIPTION CRechargeDecay::m_SaveData[] =
{
	DEFINE_FIELD( CRechargeDecay, m_flNextCharge, FIELD_TIME ),
	DEFINE_FIELD( CRechargeDecay, m_iJuice, FIELD_INTEGER ),
	DEFINE_FIELD( CRechargeDecay, m_iState, FIELD_INTEGER ),
	DEFINE_FIELD( CRechargeDecay, m_flSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CRechargeDecay, m_goToOffTime, FIELD_TIME ),
	DEFINE_FIELD( CRechargeDecay, m_goingToOff, FIELD_BOOLEAN),
	DEFINE_FIELD( CRechargeDecay, m_playingChargeSound, FIELD_BOOLEAN),
	DEFINE_FIELD( CRechargeDecay, m_triggerOnFirstUse, FIELD_STRING),
	DEFINE_FIELD( CRechargeDecay, m_triggerOnEmpty, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE( CRechargeDecay, CBaseAnimating )

void CRechargeDecay::KeyValue( KeyValueData *pkvd )
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

void CRechargeDecay::Spawn()
{
	m_iJuice = ChargerCapacity();
	Precache();

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_FLY;

	SET_MODEL(ENT(pev), "models/hev.mdl");
	UTIL_SetSize(pev, Vector(-12, -16, 0), Vector(12, 16, 48));
	UTIL_SetOrigin(pev, pev->origin);
	pev->skin = 0;

	InitBoneControllers();
	SetBoneController(RECHARGER_COIL_CONTROLLER, 360);

	if (m_iJuice > 0)
	{
		m_iState = Still;
		SetThink(&CRechargeDecay::AnimateAndWork);
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else
	{
		m_iState = Inactive;
	}
}

LINK_ENTITY_TO_CLASS(item_recharge, CRechargeDecay)

void CRechargeDecay::Precache(void)
{
	PRECACHE_MODEL("models/hev.mdl");
	PRECACHE_MODEL("models/hev_glass.mdl");
	PRECACHE_SOUND( "items/suitcharge1.wav" );
	PRECACHE_SOUND( "items/suitchargeno1.wav" );
	PRECACHE_SOUND( "items/suitchargeok1.wav" );
	PRECACHE_MODEL( "sprites/lgtning.spr" );

	CreateBeam();
	if (m_iState != Idle)
		TurnBeamOff();
	m_glass = GetClassPtr( (CRechargeGlassDecay *)NULL );
	m_glass->Spawn();
	UTIL_SetOrigin( m_glass->pev, pev->origin );
	m_glass->pev->angles = pev->angles;
}

void CRechargeDecay::AnimateAndWork()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	if (m_goalYaw < 0)
		m_currentYaw = Q_max(m_currentYaw - 10, m_goalYaw);
	else
		m_currentYaw = Q_min(m_currentYaw + 10, m_goalYaw);
	SetBoneController(RECHARGER_ARM_CONTROLLER, m_currentYaw);

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

void CRechargeDecay::SearchForPlayer()
{
	CBaseEntity* pEntity = 0;
	UTIL_MakeVectors( pev->angles );
	while((pEntity = UTIL_FindEntityInSphere(pEntity, Center(), 64)) != 0) { // this must be in sync with PLAYER_SEARCH_RADIUS from player.cpp
		if (pEntity->IsPlayer() && pEntity->IsAlive() && (static_cast<CBasePlayer*>(pEntity))->HasSuit()) {
			if (DotProduct(pEntity->pev->origin - pev->origin, gpGlobals->v_forward) < 0) {
				continue;
			}
			TurnChargeToPlayer(pEntity->pev->origin);
			switch (m_iState) {
			case RetractShot:
				if( m_fSequenceFinished )
					SetChargeState(Idle);
				break;
			case RetractArm:
				SetChargeState(Deploy);
				break;
			case Still:
				SetChargeState(Deploy);
				break;
			case Deploy:
				if (m_fSequenceFinished)
				{
					TurnBeamOn();
					SetChargeState(Idle);
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
			SetChargeState(RetractArm);
			break;
		case RetractArm:
			if (m_fSequenceFinished)
			{
				SetChargeState(Still);
				SetChargeController(0);
			}
			else
			{
				SetChargeController(m_currentYaw*0.75);
			}
			break;
		case Still:
			break;
		default:
			break;
		}
	}
}

void CRechargeDecay::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	// Make sure that we have a caller
	if( !pCaller )
		return;
	// if it's not a player, ignore
	if( !pCaller->IsPlayer() )
		return;

	CBasePlayer* pPlayer = static_cast<CBasePlayer*>(pCaller);

	// if the player doesn't have the suit, or there is no juice left, make the deny noise
	if( ( m_iJuice <= 0 ) || ( !pPlayer->HasSuit() ) || pPlayer->pev->armorvalue >= MAX_NORMAL_BATTERY )
	{
		if( m_flSoundTime <= gpGlobals->time )
		{
			m_flSoundTime = gpGlobals->time + 0.62f;
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/suitchargeno1.wav", 0.85, ATTN_NORM );
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

	TurnChargeToPlayer(pPlayer->pev->origin);
	switch (m_iState) {
	case Idle:
		m_flSoundTime = 0.56f + gpGlobals->time;
		SetChargeState(GiveShot);
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/suitchargeok1.wav", 1.0, ATTN_NORM );
		break;
	case GiveShot:
		if (m_fSequenceFinished)
		{
			SetChargeState(Healing);
		}
		break;
	case Healing:
		if (!m_playingChargeSound && m_flSoundTime <= gpGlobals->time)
		{
			m_playingChargeSound = TRUE;
			EMIT_SOUND( ENT( pev ), CHAN_STATIC, "items/suitcharge1.wav", 1.0, ATTN_NORM );
		}
		// We need to keep playing animation even though it's 1 frame only for controllers smoothing
		SetChargeState(Healing);
		break;
	default:
		ALERT(at_console, "Unexpected recharger state on use: %d\n", m_iState);
		break;
	}

	// charge the player
	if( pPlayer->pev->armorvalue < MAX_NORMAL_BATTERY )
	{
		if (m_triggerOnFirstUse)
		{
			FireTargets( STRING( m_triggerOnFirstUse ), pPlayer, this );
			m_triggerOnFirstUse = iStringNull;
		}
		m_iJuice--;
		if (m_iJuice <= 0)
		{
			pev->skin = 1;
			if (m_triggerOnEmpty)
			{
				FireTargets( STRING( m_triggerOnEmpty ), pPlayer, this );
			}
		}
		const float boneControllerValue = (m_iJuice / (float)ChargerCapacity()) * 360;
		SetBoneController(RECHARGER_COIL_CONTROLLER, 360 - boneControllerValue);
		SetBoneController(RECHARGER_COIL_CONTROLLER2,  boneControllerValue);

		pPlayer->TakeArmor(this, 1);
	}

	// govern the rate of charge
	m_flNextCharge = gpGlobals->time + 0.1f;
}

void CRechargeDecay::Recharge( void )
{
//	/EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/suitcharge1.wav", 1.0, ATTN_NORM );
	m_iJuice = ChargerCapacity();
	SetBoneController(RECHARGER_COIL_CONTROLLER, 360);
	SetBoneController(RECHARGER_COIL_CONTROLLER2, 0);
	pev->skin = 0;
	SetChargeState(Still);
	SetThink( &CRechargeDecay::AnimateAndWork );
	pev->nextthink = gpGlobals->time;
}

void CRechargeDecay::Off( void )
{
	switch (m_iState) {
	case GiveShot:
	case Healing:
		if (m_playingChargeSound) {
			STOP_SOUND( ENT( pev ), CHAN_STATIC, "items/suitcharge1.wav" );
			m_playingChargeSound = FALSE;
		}
		SetChargeState(RetractShot);
		break;
	case RetractShot:
		if (m_fSequenceFinished)
		{
			if (m_iJuice > 0) {
				SetChargeState(Idle);
				m_goingToOff = FALSE;
				pev->nextthink = gpGlobals->time;
			} else {
				SetChargeState(RetractArm);
			}
		}
		break;
	case RetractArm:
	{
		if( m_fSequenceFinished )
		{
			m_currentYaw = m_goalYaw = 0;
			SetBoneController(RECHARGER_ARM_CONTROLLER, m_currentYaw);
			if ( m_iJuice <= 0 )
			{
				SetChargeState(Inactive);
				const float rechargeTime = g_pGameRules->FlHEVChargerRechargeTime();
				if (rechargeTime > 0 ) {
					pev->nextthink = gpGlobals->time + rechargeTime;
					SetThink( &CRechargeDecay::Recharge );
				}
			}
		}
		else
		{
			SetChargeController(m_currentYaw*0.75);
		}
		break;
	}
	default:
		break;
	}
}

void CRechargeDecay::SetMySequence(const char *sequence)
{
	pev->sequence = LookupSequence( sequence );
	if (pev->sequence == -1) {
		ALERT(at_error, "unknown sequence in %s: %s\n", STRING(pev->model), sequence);
		pev->sequence = 0;
	}
	pev->frame = 0;
	ResetSequenceInfo( );
}

void CRechargeDecay::SetChargeState(int state)
{
	m_iState = state;
	switch (state) {
	case Still:
		SetMySequence("rest");
		break;
	case Deploy:
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/suitchargeok1.wav", 1.0, ATTN_NORM );
		SetMySequence("deploy");
		break;
	case Idle:
		SetMySequence("prep_charge");
		break;
	case GiveShot:
		SetMySequence("give_charge");
		break;
	case Healing:
		SetMySequence("charge_idle");
		break;
	case RetractShot:
		SetMySequence("retract_charge");
		break;
	case RetractArm:
		TurnBeamOff();
		SetMySequence("retract_arm");
		break;
	case Inactive:
		SetMySequence("rest");
	default:
		break;
	}
}

void CRechargeDecay::TurnChargeToPlayer(const Vector& player)
{
	float yaw = UTIL_VecToYaw( player - pev->origin ) - pev->angles.y;

	if( yaw > 180 )
		yaw -= 360;
	if( yaw < -180 )
		yaw += 360;

	SetChargeController( yaw );
}

void CRechargeDecay::SetChargeController(float yaw)
{
	m_goalYaw = yaw;
}

void CRechargeDecay::CreateBeam()
{
	CBeam *beam = GetClassPtr( (CBeam *)NULL );
	if( !beam )
		return;

	beam->BeamInit( "sprites/lgtning.spr", 5 );

	beam->SetType( BEAM_ENTS );
	beam->SetStartEntity( entindex() );
	beam->SetEndEntity( entindex() );
	beam->SetStartAttachment(3);
	beam->SetEndAttachment(4);
	beam->SetColor( 0, 225, 0 );
	beam->SetBrightness( 225 );
	beam->SetNoise( 10 );
	beam->RelinkBeam();

	m_beam = beam;
}

void CRechargeDecay::UpdateOnRemove()
{
	UTIL_Remove(m_beam);
	UTIL_Remove(m_glass);
	m_beam = NULL;
	m_glass = NULL;
	CBaseAnimating::UpdateOnRemove();
}
