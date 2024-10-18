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

===== weapons.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "rpgrocket.h"
#include "soundent.h"
#include "decals.h"
#include "game.h"
#include "gamerules.h"
#include "ammoregistry.h"
#include "ammo_amounts.h"
#include "common_soundscripts.h"

extern int gEvilImpulse101;

DLL_GLOBAL	short g_sModelIndexLaser;// holds the index for the laser beam
DLL_GLOBAL	const char *g_pModelNameLaser = "sprites/laserbeam.spr";
DLL_GLOBAL	short g_sModelIndexLaserDot;// holds the index for the laser beam dot
DLL_GLOBAL	short g_sModelIndexFireball;// holds the index for the fireball
DLL_GLOBAL	short g_sModelIndexSmoke;// holds the index for the smoke cloud
DLL_GLOBAL	const char* g_pModelNameSmoke = "sprites/steam1.spr";
DLL_GLOBAL	short g_sModelIndexWExplosion;// holds the index for the underwater explosion
DLL_GLOBAL	short g_sModelIndexBubbles;// holds the index for the bubbles model
DLL_GLOBAL	short g_sModelIndexBloodDrop;// holds the sprite index for the initial blood
DLL_GLOBAL	short g_sModelIndexBloodSpray;// holds the sprite index for splattered blood

ItemInfo CBasePlayerWeapon::ItemInfoArray[MAX_WEAPONS];

extern int gmsgCurWeapon;

MULTIDAMAGE gMultiDamage;

#define TRACER_FREQ		4			// Tracers fire every fourth bullet

/*
==============================================================================

MULTI-DAMAGE

Collects multiple small damages into a single damage

==============================================================================
*/

//
// ClearMultiDamage - resets the global multi damage accumulator
//
void ClearMultiDamage( void )
{
	gMultiDamage.pEntity = NULL;
	gMultiDamage.amount = 0;
	gMultiDamage.type = 0;
}

//
// ApplyMultiDamage - inflicts contents of global multi damage register on gMultiDamage.pEntity
//
// GLOBALS USED:
//		gMultiDamage
void ApplyMultiDamage( entvars_t *pevInflictor, entvars_t *pevAttacker )
{
	Vector vecSpot1;//where blood comes from
	Vector vecDir;//direction blood should go
	TraceResult tr;

	if( !gMultiDamage.pEntity )
		return;

	gMultiDamage.pEntity->TakeDamage( pevInflictor, pevAttacker, gMultiDamage.amount, gMultiDamage.type );
}

// GLOBALS USED:
//		gMultiDamage
void AddMultiDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, CBaseEntity *pEntity, float flDamage, int bitsDamageType )
{
	if( !pEntity )
		return;

	gMultiDamage.type |= bitsDamageType;

	if( pEntity != gMultiDamage.pEntity )
	{
		ApplyMultiDamage( pevInflictor, pevAttacker );
		gMultiDamage.pEntity = pEntity;
		gMultiDamage.amount = 0;
	}

	gMultiDamage.amount += flDamage;
}

/*
================
SpawnBlood
================
*/
void SpawnBlood( Vector vecSpot, int bloodColor, float flDamage )
{
	UTIL_BloodDrips( vecSpot, g_vecAttackDir, bloodColor, (int)flDamage );
}

int DamageDecal( CBaseEntity *pEntity, int bitsDamageType )
{
	if( !pEntity )
		return ( DECAL_GUNSHOT1 + RANDOM_LONG( 0, 4 ) );
	
	return pEntity->DamageDecal( bitsDamageType );
}

void DecalGunshot( TraceResult *pTrace, int iBulletType )
{
	// Is the entity valid
	if( !UTIL_IsValidEntity( pTrace->pHit ) )
		return;

	if( VARS( pTrace->pHit )->solid == SOLID_BSP || VARS( pTrace->pHit )->movetype == MOVETYPE_PUSHSTEP )
	{
		CBaseEntity *pEntity = NULL;
		// Decal the wall with a gunshot
		if( !FNullEnt( pTrace->pHit ) )
			pEntity = CBaseEntity::Instance( pTrace->pHit );

		switch( iBulletType )
		{
		case BULLET_PLAYER_9MM:
		case BULLET_MONSTER_9MM:
		case BULLET_PLAYER_MP5:
		case BULLET_MONSTER_MP5:
		case BULLET_PLAYER_BUCKSHOT:
		case BULLET_PLAYER_357:
		case BULLET_PLAYER_EAGLE:
		case BULLET_MONSTER_357:
		case BULLET_PLAYER_556:
		case BULLET_MONSTER_556:
		case BULLET_PLAYER_762:
		case BULLET_MONSTER_762:
		case BULLET_PLAYER_UZI:
		default:
			// smoke and decal
			UTIL_GunshotDecalTrace( pTrace, DamageDecal( pEntity, DMG_BULLET ) );
			break;
		case BULLET_MONSTER_12MM:
			// smoke and decal
			UTIL_GunshotDecalTrace( pTrace, DamageDecal( pEntity, DMG_BULLET ) );
			break;
		case BULLET_PLAYER_CROWBAR:
			// wall decal
			UTIL_DecalTrace( pTrace, DamageDecal( pEntity, DMG_CLUB ) );
			break;
		}
	}
}

//
// EjectBrass - tosses a brass shell from passed origin at passed velocity
//
void EjectBrass( const Vector &vecOrigin, const Vector &vecVelocity, float rotation, int model, int soundtype )
{
	// FIX: when the player shoots, their gun isn't in the same position as it is on the model other players see.
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecOrigin );
		WRITE_BYTE( TE_MODEL );
		WRITE_VECTOR( vecOrigin );
		WRITE_VECTOR( vecVelocity );
		WRITE_ANGLE( rotation );
		WRITE_SHORT( model );
		WRITE_BYTE( soundtype );
		WRITE_BYTE( 25 );// 2.5 seconds
	MESSAGE_END();
}

#if 0
// UNDONE: This is no longer used?
void ExplodeModel( const Vector &vecOrigin, float speed, int model, int count )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecOrigin );
		WRITE_BYTE( TE_EXPLODEMODEL );
		WRITE_VECTOR( vecOrigin );
		WRITE_COORD( speed );
		WRITE_SHORT( model );
		WRITE_SHORT( count );
		WRITE_BYTE( 15 );// 1.5 seconds
	MESSAGE_END();
}
#endif

bool bIsMultiplayer()
{
	return g_pGameRules->IsMultiplayer();
}

// Precaches the weapon and queues the weapon info for sending to clients
bool UTIL_PrecacheOtherWeapon( const char *szClassname )
{
	edict_t	*pent;

	pent = CREATE_NAMED_ENTITY( MAKE_STRING( szClassname ) );
	if( FNullEnt( pent ) )
	{
		ALERT( at_console, "NULL Ent in UTIL_PrecacheOtherWeapon\n" );
		return false;
	}
	
	CBaseEntity *pEntity = CBaseEntity::Instance( VARS( pent ) );

	bool result = true;
	if( pEntity )
	{
		ItemInfo II = {0};
		CBasePlayerWeapon* pWeapon = pEntity->MyWeaponPointer();
		if( pWeapon != 0 )
		{
			if (pWeapon->IsEnabledInMod())
			{
				pEntity->Precache();

				if (pWeapon->GetItemInfo( &II ))
				{
					CBasePlayerWeapon::ItemInfoArray[II.iId] = II;
				}
			}
			else
				result = false;
		}
		else
		{
			ALERT(at_console, "UTIL_PrecacheOtherWeapon: %s is not a weapon\n", szClassname);
			result = false;
		}
	}

	REMOVE_ENTITY( pent );
	return result;
}

void RegisterAmmoTypes()
{
	g_AmmoRegistry.Register("buckshot", BUCKSHOT_MAX_CARRY);
	g_AmmoRegistry.Register("9mm", _9MM_MAX_CARRY);
	g_AmmoRegistry.Register("ARgrenades", M203_GRENADE_MAX_CARRY);
	g_AmmoRegistry.Register("357", _357_MAX_CARRY);
	g_AmmoRegistry.Register("uranium", URANIUM_MAX_CARRY);
	g_AmmoRegistry.Register("rockets", ROCKET_MAX_CARRY);
	g_AmmoRegistry.Register("bolts", BOLT_MAX_CARRY);
	g_AmmoRegistry.Register("Trip Mine", TRIPMINE_MAX_CARRY, true);
	g_AmmoRegistry.Register("Satchel Charge", SATCHEL_MAX_CARRY, true);
	g_AmmoRegistry.Register("Hand Grenade", HANDGRENADE_MAX_CARRY, true);
	g_AmmoRegistry.Register("Snarks", SNARK_MAX_CARRY, true);
	g_AmmoRegistry.Register("Hornets", HORNET_MAX_CARRY);
	g_AmmoRegistry.Register("Medicine", MEDKIT_MAX_CARRY);
	g_AmmoRegistry.Register("Penguins", PENGUIN_MAX_CARRY, true);
	g_AmmoRegistry.Register("556", _556_MAX_CARRY);
	g_AmmoRegistry.Register("762", _762_MAX_CARRY);
	g_AmmoRegistry.Register("Shocks", SHOCK_MAX_CARRY);
	g_AmmoRegistry.Register("spores", SPORE_MAX_CARRY);

	for (int i = 0; i<g_modFeatures.maxAmmoCount; ++i)
	{
		g_AmmoRegistry.SetMaxAmmo(g_modFeatures.maxAmmos[i].name, g_modFeatures.maxAmmos[i].maxAmmo);
	}
}

// called by worldspawn
void W_Precache( CBaseEntity* pWorld )
{
	memset( CBasePlayerWeapon::ItemInfoArray, 0, sizeof(CBasePlayerWeapon::ItemInfoArray) );

	// custom items...

	// common world objects
	UTIL_PrecacheOther( "item_suit" );
	UTIL_PrecacheOther( "item_healthkit" );
	UTIL_PrecacheOther( "item_battery" );
	UTIL_PrecacheOther( "item_antidote" );
	UTIL_PrecacheOther( "item_security" );
	UTIL_PrecacheOther( "item_longjump" );

	UTIL_PrecacheOther( "item_flashlight" );
	UTIL_PrecacheOther( "item_nvgs" );

	// shotgun
	UTIL_PrecacheOtherWeapon( "weapon_shotgun" );
	UTIL_PrecacheOther( "ammo_buckshot" );

	// crowbar
	UTIL_PrecacheOtherWeapon( "weapon_crowbar" );

	// glock
	UTIL_PrecacheOtherWeapon( "weapon_9mmhandgun" );
	UTIL_PrecacheOther( "ammo_9mmclip" );

	// mp5
	UTIL_PrecacheOtherWeapon( "weapon_9mmAR" );
	UTIL_PrecacheOther( "ammo_9mmAR" );
	UTIL_PrecacheOther( "ammo_ARgrenades" );

	// 9mm ammo box
	UTIL_PrecacheOther( "ammo_9mmbox" );

	// python
	UTIL_PrecacheOtherWeapon( "weapon_357" );
	UTIL_PrecacheOther( "ammo_357" );

	// gauss
	UTIL_PrecacheOtherWeapon( "weapon_gauss" );
	UTIL_PrecacheOther( "ammo_gaussclip" );

	// rpg
	UTIL_PrecacheOtherWeapon( "weapon_rpg" );
	UTIL_PrecacheOther( "ammo_rpgclip" );

	// crossbow
	UTIL_PrecacheOtherWeapon( "weapon_crossbow" );
	UTIL_PrecacheOther( "ammo_crossbow" );

	// egon
	UTIL_PrecacheOtherWeapon( "weapon_egon" );

	// tripmine
	UTIL_PrecacheOtherWeapon( "weapon_tripmine" );

	// satchel charge
	UTIL_PrecacheOtherWeapon( "weapon_satchel" );

	// hand grenade
	UTIL_PrecacheOtherWeapon("weapon_handgrenade");

	// squeak grenade
	UTIL_PrecacheOtherWeapon( "weapon_snark" );

	// hornetgun
	UTIL_PrecacheOtherWeapon( "weapon_hornetgun" );
#if FEATURE_MEDKIT
	UTIL_PrecacheOtherWeapon( "weapon_medkit" );
#endif
	if( g_pGameRules->IsDeathmatch() )
	{
		UTIL_PrecacheOther( "weaponbox" );// container for dropped deathmatch weapons
	}
#if FEATURE_DESERT_EAGLE
	UTIL_PrecacheOtherWeapon( "weapon_eagle" );
#endif
#if FEATURE_PIPEWRENCH
	UTIL_PrecacheOtherWeapon( "weapon_pipewrench" );
#endif
#if FEATURE_KNIFE
	UTIL_PrecacheOtherWeapon( "weapon_knife" );
#endif
#if FEATURE_GRAPPLE
	if (UTIL_PrecacheOtherWeapon( "weapon_grapple" ))
		UTIL_PrecacheOther( "grapple_tip" );
#endif
#if FEATURE_PENGUIN
	UTIL_PrecacheOtherWeapon( "weapon_penguin" );
#endif
#if FEATURE_M249
	if (UTIL_PrecacheOtherWeapon( "weapon_m249" ))
		UTIL_PrecacheOther( "ammo_556" );
#endif
#if FEATURE_SNIPERRIFLE
	if (UTIL_PrecacheOtherWeapon( "weapon_sniperrifle" ))
		UTIL_PrecacheOther( "ammo_762" );
#endif
#if FEATURE_DISPLACER
	UTIL_PrecacheOtherWeapon( "weapon_displacer" );
#endif
#if FEATURE_SHOCKRIFLE
	UTIL_PrecacheOtherWeapon( "weapon_shockrifle" );
#endif
#if FEATURE_SPORELAUNCHER
	UTIL_PrecacheOtherWeapon( "weapon_sporelauncher" );
#endif
#if FEATURE_UZI
	UTIL_PrecacheOtherWeapon( "weapon_uzi" );
#endif
	g_sModelIndexFireball = PRECACHE_MODEL( "sprites/zerogxplode.spr" );// fireball
	g_sModelIndexWExplosion = PRECACHE_MODEL( "sprites/WXplo1.spr" );// underwater fireball
	g_sModelIndexSmoke = PRECACHE_MODEL( g_pModelNameSmoke );// smoke
	g_sModelIndexBubbles = PRECACHE_MODEL( "sprites/bubble.spr" );//bubbles
	g_sModelIndexBloodSpray = PRECACHE_MODEL( "sprites/bloodspray.spr" ); // initial blood
	g_sModelIndexBloodDrop = PRECACHE_MODEL( "sprites/blood.spr" ); // splattered blood 

	g_sModelIndexLaser = PRECACHE_MODEL( g_pModelNameLaser );
	g_sModelIndexLaserDot = PRECACHE_MODEL( "sprites/laserdot.spr" );

	// used by explosions
	PRECACHE_MODEL( "models/grenade.mdl" );
	PRECACHE_MODEL( "sprites/explode1.spr" );

	PRECACHE_SOUND( "weapons/bullet_hit1.wav" );	// hit by bullet
	PRECACHE_SOUND( "weapons/bullet_hit2.wav" );	// hit by bullet

	pWorld->RegisterAndPrecacheSoundScript(Items::weaponDropSoundScript);// weapon falls to the ground
	pWorld->RegisterAndPrecacheSoundScript(Items::weaponEmptySoundScript);

	UTIL_PrecacheOther("grenade");
}

TYPEDESCRIPTION	CBasePlayerWeapon::m_SaveData[] =
{
	DEFINE_FIELD( CBasePlayerWeapon, m_pPlayer, FIELD_CLASSPTR ),
	//DEFINE_FIELD( CBasePlayerItem, m_fKnown, FIELD_INTEGER ),Reset to zero on load
	DEFINE_FIELD( CBasePlayerWeapon, m_iId, FIELD_INTEGER ),
	// DEFINE_FIELD( CBasePlayerItem, m_iIdPrimary, FIELD_INTEGER ),
	// DEFINE_FIELD( CBasePlayerItem, m_iIdSecondary, FIELD_INTEGER ),
#if CLIENT_WEAPONS
	DEFINE_FIELD( CBasePlayerWeapon, m_flNextPrimaryAttack, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flNextSecondaryAttack, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flTimeWeaponIdle, FIELD_FLOAT ),
#else	// CLIENT_WEAPONS
	DEFINE_FIELD( CBasePlayerWeapon, m_flNextPrimaryAttack, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flNextSecondaryAttack, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flTimeWeaponIdle, FIELD_TIME ),
#endif	// CLIENT_WEAPONS
	DEFINE_FIELD( CBasePlayerWeapon, m_iPrimaryAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iSecondaryAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iClip, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iDefaultAmmo, FIELD_INTEGER ),
	//DEFINE_FIELD( CBasePlayerWeapon, m_iClientClip, FIELD_INTEGER ), reset to zero on load so hud gets updated correctly
	//DEFINE_FIELD( CBasePlayerWeapon, m_iClientWeaponState, FIELD_INTEGER ), reset to zero on load so hud gets updated correctly
};

IMPLEMENT_SAVERESTORE( CBasePlayerWeapon, CBaseAnimating )

void CBasePlayerWeapon::SetObjectCollisionBox( void )
{
	pev->absmin = pev->origin + Vector( -24, -24, 0 );
	pev->absmax = pev->origin + Vector( 24, 24, 16 ); 
}

void CBasePlayerWeapon::KeyValue(KeyValueData *pkvd)
{
	if( FStrEq( pkvd->szKeyName, "initammo" ) )
	{
		m_iDefaultAmmo = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseAnimating::KeyValue(pkvd);
}

//=========================================================
// Sets up movetype, size, solidtype for a new weapon. 
//=========================================================
void CBasePlayerWeapon::FallInit( void )
{
	if (pev->movetype < 0)
		pev->movetype = MOVETYPE_NONE;
	else if (pev->movetype == 0)
		pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );//pointsize until it lands on the ground.

	SetTouch( &CBasePlayerWeapon::DefaultTouch );
	SetThink( &CBasePlayerWeapon::FallThink );

	pev->nextthink = gpGlobals->time + 0.1f;
}

//=========================================================
// FallThink - Items that have just spawned run this think
// to catch them when they hit the ground. Once we're sure
// that the object is grounded, we change its solid type
// to trigger and set it in a large box that helps the
// player get it.
//=========================================================
void CBasePlayerWeapon::FallThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1f;

	if( (pev->flags & FL_ONGROUND) || pev->movetype != MOVETYPE_TOSS )
	{
		// clatter if we have an owner (i.e., dropped by someone)
		// don't clatter if the gun is waiting to respawn (if it's waiting, it is invisible!)
		if( !FNullEnt( pev->owner ) )
		{
			EmitSoundScript(Items::weaponDropSoundScript);
		}

		// lie flat
		pev->angles.x = 0;
		pev->angles.z = 0;

		Materialize(); 
	}
	else if( m_pPlayer )
	{
		SetThink( NULL );
	}

	if( g_pGameRules->IsBustingGame())
	{
		if( !FNullEnt( pev->owner ))
			return;

		if( FClassnameIs( pev, "weapon_egon" ))
			UTIL_Remove( this );
	}
}

//=========================================================
// Materialize - make a CBasePlayerItem visible and tangible
//=========================================================
void CBasePlayerWeapon::Materialize( void )
{
	if( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		EmitSoundScript(Items::materializeSoundScript);
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	pev->solid = SOLID_TRIGGER;

	//const int itemSize = 24;
	//UTIL_SetSize( pev, Vector( -itemSize, -itemSize, 0 ), Vector( itemSize, itemSize, itemSize ) );
	UTIL_SetOrigin( pev, pev->origin );// link into world.
	SetTouch( &CBasePlayerWeapon::DefaultTouch );
	SetThink( NULL );
}

//=========================================================
// AttemptToMaterialize - the item is trying to rematerialize,
// should it do so now or wait longer?
//=========================================================
void CBasePlayerWeapon::AttemptToMaterialize( void )
{
	float time = g_pGameRules->FlWeaponTryRespawn( this );

	if( time == 0 )
	{
		SetThink( &CBasePlayerWeapon::FallThink );
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	pev->nextthink = gpGlobals->time + time;
}

//=========================================================
// CheckRespawn - a player is taking this weapon, should 
// it respawn?
//=========================================================
void CBasePlayerWeapon::CheckRespawn( void )
{
	switch( g_pGameRules->WeaponShouldRespawn( this ) )
	{
	case GR_WEAPON_RESPAWN_YES:
		Respawn();
		break;
	case GR_WEAPON_RESPAWN_NO:
		return;
		break;
	}
}

//=========================================================
// Respawn- this item is already in the world, but it is
// invisible and intangible. Make it visible and tangible.
//=========================================================
CBaseEntity* CBasePlayerWeapon::Respawn( void )
{
	// make a copy of this weapon that is invisible and inaccessible to players (no touch function). The weapon spawn/respawn code
	// will decide when to make the weapon visible and touchable.
	CBaseEntity *pNewWeapon = CBaseEntity::Create( STRING( pev->classname ), g_pGameRules->VecWeaponRespawnSpot( this ), pev->angles, pev->owner );

	if( pNewWeapon )
	{
		pNewWeapon->pev->effects |= EF_NODRAW;// invisible for now
		pNewWeapon->SetTouch( NULL );// no touch
		pNewWeapon->SetThink( &CBasePlayerWeapon::AttemptToMaterialize );

		//DROP_TO_FLOOR( ENT( pev ) );

		// not a typo! We want to know when the weapon the player just picked up should respawn! This new entity we created is the replacement,
		// but when it should respawn is based on conditions belonging to the weapon that was taken.
		pNewWeapon->pev->nextthink = g_pGameRules->FlWeaponRespawnTime( this );
	}
	else
	{
		ALERT( at_console, "Respawn failed to create %s!\n", STRING( pev->classname ) );
	}

	return pNewWeapon;
}

static bool IsPickableByTouch(CBaseEntity* pEntity)
{
	return !FBitSet(pEntity->pev->spawnflags, SF_ITEM_USE_ONLY) &&
			(FBitSet(pEntity->pev->spawnflags, SF_ITEM_TOUCH_ONLY) || ItemsPickableByTouch());
}

static bool IsPickableByUse(CBaseEntity* pEntity)
{
	return !FBitSet(pEntity->pev->spawnflags, SF_ITEM_TOUCH_ONLY) &&
			(FBitSet(pEntity->pev->spawnflags, SF_ITEM_USE_ONLY) || ItemsPickableByUse());
}

void CBasePlayerWeapon::DefaultTouch( CBaseEntity *pOther )
{
	if (IsPickableByTouch(this)) {
		TouchOrUse(pOther);
	}
}

int CBasePlayerWeapon::ObjectCaps()
{
	if (IsPickableByUse(this) && !(pev->effects & EF_NODRAW)) {
		return CBaseAnimating::ObjectCaps() | FCAP_IMPULSE_USE | FCAP_ONLYVISIBLE_USE;
	} else {
		return CBaseAnimating::ObjectCaps();
	}
}

void CBasePlayerWeapon::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (IsPickableByUse(this) && !(pev->effects & EF_NODRAW) ) {
		TouchOrUse(pCaller);
	}
}

void CBasePlayerWeapon::TouchOrUse(CBaseEntity *pOther )
{
	// if it's not a player, ignore
	if( !pOther->IsPlayer() )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	// can I have this?
	if( !g_pGameRules->CanHavePlayerItem( pPlayer, this ) )
	{
		if( gEvilImpulse101 )
		{
			UTIL_Remove( this );
		}
		return;
	}

	if( pOther->AddPlayerItem( this ) == GOT_NEW_ITEM )
	{
		pPlayer->EmitSoundScript(GetSoundScript(Items::weaponPickupSoundScript));
	}

	SUB_UseTargets( pOther );
}

void CBasePlayerWeapon::DestroyItem( void )
{
	if( m_pPlayer )
	{
		// if attached to a player, remove.
		m_pPlayer->pev->weapons &= ~( 1 << m_iId );
		m_pPlayer->RemovePlayerItem( this, false );
		//m_pPlayer = NULL;
	}

	Kill();
}

void CBasePlayerWeapon::Drop( void )
{
	SetTouch( NULL );
	SetUse( NULL );
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CBasePlayerWeapon::Kill( void )
{
	SetTouch( NULL );
	SetUse( NULL );
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CBasePlayerWeapon::AttachToPlayer( CBasePlayer *pPlayer )
{
	pev->movetype = MOVETYPE_FOLLOW;
	pev->solid = SOLID_NOT;
	pev->aiment = pPlayer->edict();
	pev->effects = EF_NODRAW; // ??
	pev->modelindex = 0;// server won't send down to clients if modelindex == 0
	pev->model = iStringNull;
	pev->owner = pPlayer->edict();

	pev->nextthink = 0;// Remove think - prevents futher attempts to materialize

	SetTouch( NULL );
	SetThink( NULL );
}

const AmmoType* CBasePlayerWeapon::GetAmmoType(const char *name)
{
	return g_AmmoRegistry.GetByName(name);
}

void CBasePlayerWeapon::InitDefaultAmmo(int defaultGive)
{
	if (m_iDefaultAmmo == 0)
	{
		const int amount = g_AmmoAmounts.AmountForAmmoEnt(STRING(pev->classname));
		if (amount >= 0)
			m_iDefaultAmmo = amount;
		else
			m_iDefaultAmmo = defaultGive;
	}
	else if (m_iDefaultAmmo < 0)
	{
		m_iDefaultAmmo = 0;
	}
	else
	{
		// pass, already initialized
	}
}

// CALLED THROUGH the newly-touched weapon's instance. The existing player weapon is pOriginal
int CBasePlayerWeapon::AddDuplicate( CBasePlayerWeapon *pOriginal )
{
	if( m_iDefaultAmmo )
	{
		return ExtractAmmo( pOriginal );
	}
	else
	{
		// a dead player dropped this.
		return ExtractClipAmmo( pOriginal );
	}
}

int CBasePlayerWeapon::AddToPlayer( CBasePlayer *pPlayer )
{
	m_pPlayer = pPlayer;

	pPlayer->pev->weapons |= ( 1 << m_iId );

	if( !m_iPrimaryAmmoType )
	{
		m_iPrimaryAmmoType = pPlayer->GetAmmoIndex( pszAmmo1() );
		m_iSecondaryAmmoType = pPlayer->GetAmmoIndex( pszAmmo2() );
	}
	// Remove weapon's global name to avoid problems with carrying the weapon to other maps
	pev->globalname = iStringNull;

	return AddWeapon();
}

int CBasePlayerWeapon::AddToPlayerDefault( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

int CBasePlayerWeapon::UpdateClientData( CBasePlayer *pPlayer )
{
	BOOL bSend = FALSE;
	int state = 0;
	if( pPlayer->m_pActiveItem == this )
	{
		if( pPlayer->m_fOnTarget )
			state = WEAPON_IS_ONTARGET;
		else
			state = 1;
	}

	// Forcing send of all data!
	if( !pPlayer->m_fWeapon )
	{
		bSend = TRUE;
	}

	// This is the current or last weapon, so the state will need to be updated
	if( this == pPlayer->m_pActiveItem || this == pPlayer->m_pClientActiveItem )
	{
		if( pPlayer->m_pActiveItem != pPlayer->m_pClientActiveItem )
		{
			bSend = TRUE;
		}
	}

	// If the ammo, state, or fov has changed, update the weapon
	if( m_iClip != m_iClientClip || state != m_iClientWeaponState || pPlayer->m_iFOV != pPlayer->m_iClientFOV )
	{
		bSend = TRUE;
	}

	if( bSend )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pPlayer->pev );
			WRITE_BYTE( state );
			WRITE_BYTE( m_iId );
			WRITE_SHORT( m_iClip );
		MESSAGE_END();

		m_iClientClip = m_iClip;
		m_iClientWeaponState = state;
		pPlayer->m_fWeapon = TRUE;
	}

	return 1;
}

void CBasePlayerWeapon::SendWeaponAnim(int iAnim, int body )
{
	const bool skiplocal = !m_ForceSendAnimations && UseDecrement() != FALSE;

	m_pPlayer->pev->weaponanim = iAnim;

#if CLIENT_WEAPONS
	if( skiplocal && ENGINE_CANSKIP( m_pPlayer->edict() ) )
		return;
#endif
	MESSAGE_BEGIN( MSG_ONE, SVC_WEAPONANIM, NULL, m_pPlayer->pev );
		WRITE_BYTE( iAnim );		// sequence number
		WRITE_BYTE( pev->body );	// weaponmodel bodygroup.
	MESSAGE_END();
}

BOOL CBasePlayerWeapon::AddPrimaryAmmo( int iCount )
{
	int iIdAmmo;
	int maxClip = iMaxClip();
	const char* szName = pszAmmo1();

	if( maxClip < 1 )
	{
		m_iClip = -1;
		iIdAmmo = m_pPlayer->GiveAmmo( iCount, szName );
	}
	else if( m_iClip == 0 )
	{
		int i;
		i = Q_min( m_iClip + iCount, maxClip ) - m_iClip;
		m_iClip += i;
		iIdAmmo = m_pPlayer->GiveAmmo( iCount - i, szName );
	}
	else
	{
		iIdAmmo = m_pPlayer->GiveAmmo( iCount, szName );
	}

	// m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = iMaxCarry; // hack for testing

	if( iIdAmmo > 0 )
	{
		m_iPrimaryAmmoType = iIdAmmo;
		if( m_pPlayer->HasPlayerItem( this ) )
		{
			// play the "got ammo" sound only if we gave some ammo to a player that already had this gun.
			// if the player is just getting this gun for the first time, DefaultTouch will play the "picked up gun" sound for us.
			EmitSoundScript(Items::ammoPickupSoundScript);
		}
	}

	return iIdAmmo > 0 ? TRUE : FALSE;
}

BOOL CBasePlayerWeapon::AddSecondaryAmmo(int iCount)
{
	int iIdAmmo = m_pPlayer->GiveAmmo( iCount, pszAmmo2() );

	//m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] = iMax; // hack for testing

	if( iIdAmmo > 0 )
	{
		m_iSecondaryAmmoType = iIdAmmo;
		EmitSoundScript(Items::ammoPickupSoundScript);
	}
	return iIdAmmo > 0 ? TRUE : FALSE;
}

//=========================================================
// IsUseable - this function determines whether or not a 
// weapon is useable by the player in its current state. 
// (does it have ammo loaded? do I have any ammo for the 
// weapon?, etc)
//=========================================================
BOOL CBasePlayerWeapon::IsUseable( void )
{
	if( m_iClip > 0 )
	{
		return TRUE;
	}

	// Player has unlimited ammo for this weapon or does not use magazines
	if( iMaxAmmo1() == WEAPON_NOCLIP )
	{
		return TRUE;
	}

	if( m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] > 0 )
	{
		return TRUE;
	}

	if( UsesSecondaryAmmo() )
	{
		// Player has unlimited ammo for this weapon or does not use magazines
		if( iMaxAmmo2() == WEAPON_NOCLIP )
		{
			return TRUE;
		}

		if( m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] > 0 )
		{
			return TRUE;
		}
	}

	// clip is empty (or nonexistant) and the player has no more ammo of this type. 
	return FALSE;
}

BOOL CBasePlayerWeapon::DefaultDeploy( const char *szViewModel, const char *szWeaponModel, int iAnim, const char *szAnimExt, int body )
{
	if( !CanDeploy() )
		return FALSE;

	m_pPlayer->pev->viewmodel = MAKE_STRING( szViewModel );
	if (g_modFeatures.weapon_p_models)
		m_pPlayer->pev->weaponmodel = MAKE_STRING( szWeaponModel );
	strcpy( m_pPlayer->m_szAnimExtention, szAnimExt );
	SendWeaponAnim( iAnim, body );

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
	m_flLastFireTime = 0.0f;

	return TRUE;
}

void CBasePlayerWeapon::PrecachePModel(const char *name)
{
	if (g_modFeatures.weapon_p_models)
		PRECACHE_MODEL(name);
}

BOOL CBasePlayerWeapon::PlayEmptySound( void )
{
	if( m_iPlayEmptySound )
	{
		m_pPlayer->EmitSoundScript(Items::weaponEmptySoundScript);
		m_iPlayEmptySound = 0;
		return 0;
	}
	return 0;
}

//=========================================================
//=========================================================
int CBasePlayerWeapon::PrimaryAmmoIndex( void )
{
	return m_iPrimaryAmmoType;
}

//=========================================================
//=========================================================
int CBasePlayerWeapon::SecondaryAmmoIndex( void )
{
	return m_iSecondaryAmmoType;
}

void CBasePlayerWeapon::Holster()
{ 
	m_fInReload = FALSE; // cancel any reload in progress.
	m_pPlayer->pev->viewmodel = 0; 
	m_pPlayer->pev->weaponmodel = 0;
}

//=========================================================
// called by the new item with the existing item as parameter
//
// if we call ExtractAmmo(), it's because the player is picking up this type of weapon for 
// the first time. If it is spawned by the world, m_iDefaultAmmo will have a default ammo amount in it.
// if  this is a weapon dropped by a dying player, has 0 m_iDefaultAmmo, which means only the ammo in 
// the weapon clip comes along. 
//=========================================================
int CBasePlayerWeapon::ExtractAmmo( CBasePlayerWeapon *pWeapon )
{
	int iReturn = 0;

	if( UsesAmmo() )
	{
		// blindly call with m_iDefaultAmmo. It's either going to be a value or zero. If it is zero,
		// we only get the ammo in the weapon's clip, which is what we want. 
		iReturn |= pWeapon->AddPrimaryAmmo( m_iDefaultAmmo );
		m_iDefaultAmmo = 0;
	}

	if( UsesSecondaryAmmo() )
	{
		iReturn |= pWeapon->AddSecondaryAmmo( 0 );
	}

	return iReturn;
}

//=========================================================
// called by the new item's class with the existing item as parameter
//=========================================================
int CBasePlayerWeapon::ExtractClipAmmo( CBasePlayerWeapon *pWeapon )
{
	int iAmmo;

	if( m_iClip == WEAPON_NOCLIP )
	{
		iAmmo = 0;// guns with no clips always come empty if they are second-hand
	}
	else
	{
		iAmmo = m_iClip;
	}

	return pWeapon->m_pPlayer->GiveAmmo( iAmmo, pszAmmo1() ); // , &m_iPrimaryAmmoType
}
	
//=========================================================
// RetireWeapon - no more ammo for this gun, put it away.
//=========================================================
void CBasePlayerWeapon::RetireWeapon( void )
{
	// first, no viewmodel at all.
	if (m_pPlayer->m_pActiveItem == this)
	{
		m_pPlayer->pev->viewmodel = iStringNull;
		m_pPlayer->pev->weaponmodel = iStringNull;
		//m_pPlayer->pev->viewmodelindex = NULL;
	}

	if (m_pPlayer->m_pActiveItem != this)
	{
		DestroyItem();
	}
	else if( !g_pGameRules->GetNextBestWeapon( m_pPlayer, this ) )
	{
		// Another weapon wasn't selected. Get rid of current one
		if( m_pPlayer->m_pActiveItem == this )
		{
			m_pPlayer->ResetAutoaim();
			m_pPlayer->m_pActiveItem->Holster();
			m_pPlayer->m_pLastItem = NULL;
			m_pPlayer->m_pActiveItem = NULL;
		}
	}
}

//=========================================================================
// GetNextAttackDelay - An accurate way of calcualting the next attack time.
//=========================================================================
float CBasePlayerWeapon::GetNextAttackDelay( float delay )
{
	if( m_flLastFireTime == 0 || m_flNextPrimaryAttack == -1.0f )
	{
		// At this point, we are assuming that the client has stopped firing
		// and we are going to reset our book keeping variables.
		m_flLastFireTime = gpGlobals->time;
		m_flPrevPrimaryAttack = delay;
	}
	// calculate the time between this shot and the previous
	float flTimeBetweenFires = gpGlobals->time - m_flLastFireTime;
	float flCreep = 0.0f;
	if( flTimeBetweenFires > 0 )
		flCreep = flTimeBetweenFires - m_flPrevPrimaryAttack; // postive or negative

	// save the last fire time
	m_flLastFireTime = gpGlobals->time;

	float flNextAttack = UTIL_WeaponTimeBase() + delay - flCreep;
	// we need to remember what the m_flNextPrimaryAttack time is set to for each shot,
	// store it as m_flPrevPrimaryAttack.
	m_flPrevPrimaryAttack = flNextAttack - UTIL_WeaponTimeBase();
	//char szMsg[256];
	//_snprintf( szMsg, sizeof(szMsg), "next attack time: %0.4f\n", gpGlobals->time + flNextAttack );
	//OutputDebugString( szMsg );
	return flNextAttack;
}

//*********************************************************
// weaponbox code:
//*********************************************************

LINK_ENTITY_TO_CLASS( weaponbox, CWeaponBox )

TYPEDESCRIPTION	CWeaponBox::m_SaveData[] =
{
	DEFINE_ARRAY( CWeaponBox, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_TYPES ),
	DEFINE_ARRAY( CWeaponBox, m_rgiszAmmo, FIELD_STRING, MAX_AMMO_TYPES ),
	DEFINE_ARRAY( CWeaponBox, m_rgpPlayerWeapons, FIELD_CLASSPTR, MAX_WEAPONS ),
	DEFINE_FIELD( CWeaponBox, m_cAmmoTypes, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CWeaponBox, CBaseDelay )

//=========================================================
//
//=========================================================
void CWeaponBox::Precache( void )
{
	PRECACHE_MODEL( "models/w_weaponbox.mdl" );
}

//=========================================================
//=========================================================
void CWeaponBox::KeyValue( KeyValueData *pkvd )
{
	CBaseDelay::KeyValue(pkvd);
	if (!pkvd->fHandled)
	{
		if( m_cAmmoTypes < MAX_AMMO_TYPES )
		{
			PackAmmo( ALLOC_STRING( pkvd->szKeyName ), atoi( pkvd->szValue ) );
			m_cAmmoTypes++;// count this new ammo type.

			pkvd->fHandled = TRUE;
		}
		else
		{
			ALERT( at_console, "WeaponBox too full! only %d ammotypes allowed\n", MAX_AMMO_TYPES );
		}
	}
}

//=========================================================
// CWeaponBox - Spawn 
//=========================================================
void CWeaponBox::Spawn( void )
{
	Precache();

	if (pev->movetype < 0)
		pev->movetype = MOVETYPE_NONE;
	else if (pev->movetype == 0)
		pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;

	//UTIL_SetSize( pev, g_vecZero, g_vecZero );

	const int itemSize = 24;
	UTIL_SetSize( pev, Vector( -itemSize, -itemSize, 0 ), Vector( itemSize, itemSize, itemSize ) );

	SET_MODEL( ENT( pev ), "models/w_weaponbox.mdl" );
}

//=========================================================
// CWeaponBox - Kill - the think function that removes the
// box from the world.
//=========================================================
void CWeaponBox::Kill( void )
{
	CBasePlayerWeapon *pWeapon;
	int i;

	// destroy the weapons
	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		pWeapon = m_rgpPlayerWeapons[i];

		if( pWeapon )
		{
			pWeapon->SetThink( &CBaseEntity::SUB_Remove );
			pWeapon->pev->nextthink = gpGlobals->time + 0.1f;
		}
	}

	// remove the box
	UTIL_Remove( this );
}

//=========================================================
// CWeaponBox - Touch: try to add my contents to the toucher
// if the toucher is a player.
//=========================================================

void CWeaponBox::Touch( CBaseEntity *pOther )
{
	if (IsPickableByTouch(this)) {
		TouchOrUse(pOther);
	}
}

int CWeaponBox::ObjectCaps()
{
	if (IsPickableByUse(this) && !(pev->effects & EF_NODRAW)) {
		return CBaseEntity::ObjectCaps() | FCAP_IMPULSE_USE;
	} else {
		return CBaseEntity::ObjectCaps();
	}
}

void CWeaponBox::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (IsPickableByUse(this) && !(pev->effects & EF_NODRAW) ) {
		TouchOrUse(pCaller);
	}
}

void CWeaponBox::TouchOrUse( CBaseEntity *pOther )
{
	if( pev->movetype == MOVETYPE_TOSS && !( pev->flags & FL_ONGROUND ) )
	{
		return;
	}

	if( !pOther->IsPlayer() )
	{
		// only players may touch a weaponbox.
		return;
	}

	if( !pOther->IsAlive() )
	{
		// no dead guys.
		return;
	}

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	int i;

	bool shouldRemove = false;

	// go through my weapons and try to give the usable ones to the player. 
	// it's important the the player be given ammo first, so the weapons code doesn't refuse 
	// to deploy a better weapon that the player may pick up because he has no ammo for it.

	// dole out ammo
	for( i = 0; i < MAX_AMMO_TYPES; i++ )
	{
		if( !FStringNull( m_rgiszAmmo[i] ) )
		{
			// there's some ammo of this type. 
			if (pPlayer->GiveAmmo( m_rgAmmo[i], STRING( m_rgiszAmmo[i] ) ) != -1) {
				//ALERT( at_console, "Gave %d rounds of %s\n", m_rgAmmo[i], STRING( m_rgiszAmmo[i] ) );
				shouldRemove = true;
				// now empty the ammo from the weaponbox since we just gave it to the player
				m_rgiszAmmo[i] = iStringNull;
				m_rgAmmo[i] = 0;
			}
		}
	}

	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		CBasePlayerWeapon *pItem = m_rgpPlayerWeapons[i];

		// have at least one weapon in this slot
		if( pItem )
		{
			//ALERT( at_console, "trying to give %s\n", STRING( m_rgpPlayerItems[i]->pev->classname ) );

			m_rgpPlayerWeapons[i] = NULL;// unlink this weapon from the box

			if( pPlayer->AddPlayerItem( pItem ) > DID_NOT_GET_ITEM )
			{
				shouldRemove = true;
			}
		}
	}

	if (shouldRemove) {
		pOther->EmitSoundScript(GetSoundScript(Items::weaponPickupSoundScript));
		SetTouch( NULL );
		SUB_UseTargets( pOther );
		UTIL_Remove(this);
	}
}

//=========================================================
// CWeaponBox - PackWeapon: Add this weapon to the box
//=========================================================
BOOL CWeaponBox::PackWeapon( CBasePlayerWeapon *pWeapon )
{
	// is one of these weapons already packed in this box?
	if( HasWeapon( pWeapon ) )
	{
		return FALSE;// box can only hold one of each weapon type
	}

	if( pWeapon->m_pPlayer )
	{
		if( !pWeapon->m_pPlayer->RemovePlayerItem( pWeapon, true ) )
		{
			// failed to unhook the weapon from the player!
			return FALSE;
		}
	}

	InsertWeaponById(pWeapon);

	pWeapon->pev->spawnflags |= SF_NORESPAWN;// never respawn
	pWeapon->pev->movetype = MOVETYPE_NONE;
	pWeapon->pev->solid = SOLID_NOT;
	pWeapon->pev->effects = EF_NODRAW;
	pWeapon->pev->modelindex = 0;
	pWeapon->pev->model = iStringNull;
	pWeapon->pev->owner = edict();
	pWeapon->SetThink( NULL );// crowbar may be trying to swing again, etc.
	pWeapon->SetTouch( NULL );
	pWeapon->m_pPlayer = NULL;

	//ALERT( at_console, "packed %s\n", STRING( pWeapon->pev->classname ) );

	return TRUE;
}

//=========================================================
// CWeaponBox - PackAmmo
//=========================================================
BOOL CWeaponBox::PackAmmo( string_t iszName, int iCount )
{
	if( FStringNull( iszName ) )
	{
		// error here
		ALERT( at_console, "NULL String in PackAmmo!\n" );
		return FALSE;
	}

	const AmmoType* ammoType = CBasePlayerWeapon::GetAmmoType(STRING(iszName));
	if( ammoType && iCount > 0 )
	{
		//ALERT( at_console, "Packed %d rounds of %s\n", iCount, STRING( iszName ) );
		int i;

		for( i = 1; i < MAX_AMMO_TYPES && !FStringNull( m_rgiszAmmo[i] ); i++ )
		{
			if( stricmp( ammoType->name, STRING( m_rgiszAmmo[i] ) ) == 0 )
			{
				int iAdd = Q_min( iCount, ammoType->maxAmmo - m_rgAmmo[i] );
				if( iCount == 0 || iAdd > 0 )
				{
					m_rgAmmo[i] += iAdd;

					return TRUE;
				}
				return FALSE;
			}
		}
		if( i < MAX_AMMO_TYPES )
		{
			m_rgiszAmmo[i] = MAKE_STRING( ammoType->name );
			m_rgAmmo[i] = iCount;

			return TRUE;
		}
		ALERT( at_console, "out of named ammo slots\n" );
		return FALSE;
	}

	return FALSE;
}

//=========================================================
// CWeaponBox::HasWeapon - is a weapon of this type already
// packed in this box?
//=========================================================
BOOL CWeaponBox::HasWeapon( CBasePlayerWeapon *pCheckItem )
{
	return WeaponById(pCheckItem->m_iId) ? TRUE : FALSE;
}

//=========================================================
// CWeaponBox::IsEmpty - is there anything in this box?
//=========================================================
BOOL CWeaponBox::IsEmpty( void )
{
	int i;

	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		if( m_rgpPlayerWeapons[i] )
		{
			return FALSE;
		}
	}

	for( i = 0; i < MAX_AMMO_TYPES; i++ )
	{
		if( !FStringNull( m_rgiszAmmo[i] ) )
		{
			// still have a bit of this type of ammo
			return FALSE;
		}
	}

	return TRUE;
}

void CWeaponBox::SetWeaponModel(CBasePlayerWeapon *pItem)
{
	if (pItem && pItem->MyWModel())
	{
		Vector weaponAngles = pev->angles;
		weaponAngles.y += 180 + RANDOM_LONG(-15,15);

		SET_MODEL( ENT( pev ), pItem->MyWModel() );
		pev->angles = weaponAngles;
		if (pItem->m_iId == WEAPON_TRIPMINE) {
			pev->body = 3;
			pev->sequence = 8;
		}
	}
}

//=========================================================
//=========================================================
void CWeaponBox::SetObjectCollisionBox( void )
{
	pev->absmin = pev->origin + Vector( -16, -16, 0 );
	pev->absmax = pev->origin + Vector( 16, 16, 16 ); 
}

void CWeaponBox::InsertWeaponById(CBasePlayerWeapon *pItem)
{
	if (pItem && pItem->m_iId && pItem->m_iId <= MAX_WEAPONS) {
		m_rgpPlayerWeapons[pItem->m_iId-1] = pItem;
	}
}

CBasePlayerWeapon* CWeaponBox::WeaponById(int id)
{
	if (id && id <= MAX_WEAPONS) {
		return m_rgpPlayerWeapons[id-1];
	}
	return NULL;
}

void CBasePlayerWeapon::PrintState( void )
{
	ALERT( at_console, "primary:  %f\n", (double)m_flNextPrimaryAttack );
	ALERT( at_console, "idle   :  %f\n", (double)m_flTimeWeaponIdle );

	//ALERT( at_console, "nextrl :  %f\n", m_flNextReload );
	//ALERT( at_console, "nextpum:  %f\n", m_flPumpTime );

	//ALERT( at_console, "m_frt  :  %f\n", m_fReloadTime );
	ALERT( at_console, "m_finre:  %i\n", m_fInReload );
	//ALERT( at_console, "m_finsr:  %i\n", m_fInSpecialReload );

	ALERT( at_console, "m_iclip:  %i\n", m_iClip );
}

TYPEDESCRIPTION	CRpg::m_SaveData[] =
{
	DEFINE_FIELD( CRpg, m_fSpotActive, FIELD_INTEGER ),
	DEFINE_FIELD( CRpg, m_cActiveRockets, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CRpg, CBasePlayerWeapon )

TYPEDESCRIPTION	CRpgRocket::m_SaveData[] =
{
	DEFINE_FIELD( CRpgRocket, m_flIgniteTime, FIELD_TIME ),
	DEFINE_FIELD( CRpgRocket, m_hLauncher, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CRpgRocket, CGrenade )

TYPEDESCRIPTION	CShotgun::m_SaveData[] =
{
	DEFINE_FIELD( CShotgun, m_flNextReload, FIELD_TIME ),
	DEFINE_FIELD( CShotgun, m_fInSpecialReload, FIELD_INTEGER ),
	// DEFINE_FIELD( CShotgun, m_iShell, FIELD_INTEGER ),
	DEFINE_FIELD( CShotgun, m_flPumpTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CShotgun, CBasePlayerWeapon )

TYPEDESCRIPTION	CGauss::m_SaveData[] =
{
	DEFINE_FIELD( CGauss, m_fInAttack, FIELD_INTEGER ),
	//DEFINE_FIELD( CGauss, m_flStartCharge, FIELD_TIME ),
	//DEFINE_FIELD( CGauss, m_flPlayAftershock, FIELD_TIME ),
	//DEFINE_FIELD( CGauss, m_flNextAmmoBurn, FIELD_TIME ),
	DEFINE_FIELD( CGauss, m_fPrimaryFire, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CGauss, CBasePlayerWeapon )

TYPEDESCRIPTION	CEgon::m_SaveData[] =
{
	//DEFINE_FIELD( CEgon, m_pBeam, FIELD_CLASSPTR ),
	//DEFINE_FIELD( CEgon, m_pNoise, FIELD_CLASSPTR ),
	//DEFINE_FIELD( CEgon, m_pSprite, FIELD_CLASSPTR ),
	DEFINE_FIELD( CEgon, m_shootTime, FIELD_TIME ),
	DEFINE_FIELD( CEgon, m_fireState, FIELD_INTEGER ),
	DEFINE_FIELD( CEgon, m_fireMode, FIELD_INTEGER ),
	DEFINE_FIELD( CEgon, m_shakeTime, FIELD_TIME ),
	DEFINE_FIELD( CEgon, m_flAmmoUseTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CEgon, CBasePlayerWeapon )

TYPEDESCRIPTION CHgun::m_SaveData[] =
{
	DEFINE_FIELD( CHgun, m_flRechargeTime, FIELD_TIME ),
	DEFINE_FIELD( CHgun, m_iFirePhase, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CHgun, CBasePlayerWeapon )

TYPEDESCRIPTION	CSatchel::m_SaveData[] = 
{
	DEFINE_FIELD( CSatchel, m_chargeReady, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CSatchel, CBasePlayerWeapon )

#if FEATURE_DESERT_EAGLE
TYPEDESCRIPTION CEagle::m_SaveData[] =
{
	DEFINE_FIELD( CEagle, m_fEagleLaserActive, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CEagle, CBasePlayerWeapon )
#endif

#if FEATURE_PIPEWRENCH
TYPEDESCRIPTION	CPipeWrench::m_SaveData[] =
{
	DEFINE_FIELD( CPipeWrench, m_flBigSwingStart, FIELD_TIME ),
	DEFINE_FIELD( CPipeWrench, m_iSwing, FIELD_INTEGER ),
	DEFINE_FIELD( CPipeWrench, m_iSwingMode, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CPipeWrench, CBasePlayerWeapon )
#endif

#if FEATURE_KNIFE
TYPEDESCRIPTION	CKnife::m_SaveData[] =
{
	DEFINE_FIELD( CKnife, m_flStabStart, FIELD_TIME ),
	DEFINE_FIELD( CKnife, m_iSwing, FIELD_INTEGER ),
	DEFINE_FIELD( CKnife, m_iSwingMode, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CKnife, CBasePlayerWeapon )
#endif

#if FEATURE_M249
TYPEDESCRIPTION	CM249::m_SaveData[] =
{
	DEFINE_FIELD( CM249, m_fInSpecialReload, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CM249, CBasePlayerWeapon )
#endif

#if FEATURE_SNIPERRIFLE
TYPEDESCRIPTION	CSniperrifle::m_SaveData[] =
{
	DEFINE_FIELD( CSniperrifle, m_fInSpecialReload, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CSniperrifle, CBasePlayerWeapon )
#endif

#if FEATURE_DISPLACER
TYPEDESCRIPTION	CDisplacer::m_SaveData[] =
{
	DEFINE_FIELD( CDisplacer, m_iFireMode, FIELD_INTEGER ),
	DEFINE_ARRAY( CDisplacer, m_pBeam, FIELD_CLASSPTR, 3 ),
};
IMPLEMENT_SAVERESTORE( CDisplacer, CBasePlayerWeapon )
#endif

#if FEATURE_GRAPPLE
TYPEDESCRIPTION	CBarnacleGrapple::m_SaveData[] =
{
	DEFINE_FIELD( CBarnacleGrapple, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBarnacleGrapple, m_flShootTime, FIELD_TIME ),
	DEFINE_FIELD( CBarnacleGrapple, m_fireState, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CBarnacleGrapple, CBasePlayerWeapon )
#endif

#if FEATURE_MEDKIT
TYPEDESCRIPTION	CMedkit::m_SaveData[] =
{
	DEFINE_FIELD( CMedkit, m_flSoundDelay, FIELD_TIME ),
	DEFINE_FIELD( CMedkit, m_flRechargeTime, FIELD_TIME ),
	DEFINE_FIELD( CMedkit, m_secondaryAttack, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CMedkit, CBasePlayerWeapon )
#endif
