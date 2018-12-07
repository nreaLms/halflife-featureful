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

class CRecharge : public CWallCharger
{
public:
	const char* DefaultLoopingSound() { return "items/suitcharge1.wav"; }
	int RechargeTime() { return (int)g_pGameRules->FlHEVChargerRechargeTime(); }
	const char* DefaultRechargeSound() { return NULL; }
	int ChargerCapacity() { return (int)gSkillData.suitchargerCapacity; }
	const char* DefaultDenySound() { return "items/suitchargeno1.wav"; }
	const char* DefaultChargeStartSound() { return "items/suitchargeok1.wav"; }
	float SoundVolume() { return 0.85f; }
	bool GiveCharge(CBaseEntity* pActivator)
	{
		if (pActivator->pev->armorvalue < MAX_NORMAL_BATTERY)
		{
			pActivator->pev->armorvalue += 1;

			if( pActivator->pev->armorvalue > MAX_NORMAL_BATTERY )
				pActivator->pev->armorvalue = MAX_NORMAL_BATTERY;

			return true;
		}
		return false;
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

class CRechargeDecay : public CBaseAnimating
{
public:
	void Spawn();
	void Precache(void);
	void EXPORT SearchForPlayer();
	void EXPORT Off( void );
	void EXPORT Recharge( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int ObjectCaps( void ) { return ( CBaseAnimating::ObjectCaps() | FCAP_CONTINUOUS_USE ) & ~FCAP_ACROSS_TRANSITION; }
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
	CRechargeGlassDecay* m_glass;
	BOOL m_playingChargeSound;
	CBeam* m_beam;
	float m_lastYaw;

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
	DEFINE_FIELD( CRechargeDecay, m_playingChargeSound, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE( CRechargeDecay, CBaseAnimating )

void CRechargeDecay::Spawn()
{
	m_iJuice = gSkillData.suitchargerCapacity;
	Precache();

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_FLY;

	SET_MODEL(ENT(pev), "models/hev.mdl");
	UTIL_SetSize(pev, Vector(-12, -16, 0), Vector(12, 16, 48));
	UTIL_SetOrigin(pev, pev->origin);
	pev->skin = 0;

	InitBoneControllers();
	SetBoneController(1, 360);

	if (m_iJuice > 0)
	{
		m_iState = Still;
		SetThink(&CRechargeDecay::SearchForPlayer);
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

void CRechargeDecay::SearchForPlayer()
{
	StudioFrameAdvance();
	CBaseEntity* pEntity = 0;
	pev->nextthink = gpGlobals->time + 0.1;
	UTIL_MakeVectors( pev->angles );
	while((pEntity = UTIL_FindEntityInSphere(pEntity, Center(), 64)) != 0) { // this must be in sync with PLAYER_SEARCH_RADIUS from player.cpp
		if (pEntity->IsPlayer() && pEntity->IsAlive() && FBitSet(pEntity->pev->weapons, 1 << WEAPON_SUIT)) {
			if (DotProduct(pEntity->pev->origin - pev->origin, gpGlobals->v_forward) < 0) {
				continue;
			}
			TurnChargeToPlayer(pEntity->pev->origin);
			switch (m_iState) {
			case RetractShot:
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
				SetChargeController(m_lastYaw*0.75);
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
	if( !pActivator )
		return;
	// if it's not a player, ignore
	if( !pActivator->IsPlayer() )
		return;

	// if the player doesn't have the suit, or there is no juice left, make the deny noise
	if( ( m_iJuice <= 0 ) || ( !( pActivator->pev->weapons & ( 1 << WEAPON_SUIT ) ) ) || pActivator->pev->armorvalue >= 100 )
	{
		if( m_flSoundTime <= gpGlobals->time )
		{
			m_flSoundTime = gpGlobals->time + 0.62;
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/suitchargeno1.wav", 0.85, ATTN_NORM );
		}
		return;
	}

	if (m_iState != Idle && m_iState != GiveShot && m_iState != Healing && m_iState != Inactive)
		return;

	// if there is no juice left, turn it off
	if( (m_iState == Healing || m_iState == GiveShot) && m_iJuice <= 0 )
	{
		pev->skin = 1;
		SetThink(&CRechargeDecay::Off);
		pev->nextthink = gpGlobals->time;
	}
	else
	{
		SetThink(&CRechargeDecay::Off);
		pev->nextthink = gpGlobals->time + 0.25;
	}

	// Time to recharge yet?
	if( m_flNextCharge >= gpGlobals->time )
		return;

	TurnChargeToPlayer(pActivator->pev->origin);
	switch (m_iState) {
	case Idle:
		m_flSoundTime = 0.56 + gpGlobals->time;
		SetChargeState(GiveShot);
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/suitchargeok1.wav", 1.0, ATTN_NORM );
		break;
	case GiveShot:
		SetChargeState(Healing);
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
	if( pActivator->pev->armorvalue < MAX_NORMAL_BATTERY )
	{
		m_iJuice--;
		if (m_iJuice <= 0)
			pev->skin = 1;
		pActivator->pev->armorvalue += 1;
		const float boneControllerValue = (m_iJuice / gSkillData.suitchargerCapacity) * 360;
		SetBoneController(1, 360 - boneControllerValue);
		SetBoneController(2,  boneControllerValue);

		if( pActivator->pev->armorvalue > MAX_NORMAL_BATTERY )
			pActivator->pev->armorvalue = MAX_NORMAL_BATTERY;
	}

	// govern the rate of charge
	m_flNextCharge = gpGlobals->time + 0.1;
}

void CRechargeDecay::Recharge( void )
{
//	/EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/suitcharge1.wav", 1.0, ATTN_NORM );
	m_iJuice = gSkillData.healthchargerCapacity;
	SetBoneController(1, 360);
	SetBoneController(2, 0);
	pev->skin = 0;
	SetChargeState(Still);
	SetThink( &CRechargeDecay::SearchForPlayer );
	pev->nextthink = gpGlobals->time;
}

void CRechargeDecay::Off( void )
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;
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
		if (m_iJuice > 0) {
			SetChargeState(Idle);
			SetThink( &CRechargeDecay::SearchForPlayer );
			pev->nextthink = gpGlobals->time;
		} else {
			SetChargeState(RetractArm);
		}
		break;
	case RetractArm:
	{
		if( m_fSequenceFinished )
		{
			SetChargeController(0);
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
			SetChargeController(m_lastYaw*0.75);
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
	m_lastYaw = yaw;
	SetBoneController(3, yaw);
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
	CBaseAnimating::UpdateOnRemove();
	UTIL_Remove(m_beam);
	UTIL_Remove(m_glass);
	m_beam = NULL;
	m_glass = NULL;
}
