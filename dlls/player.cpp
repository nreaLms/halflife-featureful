﻿/***
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

===== player.cpp ========================================================

  functions dealing with the player

*/

#include "extdll.h"
#include "util.h"

#include "cbase.h"
#include "player.h"
#include "trains.h"
#include "nodes.h"
#include "items.h"
#include "weapons.h"
#include "soundent.h"
#include "monsters.h"
#include "shake.h"
#include "decals.h"
#include "gamerules.h"
#include "game.h"
#include "pm_shared.h"
#include "hltv.h"
#include "talkmonster.h"
#include "color_utils.h"
#include "inventory.h"
#include "followers.h"
#include "common_soundscripts.h"
#include "error_collector.h"

#if FEATURE_ROPE
#include "ropes.h"
#endif

// #define DUCKFIX

extern DLL_GLOBAL ULONG g_ulModelIndexPlayer;
extern DLL_GLOBAL BOOL g_fGameOver;
extern DLL_GLOBAL BOOL g_fDrawLines;
int gEvilImpulse101;
extern DLL_GLOBAL int g_iSkillLevel, gDisplayTitle;

BOOL gInitHUD = TRUE;

extern void CopyToBodyQue( entvars_t *pev);
extern void respawn( entvars_t *pev, BOOL fCopyCorpse );
extern edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer );

#define TRAIN_ACTIVE		0x80
#define TRAIN_NEW		0xc0
#define TRAIN_OFF		0x00
#define TRAIN_NEUTRAL		0x01
#define TRAIN_SLOW		0x02
#define TRAIN_MEDIUM		0x03
#define TRAIN_FAST		0x04
#define TRAIN_BACK		0x05

// Global Savedata for player
TYPEDESCRIPTION	CBasePlayer::m_playerSaveData[] =
{
	DEFINE_FIELD( CBasePlayer, m_flFlashLightTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_iFlashBattery, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_afButtonLast, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonPressed, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonReleased, FIELD_INTEGER ),

	DEFINE_ARRAY( CBasePlayer, m_rgItems, FIELD_INTEGER, MAX_ITEMS ),
	DEFINE_FIELD( CBasePlayer, m_afPhysicsFlags, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_flTimeStepSound, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flTimeWeaponIdle, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flSwimTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flDuckTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flWallJumpTime, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_flSuitUpdate, FIELD_TIME ),
	DEFINE_ARRAY( CBasePlayer, m_rgSuitPlayList, FIELD_INTEGER, CSUITPLAYLIST ),
	DEFINE_FIELD( CBasePlayer, m_iSuitPlayNext, FIELD_INTEGER ),
	DEFINE_ARRAY( CBasePlayer, m_rgiSuitNoRepeat, FIELD_INTEGER, CSUITNOREPEAT ),
	DEFINE_ARRAY( CBasePlayer, m_rgflSuitNoRepeatTime, FIELD_TIME, CSUITNOREPEAT ),
	DEFINE_FIELD( CBasePlayer, m_lastDamageAmount, FIELD_INTEGER ),

	DEFINE_ARRAY( CBasePlayer, m_rgpPlayerWeapons, FIELD_CLASSPTR, MAX_WEAPONS ),
	DEFINE_FIELD( CBasePlayer, m_pActiveItem, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBasePlayer, m_pLastItem, FIELD_CLASSPTR ),

	DEFINE_ARRAY( CBasePlayer, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_TYPES ),
	DEFINE_FIELD( CBasePlayer, m_idrowndmg, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_idrownrestored, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_iTrain, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_bitsHUDDamage, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_flFallVelocity, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_iTargetVolume, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iWeaponVolume, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iExtraSoundTypes, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iWeaponFlash, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_fLongJump, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_fInitHUD, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_tbdPrev, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_pTank, FIELD_EHANDLE ),
	DEFINE_FIELD( CBasePlayer, m_hViewEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( CBasePlayer, m_iHideHUD, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iFOV, FIELD_INTEGER ),
#if FEATURE_DISPLACER
	DEFINE_FIELD(CBasePlayer, m_fInXen, FIELD_BOOLEAN),
#endif
#if FEATURE_NIGHTVISION
	DEFINE_FIELD(CBasePlayer, m_fNVGisON, FIELD_BOOLEAN),
#endif
#if FEATURE_ROPE
	DEFINE_FIELD(CBasePlayer, m_pRope, FIELD_CLASSPTR),
#endif
	DEFINE_FIELD(CBasePlayer, m_iItemsBits, FIELD_INTEGER),
	DEFINE_ARRAY(CBasePlayer, m_timeBasedDmgModifiers, FIELD_CHARACTER, CDMG_TIMEBASED),
	DEFINE_FIELD(CBasePlayer, m_settingsLoaded, FIELD_BOOLEAN),
	DEFINE_FIELD(CBasePlayer, m_buddha, FIELD_BOOLEAN),
	DEFINE_FIELD(CBasePlayer, m_suppressedCapabilities, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_maxSpeedFraction, FIELD_FLOAT),
	DEFINE_FIELD(CBasePlayer, m_armorStrength, FIELD_FLOAT),

	DEFINE_FIELD(CBasePlayer, m_loopedMp3, FIELD_STRING),

	DEFINE_ARRAY(CBasePlayer, m_inventoryItems, FIELD_STRING, MAX_INVENTORY_ITEMS),
	DEFINE_ARRAY(CBasePlayer, m_inventoryItemCounts, FIELD_SHORT, MAX_INVENTORY_ITEMS),

	DEFINE_FIELD(CBasePlayer, m_camera, FIELD_EHANDLE),
	DEFINE_FIELD(CBasePlayer, m_cameraFlags, FIELD_INTEGER),

	//DEFINE_FIELD( CBasePlayer, m_fDeadTime, FIELD_FLOAT ), // only used in multiplayer games
	//DEFINE_FIELD( CBasePlayer, m_fGameHUDInitialized, FIELD_INTEGER ), // only used in multiplayer games
	//DEFINE_FIELD( CBasePlayer, m_flStopExtraSoundTime, FIELD_TIME ),
	//DEFINE_FIELD( CBasePlayer, m_fKnownItem, FIELD_INTEGER ), // reset to zero on load
	//DEFINE_FIELD( CBasePlayer, m_iPlayerSound, FIELD_INTEGER ),	// Don't restore, set in Precache()
	DEFINE_FIELD( CBasePlayer, m_pentSndLast, FIELD_EDICT ),
	DEFINE_FIELD( CBasePlayer, m_SndRoomtype, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_flSndRange, FIELD_FLOAT ),
	//DEFINE_FIELD( CBasePlayer, m_fNewAmmo, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_flgeigerRange, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_flgeigerDelay, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_igeigerRangePrev, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_iStepLeft, FIELD_INTEGER ), // Don't need to restore
	//DEFINE_ARRAY( CBasePlayer, m_szTextureName, FIELD_CHARACTER, CBTEXTURENAMEMAX ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_chTextureType, FIELD_CHARACTER ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_fNoPlayerSound, FIELD_BOOLEAN ), // Don't need to restore, debug
	//DEFINE_FIELD( CBasePlayer, m_iUpdateTime, FIELD_INTEGER ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_iClientHealth, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_iClientBattery, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_iClientHideHUD, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_fWeapon, FIELD_BOOLEAN ),  // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_nCustomSprayFrames, FIELD_INTEGER ), // Don't restore, depends on server message after spawning and only matters in multiplayer
	//DEFINE_FIELD( CBasePlayer, m_vecAutoAim, FIELD_VECTOR ), // Don't save/restore - this is recomputed
	//DEFINE_ARRAY( CBasePlayer, m_rgAmmoLast, FIELD_INTEGER, MAX_AMMO_SLOTS ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_fOnTarget, FIELD_BOOLEAN ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_nCustomSprayFrames, FIELD_INTEGER ), // Don't need to restore
};	

int giPrecacheGrunt = 0;
int gmsgShake = 0;
int gmsgFade = 0;
int gmsgSelAmmo = 0;
int gmsgFlashlight = 0;
int gmsgFlashBattery = 0;
int gmsgResetHUD = 0;
int gmsgInitHUD = 0;
int gmsgSetFog = 0;
int gmsgKeyedDLight = 0;
int gmsgShowGameTitle = 0;
int gmsgCurWeapon = 0;
int gmsgHealth = 0;
int gmsgDamage = 0;
int gmsgBattery = 0;
int gmsgTrain = 0;
int gmsgLogo = 0;
int gmsgAmmoList = 0;
int gmsgWeaponList = 0;
int gmsgAmmoX = 0;
int gmsgHudText = 0;
int gmsgDeathMsg = 0;
int gmsgScoreInfo = 0;
int gmsgTeamInfo = 0;
int gmsgTeamScore = 0;
int gmsgGameMode = 0;
int gmsgMOTD = 0;
int gmsgParseErrors = 0;
int gmsgServerName = 0;
int gmsgAmmoPickup = 0;
int gmsgWeapPickup = 0;
int gmsgItemPickup = 0;
int gmsgHideWeapon = 0;
int gmsgSetCurWeap = 0;
int gmsgSayText = 0;
int gmsgTextMsg = 0;
int gmsgSetFOV = 0;
int gmsgShowMenu = 0;
int gmsgGeigerRange = 0;
int gmsgTeamNames = 0;
int gmsgPlayMP3 = 0;
int gmsgItems = 0;

int gmsgStatusText = 0;
int gmsgStatusValue = 0;
int gmsgStatusIcon = 0;

int gmsgRandomGibs = 0;
int gmsgMuzzleLight = 0;
int gmsgCustomBeam = 0;
int gmsgSprite = 0;
int gmsgSpriteTrail = 0;
int gmsgSpray = 0;
int gmsgStreaks = 0;
int gmsgSmoke = 0;
int gmsgSparkShower = 0;
int gmsgParticleShooter = 0;

#if FEATURE_NIGHTVISION
int gmsgNightvision = 0;
#endif

int gmsgMovementState = 0;

int gmsgUseSound = 0;

int gmsgCaption = 0;

int gmsgInventory = 0;

int gmsgRain = 0;
int gmsgSnow = 0;

static CFollowingMonster* CanRecruit(CBaseEntity* pFriend, CBasePlayer* player)
{
	if (!pFriend->IsFullyAlive())
		return NULL;
	CBaseMonster *pMonster = pFriend->MyMonsterPointer();
	if( !pMonster || pMonster->m_MonsterState == MONSTERSTATE_SCRIPT || pMonster->m_MonsterState == MONSTERSTATE_PRONE )
		return NULL;
	const int rel = pMonster->IRelationship(player);
	if ( rel >= R_DL || rel == R_FR ) {
		return NULL;
	}
	CFollowingMonster* pFollowingMonster = pMonster->MyFollowingMonsterPointer();
	if (pFollowingMonster)
	{
		if (pFollowingMonster->ShouldDeclineFollowing())
			return NULL;
	}
	return pFollowingMonster;
}

void LinkUserMessages( void )
{
	// Already taken care of?
	if( gmsgSelAmmo )
	{
		return;
	}

	gmsgSelAmmo = REG_USER_MSG( "SelAmmo", sizeof(SelAmmo) );
	gmsgCurWeapon = REG_USER_MSG( "CurWeapon", 4 );
	gmsgGeigerRange = REG_USER_MSG( "Geiger", 1 );
	gmsgFlashlight = REG_USER_MSG( "Flashlight", 2 );
	gmsgFlashBattery = REG_USER_MSG( "FlashBat", 1 );
	gmsgHealth = REG_USER_MSG( "Health", 4 );
	gmsgDamage = REG_USER_MSG( "Damage", 12 );
	gmsgBattery = REG_USER_MSG( "Battery", 4);
	gmsgTrain = REG_USER_MSG( "Train", 1 );
	//gmsgHudText = REG_USER_MSG( "HudTextPro", -1 );
	gmsgHudText = REG_USER_MSG( "HudText", -1 ); // we don't use the message but 3rd party addons may!
	gmsgSayText = REG_USER_MSG( "SayText", -1 );
	gmsgTextMsg = REG_USER_MSG( "TextMsg", -1 );
	gmsgAmmoList = REG_USER_MSG( "AmmoList", -1 );
	gmsgWeaponList = REG_USER_MSG( "WeaponList", -1 );
	gmsgResetHUD = REG_USER_MSG( "ResetHUD", 1 );		// called every respawn
	gmsgInitHUD = REG_USER_MSG( "InitHUD", 0 );		// called every time a new player joins the server

	gmsgSetFog = REG_USER_MSG("SetFog", 15 );
	gmsgKeyedDLight = REG_USER_MSG("KeyedDLight", -1 );

	gmsgShowGameTitle = REG_USER_MSG( "GameTitle", 1 );
	gmsgDeathMsg = REG_USER_MSG( "DeathMsg", -1 );
	gmsgScoreInfo = REG_USER_MSG( "ScoreInfo", 9 );
	gmsgTeamInfo = REG_USER_MSG( "TeamInfo", -1 );  // sets the name of a player's team
	gmsgTeamScore = REG_USER_MSG( "TeamScore", -1 );  // sets the score of a team on the scoreboard
	gmsgGameMode = REG_USER_MSG( "GameMode", 1 );
	gmsgMOTD = REG_USER_MSG( "MOTD", -1 );
	gmsgParseErrors = REG_USER_MSG( "ParseErrors", -1 );
	gmsgServerName = REG_USER_MSG( "ServerName", -1 );
	gmsgAmmoPickup = REG_USER_MSG( "AmmoPickup", 3 );
	gmsgWeapPickup = REG_USER_MSG( "WeapPickup", 1 );
	gmsgItemPickup = REG_USER_MSG( "ItemPickup", -1 );
	gmsgHideWeapon = REG_USER_MSG( "HideWeapon", 1 );
	gmsgSetFOV = REG_USER_MSG( "SetFOV", 1 );
	gmsgShowMenu = REG_USER_MSG( "ShowMenu", -1 );
	gmsgShake = REG_USER_MSG( "ScreenShake", sizeof(ScreenShake) );
	gmsgFade = REG_USER_MSG( "ScreenFade", sizeof(ScreenFade) );
	gmsgAmmoX = REG_USER_MSG( "AmmoX", 3 );
	gmsgTeamNames = REG_USER_MSG( "TeamNames", -1 );
	gmsgPlayMP3 = REG_USER_MSG( "PlayMP3", -1 );
	gmsgItems = REG_USER_MSG( "Items", 4 );

	gmsgStatusText = REG_USER_MSG( "StatusText", -1 );
	gmsgStatusValue = REG_USER_MSG( "StatusValue", 3 );
	gmsgStatusIcon = REG_USER_MSG( "StatusIcon", -1 );

	gmsgRandomGibs = REG_USER_MSG( "RandomGibs", 27 );
	gmsgMuzzleLight = REG_USER_MSG( "MuzzleLight", 6 );
	gmsgCustomBeam = REG_USER_MSG( "CustomBeam", -1 );
	gmsgSprite = REG_USER_MSG( "Sprite", 18 );
	gmsgSpriteTrail = REG_USER_MSG( "SpriteTrail", 26 );
	gmsgSpray = REG_USER_MSG( "Spray", 27 );
	gmsgStreaks = REG_USER_MSG( "Streaks", 23 );
	gmsgSmoke = REG_USER_MSG( "Smoke", -1 );
	gmsgSparkShower = REG_USER_MSG( "SparkShower", 20 );
	gmsgParticleShooter = REG_USER_MSG( "Particle", 27 );

#if FEATURE_NIGHTVISION
	gmsgNightvision = REG_USER_MSG( "Nightvision", 1 );
#endif
	gmsgMovementState = REG_USER_MSG( "MoveMode", 2 );
	gmsgUseSound = REG_USER_MSG( "UseSound", 1 );

	gmsgCaption = REG_USER_MSG("Caption", -1);

	gmsgInventory = REG_USER_MSG("Inventory", -1);

	gmsgRain = REG_USER_MSG("Rain", -1);
	gmsgSnow = REG_USER_MSG("Snow", -1);
}

LINK_ENTITY_TO_CLASS( player, CBasePlayer )

void CBasePlayer::Pain( void )
{
	float flRndSound;//sound randomizer

	flRndSound = RANDOM_FLOAT( 0.0f, 1.0f ); 

	if( flRndSound <= 0.33f )
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_pain5.wav", 1, ATTN_NORM );
	else if( flRndSound <= 0.66f )	
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_pain6.wav", 1, ATTN_NORM );
	else
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_pain7.wav", 1, ATTN_NORM );
}

Vector VecVelocityForDamage( float flDamage )
{
	Vector vec( RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( 200, 300 ) );

	if( flDamage > -50 )
		vec = vec * 0.7f;
	else if( flDamage > -200 )
		vec = vec * 2;
	else
		vec = vec * 10;

	return vec;
}

#if 0 
static void ThrowGib( entvars_t *pev, char *szGibModel, float flDamage )
{
	edict_t *pentNew = CREATE_ENTITY();
	entvars_t *pevNew = VARS( pentNew );

	pevNew->origin = pev->origin;
	SET_MODEL( ENT( pevNew ), szGibModel );
	UTIL_SetSize( pevNew, g_vecZero, g_vecZero );

	pevNew->velocity = VecVelocityForDamage( flDamage );
	pevNew->movetype = MOVETYPE_BOUNCE;
	pevNew->solid = SOLID_NOT;
	pevNew->avelocity.x = RANDOM_FLOAT( 0, 600 );
	pevNew->avelocity.y = RANDOM_FLOAT( 0, 600 );
	pevNew->avelocity.z = RANDOM_FLOAT( 0, 600 );
	CHANGE_METHOD( ENT( pevNew ), em_think, SUB_Remove );
	pevNew->ltime = gpGlobals->time;
	pevNew->nextthink = gpGlobals->time + RANDOM_FLOAT( 10, 20 );
	pevNew->frame = 0;
	pevNew->flags = 0;
}

static void ThrowHead( entvars_t *pev, char *szGibModel, floatflDamage )
{
	SET_MODEL( ENT( pev ), szGibModel );
	pev->frame = 0;
	pev->nextthink = -1;
	pev->movetype = MOVETYPE_BOUNCE;
	pev->takedamage = DAMAGE_NO;
	pev->solid = SOLID_NOT;
	pev->view_ofs = Vector( 0, 0, 8 );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 56 ) );
	pev->velocity = VecVelocityForDamage( flDamage );
	pev->avelocity = RANDOM_FLOAT( -1, 1 ) * Vector( 0, 600, 0 );
	pev->origin.z -= 24;
	ClearBits( pev->flags, FL_ONGROUND );
}
#endif

int TrainSpeed( int iSpeed, int iMax )
{
	float fSpeed, fMax;
	int iRet = 0;

	fMax = (float)iMax;
	fSpeed = iSpeed;

	fSpeed = fSpeed / fMax;

	if( iSpeed < 0 )
		iRet = TRAIN_BACK;
	else if( iSpeed == 0.0f )
		iRet = TRAIN_NEUTRAL;
	else if( fSpeed < 0.33f )
		iRet = TRAIN_SLOW;
	else if( fSpeed < 0.66f )
		iRet = TRAIN_MEDIUM;
	else
		iRet = TRAIN_FAST;

	return iRet;
}

void CBasePlayer::DeathSound( void )
{
	// water death sounds
	/*
	if( pev->waterlevel == 3 )
	{
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/h2odeath.wav", 1, ATTN_NONE );
		return;
	}
	*/

	// temporarily using pain sounds for death sounds
	switch( RANDOM_LONG( 1, 5 ) )
	{
	case 1: 
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_pain5.wav", 1, ATTN_NORM );
		break;
	case 2: 
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_pain6.wav", 1, ATTN_NORM );
		break;
	case 3: 
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_pain7.wav", 1, ATTN_NORM );
		break;
	}

	// play one of the suit death alarms
	EMIT_GROUPNAME_SUIT( ENT( pev ), "HEV_DEAD" );
}

// override takehealth
// bitsDamageType indicates type of damage healed. 
int CBasePlayer::TakeHealth( CBaseEntity* pHealer, float flHealth, int bitsDamageType )
{
	const int healed = CBaseMonster::TakeHealth(pHealer, (int)flHealth, bitsDamageType);
#if FEATURE_MEDKIT
	CBasePlayerWeapon* pPlayerMedkit = WeaponById(WEAPON_MEDKIT);
	if ((bitsDamageType & HEAL_CHARGE) != 0 && pPlayerMedkit) {
		const int rest = (int)flHealth - healed;
		if (rest > 0) {
			const int medAmmoIndex = GetAmmoIndex(pPlayerMedkit->pszAmmo1());
			const int medAmmo = AmmoInventory(medAmmoIndex);
			if (medAmmo >= 0 && medAmmo < pPlayerMedkit->iMaxAmmo1()) {
				const int toAdd = Q_min(rest, pPlayerMedkit->iMaxAmmo1() - medAmmo);
				m_rgAmmo[medAmmoIndex] += toAdd;

				if (flHealth > 1)
				{
					MESSAGE_BEGIN( MSG_ONE, gmsgAmmoPickup, NULL, pev );
						WRITE_BYTE( medAmmoIndex );		// ammo ID
						WRITE_SHORT( toAdd );		// amount
					MESSAGE_END();

					if (healed == 0) {
						EmitSoundScript(Items::ammoPickupSoundScript);
					}
				}

				return healed + toAdd;
			}
		}
	}
#endif
	return healed;
}

void CBasePlayer::SetHealth(int health, bool allowOverheal)
{
	pev->health = health;
	if (pev->health > pev->max_health && !allowOverheal)
	{
		pev->health = pev->max_health;
	}
}

void CBasePlayer::SetMaxHealth(int maxHealth, bool clampValue)
{
	pev->max_health = maxHealth;
	if (clampValue && pev->health > pev->max_health)
	{
		pev->health = pev->max_health;
	}
}

int CBasePlayer::TakeArmor(CBaseEntity *pCharger, float flArmor, int flags)
{
	if (!flArmor)
		return false;
	const int maxArmor = MaxArmor();
	const bool allowOvercharge = FBitSet(flags, GIVEARMOR_ALLOW_OVERFLOW);

	if (flArmor > 0 && pev->armorvalue >= maxArmor && !allowOvercharge)
		return false;
	pev->armorvalue += flArmor;
	if (flArmor > 0 && !allowOvercharge)
	{
		pev->armorvalue = Q_min( pev->armorvalue, maxArmor );
	}
	if (pev->armorvalue < 0)
		pev->armorvalue = 0;
	return true;
}

int CBasePlayer::MaxArmor()
{
	if (pev->armortype > 0)
	{
		return pev->armortype;
	}
	return g_modFeatures.MaxPlayerArmor();
}

void CBasePlayer::SetMaxArmor(int maxArmor, bool clampValue)
{
	pev->armortype = maxArmor;
	if (clampValue && pev->armorvalue > MaxArmor())
	{
		pev->armorvalue = MaxArmor();
	}
}

void CBasePlayer::SetArmor(int armor, bool allowOvercharge)
{
	pev->armorvalue = armor;
	const int maxArmor = MaxArmor();
	if (pev->armorvalue > maxArmor && !allowOvercharge)
	{
		pev->armorvalue = maxArmor;
	}
	if (pev->armorvalue < 0)
		pev->armorvalue = 0;
}

float CBasePlayer::ArmorStrength()
{
	if (m_armorStrength > 0)
		return m_armorStrength;
	return gSkillData.plrArmorStrength;
}

Vector CBasePlayer::GetGunPosition()
{
	//UTIL_MakeVectors( pev->v_angle );
	//m_HackedGunPos = pev->view_ofs;
	Vector origin;

	origin = pev->origin + pev->view_ofs;

	return origin;
}

bool CBasePlayer::IsInvulnerable()
{
	if (m_camera != 0 && FBitSet(m_cameraFlags, PLAYER_CAMERA_INVULNERABLE))
		return true;

	return false;
}

//=========================================================
// TraceAttack
//=========================================================
void CBasePlayer::TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if (IsInvulnerable())
		return;

	if( pev->takedamage )
	{
		m_LastHitGroup = ptr->iHitgroup;

		switch( ptr->iHitgroup )
		{
		case HITGROUP_GENERIC:
			break;
		case HITGROUP_HEAD:
			flDamage *= gSkillData.plrHead;
			break;
		case HITGROUP_CHEST:
			flDamage *= gSkillData.plrChest;
			break;
		case HITGROUP_STOMACH:
			flDamage *= gSkillData.plrStomach;
			break;
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			flDamage *= gSkillData.plrArm;
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			flDamage *= gSkillData.plrLeg;
			break;
		default:
			break;
		}

		SpawnBlood( ptr->vecEndPos, BloodColor(), flDamage );// a little surface blood.
		TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
		AddMultiDamage( pevInflictor, pevAttacker, this, flDamage, bitsDamageType );
	}
}

/*
	Take some damage.  
	NOTE: each call to TakeDamage with bitsDamageType set to a time-based damage
	type will cause the damage time countdown to be reset.  Thus the ongoing effects of poison, radiation
	etc are implemented with subsequent calls to TakeDamage using DMG_GENERIC.
*/

static BYTE TBDModFromDmgType(int bitsDamageType)
{
	BYTE result = 0;
	if (bitsDamageType & DMG_TIMEDNONLETHAL)
	{
		result |= DMG_TIMED_MOD_NONLETHAL;
	}
	if (bitsDamageType & DMG_IGNORE_ARMOR)
	{
		result |= DMG_TIMED_MOD_IGNORE_ARMOR;
	}
	return result;
}

static int DmgTypeFromTBDMod(BYTE timeBasedModifier)
{
	int result = 0;
	if (timeBasedModifier & DMG_TIMED_MOD_NONLETHAL)
	{
		result |= DMG_NONLETHAL;
	}
	if (timeBasedModifier & DMG_TIMED_MOD_IGNORE_ARMOR)
	{
		result |= DMG_IGNORE_ARMOR;
	}
	return result;
}

int CBasePlayer::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// have suit diagnose the problem - ie: report damage type
	int bitsDamage = bitsDamageType;
	int ffound = TRUE;
	int fmajor;
	int fcritical;
	int fTookDamage;
	int ftrivial;
	float flRatio;
	float flArmorStrength;
	float flHealthPrev = pev->health;

	flArmorStrength = ArmorStrength();
	flRatio = ARMOR_RATIO;

	if( ( bitsDamageType & DMG_BLAST ) && g_pGameRules->IsMultiplayer() )
	{
		// blasts damage armor more.
		flArmorStrength = Q_min(flArmorStrength, 1.0f);
	}

	// Already dead
	if( !IsAlive() )
		return 0;

	if (IsInvulnerable())
		return 0;

	// go take the damage first
	CBaseEntity *pAttacker = CBaseEntity::Instance( pevAttacker );

	if( !g_pGameRules->FPlayerCanTakeDamage( this, pAttacker ) )
	{
		// Refuse the damage
		return 0;
	}

	// keep track of amount of damage last sustained
	m_lastDamageAmount = (int)flDamage;

	// Armor. 
	if( !( pev->flags & FL_GODMODE ) && pev->armorvalue && !( bitsDamageType & ( DMG_FALL | DMG_DROWN | DMG_IGNORE_ARMOR ) ) )// armor doesn't protect against fall or drown damage!
	{
		float flNew = flDamage * flRatio;

		float flArmor = ( flDamage - flNew ) / flArmorStrength;

		// Does this use more armor than we have?
		if( flArmor > pev->armorvalue )
		{
			flArmor = pev->armorvalue;
			flArmor *= flArmorStrength;
			flNew = flDamage - flArmor;
			pev->armorvalue = 0;
		}
		else
			pev->armorvalue -= flArmor;

		flDamage = flNew;
	}

	// this cast to INT is critical!!! If a player ends up with 0.5 health, the engine will get that
	// as an int (zero) and think the player is dead! (this will incite a clientside screentilt, etc)
	fTookDamage = CBaseMonster::TakeDamage( pevInflictor, pevAttacker, (int)flDamage, bitsDamageType );

	// reset damage time countdown for each type of time based damage player just sustained
	{
		const BYTE timeBasedModifier = TBDModFromDmgType(bitsDamageType);
		for( int i = 0; i < CDMG_TIMEBASED; i++ )
			if( bitsDamageType & ( DMG_PARALYZE << i ) )
			{
				m_rgbTimeBasedDamage[i] = 0;
				m_timeBasedDmgModifiers[i] = timeBasedModifier;
			}
	}

	// tell director about it
	MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
		WRITE_BYTE( 9 );	// command length in bytes
		WRITE_BYTE( DRC_CMD_EVENT );	// take damage event
		WRITE_SHORT( ENTINDEX( this->edict() ) );	// index number of primary entity
		WRITE_SHORT( ENTINDEX( ENT( pevInflictor ) ) );	// index number of secondary entity
		WRITE_LONG( 5 );   // eventflags (priority and flags)
	MESSAGE_END();

	// how bad is it, doc?
	ftrivial = ( pev->health > 75 || m_lastDamageAmount < 5 );
	fmajor = ( m_lastDamageAmount > 25 );
	fcritical = ( pev->health < 30 );

	// handle all bits set in this damage message,
	// let the suit give player the diagnosis

	// UNDONE: add sounds for types of damage sustained (ie: burn, shock, slash )

	// UNDONE: still need to record damage and heal messages for the following types

		// DMG_BURN
		// DMG_FREEZE
		// DMG_BLAST
		// DMG_SHOCK

	m_bitsDamageType |= bitsDamage; // Save this so we can report it to the client
	m_bitsHUDDamage = -1;  // make sure the damage bits get resent

	while( fTookDamage && ( !ftrivial || ( bitsDamage & DMG_TIMEBASED ) ) && ffound && bitsDamage )
	{
		ffound = FALSE;

		if( bitsDamage & DMG_CLUB )
		{
			if( fmajor )
				SetSuitUpdate( "!HEV_DMG4", FALSE, SUIT_NEXT_IN_30SEC );	// minor fracture
			bitsDamage &= ~DMG_CLUB;
			ffound = TRUE;
		}
		if( bitsDamage & ( DMG_FALL | DMG_CRUSH ) )
		{
			if( fmajor )
				SetSuitUpdate( "!HEV_DMG5", FALSE, SUIT_NEXT_IN_30SEC );	// major fracture
			else
				SetSuitUpdate( "!HEV_DMG4", FALSE, SUIT_NEXT_IN_30SEC );	// minor fracture

			bitsDamage &= ~( DMG_FALL | DMG_CRUSH );
			ffound = TRUE;
		}

		if( bitsDamage & DMG_BULLET )
		{
			if( m_lastDamageAmount > 5 )
				SetSuitUpdate( "!HEV_DMG6", FALSE, SUIT_NEXT_IN_30SEC );	// blood loss detected
			//else
			//	SetSuitUpdate( "!HEV_DMG0", FALSE, SUIT_NEXT_IN_30SEC );	// minor laceration

			bitsDamage &= ~DMG_BULLET;
			ffound = TRUE;
		}

		if( bitsDamage & DMG_SLASH )
		{
			if( fmajor )
				SetSuitUpdate( "!HEV_DMG1", FALSE, SUIT_NEXT_IN_30SEC );	// major laceration
			else
				SetSuitUpdate( "!HEV_DMG0", FALSE, SUIT_NEXT_IN_30SEC );	// minor laceration

			bitsDamage &= ~DMG_SLASH;
			ffound = TRUE;
		}

		if( bitsDamage & DMG_SONIC )
		{
			if( fmajor )
				SetSuitUpdate( "!HEV_DMG2", FALSE, SUIT_NEXT_IN_1MIN );	// internal bleeding
			bitsDamage &= ~DMG_SONIC;
			ffound = TRUE;
		}

		if( bitsDamage & ( DMG_POISON | DMG_PARALYZE ) )
		{
			SetSuitUpdate( "!HEV_DMG3", FALSE, SUIT_NEXT_IN_1MIN );	// blood toxins detected
			bitsDamage &= ~( DMG_POISON | DMG_PARALYZE );
			ffound = TRUE;
		}

		if( bitsDamage & DMG_ACID )
		{
			SetSuitUpdate( "!HEV_DET1", FALSE, SUIT_NEXT_IN_1MIN );	// hazardous chemicals detected
			bitsDamage &= ~DMG_ACID;
			ffound = TRUE;
		}

		if( bitsDamage & DMG_NERVEGAS )
		{
			SetSuitUpdate( "!HEV_DET0", FALSE, SUIT_NEXT_IN_1MIN );	// biohazard detected
			bitsDamage &= ~DMG_NERVEGAS;
			ffound = TRUE;
		}

		if( bitsDamage & DMG_RADIATION )
		{
			SetSuitUpdate( "!HEV_DET2", FALSE, SUIT_NEXT_IN_1MIN );	// radiation detected
			bitsDamage &= ~DMG_RADIATION;
			ffound = TRUE;
		}
		if( bitsDamage & DMG_SHOCK )
		{
			bitsDamage &= ~DMG_SHOCK;
			ffound = TRUE;
		}
	}

	if (!FBitSet(bitsDamageType, DMG_NO_PUNCH))
		pev->punchangle.x = -2;

	if( fTookDamage && !ftrivial && fmajor && flHealthPrev >= 75 )
	{
		// first time we take major damage...
		// turn automedic on if not on
		SetSuitUpdate( "!HEV_MED1", FALSE, SUIT_NEXT_IN_30MIN );	// automedic on

		// give morphine shot if not given recently
		SetSuitUpdate( "!HEV_HEAL7", FALSE, SUIT_NEXT_IN_30MIN );	// morphine shot
	}

	if( fTookDamage && !ftrivial && fcritical && flHealthPrev < 75 )
	{
		// already took major damage, now it's critical...
		if( pev->health < 6 )
			SetSuitUpdate( "!HEV_HLTH3", FALSE, SUIT_NEXT_IN_10MIN );	// near death
		else if( pev->health < 20 )
			SetSuitUpdate( "!HEV_HLTH2", FALSE, SUIT_NEXT_IN_10MIN );	// health critical

		// give critical health warnings
		if( !RANDOM_LONG( 0, 3 ) && flHealthPrev < 50 )
			SetSuitUpdate( "!HEV_DMG7", FALSE, SUIT_NEXT_IN_5MIN ); //seek medical attention
	}

	// if we're taking time based damage, warn about its continuing effects
	if( fTookDamage && ( bitsDamageType & DMG_TIMEBASED ) && flHealthPrev < 75 )
	{
		if( flHealthPrev < 50 )
		{
			if( !RANDOM_LONG( 0, 3 ) )
				SetSuitUpdate( "!HEV_DMG7", FALSE, SUIT_NEXT_IN_5MIN ); //seek medical attention
		}
		else
			SetSuitUpdate( "!HEV_HLTH1", FALSE, SUIT_NEXT_IN_10MIN );	// health dropping
	}

	if (fTookDamage > 0 && pAttacker != NULL)
	{
		CBaseMonster* pAttackerMonster = pAttacker->MyMonsterPointer();
		if (pAttackerMonster &&
				pAttackerMonster->IRelationship(this) >= R_DL) // that was intentional attack
		{
			for (int i=0; i<TLK_CFRIENDS; ++i)
			{
				CTalkMonster::TalkFriend friendClass = CTalkMonster::m_szFriends[i];
				if (!friendClass.name[0])
					break;
				if (friendClass.category == TALK_FRIEND_SOLDIER)
				{
					CBaseEntity* pEntity = NULL;
					while((pEntity = UTIL_FindEntityByClassname(pEntity, friendClass.name)) != NULL)
					{
						CSquadMonster* pSquadMonster = pEntity->MySquadMonsterPointer();
						if (pSquadMonster)
						{
							CFollowingMonster* pFollowingMonster = pSquadMonster->MyFollowingMonsterPointer();
							if (pFollowingMonster && pFollowingMonster->m_hEnemy == 0 &&
									pFollowingMonster->m_pSchedule && (pFollowingMonster->m_pSchedule->iInterruptMask & bits_COND_NEW_ENEMY) &&
									pFollowingMonster->IsFollowingPlayer(this) && pFollowingMonster->IRelationship(pAttacker) >= R_DL)
							{
								ALERT(at_aiconsole, "%s is gonna attack player's attacker %s\n", STRING(pFollowingMonster->pev->classname), STRING(pAttacker->pev->classname));
								pFollowingMonster->SetEnemy(pAttacker);
								pFollowingMonster->SetConditions( bits_COND_NEW_ENEMY );
							}
						}
					}
				}
			}
		}
	}

	return fTookDamage;
}

struct AmmoCountInfo
{
	int ammoCount;
	int weaponCount; // number of weapons that use this kind of ammo
};

//=========================================================
// PackDeadPlayerItems - call this when a player dies to
// pack up the appropriate weapons and ammo items, and to
// destroy anything that shouldn't be packed.
//
// This is pretty brute force :(
//=========================================================
void CBasePlayer::PackDeadPlayerItems( void )
{
	int iWeaponRules;
	int iAmmoRules;
	int i;
	CBasePlayerWeapon *rgpPackWeapons[MAX_WEAPONS] = {0,};
	AmmoCountInfo iPackAmmo[MAX_AMMO_TYPES];
	int iPW = 0;// index into packweapons array

	memset( iPackAmmo, 0, sizeof(iPackAmmo) );

	// get the game rules
	iWeaponRules = g_pGameRules->DeadPlayerWeapons( this );
	iAmmoRules = g_pGameRules->DeadPlayerAmmo( this );

	if( iWeaponRules == GR_PLR_DROP_GUN_NO && iAmmoRules == GR_PLR_DROP_AMMO_NO )
	{
		// nothing to pack. Remove the weapons and return. Don't call create on the box!
		RemoveAllItems( STRIP_ALL_ITEMS );
		return;
	}

	// go through all of the weapons and make a list of the ones to pack
	for( i = 0; i < MAX_WEAPONS && iPW < MAX_WEAPONS; i++ )
	{
		// there's a weapon here. Should I pack it?
		CBasePlayerWeapon *pPlayerItem = m_rgpPlayerWeapons[i];

		if ( pPlayerItem && iPW < MAX_WEAPONS )
		{
			int ammoIndex = GetAmmoIndex( pPlayerItem->pszAmmo1() );
			if (ammoIndex >= 0) {
				if (!iPackAmmo[ammoIndex].weaponCount) {
					iPackAmmo[ammoIndex].ammoCount = AmmoInventory(ammoIndex);
				}
				iPackAmmo[ammoIndex].weaponCount++;
			}
			int ammo2Index = GetAmmoIndex( pPlayerItem->pszAmmo2() );
			if (ammo2Index >= 0) {
				if (!iPackAmmo[ammo2Index].weaponCount) {
					iPackAmmo[ammo2Index].ammoCount = AmmoInventory(ammo2Index);
				}
				iPackAmmo[ammo2Index].weaponCount++;
			}

			switch( iWeaponRules )
			{
			case GR_PLR_DROP_GUN_ACTIVE:
				if( m_pActiveItem && pPlayerItem == m_pActiveItem )
				{
					// this is the active item. Pack it.
					rgpPackWeapons[iPW] = pPlayerItem;
				}
				break;
			case GR_PLR_DROP_GUN_ALL:
				rgpPackWeapons[iPW] = pPlayerItem;
				break;
			default:
				break;
			}

			if( rgpPackWeapons[iPW] )
			{
				if (rgpPackWeapons[iPW]->iMaxClip() != WEAPON_NOCLIP)
				{
					// complete the reload.
					// TODO: make it depend on the game rules
					const int j = Q_min( rgpPackWeapons[iPW]->iMaxClip() - rgpPackWeapons[iPW]->m_iClip, iPackAmmo[rgpPackWeapons[iPW]->m_iPrimaryAmmoType].ammoCount );

					// Add them to the clip
					rgpPackWeapons[iPW]->m_iClip += j;
					iPackAmmo[rgpPackWeapons[iPW]->m_iPrimaryAmmoType].ammoCount -= j;
				}
				iPW++;
			}
		}
	}

	iPW = 0;

	const float angleStart = RANDOM_LONG(0,360);
	while( rgpPackWeapons[iPW] )
	{
		// create a box for each weapon
		CWeaponBox *pWeaponBox = (CWeaponBox *)CBaseEntity::Create( "weaponbox", pev->origin, pev->angles, edict() );

		pWeaponBox->pev->angles.x = 0;// don't let weaponbox tilt.
		pWeaponBox->pev->angles.z = 0;
		pWeaponBox->pev->angles.y += RANDOM_LONG(-180,180);

		if (weapon_respawndelay.value != -1) {
			pWeaponBox->SetThink( &CWeaponBox::Kill );
			pWeaponBox->pev->nextthink = gpGlobals->time + 120;
		}

		Vector weaponVelocity = pev->velocity;
		weaponVelocity.x *= RANDOM_FLOAT(0.6, 1.8);
		weaponVelocity.y *= RANDOM_FLOAT(0.6, 1.8);
		weaponVelocity.z *= RANDOM_FLOAT(0.6, 1.8);

		// Put items around the corpse
		Vector addAngles(0,0,0);
		addAngles.y += angleStart + iPW * 45 + RANDOM_LONG(10,20);
		UTIL_MakeVectors(addAngles);
		Vector addVector = (gpGlobals->v_forward + gpGlobals->v_right + gpGlobals->v_up) * 48;

		weaponVelocity = weaponVelocity + addVector;
		pWeaponBox->pev->velocity = weaponVelocity;// weaponbox has player's velocity, then some.

		CBasePlayerWeapon* weapon = rgpPackWeapons[iPW];
		if (pWeaponBox->PackWeapon( weapon ))
		{
			pWeaponBox->SetWeaponModel(weapon);

			if (g_pGameRules->IsBustingGame() && FClassnameIs( weapon->pev, "weapon_egon" ))
			{
				pWeaponBox->pev->velocity = g_vecZero;
				pWeaponBox->pev->renderfx = kRenderFxGlowShell;
				pWeaponBox->pev->renderamt = 25;
				pWeaponBox->pev->rendercolor = Vector( 0, 75, 250 );
			}

			const AmmoType* ammoType = g_AmmoRegistry.GetByName( weapon->pszAmmo1() );
			if (ammoType)
			{
				const int ammoIndex = ammoType->id;
				if (iPackAmmo[ammoIndex].ammoCount && iPackAmmo[ammoIndex].weaponCount) {
					const int toPack = iPackAmmo[ammoIndex].ammoCount / iPackAmmo[ammoIndex].weaponCount;
					iPackAmmo[ammoIndex].ammoCount -= toPack;
					pWeaponBox->PackAmmo( MAKE_STRING( ammoType->name ), toPack );
					iPackAmmo[ammoIndex].weaponCount--;
				}
			}
			const AmmoType* ammo2Type = g_AmmoRegistry.GetByName( weapon->pszAmmo2() );
			if (ammo2Type)
			{
				const int ammo2Index = ammo2Type->id;
				if (iPackAmmo[ammo2Index].ammoCount && iPackAmmo[ammo2Index].weaponCount) {
					const int toPack = iPackAmmo[ammo2Index].ammoCount / iPackAmmo[ammo2Index].weaponCount;
					iPackAmmo[ammo2Index].ammoCount -= toPack;
					pWeaponBox->PackAmmo( MAKE_STRING( ammo2Type->name ), toPack );
					iPackAmmo[ammo2Index].weaponCount--;
				}
			}
		}
		iPW++;
	}

	RemoveAllItems( STRIP_ALL_ITEMS );// now strip off everything that wasn't handled by the code above.
}

void CBasePlayer::RemoveAllItems( int stripFlags )
{
	int i;

	if( m_pActiveItem )
	{
		ResetAutoaim();
		m_pActiveItem->Holster();
		m_pActiveItem = NULL;
	}

	m_pLastItem = NULL;

	if( m_pTank != 0 )
		m_pTank->Use( this, this, USE_OFF, 0 );

	m_iTrain = TRAIN_NEW; // turn off train

	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		if (m_rgpPlayerWeapons[i]) {
			m_rgpPlayerWeapons[i]->Drop();
			m_rgpPlayerWeapons[i] = NULL;
		}
	}
	m_pActiveItem = NULL;

	pev->viewmodel = 0;
	pev->weaponmodel = 0;

	if (FBitSet(stripFlags, STRIP_SUITLIGHT) || !FBitSet(stripFlags, STRIP_DONT_TURNOFF_FLASHLIGHT)) {
		SuitLightTurnOff(false);
	}

	pev->weapons = 0;
	if( FBitSet(stripFlags, STRIP_SUIT) )
		m_iItemsBits &= ~PLAYER_ITEM_SUIT;
	if ( FBitSet(stripFlags, STRIP_SUITLIGHT) )
		RemoveSuitLight();

	if (FBitSet(stripFlags, STRIP_LONGJUMP)) {
		SetLongjump(false);
	}

	for( i = 0; i < MAX_AMMO_TYPES; i++ )
		m_rgAmmo[i] = 0;

	if( satchelfix.value )
		DeactivateSatchels( this );

	UpdateClientData();

	SendCurWeaponClear();
}

/*
 * GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
 *
 * ENTITY_METHOD(PlayerDie)
 */

void CBasePlayer::Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib )
{
	if (m_buddha && pev->health < 1) {
		pev->health = 1;
		return;
	}

	CSound *pSound;

	// Holster weapon immediately, to allow it to cleanup
	if( m_pActiveItem )
		m_pActiveItem->Holster();

	g_pGameRules->PlayerKilled( this, pevAttacker, pevInflictor );

	if( m_pTank != 0 )
		m_pTank->Use( this, this, USE_OFF, 0 );

	// this client isn't going to be thinking for a while, so reset the sound until they respawn
	pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( edict() ) );
	{
		if( pSound )
		{
			pSound->Reset();
		}
	}

	SetAnimation( PLAYER_DIE );

	m_flRespawnTimer = 0;

	pev->modelindex = g_ulModelIndexPlayer;    // don't use eyes

	pev->deadflag = DEAD_DYING;
	pev->movetype = MOVETYPE_TOSS;
	ClearBits( pev->flags, FL_ONGROUND );
	if( pev->velocity.z < 10 )
		pev->velocity.z += RANDOM_FLOAT( 0, 300 );

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate( NULL, FALSE, 0 );

	// send "health" update message to zero
	m_iClientHealth = 0;
	m_iClientMaxHealth = (int)pev->max_health;
	MESSAGE_BEGIN( MSG_ONE, gmsgHealth, NULL, pev );
		WRITE_SHORT( m_iClientHealth );
		WRITE_SHORT( (int)pev->max_health );
	MESSAGE_END();

	// Tell Ammo Hud that the player is dead
	SendCurWeaponDead();

	// reset FOV
	pev->fov = m_iFOV = m_iClientFOV = 0;

	MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
		WRITE_BYTE( 0 );
	MESSAGE_END();

	RemoveAllInventoryItems();

	// UNDONE: Put this in, but add FFADE_PERMANENT and make fade time 8.8 instead of 4.12
	// UTIL_ScreenFade( edict(), Vector( 128, 0, 0 ), 6, 15, 255, FFADE_OUT | FFADE_MODULATE );

	if( g_pGameRules->IsMultiplayer())
		pev->solid = SOLID_NOT;

	if( ( pev->health < -40 && iGib != GIB_NEVER ) || iGib == GIB_ALWAYS )
	{
		pev->solid = SOLID_NOT;
		GibMonster();	// This clears pev->model
		pev->effects |= EF_NODRAW;
		return;
	}

	DeathSound();

	pev->angles.x = 0;
	pev->angles.z = 0;

	SetThink( &CBasePlayer::PlayerDeathThink );
	pev->nextthink = gpGlobals->time + 0.1f;
}

// Set the activity based on an event or current state
void CBasePlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	int animDesired;
	float speed;
	char szAnim[64];

	speed = pev->velocity.Length2D();

	if( pev->flags & FL_FROZEN )
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

	switch( playerAnim )
	{
	case PLAYER_JUMP:
		m_IdealActivity = ACT_HOP;
		break;
	case PLAYER_SUPERJUMP:
		m_IdealActivity = ACT_LEAP;
		break;
	case PLAYER_DIE:
		m_IdealActivity = ACT_DIESIMPLE;
		m_IdealActivity = GetDeathActivity();
		break;
	case PLAYER_ATTACK1:
		switch( m_Activity )
		{
		case ACT_HOVER:
		case ACT_SWIM:
		case ACT_HOP:
		case ACT_LEAP:
		case ACT_DIESIMPLE:
			m_IdealActivity = m_Activity;
			break;
		default:
			m_IdealActivity = ACT_RANGE_ATTACK1;
			break;
		}
		break;
	case PLAYER_IDLE:
	case PLAYER_WALK:
		if( !FBitSet( pev->flags, FL_ONGROUND ) && ( m_Activity == ACT_HOP || m_Activity == ACT_LEAP ) )	// Still jumping
		{
			m_IdealActivity = m_Activity;
		}
		else if( pev->waterlevel > 1 )
		{
			if( speed == 0 )
				m_IdealActivity = ACT_HOVER;
			else
				m_IdealActivity = ACT_SWIM;
		}
		else
		{
			m_IdealActivity = ACT_WALK;
		}
		break;
	case PLAYER_GRAPPLE:
		{
			if (FBitSet(pev->flags, FL_ONGROUND))
			{
				if (pev->waterlevel > 1)
				{
					if (speed == 0)
						m_IdealActivity = ACT_HOVER;
					else
						m_IdealActivity = ACT_SWIM;
				}
				else
				{
					m_IdealActivity = ACT_WALK;
				}
			}
			else if (speed == 0)
			{
				m_IdealActivity = ACT_HOVER;
			}
			else
			{
				m_IdealActivity = ACT_SWIM;
			}
		}
		break;
	}

	switch( m_IdealActivity )
	{
	case ACT_HOVER:
	case ACT_LEAP:
	case ACT_SWIM:
	case ACT_HOP:
	case ACT_DIESIMPLE:
	default:
		if( m_Activity == m_IdealActivity )
			return;
		m_Activity = m_IdealActivity;

		animDesired = LookupActivity( m_Activity );

		// Already using the desired animation?
		if( pev->sequence == animDesired )
			return;

		pev->gaitsequence = 0;
		pev->sequence = animDesired;
		pev->frame = 0;
		ResetSequenceInfo();
		return;
	case ACT_RANGE_ATTACK1:
		if( FBitSet( pev->flags, FL_DUCKING ) )	// crouching
			strcpy( szAnim, "crouch_shoot_" );
		else
			strcpy( szAnim, "ref_shoot_" );
		strcat( szAnim, m_szAnimExtention );
		animDesired = LookupSequence( szAnim );
		if( animDesired == -1 )
			animDesired = 0;

		if( pev->sequence != animDesired || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		if( !m_fSequenceLoops )
		{
			pev->effects |= EF_NOINTERP;
		}

		m_Activity = m_IdealActivity;

		pev->sequence = animDesired;
		ResetSequenceInfo();
		break;
	case ACT_WALK:
		if( m_Activity != ACT_RANGE_ATTACK1 || m_fSequenceFinished )
		{
			if( FBitSet( pev->flags, FL_DUCKING ) )	// crouching
				strcpy( szAnim, "crouch_aim_" );
			else
				strcpy( szAnim, "ref_aim_" );
			strcat( szAnim, m_szAnimExtention );
			animDesired = LookupSequence( szAnim );
			if( animDesired == -1 )
				animDesired = 0;
			m_Activity = ACT_WALK;
		}
		else
		{
			animDesired = pev->sequence;
		}
	}

	if( FBitSet( pev->flags, FL_DUCKING ) )
	{
		if( speed == 0 )
		{
			pev->gaitsequence = LookupActivity( ACT_CROUCHIDLE );
			// pev->gaitsequence = LookupActivity( ACT_CROUCH );
		}
		else
		{
			pev->gaitsequence = LookupActivity( ACT_CROUCH );
		}
	}
	else if( speed > 220 )
	{
		pev->gaitsequence = LookupActivity( ACT_RUN );
	}
	else if( speed > 0 )
	{
		pev->gaitsequence = LookupActivity( ACT_WALK );
	}
	else
	{
		// pev->gaitsequence = LookupActivity( ACT_WALK );
		pev->gaitsequence = LookupSequence( "deep_idle" );
	}

	// Already using the desired animation?
	if( pev->sequence == animDesired )
		return;

	//ALERT( at_console, "Set animation to %d\n", animDesired );
	// Reset to first frame of desired animation
	pev->sequence = animDesired;
	pev->frame = 0;
	ResetSequenceInfo();
}

/*
===========
WaterMove
============
*/
#define AIRTIME	12		// lung full of air lasts this many seconds

void CBasePlayer::WaterMove()
{
	int air;

	if( pev->movetype == MOVETYPE_NOCLIP )
		return;

	if( pev->health < 0 )
		return;

	// waterlevel 0 - not in water
	// waterlevel 1 - feet in water
	// waterlevel 2 - waist in water
	// waterlevel 3 - head in water

	if( pev->waterlevel != 3 ) 
	{
		// not underwater

		// play 'up for air' sound
		if( pev->air_finished < gpGlobals->time )
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_wade1.wav", 1, ATTN_NORM );
		else if( pev->air_finished < gpGlobals->time + 9 )
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_wade2.wav", 1, ATTN_NORM );

		pev->air_finished = gpGlobals->time + AIRTIME;
		pev->dmg = 2;

		// if we took drowning damage, give it back slowly
		if( m_idrowndmg > m_idrownrestored )
		{
			// set drowning damage bit.  hack - dmg_drownrecover actually
			// makes the time based damage code 'give back' health over time.
			// make sure counter is cleared so we start count correctly.

			// NOTE: this actually causes the count to continue restarting
			// until all drowning damage is healed.

			m_bitsDamageType |= DMG_DROWNRECOVER;
			m_bitsDamageType &= ~DMG_DROWN;
			m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;
		}
	}
	else
	{	// fully under water
		// stop restoring damage while underwater
		m_bitsDamageType &= ~DMG_DROWNRECOVER;
		m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;

		if( pev->air_finished < gpGlobals->time )		// drown!
		{
			if( pev->pain_finished < gpGlobals->time )
			{
				// take drowning damage
				pev->dmg += 1;
				if( pev->dmg > 5 )
					pev->dmg = 5;
				TakeDamage( VARS( eoNullEntity ), VARS( eoNullEntity ), pev->dmg, DMG_DROWN );
				pev->pain_finished = gpGlobals->time + 1;

				// track drowning damage, give it back when
				// player finally takes a breath

				m_idrowndmg += (int)pev->dmg;
			} 
		}
		else
		{
			m_bitsDamageType &= ~DMG_DROWN;
		}
	}

	if( !pev->waterlevel )
	{
		if( FBitSet( pev->flags, FL_INWATER ) )
		{
			ClearBits( pev->flags, FL_INWATER );
		}
		return;
	}

	// make bubbles
	if( pev->waterlevel == 3 )
	{
		air = (int)( pev->air_finished - gpGlobals->time );
		if( !RANDOM_LONG( 0, 0x1f ) && RANDOM_LONG( 0, AIRTIME - 1 ) >= air )
		{
			switch( RANDOM_LONG( 0, 3 ) )
			{
				case 0:
					EMIT_SOUND( ENT( pev ), CHAN_BODY, "player/pl_swim1.wav", 0.8, ATTN_NORM );
					break;
				case 1:
					EMIT_SOUND( ENT( pev ), CHAN_BODY, "player/pl_swim2.wav", 0.8, ATTN_NORM );
					break;
				case 2:
					EMIT_SOUND( ENT( pev ), CHAN_BODY, "player/pl_swim3.wav", 0.8, ATTN_NORM );
					break;
				case 3:
					EMIT_SOUND( ENT( pev ), CHAN_BODY, "player/pl_swim4.wav", 0.8, ATTN_NORM );
					break;
			}
		}
	}

	if( pev->watertype == CONTENT_LAVA )		// do damage
	{
		if( pev->dmgtime < gpGlobals->time )
			TakeDamage( VARS( eoNullEntity ), VARS( eoNullEntity ), 10 * pev->waterlevel, DMG_BURN );
	}
	else if( pev->watertype == CONTENT_SLIME )		// do damage
	{
		pev->dmgtime = gpGlobals->time + 1;
		TakeDamage( VARS( eoNullEntity ), VARS( eoNullEntity ), 4 * pev->waterlevel, DMG_ACID );
	}

	if( !FBitSet( pev->flags, FL_INWATER ) )
	{
		SetBits( pev->flags, FL_INWATER );
		pev->dmgtime = 0;
	}
}

// TRUE if the player is attached to a ladder
BOOL CBasePlayer::IsOnLadder( void )
{ 
	return ( pev->movetype == MOVETYPE_FLY );
}

void CBasePlayer::PlayerDeathThink( void )
{
	float flForward;

	if( FBitSet( pev->flags, FL_ONGROUND ) )
	{
		flForward = pev->velocity.Length() - 20;
		if( flForward <= 0 )
			pev->velocity = g_vecZero;
		else    
			pev->velocity = flForward * pev->velocity.Normalize();
	}

	if( HasWeapons() )
	{
		// we drop the guns here because weapons that have an area effect and can kill their user
		// will sometimes crash coming back from CBasePlayer::Killed() if they kill their owner because the
		// player class sometimes is freed. It's safer to manipulate the weapons once we know
		// we aren't calling into any of their code anymore through the player pointer.
		PackDeadPlayerItems();
	}

	if( pev->modelindex && ( !m_fSequenceFinished ) && ( pev->deadflag == DEAD_DYING ))
	{
		StudioFrameAdvance();

		m_flRespawnTimer = gpGlobals->frametime + m_flRespawnTimer;	// Note, these aren't necessarily real "frames", so behavior is dependent on # of client movement commands
		if( m_flRespawnTimer < 4.0f )   // Animations should be no longer than this
			return;
	}

	if( pev->deadflag == DEAD_DYING )
	{
		if( g_pGameRules->IsMultiplayer() && m_fSequenceFinished && pev->movetype == MOVETYPE_NONE )
		{
			CopyToBodyQue( pev );
			pev->modelindex = 0;
		}
		pev->deadflag = DEAD_DEAD;
	}

	// once we're done animating our death and we're on the ground, we want to set movetype to None so our dead body won't do collisions and stuff anymore
	// this prevents a bug where the dead body would go to a player's head if he walked over it while the dead player was clicking their button to respawn
	if( pev->movetype != MOVETYPE_NONE && FBitSet( pev->flags, FL_ONGROUND ) )
		pev->movetype = MOVETYPE_NONE;

	StopAnimation();

	pev->effects |= EF_NOINTERP;
	pev->framerate = 0.0;

	BOOL fAnyButtonDown = ( pev->button & ~IN_SCORE );

	// wait for all buttons released
	if( pev->deadflag == DEAD_DEAD )
	{
		if( fAnyButtonDown )
			return;

		if( g_pGameRules->FPlayerCanRespawn( this ) )
		{
			m_fDeadTime = gpGlobals->time;
			pev->deadflag = DEAD_RESPAWNABLE;
		}

		return;
	}

	// if the player has been dead for one second longer than allowed by forcerespawn,
	// forcerespawn isn't on. Send the player off to an intermission camera until they
	// choose to respawn.
	if( g_pGameRules->IsMultiplayer() && ( gpGlobals->time > ( m_fDeadTime + 6 ) ) && !( m_afPhysicsFlags & PFLAG_OBSERVER ) )
	{
		// go to dead camera. 
		StartDeathCam();
	}

	if( pev->iuser1 )	// player is in spectator mode
		return;

	if (g_pGameRules->IsMultiplayer() && respawndelay.value)
	{
		if (m_fDeadTime + respawndelay.value > gpGlobals->time)
		{
			if (m_flNextRespawnMessageTime <= gpGlobals->time)
			{
				const int secondsBeforeRespawn = (int)ceil(m_fDeadTime + respawndelay.value - gpGlobals->time);
				ClientPrint( pev, HUD_PRINTCENTER, UTIL_VarArgs( "Respawn allowed in %d seconds", secondsBeforeRespawn ));
				m_flNextRespawnMessageTime = gpGlobals->time + 1;
			}
			return;
		}
		else
		{
			if (m_flNextRespawnMessageTime <= gpGlobals->time)
			{
				ClientPrint( pev, HUD_PRINTCENTER, "You can respawn now!" );
				m_flNextRespawnMessageTime = gpGlobals->time + 2;
			}
		}
	}

	// wait for any button down,  or mp_forcerespawn is set and the respawn time is up
	if( !fAnyButtonDown && !( g_pGameRules->IsMultiplayer() && forcerespawn.value > 0 && ( gpGlobals->time > ( m_fDeadTime + 5 ) ) ) )
		return;

	pev->button = 0;
	m_flRespawnTimer = 0;

	//ALERT( at_console, "Respawn\n" );

	if (g_pGameRules->IsMultiplayer() && respawndelay.value)
	{
		ClientPrint( pev, HUD_PRINTCENTER, "" );
	}
	respawn( pev, !( m_afPhysicsFlags & PFLAG_OBSERVER ) );// don't copy a corpse if we're in deathcam.
	pev->nextthink = -1;
}

//=========================================================
// StartDeathCam - find an intermission spot and send the
// player off into observer mode
//=========================================================
void CBasePlayer::StartDeathCam( void )
{
	edict_t *pSpot, *pNewSpot;
	int iRand;

	if( pev->view_ofs == g_vecZero )
	{
		// don't accept subsequent attempts to StartDeathCam()
		return;
	}

	pSpot = FIND_ENTITY_BY_CLASSNAME( NULL, "info_intermission" );

	if( !FNullEnt( pSpot ) )
	{
		// at least one intermission spot in the world.
		iRand = RANDOM_LONG( 0, 3 );

		while( iRand > 0 )
		{
			pNewSpot = FIND_ENTITY_BY_CLASSNAME( pSpot, "info_intermission" );

			if( pNewSpot )
			{
				pSpot = pNewSpot;
			}

			iRand--;
		}

		CopyToBodyQue( pev );

		UTIL_SetOrigin( pev, pSpot->v.origin );
		pev->angles = pev->v_angle = pSpot->v.v_angle;
	}
	else
	{
		// no intermission spot. Push them up in the air, looking down at their corpse
		TraceResult tr;
		CopyToBodyQue( pev );
		UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, 128 ), ignore_monsters, edict(), &tr );

		UTIL_SetOrigin( pev, tr.vecEndPos );
		pev->angles = pev->v_angle = UTIL_VecToAngles( tr.vecEndPos - pev->origin );
	}

	// start death cam
	m_afPhysicsFlags |= PFLAG_OBSERVER;
	pev->view_ofs = g_vecZero;
	pev->fixangle = TRUE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	pev->modelindex = 0;
}

void CBasePlayer::StartObserver( Vector vecPosition, Vector vecViewAngle )
{
	// clear any clientside entities attached to this player
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_KILLPLAYERATTACHMENTS );
		WRITE_BYTE( (BYTE)entindex() );
	MESSAGE_END();

	// Holster weapon immediately, to allow it to cleanup
	if( m_pActiveItem )
		m_pActiveItem->Holster();

	if( m_pTank != 0 )
		m_pTank->Use( this, this, USE_OFF, 0 );

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate( NULL, FALSE, 0 );

	// Tell Ammo Hud that the player is dead
	SendCurWeaponDead();

	// reset FOV
	m_iFOV = m_iClientFOV = 0;
	pev->fov = m_iFOV;
	MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
		WRITE_BYTE( 0 );
	MESSAGE_END();

	// Setup flags
	m_iHideHUD = ( HIDEHUD_HEALTH | HIDEHUD_FLASHLIGHT | HIDEHUD_WEAPONS );
	m_afPhysicsFlags |= PFLAG_OBSERVER;
	pev->effects = EF_NODRAW;
	pev->view_ofs = g_vecZero;
	pev->angles = pev->v_angle = vecViewAngle;
	pev->fixangle = TRUE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	ClearBits( m_afPhysicsFlags, PFLAG_DUCKING );
	ClearBits( pev->flags, FL_DUCKING );
	pev->deadflag = DEAD_RESPAWNABLE;
	pev->health = 1;

	// Clear out the status bar
	m_fInitHUD = TRUE;

	pev->team = 0;
	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
		WRITE_BYTE( ENTINDEX(edict()) );
		WRITE_STRING( "" );
	MESSAGE_END();

	// Remove all the player's stuff
	RemoveAllItems( STRIP_ALL_ITEMS );

	// Move them to the new position
	UTIL_SetOrigin( pev, vecPosition );

	// Find a player to watch
	m_flNextObserverInput = 0;
	Observer_SetMode( m_iObserverLastMode );
}

//
// PlayerUse - handles USE keypress
//
#define	PLAYER_SEARCH_RADIUS	(float)64

void CBasePlayer::PlayerUse( void )
{
	if( IsObserver() )
		return;

	// Was use pressed or released?
	if( !( ( pev->button | m_afButtonPressed | m_afButtonReleased) & IN_USE ) )
		return;

	if (FBitSet(m_suppressedCapabilities, PLAYER_SUPPRESS_USE))
		return;

	// Hit Use on a train?
	if( m_afButtonPressed & IN_USE )
	{
		if( m_camera != 0 && FBitSet(m_camera->pev->spawnflags, SF_CAMERA_STOP_BY_PLAYER_INPUT_USE) )
		{
			m_camera->Use(this, this, USE_OFF, 0.0f);
			m_camera = 0;
			return;
		}
		else if( m_pTank != 0 )
		{
			// Stop controlling the tank
			// TODO: Send HUD Update
			m_pTank->Use( this, this, USE_OFF, 0 );
			return;
		}
		else
		{
			if( m_afPhysicsFlags & PFLAG_ONTRAIN )
			{
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW|TRAIN_OFF;

				CBaseEntity *pTrain = Instance( pev->groundentity );
				if( pTrain && pTrain->Classify() == CLASS_VEHICLE )
				{
					( (CFuncVehicle *)pTrain )->m_pDriver = NULL;
				}
				return;
			}
			else
			{	// Start controlling the train!
				CBaseEntity *pTrain = CBaseEntity::Instance( pev->groundentity );

				if( pTrain && !( pev->button & IN_JUMP ) && FBitSet( pev->flags, FL_ONGROUND ) && ( pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE ) && pTrain->OnControls( pev ) )
				{
					m_afPhysicsFlags |= PFLAG_ONTRAIN;
					m_iTrain = TrainSpeed( (int)pTrain->pev->speed, pTrain->pev->impulse );
					m_iTrain |= TRAIN_NEW;

					if( pTrain->Classify() == CLASS_VEHICLE )
					{
						EmitSoundScript(Player::vehicleIgnitionSoundScript);
						( (CFuncVehicle *)pTrain )->m_pDriver = this;
					}
					else
						EmitSoundScript(Player::trainUseSoundScript);
					return;
				}
			}
		}
	}

	CBaseEntity *pObject = NULL;
	CBaseEntity *pClosest = NULL;
	Vector vecLOS;
	float flMaxDot = VIEW_FIELD_NARROW;
	float flDot;
	TraceResult tr;
	int caps;

	UTIL_MakeVectors( pev->v_angle );// so we know which way we are facing

	UTIL_TraceLine( pev->origin + pev->view_ofs,
						pev->origin + pev->view_ofs + (gpGlobals->v_forward * PLAYER_SEARCH_RADIUS),
						dont_ignore_monsters, ENT(pev), &tr );
	if (tr.pHit)
	{
		pObject = CBaseEntity::Instance(tr.pHit);
		if (!pObject || !(pObject->ObjectCaps() & (FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE)))
		{
			pObject = NULL;
		}
	}

	if (!pObject)
	{
		while( ( pObject = UTIL_FindEntityInSphere( pObject, pev->origin, PLAYER_SEARCH_RADIUS ) ) != NULL )
		{
			caps = pObject->ObjectCaps();
			if( caps & ( FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE ) &&
					!(caps & FCAP_ONLYDIRECT_USE) )
			{
				// !!!PERFORMANCE- should this check be done on a per case basis AFTER we've determined that
				// this object is actually usable? This dot is being done for every object within PLAYER_SEARCH_RADIUS
				// when player hits the use key. How many objects can be in that area, anyway? (sjb)
				vecLOS = ( VecBModelOrigin( pObject->pev ) - ( pev->origin + pev->view_ofs ) );

				// This essentially moves the origin of the target to the corner nearest the player to test to see
				// if it's "hull" is in the view cone
				vecLOS = UTIL_ClampVectorToBox( vecLOS, pObject->pev->size * 0.5 );

				if (!AllowUseThroughWalls() || (caps & FCAP_ONLYVISIBLE_USE) )
				{
					UTIL_TraceLine(pev->origin + pev->view_ofs, pObject->Center(), dont_ignore_monsters, edict(), &tr);
					if (tr.flFraction < 1.0f && tr.pHit != pObject->edict())
					{
						continue;
					}
				}

				flDot = DotProduct( vecLOS , gpGlobals->v_forward );
				if( flDot > flMaxDot )
				{
					// only if the item is in front of the user
					pClosest = pObject;
					flMaxDot = flDot;
					//ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
				}
				//ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
			}
		}

		pObject = pClosest;
	}

	// Found an object
	if( pObject )
	{
		caps = pObject->ObjectCaps();

		if( m_afButtonPressed & IN_USE )
		{
#if FEATURE_CLIENTSIDE_HUDSOUND
			if (IsNetClient())
			{
					MESSAGE_BEGIN( MSG_ONE, gmsgUseSound, NULL, pev );
							WRITE_BYTE( 1 );
					MESSAGE_END();
			}
#else
			EMIT_SOUND( ENT(pev), CHAN_ITEM, "common/wpn_select.wav", 0.4, ATTN_NORM );
#endif
		}

		if( ( ( pev->button & IN_USE ) && ( caps & FCAP_CONTINUOUS_USE ) ) ||
			 ( ( m_afButtonPressed & IN_USE ) && ( caps & ( FCAP_IMPULSE_USE | FCAP_ONOFF_USE ) ) ) )
		{
			if( caps & FCAP_CONTINUOUS_USE )
				m_afPhysicsFlags |= PFLAG_USING;

			pObject->Use( this, this, USE_SET, 1 );
		}
		// UNDONE: Send different USE codes for ON/OFF.  Cache last ONOFF_USE object to send 'off' if you turn away
		else if( ( m_afButtonReleased & IN_USE ) && ( pObject->ObjectCaps() & FCAP_ONOFF_USE ) )	// BUGBUG This is an "off" use
		{
			pObject->Use( this, this, USE_SET, 0 );
		}
	}
	else
	{
		if( m_afButtonPressed & IN_USE )
		{
#if FEATURE_CLIENTSIDE_HUDSOUND
			if (IsNetClient())
			{
					MESSAGE_BEGIN( MSG_ONE, gmsgUseSound, NULL, pev );
							WRITE_BYTE( 0 );
					MESSAGE_END();
			}
#else
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "common/wpn_denyselect.wav", 0.4, ATTN_NORM );
#endif
		}
	}
}

void CBasePlayer::Jump()
{
	Vector vecWallCheckDir;// direction we're tracing a line to find a wall when walljumping
	Vector vecAdjustedVelocity;
	Vector vecSpot;
	TraceResult tr;

	if( FBitSet( pev->flags, FL_WATERJUMP ) )
		return;

	if( pev->waterlevel >= 2 )
	{
		return;
	}

	// jump velocity is sqrt( height * gravity * 2)

	// If this isn't the first frame pressing the jump button, break out.
	if( !FBitSet( m_afButtonPressed, IN_JUMP ) )
		return;         // don't pogo stick

	if( !( pev->flags & FL_ONGROUND ) || !pev->groundentity )
	{
		return;
	}

	// many features in this function use v_forward, so makevectors now.
	UTIL_MakeVectors( pev->angles );

	// ClearBits( pev->flags, FL_ONGROUND );		// don't stairwalk

	SetAnimation( PLAYER_JUMP );

	if( m_fLongJump &&
		( pev->button & IN_DUCK ) &&
		( pev->flDuckTime > 0 ) &&
		pev->velocity.Length() > 50 )
	{
		SetAnimation( PLAYER_SUPERJUMP );
	}

	// If you're standing on a conveyor, add it's velocity to yours (for momentum)
	entvars_t *pevGround = VARS( pev->groundentity );
	if( pevGround )
	{
		if( pevGround->flags & FL_CONVEYOR )
		{
			pev->velocity = pev->velocity + pev->basevelocity;
		}

		if( FClassnameIs( pevGround, "func_vehicle" ))
		{
			pev->velocity = pevGround->velocity + pev->velocity;
		}
	}
}

// This is a glorious hack to find free space when you've crouched into some solid space
// Our crouching collisions do not work correctly for some reason and this is easier
// than fixing the problem :(
void FixPlayerCrouchStuck( edict_t *pPlayer )
{
	TraceResult trace;

	// Move up as many as 18 pixels if the player is stuck.
	for( int i = 0; i < 18; i++ )
	{
		UTIL_TraceHull( pPlayer->v.origin, pPlayer->v.origin, dont_ignore_monsters, head_hull, pPlayer, &trace );
		if( trace.fStartSolid )
			pPlayer->v.origin.z++;
		else
			break;
	}
}

void CBasePlayer::Duck()
{
	if( pev->button & IN_DUCK )
	{
		if( m_IdealActivity != ACT_LEAP )
		{
			SetAnimation( PLAYER_WALK );
		}
	}
}

//
// ID's player as such.
//

int CBasePlayer::DefaultClassify( void )
{
	return CLASS_PLAYER;
}

int CBasePlayer::Classify( void )
{
	return CLASS_PLAYER;
}

void CBasePlayer::AddPoints( int score, BOOL bAllowNegativeScore )
{
	AddFloatPoints(score, bAllowNegativeScore);
}

void CBasePlayer::AddFloatPoints( float score, BOOL bAllowNegativeScore )
{
	// Positive score always adds
	if( score < 0 )
	{
		if( !bAllowNegativeScore )
		{
			if( pev->frags < 0 )		// Can't go more negative
				return;

			if( -score > pev->frags )	// Will this go negative?
			{
				score = (int)( -pev->frags );		// Sum will be 0
			}
		}
	}

	pev->frags += score;

	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX( edict() ) );
		WRITE_SHORT( (int)pev->frags );
		WRITE_SHORT( m_iDeaths );
		WRITE_SHORT( 0 );
		WRITE_SHORT( g_pGameRules->GetTeamIndex( m_szTeamName ) + 1 );
	MESSAGE_END();
}

void CBasePlayer::AddPointsToTeam( int score, BOOL bAllowNegativeScore )
{
	int index = entindex();

	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

		if( pPlayer && i != index )
		{
			if( g_pGameRules->PlayerRelationship( this, pPlayer ) == GR_TEAMMATE )
			{
				pPlayer->AddPoints( score, bAllowNegativeScore );
			}
		}
	}
}

//Player ID
void CBasePlayer::InitStatusBar()
{
	m_flStatusBarDisappearDelay = 0;
	m_SbarString1[0] = m_SbarString0[0] = 0; 

	m_lastSeenEntityIndex = -1;
	m_lastSeenHealth = -1;
	m_lastSeenArmor = -1;
	m_lastSeenTime = 0;
}

static void ClearMonsterInfoChannel(CBasePlayer* player)
{
	if (player->m_lastSeenEntityIndex < 0)
		return;
	player->m_lastSeenEntityIndex = -1;
	player->m_lastSeenHealth = -1;
	player->m_lastSeenArmor = -1;

	hudtextparms_t  textParms;
	textParms.channel = 3;
	textParms.x = 0.1;
	textParms.y = 0.6;
	textParms.effect = 0;
	textParms.r1 = 100;
	textParms.g1 = 100;
	textParms.b1 = 100;
	textParms.a1 = 0;
	textParms.r2 = 240;
	textParms.g2 = 210;
	textParms.b2 = 0;
	textParms.a2 = 0;
	textParms.fadeinTime = 0;
	textParms.fadeoutTime = 0;
	textParms.holdTime = 0.1;
	textParms.fxTime = 0;
	UTIL_HudMessage(player, textParms, "");
}

#define MONSTERINFO_LINGER_TIME 2

void CBasePlayer::UpdateStatusBar()
{
	int newSBarState[SBAR_END] = {0};
	char sbuf0[SBAR_STRING_SIZE];
	char sbuf1[ SBAR_STRING_SIZE ];

	strcpy( sbuf0, m_SbarString0 );
	strcpy( sbuf1, m_SbarString1 );

	// Find an ID Target
	TraceResult tr;
	UTIL_MakeVectors( pev->v_angle + pev->punchangle );
	Vector vecSrc = EyePosition();
	Vector vecEnd = vecSrc + ( gpGlobals->v_forward * MAX_ID_RANGE );
	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, edict(), &tr );

	CBaseEntity *pEntity = NULL;
	if( tr.flFraction != 1.0f && !FNullEnt( tr.pHit ) )
	{
		pEntity = CBaseEntity::Instance( tr.pHit );
	}

	bool showMonsterInfo = false;

	if (pEntity && g_pGameRules->IsCoOp())
	{
		CBaseMonster* pMonster = pEntity->MyMonsterPointer();
		if (pMonster && pMonster->IsAlive() && pMonster->m_IdealMonsterState != MONSTERSTATE_DEAD && g_pGameRules->IsMultiplayer()) {
			const int entityIndex = ENTINDEX( pEntity->edict() );
			int health = (int)ceil(pEntity->pev->health);
			if (health < 0) {
				health = 0;
			}
			const int armor = (int)pEntity->pev->armorvalue;

			const bool isFriendPlayer = pEntity->IsPlayer() && g_pGameRules->PlayerRelationship(this, pEntity) == GR_TEAMMATE;
			const bool isFriendMonster = (pMonster->IDefaultRelationship(this) == R_AL);
			showMonsterInfo = isFriendPlayer || (allowmonsterinfo.value == 1 && !pMonster->IsPlayer()) || (allowmonsterinfo.value == 2 && isFriendMonster);
			if (showMonsterInfo)
				m_lastSeenTime = gpGlobals->time;

			if (showMonsterInfo && (m_lastSeenEntityIndex != entityIndex || m_lastSeenHealth != health || (m_lastSeenArmor != armor && isFriendPlayer))) {
				m_lastSeenEntityIndex = entityIndex;
				m_lastSeenHealth = health;
				m_lastSeenArmor = armor;

				hudtextparms_t  textParms;
				textParms.channel = 3;
				textParms.x = 0.1;
				textParms.y = 0.6;
				textParms.effect = 0;
				if (isFriendMonster || isFriendPlayer) {
					textParms.r1 = 0;
					textParms.g1 = 255;
					textParms.b1 = 0;
				} else {
					textParms.r1 = 255;
					textParms.g1 = 0;
					textParms.b1 = 0;
				}
				textParms.a1 = 0;
				textParms.r2 = 100;
				textParms.g2 = 100;
				textParms.b2 = 100;
				textParms.a2 = 0;
				textParms.fadeinTime = 0.1;
				textParms.fadeoutTime = 0;
				textParms.holdTime = 1000.0;
				textParms.fxTime = 0;

				char buf[512];
				if (isFriendPlayer) {
					sprintf(buf, "%s\nHealth: %d\nArmor: %d", STRING(pEntity->pev->netname), health, armor);
				} else {
					const char* displayName = pMonster->DisplayName();
					const char* className = STRING(pEntity->pev->classname);
					if (!displayName)
					{
						if (strncmp(className, "monster_", 8) == 0)
							displayName = className + 8;
						else
							displayName = className;
					}

					sprintf(buf, "%s\nHealth: %d/%d", displayName, health, (int)ceil(pEntity->pev->max_health));
					if (displayName == className + 8) {
						buf[0] = toupper(buf[0]); //Capitalize monster name
						char* str = buf;
						str++;
						bool wasSpace = false;
						while(*str != '\0' && *str != '\n')
						{
							if (*str == '_')
							{
								*str = ' ';
								wasSpace = true;
							}
							else
							{
								if (wasSpace)
									*str = toupper(*str);
								wasSpace = false;
							}
							str++;
						}
					}
				}
				UTIL_HudMessage(this, textParms, buf);
			}
		}

		if (!showMonsterInfo)
		{
			if( pEntity->IsPlayer() )
			{
				newSBarState[SBAR_ID_TARGETNAME] = ENTINDEX( pEntity->edict() );
				strcpy( sbuf1, "1 %p1\n2 Health: %i2%%\n3 Armor: %i3%%" );

				// allies and medics get to see the targets health
				if( g_pGameRules->PlayerRelationship( this, pEntity ) == GR_TEAMMATE )
				{
					newSBarState[SBAR_ID_TARGETHEALTH] = (int)( 100 * ( pEntity->pev->health / pEntity->pev->max_health ) );
					newSBarState[SBAR_ID_TARGETARMOR] = (int)pEntity->pev->armorvalue; //No need to get it % based since 100 it's the max.
				}

				m_flStatusBarDisappearDelay = gpGlobals->time + 1.0f;
			}
		}
	}
	else
	{
		if (m_flStatusBarDisappearDelay > gpGlobals->time)
		{
			// hold the values for a short amount of time after viewing the object
			newSBarState[SBAR_ID_TARGETNAME] = m_izSBarState[SBAR_ID_TARGETNAME];
			newSBarState[SBAR_ID_TARGETHEALTH] = m_izSBarState[SBAR_ID_TARGETHEALTH];
			newSBarState[SBAR_ID_TARGETARMOR] = m_izSBarState[SBAR_ID_TARGETARMOR];
		}
	}

	if (showMonsterInfo)
	{
		return;
	}
	else if (m_lastSeenEntityIndex >= 0 && m_lastSeenTime + MONSTERINFO_LINGER_TIME <= gpGlobals->time  && g_pGameRules->IsCoOp())
	{
		ClearMonsterInfoChannel(this);
	}

	BOOL bForceResend = FALSE;

	if( strcmp( sbuf0, m_SbarString0 ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgStatusText, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_STRING( sbuf0 );
		MESSAGE_END();

		strcpy( m_SbarString0, sbuf0 );

		// make sure everything's resent
		bForceResend = TRUE;
	}

	if( strcmp( sbuf1, m_SbarString1 ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgStatusText, NULL, pev );
			WRITE_BYTE( 1 );
			WRITE_STRING( sbuf1 );
		MESSAGE_END();

		strcpy( m_SbarString1, sbuf1 );

		// make sure everything's resent
		bForceResend = TRUE;
	}

	// Check values and send if they don't match
	for( int i = 1; i < SBAR_END; i++ )
	{
		if( newSBarState[i] != m_izSBarState[i] || bForceResend )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgStatusValue, NULL, pev );
				WRITE_BYTE( i );
				WRITE_SHORT( newSBarState[i] );
			MESSAGE_END();

			m_izSBarState[i] = newSBarState[i];
		}
	}
}

#define CLIMB_SHAKE_FREQUENCY		22	// how many frames in between screen shakes when climbing
#define	MAX_CLIMB_SPEED			200	// fastest vertical climbing speed possible
#define	CLIMB_SPEED_DEC			15	// climbing deceleration rate
#define	CLIMB_PUNCH_X			-7  // how far to 'punch' client X axis when climbing
#define CLIMB_PUNCH_Z			7	// how far to 'punch' client Z axis when climbing

enum
{
	MovementStand,
	MovementRun,
	MovementCrouch,
	MovementJump,
};

void CBasePlayer::SetMovementMode()
{
	if (m_fInitHUD)
		return;
	short currentMovementState;
	if (!FBitSet( pev->flags, FL_ONGROUND ))
	{
		currentMovementState = MovementJump;
		if (IsOnLadder())
		{
			currentMovementState = MovementStand;
		}
#if FEATURE_ROPE
		if (IsOnRope())
		{
			currentMovementState = MovementStand;
		}
#endif
	}
	else if (FBitSet( pev->flags, FL_DUCKING ))
	{
		currentMovementState = MovementCrouch;
	}
	else if (pev->velocity.Length2D() > 220)
	{
		currentMovementState = MovementRun;
	}
	else
	{
		currentMovementState = MovementStand;
	}
	if (currentMovementState != m_movementState)
	{
		m_movementState = currentMovementState;
		if( gmsgMovementState )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgMovementState, NULL, pev );
				WRITE_SHORT( m_movementState );
			MESSAGE_END();
		}
	}
}

void CBasePlayer::PreThink( void )
{
	SetMovementMode();

	int buttonsChanged = ( m_afButtonLast ^ pev->button );	// These buttons have changed this frame

	// Debounced button codes for pressed/released
	// UNDONE: Do we need auto-repeat?
	m_afButtonPressed =  buttonsChanged & pev->button;		// The changed ones still down are "pressed"
	m_afButtonReleased = buttonsChanged & ( ~pev->button );	// The ones not down are "released"

	g_pGameRules->PlayerThink( this );
	pev->maxspeed = m_maxSpeedFraction * g_psv_maxspeed->value;

	if( g_fGameOver )
		return;         // intermission or finale

	UTIL_MakeVectors( pev->v_angle );             // is this still used?

	ItemPreFrame();
	WaterMove();

	if( g_pGameRules && g_pGameRules->FAllowFlashlight() )
		m_iHideHUD &= ~HIDEHUD_FLASHLIGHT;
	else
		m_iHideHUD |= HIDEHUD_FLASHLIGHT;

	if (m_bResetViewEntity)
	{
		m_bResetViewEntity = false;

		CBaseEntity* viewEntity = m_hViewEntity;

		if (viewEntity)
		{
			SET_VIEW(edict(), viewEntity->edict());
		}
	}

	// JOHN: checks if new client data (for HUD and view control) needs to be sent to the client
	UpdateClientData();

	CheckTimeBasedDamage();

	CheckSuitUpdate();

	GlowShellUpdate();

	// Observer Button Handling
	if( IsObserver() )
	{
		Observer_HandleButtons();
		Observer_CheckTarget();
		Observer_CheckProperties();
		pev->impulse = 0;
		return;
	}

	if( pev->deadflag >= DEAD_DYING )
	{
		PlayerDeathThink();
		return;
	}

	// So the correct flags get sent to client asap.
	//
	if( m_afPhysicsFlags & PFLAG_ONTRAIN )
		pev->flags |= FL_ONTRAIN;
	else 
		pev->flags &= ~FL_ONTRAIN;

#if FEATURE_ROPE
	//We're on a rope. - Solokiller
	if( (m_afPhysicsFlags & PFLAG_ONROPE) && m_pRope )
	{
		pev->velocity = g_vecZero;

		Vector vecAttachPos = m_pRope->GetAttachedObjectsPosition();
		vecAttachPos.z -= FBitSet(pev->flags, FL_DUCKING) ? 12 : 24;
		if (!SetClosestOriginOnRope(vecAttachPos))
		{
			ALERT(at_aiconsole, "Can't set attach pos as origin for player (would lead to stuck)\n");
			LetGoRope(0.2);
			return;
		}

		Vector vecForce;

		/*
		//This causes sideways acceleration that doesn't occur in Op4. - Solokiller
		if( pev->button & IN_DUCK )
		{
			vecForce.x = gpGlobals->v_right.x;
			vecForce.y = gpGlobals->v_right.y;
			vecForce.z = 0;
			m_pRope->ApplyForceFromPlayer( vecForce );
		}
		if( pev->button & IN_JUMP )
		{
			vecForce.x = -gpGlobals->v_right.x;
			vecForce.y = -gpGlobals->v_right.y;
			vecForce.z = 0;
			m_pRope->ApplyForceFromPlayer( vecForce );
		}
		*/

		//Determine if any force should be applied to the rope, or if we should move around. - Solokiller
		if( pev->button & ( IN_BACK | IN_FORWARD ) )
		{
			if( ( gpGlobals->v_forward.x * gpGlobals->v_forward.x +
				gpGlobals->v_forward.y * gpGlobals->v_forward.y -
				gpGlobals->v_forward.z * gpGlobals->v_forward.z ) <= 0.0 )
			{
				if( m_bIsClimbing )
				{
					const float flDelta = gpGlobals->time - m_flLastClimbTime;
					m_flLastClimbTime = gpGlobals->time;
					if( pev->button & IN_FORWARD )
					{
						if( gpGlobals->v_forward.z < 0.0 )
						{
							if( !m_pRope->MoveDown( flDelta ) )
							{
								//Let go of the rope, detach. - Solokiller
								LetGoRope();
							}
						}
						else
						{
							m_pRope->MoveUp( flDelta );
						}
					}
					if( pev->button & IN_BACK )
					{
						if( gpGlobals->v_forward.z < 0.0 )
						{
							m_pRope->MoveUp( flDelta );
						}
						else if( !m_pRope->MoveDown( flDelta ) )
						{
							LetGoRope();
						}
					}
				}
				else
				{
					m_bIsClimbing = true;
					m_flLastClimbTime = gpGlobals->time;
				}
			}
			else
			{
				vecForce.x = gpGlobals->v_forward.x;
				vecForce.y = gpGlobals->v_forward.y;
				vecForce.z = 0.0;
				if( pev->button & IN_BACK )
				{
					vecForce.x = -gpGlobals->v_forward.x;
					vecForce.y = -gpGlobals->v_forward.y;
					vecForce.z = 0;
				}
				m_pRope->ApplyForceFromPlayer( vecForce );
				m_bIsClimbing = false;
			}
		}
		else
		{
			m_bIsClimbing = false;
		}

		if( m_afButtonPressed & IN_JUMP )
		{
			CRope* rope = m_pRope;
			LetGoRope();
			//We've jumped off the rope, give us some momentum - Solokiller

			Vector vecDir = gpGlobals->v_up * 165.0 + gpGlobals->v_forward * 150.0;

			Vector vecVelocity = rope->GetAttachedObjectsVelocity() * 2;

			vecVelocity = vecVelocity.Normalize();

			vecVelocity = vecVelocity * 200;

			pev->velocity = vecVelocity + vecDir;
		}
		return;
	}
#endif

	// Train speed control
	if( m_afPhysicsFlags & PFLAG_ONTRAIN )
	{
		CBaseEntity *pTrain = CBaseEntity::Instance( pev->groundentity );
		float vel;
		int iGearId;	// Vit_amiN: keeps the train control HUD in sync

		if( !pTrain )
		{
			TraceResult trainTrace;
			// Maybe this is on the other side of a level transition
			UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -38 ), ignore_monsters, ENT( pev ), &trainTrace );

			// HACKHACK - Just look for the func_tracktrain classname
			if( trainTrace.flFraction != 1.0f && trainTrace.pHit )
			pTrain = CBaseEntity::Instance( trainTrace.pHit );

			if( !pTrain || !( pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE ) || !pTrain->OnControls( pev ) )
			{
				//ALERT( at_error, "In train mode with no train!\n" );
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW|TRAIN_OFF;
				if( pTrain )
					( (CFuncVehicle *)pTrain )->m_pDriver = NULL;
				return;
			}
		}
		else if( !FBitSet( pev->flags, FL_ONGROUND ) || FBitSet( pTrain->pev->spawnflags, SF_TRACKTRAIN_NOCONTROL )
			|| ( ( pev->button & ( IN_MOVELEFT | IN_MOVERIGHT )) && pTrain->Classify() != CLASS_VEHICLE ))
		{
			// Turn off the train if you jump, strafe, or the train controls go dead
			m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
			m_iTrain = TRAIN_NEW | TRAIN_OFF;
			( (CFuncVehicle *)pTrain )->m_pDriver = NULL;
			return;
		}

		pev->velocity = g_vecZero;
		vel = 0;

		if( pTrain->Classify() == CLASS_VEHICLE )
		{
			if( pev->button & IN_FORWARD )
			{
				vel = 1;
				pTrain->Use( this, this, USE_SET, vel );
			}

			if( pev->button & IN_BACK )
			{
				vel = -1;
				pTrain->Use( this, this, USE_SET, vel );
			}

			if( pev->button & IN_MOVELEFT )
			{
				vel = 20;
				pTrain->Use( this, this, USE_SET, vel );
			}
			if( pev->button & IN_MOVERIGHT )
			{
				vel = 30;
				pTrain->Use( this, this, USE_SET, vel );
			}
		}
		else
		{
			if( m_afButtonPressed & IN_FORWARD )
			{
				vel = 1;
				pTrain->Use( this, this, USE_SET, vel );
			}
			else if( m_afButtonPressed & IN_BACK )
			{
				vel = -1;
				pTrain->Use( this, this, USE_SET, vel );
			}
		}
		iGearId = TrainSpeed( pTrain->pev->speed, pTrain->pev->impulse );

		if( iGearId != ( m_iTrain & 0x0F ) )	// Vit_amiN: speed changed
		{
			m_iTrain = iGearId;
			m_iTrain |= TRAIN_ACTIVE | TRAIN_NEW;
		}
	}
	else if( m_iTrain & TRAIN_ACTIVE )
		m_iTrain = TRAIN_NEW; // turn off train

	if( pev->button & IN_JUMP )
	{
		// If on a ladder, jump off the ladder
		// else Jump
		Jump();
	}

	// If trying to duck, already ducked, or in the process of ducking
	if( ( pev->button & IN_DUCK ) || FBitSet( pev->flags,FL_DUCKING ) || ( m_afPhysicsFlags & PFLAG_DUCKING ) )
		Duck();

	if( !FBitSet( pev->flags, FL_ONGROUND ) )
	{
		m_flFallVelocity = -pev->velocity.z;
	}

	// StudioFrameAdvance();//!!!HACKHACK!!! Can't be hit by traceline when not animating?

	// Clear out ladder pointer
	m_hEnemy = NULL;

	if( m_afPhysicsFlags & PFLAG_ONBARNACLE )
	{
		pev->velocity = g_vecZero;
	}
}

#if FEATURE_ROPE
void CBasePlayer::LetGoRope(float delay)
{
	//Let go of the rope, detach. - Solokiller
	pev->movetype = MOVETYPE_WALK;
	pev->solid = SOLID_SLIDEBOX;
	m_afPhysicsFlags &= ~PFLAG_ONROPE;
	if (m_pRope)
	{
		m_pRope->DetachObject(delay);
		m_pRope = NULL;
	}
	m_bIsClimbing = false;
}

bool CBasePlayer::SetClosestOriginOnRope(const Vector &vecPos)
{
	TraceResult trace;
	UTIL_TraceHull( vecPos, vecPos, ignore_monsters, FBitSet(pev->flags, FL_DUCKING) ? head_hull : human_hull, edict(), &trace );

	if( trace.fStartSolid )
	{
		const Vector difference = vecPos - pev->origin;
		const int stepNumber = 8;
		const Vector diffStep = difference/stepNumber;

		Vector vecCheckPos = pev->origin;

		for (int j=0; j<stepNumber; ++j)
		{
			vecCheckPos = vecCheckPos + diffStep;
			UTIL_TraceHull( vecCheckPos, vecCheckPos, ignore_monsters, FBitSet(pev->flags, FL_DUCKING) ? head_hull : human_hull, edict(), &trace );
			if (!trace.fStartSolid)
			{
				pev->origin = vecCheckPos;
			}
			else
			{
				if (j == 0)
				{
					return (vecPos - pev->origin).Length() <= 16.0f;
				}
				return true;
			}
		}
		return true;
	}
	else
	{
		pev->origin = vecPos;
		return true;
	}
}

#endif

/* Time based Damage works as follows: 
	1) There are several types of timebased damage:

		#define DMG_PARALYZE		(1 << 14)	// slows affected creature down
		#define DMG_NERVEGAS		(1 << 15)	// nerve toxins, very bad
		#define DMG_POISON		(1 << 16)	// blood poisioning
		#define DMG_RADIATION		(1 << 17)	// radiation exposure
		#define DMG_DROWNRECOVER	(1 << 18)	// drown recovery
		#define DMG_ACID		(1 << 19)	// toxic chemicals or acid burns
		#define DMG_SLOWBURN		(1 << 20)	// in an oven
		#define DMG_SLOWFREEZE		(1 << 21)	// in a subzero freezer

	2) A new hit inflicting tbd restarts the tbd counter - each monster has an 8bit counter,
		per damage type. The counter is decremented every second, so the maximum time 
		an effect will last is 255/60 = 4.25 minutes.  Of course, staying within the radius
		of a damaging effect like fire, nervegas, radiation will continually reset the counter to max.

	3) Every second that a tbd counter is running, the player takes damage.  The damage
		is determined by the type of tdb.  
			Paralyze		- 1/2 movement rate, 30 second duration.
			Nervegas		- 5 points per second, 16 second duration = 80 points max dose.
			Poison			- 2 points per second, 25 second duration = 50 points max dose.
			Radiation		- 1 point per second, 50 second duration = 50 points max dose.
			Drown			- 5 points per second, 2 second duration.
			Acid/Chemical	- 5 points per second, 10 second duration = 50 points max.
			Burn			- 10 points per second, 2 second duration.
			Freeze			- 3 points per second, 10 second duration = 30 points max.

	4) Certain actions or countermeasures counteract the damaging effects of tbds:

		Armor/Heater/Cooler - Chemical(acid),burn, freeze all do damage to armor power, then to body
							- recharged by suit recharger
		Air In Lungs		- drowning damage is done to air in lungs first, then to body
							- recharged by poking head out of water
							- 10 seconds if swiming fast
		Air In SCUBA		- drowning damage is done to air in tanks first, then to body
							- 2 minutes in tanks. Need new tank once empty.
		Radiation Syringe	- Each syringe full provides protection vs one radiation dosage
		Antitoxin Syringe	- Each syringe full provides protection vs one poisoning (nervegas or poison).
		Health kit			- Immediate stop to acid/chemical, fire or freeze damage.
		Radiation Shower	- Immediate stop to radiation damage, acid/chemical or fire damage.
*/

// If player is taking time based damage, continue doing damage to player -
// this simulates the effect of being poisoned, gassed, dosed with radiation etc -
// anything that continues to do damage even after the initial contact stops.
// Update all time based damage counters, and shut off any that are done.

// The m_bitsDamageType bit MUST be set if any damage is to be taken.
// This routine will detect the initial on value of the m_bitsDamageType
// and init the appropriate counter.  Only processes damage every second.

//#define PARALYZE_DURATION		30		// number of 2 second intervals to take damage
//#define PARALYZE_DAMAGE		0.0		// damage to take each 2 second interval

//#define NERVEGAS_DURATION		16
//#define NERVEGAS_DAMAGE		5.0

//#define POISON_DURATION		25
//#define POISON_DAMAGE			2.0

//#define RADIATION_DURATION		50
//#define RADIATION_DAMAGE		1.0

//#define ACID_DURATION			10
//#define ACID_DAMAGE			5.0

//#define SLOWBURN_DURATION		2
//#define SLOWBURN_DAMAGE		1.0

//#define SLOWFREEZE_DURATION		1.0
//#define SLOWFREEZE_DAMAGE		3.0

void CBasePlayer::CheckTimeBasedDamage() 
{
	int i;
	BYTE bDuration = 0;

	//static float gtbdPrev = 0.0;

	if( !( m_bitsDamageType & DMG_TIMEBASED ) )
		return;

	// only check for time based damage approx. every 2 seconds
	if( fabs( gpGlobals->time - m_tbdPrev ) < 2.0f )
		return;

	m_tbdPrev = gpGlobals->time;

	for( i = 0; i < CDMG_TIMEBASED; i++ )
	{
		// make sure bit is set for damage type
		if( m_bitsDamageType & ( DMG_PARALYZE << i ) )
		{
			switch( i )
			{
			case itbd_Paralyze:
				// UNDONE - flag movement as half-speed
				bDuration = PARALYZE_DURATION;
				break;
			case itbd_NerveGas:
				//TakeDamage( pev, pev, NERVEGAS_DAMAGE, DMG_GENERIC );
				bDuration = NERVEGAS_DURATION;
				break;
			case itbd_Poison:
				TakeDamage( pev, pev, POISON_DAMAGE, DMG_GENERIC | (DmgTypeFromTBDMod(m_timeBasedDmgModifiers[i])) );
				bDuration = POISON_DURATION;
				break;
			case itbd_Radiation:
				//TakeDamage( pev, pev, RADIATION_DAMAGE, DMG_GENERIC );
				bDuration = RADIATION_DURATION;
				break;
			case itbd_DrownRecover:
				// NOTE: this hack is actually used to RESTORE health
				// after the player has been drowning and finally takes a breath
				if( m_idrowndmg > m_idrownrestored )
				{
					int idif = Q_min( m_idrowndmg - m_idrownrestored, 10 );

					TakeHealth( this, idif, DMG_GENERIC );
					m_idrownrestored += idif;
				}
				bDuration = 4;	// get up to 5*10 = 50 points back
				break;
			case itbd_Acid:
				//TakeDamage( pev, pev, ACID_DAMAGE, DMG_GENERIC );
				bDuration = ACID_DURATION;
				break;
			case itbd_SlowBurn:
				//TakeDamage( pev, pev, SLOWBURN_DAMAGE, DMG_GENERIC );
				bDuration = SLOWBURN_DURATION;
				break;
			case itbd_SlowFreeze:
				//TakeDamage( pev, pev, SLOWFREEZE_DAMAGE, DMG_GENERIC );
				bDuration = SLOWFREEZE_DURATION;
				break;
			default:
				bDuration = 0;
			}

			if( m_rgbTimeBasedDamage[i] )
			{
				// use up an antitoxin on poison or nervegas after a few seconds of damage					
				if( ( ( i == itbd_NerveGas ) && ( m_rgbTimeBasedDamage[i] < NERVEGAS_DURATION ) ) ||
					( ( i == itbd_Poison ) && ( m_rgbTimeBasedDamage[i] < POISON_DURATION ) ) )
				{
					if( m_rgItems[ITEM_ANTIDOTE] )
					{
						m_rgbTimeBasedDamage[i] = 0;
						m_rgItems[ITEM_ANTIDOTE]--;
						SetSuitUpdate( "!HEV_HEAL4", FALSE, SUIT_REPEAT_OK );
					}
				}

				// decrement damage duration, detect when done.
				if( !m_rgbTimeBasedDamage[i] || --m_rgbTimeBasedDamage[i] == 0 )
				{
					m_rgbTimeBasedDamage[i] = 0;
					m_timeBasedDmgModifiers[i] = 0;

					// if we're done, clear damage bits
					m_bitsDamageType &= ~( DMG_PARALYZE << i );	
				}
			}
			else
				// first time taking this damage type - init damage duration
				m_rgbTimeBasedDamage[i] = bDuration;
		}
	}
}

/*
THE POWER SUIT

The Suit provides 3 main functions: Protection, Notification and Augmentation. 
Some functions are automatic, some require power. 
The player gets the suit shortly after getting off the train in C1A0 and it stays
with him for the entire game.

Protection

	Heat/Cold
		When the player enters a hot/cold area, the heating/cooling indicator on the suit 
		will come on and the battery will drain while the player stays in the area. 
		After the battery is dead, the player starts to take damage. 
		This feature is built into the suit and is automatically engaged.
	Radiation Syringe
		This will cause the player to be immune from the effects of radiation for N seconds. Single use item.
	Anti-Toxin Syringe
		This will cure the player from being poisoned. Single use item.
	Health
		Small (1st aid kits, food, etc.)
		Large (boxes on walls)
	Armor
		The armor works using energy to create a protective field that deflects a
		percentage of damage projectile and explosive attacks. After the armor has been deployed,
		it will attempt to recharge itself to full capacity with the energy reserves from the battery.
		It takes the armor N seconds to fully charge. 

Notification (via the HUD)

x	Health
x	Ammo  
x	Automatic Health Care
		Notifies the player when automatic healing has been engaged. 
x	Geiger counter
		Classic Geiger counter sound and status bar at top of HUD 
		alerts player to dangerous levels of radiation. This is not visible when radiation levels are normal.
x	Poison
	Armor
		Displays the current level of armor. 

Augmentation 

	Reanimation (w/adrenaline)
		Causes the player to come back to life after he has been dead for 3 seconds. 
		Will not work if player was gibbed. Single use.
	Long Jump
		Used by hitting the ??? key(s). Caused the player to further than normal.
	SCUBA	
		Used automatically after picked up and after player enters the water. 
		Works for N seconds. Single use.	
	
Things powered by the battery

	Armor		
		Uses N watts for every M units of damage.
	Heat/Cool	
		Uses N watts for every second in hot/cold area.
	Long Jump	
		Uses N watts for every jump.
	Alien Cloak	
		Uses N watts for each use. Each use lasts M seconds.
	Alien Shield	
		Augments armor. Reduces Armor drain by one half
*/

// if in range of radiation source, ping geiger counter

#if FEATURE_GEIGER_SOUNDS_FIX
#define GEIGERDELAY 0.28f
#else
#define GEIGERDELAY 0.25f
#endif

void CBasePlayer::UpdateGeigerCounter( void )
{
	BYTE range;

	// delay per update ie: don't flood net with these msgs
	if( gpGlobals->time < m_flgeigerDelay )
		return;

	m_flgeigerDelay = gpGlobals->time + GEIGERDELAY;

	// send range to radition source to client
	range = (BYTE)( m_flgeigerRange / 4 );

	if( range != m_igeigerRangePrev )
	{
		m_igeigerRangePrev = range;

		MESSAGE_BEGIN( MSG_ONE, gmsgGeigerRange, NULL, pev );
			WRITE_BYTE( range );
		MESSAGE_END();
	}

	// reset counter and semaphore
	if( !RANDOM_LONG( 0, 3 ) )
		m_flgeigerRange = 1000;
}

/*
================
CheckSuitUpdate

Play suit update if it's time
================
*/

#define SUITUPDATETIME		3.5f
#define SUITFIRSTUPDATETIME	0.1f

void CBasePlayer::CheckSuitUpdate()
{
	int i;
	int isentence = 0;
	int isearch = m_iSuitPlayNext;

	// Ignore suit updates if no suit
	if( !HasSuit() )
		return;

	// if in range of radiation source, ping geiger counter
	UpdateGeigerCounter();

	if (!g_modFeatures.suit_sentences)
		return;

	if( g_pGameRules->IsMultiplayer() )
	{
		// don't bother updating HEV voice in multiplayer.
		return;
	}

	if( gpGlobals->time >= m_flSuitUpdate && m_flSuitUpdate > 0 )
	{
		// play a sentence off of the end of the queue
		for( i = 0; i < CSUITPLAYLIST; i++ )
		{
			if( ( isentence = m_rgSuitPlayList[isearch] ) )
				break;

			if( ++isearch == CSUITPLAYLIST )
				isearch = 0;
		}

		if( isentence )
		{
			m_rgSuitPlayList[isearch] = 0;
			if( isentence > 0 )
			{
				// play sentence number
				char sentence[CBSENTENCENAME_MAX + 1];
				strcpy( sentence, "!" );
				strcat( sentence, gszallsentencenames[isentence] );
				EMIT_SOUND_SUIT( ENT( pev ), sentence );
			}
			else
			{
				// play sentence group
				EMIT_GROUPID_SUIT( ENT( pev ), -isentence );
			}
			m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME;
		}
		else
			// queue is empty, don't check 
			m_flSuitUpdate = 0;
	}
}

// add sentence to suit playlist queue. if fgroup is true, then
// name is a sentence group (HEV_AA), otherwise name is a specific
// sentence name ie: !HEV_AA0.  If iNoRepeat is specified in
// seconds, then we won't repeat playback of this word or sentence
// for at least that number of seconds.

void CBasePlayer::SetSuitUpdate( const char *name, int fgroup, int iNoRepeatTime )
{
	int i;
	int isentence;
	int iempty = -1;

	// Ignore suit updates if no suit
	if( !HasSuit() )
		return;

	if (!g_modFeatures.suit_sentences)
		return;

	if( g_pGameRules->IsMultiplayer() )
	{
		// due to static channel design, etc. We don't play HEV sounds in multiplayer right now.
		return;
	}

	// if name == NULL, then clear out the queue
	if( !name )
	{
		for( i = 0; i < CSUITPLAYLIST; i++ )
			m_rgSuitPlayList[i] = 0;
		return;
	}

	// get sentence or group number
	if( !fgroup )
	{
		isentence = SENTENCEG_Lookup( name, NULL );
		if( isentence < 0 )
			return;
	}
	else
		// mark group number as negative
		isentence = -SENTENCEG_GetIndex( name );

	// check norepeat list - this list lets us cancel
	// the playback of words or sentences that have already
	// been played within a certain time.
	for( i = 0; i < CSUITNOREPEAT; i++ )
	{
		if( isentence == m_rgiSuitNoRepeat[i] )
		{
			// this sentence or group is already in 
			// the norepeat list
			if( m_rgflSuitNoRepeatTime[i] < gpGlobals->time )
			{
				// norepeat time has expired, clear it out
				m_rgiSuitNoRepeat[i] = 0;
				m_rgflSuitNoRepeatTime[i] = 0.0;
				iempty = i;
				break;
			}
			else
			{
				// don't play, still marked as norepeat
				return;
			}
		}
		// keep track of empty slot
		if( !m_rgiSuitNoRepeat[i] )
			iempty = i;
	}

	// sentence is not in norepeat list, save if norepeat time was given
	if( iNoRepeatTime )
	{
		if( iempty < 0 )
			iempty = RANDOM_LONG( 0, CSUITNOREPEAT - 1 ); // pick random slot to take over
		m_rgiSuitNoRepeat[iempty] = isentence;
		m_rgflSuitNoRepeatTime[iempty] = iNoRepeatTime + gpGlobals->time;
	}

	// find empty spot in queue, or overwrite last spot
	m_rgSuitPlayList[m_iSuitPlayNext++] = isentence;
	if( m_iSuitPlayNext == CSUITPLAYLIST )
		m_iSuitPlayNext = 0;

	if( m_flSuitUpdate <= gpGlobals->time )
	{
		if( m_flSuitUpdate == 0 )
			// play queue is empty, don't delay too long before playback
			m_flSuitUpdate = gpGlobals->time + SUITFIRSTUPDATETIME;
		else 
			m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME; 
	}
}

/*
================
CheckPowerups

Check for turning off powerups

GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
================
*/
static void CheckPowerups( entvars_t *pev )
{
	if( pev->health <= 0 )
		return;

	pev->modelindex = g_ulModelIndexPlayer;    // don't use eyes
}

//=========================================================
// UpdatePlayerSound - updates the position of the player's
// reserved sound slot in the sound list.
//=========================================================
void CBasePlayer::UpdatePlayerSound( void )
{
	int iBodyVolume;
	int iVolume;
	CSound *pSound;

	pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( edict() ) );

	if( !pSound )
	{
		ALERT( at_console, "Client lost reserved sound!\n" );
		return;
	}

	pSound->m_iType = bits_SOUND_NONE;

	// now calculate the best target volume for the sound. If the player's weapon
	// is louder than his body/movement, use the weapon volume, else, use the body volume.
	if( FBitSet( pev->flags, FL_ONGROUND ) )
	{
		iBodyVolume = (int)pev->velocity.Length(); 

		// clamp the noise that can be made by the body, in case a push trigger,
		// weapon recoil, or anything shoves the player abnormally fast. 
		if( iBodyVolume > 512 )
		{
			iBodyVolume = 512;
		}
	}
	else
	{
		iBodyVolume = 0;
	}

	if( pev->button & IN_JUMP )
	{
		iBodyVolume += 100;
	}

	// convert player move speed and actions into sound audible by monsters.
	if( m_iWeaponVolume > iBodyVolume )
	{
		m_iTargetVolume = m_iWeaponVolume;

		// OR in the bits for COMBAT sound if the weapon is being louder than the player. 
		pSound->m_iType |= bits_SOUND_COMBAT;
	}
	else
	{
		m_iTargetVolume = iBodyVolume;
	}

	// decay weapon volume over time so bits_SOUND_COMBAT stays set for a while
	m_iWeaponVolume -= (int)( 250 * gpGlobals->frametime );
	if( m_iWeaponVolume < 0 )
	{
		iVolume = 0;
	}

	// if target volume is greater than the player sound's current volume, we paste the new volume in 
	// immediately. If target is less than the current volume, current volume is not set immediately to the
	// lower volume, rather works itself towards target volume over time. This gives monsters a much better chance
	// to hear a sound, especially if they don't listen every frame.
	iVolume = pSound->m_iVolume;

	if( m_iTargetVolume > iVolume )
	{
		iVolume = m_iTargetVolume;
	}
	else if( iVolume > m_iTargetVolume )
	{
		iVolume -= (int)( 250 * gpGlobals->frametime );

		if( iVolume < m_iTargetVolume )
		{
			iVolume = 0;
		}
	}

	if( m_fNoPlayerSound )
	{
		// debugging flag, lets players move around and shoot without monsters hearing.
		iVolume = 0;
	}

	if( gpGlobals->time > m_flStopExtraSoundTime )
	{
		// since the extra sound that a weapon emits only lasts for one client frame, we keep that sound around for a server frame or two 
		// after actual emission to make sure it gets heard.
		m_iExtraSoundTypes = 0;
	}

	if( pSound )
	{
		pSound->m_vecOrigin = pev->origin;
		pSound->m_iType |= ( bits_SOUND_PLAYER | m_iExtraSoundTypes );
		pSound->m_iVolume = iVolume;
	}

	// keep track of virtual muzzle flash
	m_iWeaponFlash -= (int)( 256 * gpGlobals->frametime );
	if( m_iWeaponFlash < 0 )
		m_iWeaponFlash = 0;

	//UTIL_MakeVectors( pev->angles );
	//gpGlobals->v_forward.z = 0;

	// Below are a couple of useful little bits that make it easier to determine just how much noise the 
	// player is making. 
	// UTIL_ParticleEffect( pev->origin + gpGlobals->v_forward * iVolume, g_vecZero, 255, 25 );
	//ALERT( at_console, "%d/%d\n", iVolume, m_iTargetVolume );
}

void CBasePlayer::PostThink()
{
	if( g_fGameOver )
		goto pt_end;	// intermission or finale

	if( !IsAlive() )
		goto pt_end;

	// Handle Tank controlling
	if( m_pTank != 0 )
	{
		// if they've moved too far from the gun,  or selected a weapon, unuse the gun
		if( m_pTank->OnControls( pev ) && !pev->viewmodel )
		{  
			m_pTank->Use( this, this, USE_SET, 2 );	// try fire the gun
		}
		else
		{
			// they've moved off the platform
			m_pTank->Use( this, this, USE_OFF, 0 );
		}
	}

	// do weapon stuff
	ItemPostFrame();

	// check to see if player landed hard enough to make a sound
	// falling farther than half of the maximum safe distance, but not as far a max safe distance will
	// play a bootscrape sound, and no damage will be inflicted. Fallling a distance shorter than half
	// of maximum safe distance will make no sound. Falling farther than max safe distance will play a 
	// fallpain sound, and damage will be inflicted based on how far the player fell

	if( ( FBitSet( pev->flags, FL_ONGROUND ) ) && ( pev->health > 0 ) && m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD )
	{
		// ALERT( at_console, "%f\n", m_flFallVelocity );
		if( pev->watertype == CONTENT_WATER )
		{
			// Did he hit the world or a non-moving entity?
			// BUG - this happens all the time in water, especially when 
			// BUG - water has current force
			// if( !pev->groundentity || VARS(pev->groundentity )->velocity.z == 0 )
				// EMIT_SOUND( ENT( pev ), CHAN_BODY, "player/pl_wade1.wav", 1, ATTN_NORM );
		}
		else if( m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED )
		{
			// after this point, we start doing damage
			float flFallDamage = g_pGameRules->FlPlayerFallDamage( this );

			if( flFallDamage > pev->health )
			{
				//splat
				// note: play on item channel because we play footstep landing on body channel
				EmitSoundScript(Player::fallBodySplatSoundScript);
			}

			if( flFallDamage > 0 )
			{
				TakeDamage( VARS( eoNullEntity ), VARS( eoNullEntity ), flFallDamage, DMG_FALL ); 
				pev->punchangle.x = 0;
			}
		}

		if( IsAlive() )
		{
			SetAnimation( PLAYER_WALK );
		}
	}

	if( FBitSet( pev->flags, FL_ONGROUND ) )
	{
		if( m_flFallVelocity > 64 && !g_pGameRules->IsMultiplayer() )
		{
			CSoundEnt::InsertSound( bits_SOUND_PLAYER, pev->origin, (int)m_flFallVelocity, 0.2 );
			// ALERT( at_console, "fall %f\n", m_flFallVelocity );
		}
		m_flFallVelocity = 0;
	}

	// select the proper animation for the player character	
	if( IsAlive() )
	{
		if( !pev->velocity.x && !pev->velocity.y )
			SetAnimation( PLAYER_IDLE );
		else if( ( pev->velocity.x || pev->velocity.y ) && ( FBitSet( pev->flags, FL_ONGROUND ) ) )
			SetAnimation( PLAYER_WALK );
		else if( pev->waterlevel > 1 )
			SetAnimation( PLAYER_WALK );
	}

	StudioFrameAdvance();
	CheckPowerups( pev );

	UpdatePlayerSound();
pt_end:
	if( pev->deadflag == DEAD_NO )
		m_vecLastViewAngles = pev->angles;
	else
		pev->angles = m_vecLastViewAngles;

	// Track button info so we can detect 'pressed' and 'released' buttons next frame
	m_afButtonLast = pev->button;

#if CLIENT_WEAPONS
	// Decay timers on weapons
	// go through all of the weapons and make a list of the ones to pack
	for( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CBasePlayerWeapon *pPlayerItem = m_rgpPlayerWeapons[i];

		if ( pPlayerItem )
		{
			CBasePlayerWeapon *gun = pPlayerItem;

			if( gun && gun->UseDecrement() )
			{
				gun->m_flNextPrimaryAttack = Q_max( gun->m_flNextPrimaryAttack - gpGlobals->frametime, -1.0f );
				gun->m_flNextSecondaryAttack = Q_max( gun->m_flNextSecondaryAttack - gpGlobals->frametime, -0.001f );

				if( gun->m_flTimeWeaponIdle != 1000.0f )
				{
					gun->m_flTimeWeaponIdle = Q_max( gun->m_flTimeWeaponIdle - gpGlobals->frametime, -0.001f );
				}

				if( gun->pev->fuser1 != 1000.0f )
				{
					gun->pev->fuser1 = Q_max( gun->pev->fuser1 - gpGlobals->frametime, -0.001f );
				}

				// Only decrement if not flagged as NO_DECREMENT
				/*if( gun->m_flPumpTime != 1000 )
				{
					gun->m_flPumpTime = Q_max( gun->m_flPumpTime - gpGlobals->frametime, -0.001 );
				}*/
			}
		}
	}

	m_flNextAttack -= gpGlobals->frametime;
	if( m_flNextAttack < -0.001f )
		m_flNextAttack = -0.001f;
	
	if( m_flNextAmmoBurn != 1000.0f )
	{
		m_flNextAmmoBurn -= gpGlobals->frametime;

		if( m_flNextAmmoBurn < -0.001f )
			m_flNextAmmoBurn = -0.001f;
	}

	if( m_flAmmoStartCharge != 1000.0f )
	{
		m_flAmmoStartCharge -= gpGlobals->frametime;

		if( m_flAmmoStartCharge < -0.001f )
			m_flAmmoStartCharge = -0.001f;
	}
#else
	return;
#endif
}

#define SF_SPAWNPOINT_OFF 2

// checks if the spot is clear of players
BOOL IsSpawnPointValid( CBaseEntity *pPlayer, CBaseEntity *pSpot )
{
	CBaseEntity *ent = NULL;

	if( FBitSet(pSpot->pev->spawnflags, SF_SPAWNPOINT_OFF) || !pSpot->IsTriggered( pPlayer ) )
	{
		return FALSE;
	}

	while( ( ent = UTIL_FindEntityInSphere( ent, pSpot->pev->origin, 128 ) ) != NULL )
	{
		// if ent is a client, don't spawn on 'em
		if( ent->IsPlayer() && ent != pPlayer )
			return FALSE;
	}

	return TRUE;
}

DLL_GLOBAL CBaseEntity	*g_pLastSpawn;
inline int FNullEnt( CBaseEntity *ent ) { return ( ent == NULL ) || FNullEnt( ent->edict() ); }

inline bool SpawnPointIsOn( CBaseEntity* ent ) { return !FNullEnt(ent) && !FBitSet(ent->pev->spawnflags, SF_SPAWNPOINT_OFF); }

/*
============
EntSelectSpawnPoint

Returns the entity to spawn at

USES AND SETS GLOBAL g_pLastSpawn
============
*/
CBaseEntity* SelectRandomSpawnPoint( CBaseEntity* pPlayer, const char* spawnPointName )
{
	edict_t *player = pPlayer->edict();

	CBaseEntity *pSpot = g_pLastSpawn;
	// Randomize the start spot
	for( int i = RANDOM_LONG( 1, 9 ); i > 0; i-- )
		pSpot = UTIL_FindEntityByClassname( pSpot, spawnPointName );
	if( !SpawnPointIsOn( pSpot ) )  // skip over the null point
		pSpot = UTIL_FindEntityByClassname( pSpot, spawnPointName );

	CBaseEntity *pFirstSpot = pSpot;

	do
	{
		if( pSpot )
		{
			// check if pSpot is valid
			if( IsSpawnPointValid( pPlayer, pSpot ) )
			{
				if( pSpot->pev->origin == Vector( 0, 0, 0 ) )
				{
					pSpot = UTIL_FindEntityByClassname( pSpot, spawnPointName );
					continue;
				}

				// if so, go to pSpot
				return pSpot;
			}
		}
		// increment pSpot
		pSpot = UTIL_FindEntityByClassname( pSpot, spawnPointName );
	} while( pSpot != pFirstSpot ); // loop if we're not back to the start

	// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
	if( SpawnPointIsOn( pSpot ) )
	{
		CBaseEntity *ent = NULL;
		while( ( ent = UTIL_FindEntityInSphere( ent, pSpot->pev->origin, 128 ) ) != NULL )
		{
			// if ent is a client, kill em (unless they are ourselves)
			if( ent->IsPlayer() && !(ent->edict() == player) )
				ent->TakeDamage( VARS( INDEXENT( 0 ) ), VARS( INDEXENT( 0 ) ), 300, DMG_GENERIC );
		}
		return pSpot;
	}
	return NULL;
}

edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer )
{
	CBaseEntity *pSpot;
	edict_t *player;

	int nNumRandomSpawnsToTry = 10;

	player = pPlayer->edict();

	// choose a info_player_deathmatch point
	if( g_pGameRules->IsCoOp() )
	{
		pSpot = SelectRandomSpawnPoint(pPlayer, "info_player_coop");
		if( pSpot )
			goto ReturnSpot;
	}
	if( g_pGameRules->IsDeathmatch() )
	{
		if( !g_pLastSpawn )
		{
			nNumRandomSpawnsToTry = 0;
			CBaseEntity* pEnt = 0;

			while( ( pEnt = UTIL_FindEntityByClassname( pEnt, "info_player_deathmatch" )))
				nNumRandomSpawnsToTry++;
		}

		pSpot = g_pLastSpawn;
		// Randomize the start spot
		for( int i = RANDOM_LONG( 1, nNumRandomSpawnsToTry - 1 ); i > 0; i-- )
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );
		if( FNullEnt( pSpot ) )  // skip over the null point
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );

		CBaseEntity *pFirstSpot = pSpot;

		do 
		{
			if( pSpot )
			{
				// check if pSpot is valid
				if( IsSpawnPointValid( pPlayer, pSpot ) )
				{
					if( pSpot->pev->origin == Vector( 0, 0, 0 ) )
					{
						pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );
						continue;
					}

					// if so, go to pSpot
					goto ReturnSpot;
				}
			}
			// increment pSpot
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );
		} while( pSpot != pFirstSpot ); // loop if we're not back to the start

		// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
		if( !FNullEnt( pSpot ) )
		{
			CBaseEntity *ent = NULL;
			while( ( ent = UTIL_FindEntityInSphere( ent, pSpot->pev->origin, 128 ) ) != NULL )
			{
				// if ent is a client, kill em (unless they are ourselves)
				if( ent->IsPlayer() && !(ent->edict() == player) )
					ent->TakeDamage( VARS( INDEXENT( 0 ) ), VARS( INDEXENT( 0 ) ), 300, DMG_GENERIC );
			}
			goto ReturnSpot;
		}
	}

	// If startspot is set, (re)spawn there.
	if( FStringNull( gpGlobals->startspot ) || (STRING( gpGlobals->startspot ) )[0] == '\0')
	{
		pSpot = UTIL_FindEntityByClassname( NULL, "info_player_start" );
		if( SpawnPointIsOn( pSpot ) )
			goto ReturnSpot;
	}
	else
	{
		pSpot = UTIL_FindEntityByTargetname( NULL, STRING( gpGlobals->startspot ) );
		if( !FNullEnt( pSpot ) )
			goto ReturnSpot;
	}

ReturnSpot:
	if( FNullEnt( pSpot ) )
	{
		ALERT( at_error, "PutClientInServer: no info_player_start on level\n" );
		return INDEXENT( 0 );
	}

	g_pLastSpawn = pSpot;
	return pSpot->edict();
}

void CBasePlayer::Spawn( void )
{
	m_flStartCharge = gpGlobals->time;
	pev->classname = MAKE_STRING( "player" );
	pev->armorvalue = 0;
	SetMaxArmor(g_modFeatures.MaxPlayerArmor());
	pev->takedamage = DAMAGE_AIM;
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_WALK;
	pev->health = pev->max_health = g_modFeatures.MaxPlayerHealth();
	pev->flags &= FL_PROXY;	// keep proxy flag sey by engine
	pev->flags |= FL_CLIENT;
	pev->air_finished = gpGlobals->time + 12;
	pev->dmg = 2;				// initial water damage
	pev->effects = 0;
	pev->deadflag = DEAD_NO;
	pev->dmg_take = 0;
	pev->dmg_save = 0;
	pev->friction = 1.0f;
	pev->gravity = 1.0f;
	m_bitsHUDDamage = -1;
	m_bitsDamageType = 0;
	m_afPhysicsFlags = 0;
	m_fLongJump = FALSE;// no longjump module. 

	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "slj", "0" );
	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "hl", "1" );
	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "fr", "1" );
	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "bj", sv_bunnyhop.value ? "1" : "0" );

	pev->fov = m_iFOV = 0;// init field of view.
	m_iClientFOV = -1; // make sure fov reset is sent
	m_ClientSndRoomtype = -1;

	m_flNextDecalTime = 0;// let this player decal as soon as he spawns.

	m_flgeigerDelay = gpGlobals->time + 2.0f;	// wait a few seconds until user-defined message registrations
							// are recieved by all clients

	m_flTimeStepSound = 0;
	m_iStepLeft = 0;
	SetMyFieldOfView(0.5f);// some monsters use this to determine whether or not the player is looking at them.

	m_bloodColor = BLOOD_COLOR_RED;
	m_flNextAttack = UTIL_WeaponTimeBase();

	m_iFlashBattery = 99;
	m_flFlashLightTime = 1; // force first message

	// dont let uninitialized value here hurt the player
	m_flFallVelocity = 0;

	g_pGameRules->SetDefaultPlayerTeam( this );
	g_pGameRules->GetPlayerSpawnSpot( this );

	SET_MODEL( ENT( pev ), "models/player.mdl" );
	g_ulModelIndexPlayer = pev->modelindex;
	pev->sequence = LookupActivity( ACT_IDLE );

	if( FBitSet( pev->flags, FL_DUCKING ) ) 
		UTIL_SetSize( pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX );
	else
		UTIL_SetSize( pev, VEC_HULL_MIN, VEC_HULL_MAX );

	pev->view_ofs = VEC_VIEW;
	Precache();
	m_HackedGunPos = Vector( 0, 32, 0 );

	if( m_iPlayerSound == SOUNDLIST_EMPTY )
	{
		ALERT( at_console, "Couldn't alloc player sound slot!\n" );
	}

	m_fNoPlayerSound = FALSE;// normal sound behavior.

	m_pLastItem = NULL;
	m_fInitHUD = TRUE;
	m_iClientHideHUD = -1;  // force this to be recalculated
	m_fWeapon = FALSE;
	m_pClientActiveItem = NULL;
	m_iClientBattery = -1;
	m_iClientMaxBattery = -1;

	// reset all ammo values to 0
	for( int i = 0; i < MAX_AMMO_TYPES; i++ )
	{
		m_rgAmmo[i] = 0;
		m_rgAmmoLast[i] = 0;  // client ammo values also have to be reset  (the death hud clear messages does on the client side)
	}

	m_lastx = m_lasty = 0;

	m_flNextChatTime = gpGlobals->time;
#if FEATURE_DISPLACER
	m_fInXen = FALSE;
#endif

	g_pGameRules->PlayerSpawn( this );
}

void CBasePlayer::Precache( void )
{
	// SOUNDS / MODELS ARE PRECACHED in ClientPrecache() (game specific)
	// because they need to precache before any clients have connected

	// init geiger counter vars during spawn and each time
	// we cross a level transition

	m_flgeigerRange = 1000;
	m_igeigerRangePrev = 1000;

	m_bitsDamageType = 0;
	m_bitsHUDDamage = -1;

	m_iClientBattery = -1;
	m_iClientMaxBattery = -1;

	m_flFlashLightTime = 1;

	m_iTrain |= TRAIN_NEW;

	// Make sure any necessary user messages have been registered
	LinkUserMessages();

	m_iUpdateTime = 5;  // won't update for 1/2 a second

	if( gInitHUD )
		m_fInitHUD = TRUE;

	pev->fov = m_iFOV;	// Vit_amiN: restore the FOV on level change or map/saved game load
	m_movementState = MovementStand;
}

int CBasePlayer::Save( CSave &save )
{
	if( !CBaseMonster::Save( save ) )
		return 0;

	return save.WriteFields( "PLAYER", this, m_playerSaveData, ARRAYSIZE( m_playerSaveData ) );
}

//
// Marks everything as new so the player will resend this to the hud.
//
void CBasePlayer::RenewItems( void )
{

}

int CBasePlayer::Restore( CRestore &restore )
{
	if( !CBaseMonster::Restore( restore ) )
		return 0;

	int status = restore.ReadFields( "PLAYER", this, m_playerSaveData, ARRAYSIZE( m_playerSaveData ) );

	SAVERESTOREDATA *pSaveData = (SAVERESTOREDATA *)gpGlobals->pSaveData;
	// landmark isn't present.
	if( !pSaveData->fUseLandmark )
	{
		ALERT( at_console, "No Landmark:%s\n", pSaveData->szLandmarkName );

		// default to normal spawn
		edict_t *pentSpawnSpot = EntSelectSpawnPoint( this );
		pev->origin = VARS( pentSpawnSpot )->origin + Vector( 0, 0, 1 );
		pev->angles = VARS( pentSpawnSpot )->angles;
	}
	pev->v_angle.z = 0;	// Clear out roll
	pev->angles = pev->v_angle;

	pev->fixangle = TRUE;           // turn this way immediately

	m_ClientSndRoomtype = -1;

	// Copied from spawn() for now
	m_bloodColor = BLOOD_COLOR_RED;

	g_ulModelIndexPlayer = pev->modelindex;

	if( FBitSet( pev->flags, FL_DUCKING ) )
	{
		// Use the crouch HACK
		//FixPlayerCrouchStuck( edict() );
		// Don't need to do this with new player prediction code.
		UTIL_SetSize( pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX );
	}
	else
	{
		UTIL_SetSize( pev, VEC_HULL_MIN, VEC_HULL_MAX );
	}

	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "hl", "1" );

	if( m_fLongJump )
	{
		g_engfuncs.pfnSetPhysicsKeyValue( edict(), "slj", "1" );
	}
	else
	{
		g_engfuncs.pfnSetPhysicsKeyValue( edict(), "slj", "0" );
	}

	SetPhysicsKeyValues();
	RenewItems();

#if CLIENT_WEAPONS
	// HACK:	This variable is saved/restored in CBaseMonster as a time variable, but we're using it
	//			as just a counter.  Ideally, this needs its own variable that's saved as a plain float.
	//			Barring that, we clear it out here instead of using the incorrect restored time value.
	m_flNextAttack = UTIL_WeaponTimeBase();
#endif
	if( m_flFlashLightTime == 0.0f )
		m_flFlashLightTime = 1.0f;

	m_bResetViewEntity = true;

	return status;
}

void CBasePlayer::SetPhysicsKeyValues()
{
	if( FBitSet(m_suppressedCapabilities, PLAYER_SUPPRESS_JUMP) )
	{
		g_engfuncs.pfnSetPhysicsKeyValue( edict(), "noj", "1" );
	}
	else
	{
		g_engfuncs.pfnSetPhysicsKeyValue( edict(), "noj", "0" );
	}
	if( FBitSet(m_suppressedCapabilities, PLAYER_SUPPRESS_DUCK) )
	{
		pev->iuser3 = 1;
	}
	else
	{
		pev->iuser3 = 0;
	}
	if( FBitSet(m_suppressedCapabilities, PLAYER_SUPPRESS_STEP_SOUND) )
	{
		g_engfuncs.pfnSetPhysicsKeyValue( edict(), "nos", "1" );
	}
	else
	{
		g_engfuncs.pfnSetPhysicsKeyValue( edict(), "nos", "0" );
	}
}

void CBasePlayer::SelectItem( const char *pstr )
{
	if( !pstr )
		return;

	CBasePlayerWeapon *pItem = NULL;

	for( int i = 0; i < MAX_WEAPONS; i++ )
	{
		pItem = m_rgpPlayerWeapons[i];

		if( pItem && FClassnameIs( pItem->pev, pstr ) )
			break;
	}

	if( !pItem || !pItem->CanDeploy() )
		return;

	if( pItem == m_pActiveItem )
		return;

	ResetAutoaim();

	// FIX, this needs to queue them up and delay
	if( m_pActiveItem )
		m_pActiveItem->Holster();

	m_pLastItem = m_pActiveItem;
	m_pActiveItem = pItem;

	if( m_pActiveItem )
	{
		m_pActiveItem->m_ForceSendAnimations = true;
		m_pActiveItem->Deploy();
		m_pActiveItem->m_ForceSendAnimations = false;
		m_pActiveItem->UpdateItemInfo();
	}
}

void CBasePlayer::SelectLastItem( void )
{
	if( !m_pLastItem || !m_pLastItem->CanDeploy() )
	{
		return;
	}

	if( m_pActiveItem && !m_pActiveItem->CanHolster() )
	{
		return;
	}

	ResetAutoaim();

	// FIX, this needs to queue them up and delay
	if( m_pActiveItem )
		m_pActiveItem->Holster();

	CBasePlayerWeapon *pTemp = m_pActiveItem;
	m_pActiveItem = m_pLastItem;
	m_pLastItem = pTemp;

	m_pActiveItem->m_ForceSendAnimations = true;
	m_pActiveItem->Deploy();
	m_pActiveItem->m_ForceSendAnimations = false;
	m_pActiveItem->UpdateItemInfo();
}

//==============================================
// HasWeapons - do I have any weapons at all?
//==============================================
BOOL CBasePlayer::HasWeapons( void )
{
	int i;

	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		if( m_rgpPlayerWeapons[i] )
		{
			return TRUE;
		}
	}

	return FALSE;
}

void CBasePlayer::SendCurWeaponClear()
{
	MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 0 );
		WRITE_SHORT( 0 );
	MESSAGE_END();
}

void CBasePlayer::SendCurWeaponDead()
{
	MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 0XFF );
		WRITE_SHORT( -1 );
	MESSAGE_END();
}

void CBasePlayer::SelectPrevItem( int iItem )
{
}

const char *CBasePlayer::TeamID( void )
{
	if( pev == NULL )		// Not fully connected yet
		return "";

	// return their team name
	return m_szTeamName;
}

//==============================================
// !!!UNDONE:ultra temporary SprayCan entity to apply
// decal frame at a time. For PreAlpha CD
//==============================================
class CSprayCan : public CBaseEntity
{
public:
	void Spawn( entvars_t *pevOwner );
	void Think( void );

	virtual int ObjectCaps( void ) { return FCAP_DONT_SAVE; }
};

void CSprayCan::Spawn( entvars_t *pevOwner )
{
	pev->origin = pevOwner->origin + Vector( 0, 0, 32 );
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT( pevOwner );
	pev->frame = 0;

	pev->nextthink = gpGlobals->time + 0.1f;
	EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/sprayer.wav", 1, ATTN_NORM );
}

void CSprayCan::Think( void )
{
	TraceResult tr;
	int playernum;
	int nFrames;
	CBasePlayer *pPlayer;

	pPlayer = (CBasePlayer *)GET_PRIVATE( pev->owner );

	if( pPlayer )
		nFrames = pPlayer->GetCustomDecalFrames();
	else
		nFrames = -1;

	playernum = ENTINDEX( pev->owner );

	// ALERT( at_console, "Spray by player %i, %i of %i\n", playernum, (int)( pev->frame + 1 ), nFrames );

	UTIL_MakeVectors( pev->angles );
	UTIL_TraceLine( pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, & tr );

	// No customization present.
	if( nFrames == -1 )
	{
		UTIL_DecalTrace( &tr, DECAL_LAMBDA6 );
		UTIL_Remove( this );
	}
	else
	{
		UTIL_PlayerDecalTrace( &tr, playernum, (int)pev->frame, TRUE );
		// Just painted last custom frame.
		if( pev->frame++ >= ( nFrames - 1 ) )
			UTIL_Remove( this );
	}

	pev->nextthink = gpGlobals->time + 0.1f;
}

class CBloodSplat : public CBaseEntity
{
public:
	void Spawn( entvars_t *pevOwner );
	void Spray( void );
};

void CBloodSplat::Spawn( entvars_t *pevOwner )
{
	pev->origin = pevOwner->origin + Vector( 0, 0, 32 );
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT( pevOwner );

	SetThink( &CBloodSplat::Spray );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CBloodSplat::Spray( void )
{
	TraceResult tr;	

	UTIL_MakeVectors( pev->angles );
	UTIL_TraceLine( pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, & tr );

	UTIL_BloodDecalTrace( &tr, BLOOD_COLOR_RED );

	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1f;
}

//==============================================
void CBasePlayer::GiveNamedItem(const char *pszName , int spawnFlags)
{
	edict_t	*pent;

	int istr = MAKE_STRING( pszName );

	pent = CREATE_NAMED_ENTITY( istr );
	if( FNullEnt( pent ) )
	{
		ALERT( at_console, "NULL Ent in GiveNamedItem!\n" );
		return;
	}
	VARS( pent )->origin = pev->origin;
	pent->v.spawnflags |= SF_NORESPAWN;
	pent->v.spawnflags |= spawnFlags;

	if ( DispatchSpawn( pent ) == -1 )
	{
		ALERT( at_console, "Game rejected to spawn '%s' (probably not enabled)\n", pszName );
		REMOVE_ENTITY(pent);
		return;
	}

	if (ItemsPickableByUse()) {
		CBaseEntity* entity = (CBaseEntity *)GET_PRIVATE( pent );
		if (entity) {
			entity->Use(this, this, USE_TOGGLE, 0.0f);
		}
	} else {
		DispatchTouch( pent, ENT( pev ) );
	}
}

CBaseEntity *FindEntityForward( CBaseEntity *pMe )
{
	TraceResult tr;

	UTIL_MakeVectors( pMe->pev->v_angle );
	UTIL_TraceLine( pMe->pev->origin + pMe->pev->view_ofs,pMe->pev->origin + pMe->pev->view_ofs + gpGlobals->v_forward * 8192,dont_ignore_monsters, pMe->edict(), &tr );
	if( tr.flFraction != 1.0f && !FNullEnt( tr.pHit ) )
	{
		CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
		return pHit;
	}
	return NULL;
}

void CBasePlayer::SuitLightTurnOn( void )
{
	if (HasFlashlight())
	{
		FlashlightTurnOn();
	}
	else if (HasNVG())
	{
		NVGTurnOn();
	}
}

void CBasePlayer::SuitLightTurnOff( bool playOffSound )
{
	NVGTurnOff( playOffSound );
	FlashlightTurnOff( playOffSound );
}

void CBasePlayer::UpdateSuitLightBattery(bool on)
{
	MESSAGE_BEGIN( MSG_ONE, gmsgFlashlight, NULL, pev );
		WRITE_BYTE( on ? 1 : 0 );
		WRITE_BYTE( m_iFlashBattery );
	MESSAGE_END();

	m_flFlashLightTime = gSkillData.flashlightDrainTime/100 + gpGlobals->time;
}

void CBasePlayer::FlashlightToggle()
{
	if (FlashlightIsOn())
	{
		FlashlightTurnOff();
	}
	else
	{
		FlashlightTurnOn();
	}
}

void CBasePlayer::FlashlightTurnOn()
{
	if( !HasFlashlight() || !g_pGameRules->FAllowFlashlight() )
	{
		return;
	}

	if (!FlashlightIsOn())
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, SOUND_FLASHLIGHT_ON, 1.0, ATTN_NORM, 0, PITCH_NORM );
		SetBits( pev->effects, EF_DIMLIGHT );

		NVGTurnOff(false);
		UpdateSuitLightBattery(true);
	}
}

void CBasePlayer::FlashlightTurnOff( bool playOffSound )
{
	if (FlashlightIsOn())
	{
		if (playOffSound)
			EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, SOUND_FLASHLIGHT_OFF, 1.0, ATTN_NORM, 0, PITCH_NORM );

		ClearBits( pev->effects, EF_DIMLIGHT );
		UpdateSuitLightBattery(false);
	}
}

void CBasePlayer::NVGToggle()
{
	if (NVGIsOn())
	{
		NVGTurnOff();
	}
	else
	{
		NVGTurnOn();
	}
}

void CBasePlayer::NVGTurnOn()
{
	if( !HasNVG() || !g_pGameRules->FAllowFlashlight() )
	{
		return;
	}

	if (!m_fNVGisON)
	{
		if (*g_modFeatures.nvg_sound_on)
			EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, g_modFeatures.nvg_sound_on, 1.0, ATTN_NORM, 0, PITCH_NORM );

		m_fNVGisON = TRUE;
		MESSAGE_BEGIN( MSG_ONE, gmsgNightvision, NULL, pev );
			WRITE_BYTE( 1 );
		MESSAGE_END();

		FlashlightTurnOff(false);
		UpdateSuitLightBattery(true);
	}
}

void CBasePlayer::NVGTurnOff(bool playOffSound)
{
	if (m_fNVGisON)
	{
		if (playOffSound)
		{
			if (*g_modFeatures.nvg_sound_off)
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, g_modFeatures.nvg_sound_off, 1.0, ATTN_NORM, 0, PITCH_NORM );
		}

		m_fNVGisON = FALSE;
		MESSAGE_BEGIN( MSG_ONE, gmsgNightvision, NULL, pev );
			WRITE_BYTE( 0 );
		MESSAGE_END();

		UpdateSuitLightBattery(false);
	}
}

/*
===============
ForceClientDllUpdate

When recording a demo, we need to have the server tell us the entire client state
so that the client side .dll can behave correctly.
Reset stuff so that the state is transmitted.
===============
*/
void CBasePlayer::ForceClientDllUpdate( void )
{
	m_iClientHealth = -1;
	m_iClientMaxHealth = -1;
	m_iClientBattery = -1;
	m_iClientMaxBattery = -1;
	m_iClientHideHUD = -1;	// Vit_amiN: forcing to update
	m_iClientFOV = -1;	// Vit_amiN: force client weapons to be sent
	m_ClientSndRoomtype = -1;
	m_iTrain |= TRAIN_NEW;  // Force new train message.
	m_fWeapon = FALSE;          // Force weapon send
	m_fKnownItem = FALSE;    // Force weaponinit messages.
	m_fInitHUD = TRUE;		// Force HUD gmsgResetHUD message
	memset( m_rgAmmoLast, 0, sizeof( m_rgAmmoLast )); // a1ba: Force update AmmoX
	m_iClientItemsBits = 0;

	// Now force all the necessary messages
	//  to be sent.
	UpdateClientData();
}

/*
============
ImpulseCommands
============
*/
extern cvar_t *g_enable_cheats;

void CBasePlayer::ImpulseCommands()
{
	TraceResult tr;// UNDONE: kill me! This is temporary for PreAlpha CDs

	// Handle use events
	PlayerUse();

	int iImpulse = (int)pev->impulse;
	switch( iImpulse )
	{
	case 99:
		int iOn;

		if( !gmsgLogo )
		{
			iOn = 1;
			gmsgLogo = REG_USER_MSG( "Logo", 1 );
		} 
		else 
		{
			iOn = 0;
		}

		ASSERT( gmsgLogo > 0 );

		// send "health" update message
		MESSAGE_BEGIN( MSG_ONE, gmsgLogo, NULL, pev );
			WRITE_BYTE( iOn );
		MESSAGE_END();

		if(!iOn)
			gmsgLogo = 0;
		break;
	case 100:
		// temporary flashlight for level designers
		if (HasNVG() && HasFlashlight()) // if player has both, this command effects flashlight
		{
			if( FlashlightIsOn() )
				FlashlightTurnOff();
			else
				FlashlightTurnOn();
		}
		else
		{
			if( SuitLightIsOn() )
				SuitLightTurnOff();
			else
				SuitLightTurnOn();
		}
		break;
	case 201:
		// paint decal
		if( gpGlobals->time < m_flNextDecalTime )
		{
			// too early!
			break;
		}

		UTIL_MakeVectors( pev->v_angle );
		UTIL_TraceLine( pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, ENT( pev ), &tr );

		if( tr.flFraction != 1.0f )
		{
			// line hit something, so paint a decal
			m_flNextDecalTime = gpGlobals->time + decalfrequency.value;
			CSprayCan *pCan = GetClassPtr( (CSprayCan *)NULL );
			pCan->Spawn( pev );
		}
		break;
	default:
		// check all of the cheat impulse commands now
		CheatImpulseCommands( iImpulse );
		break;
	}

	pev->impulse = 0;
}

//=========================================================
//=========================================================
void CBasePlayer::CheatImpulseCommands( int iImpulse )
{
	if( g_enable_cheats->value == 0 )
	{
		return;
	}

	CBaseEntity *pEntity;
	TraceResult tr;

	switch( iImpulse )
	{
	case 76:
		if( !giPrecacheGrunt )
		{
			giPrecacheGrunt = 1;
			ALERT( at_console, "You must now restart to use Grunt-o-matic.\n" );
		}
		else
		{
			UTIL_MakeVectors( Vector( 0, pev->v_angle.y, 0 ) );
			Create( "monster_human_grunt", pev->origin + gpGlobals->v_forward * 128, pev->angles );
		}
		break;
	case 101:
		gEvilImpulse101 = TRUE;
		GiveNamedItem( "item_suit", SF_ITEM_NOFALL );
		SetDefaultLight();
		GiveNamedItem( "item_battery", SF_ITEM_NOFALL );
		GiveNamedItem( "weapon_crowbar" );
		GiveNamedItem( "weapon_9mmhandgun" );
		GiveAmmo(AMMO_GLOCKCLIP_GIVE, "9mm"); //GiveNamedItem( "ammo_9mmclip" );
		GiveNamedItem( "weapon_shotgun" );
		GiveAmmo(AMMO_BUCKSHOTBOX_GIVE, "buckshot"); //GiveNamedItem( "ammo_buckshot" );
		GiveNamedItem( "weapon_9mmAR" );
		GiveAmmo(AMMO_MP5CLIP_GIVE, "9mm"); //GiveNamedItem( "ammo_9mmAR" );
		GiveAmmo(AMMO_M203BOX_GIVE, "ARgrenades"); //GiveNamedItem( "ammo_ARgrenades" );
		GiveNamedItem( "weapon_handgrenade" );
		GiveNamedItem( "weapon_tripmine" );
		GiveNamedItem( "weapon_357" );
		GiveAmmo(AMMO_357BOX_GIVE, "357"); //GiveNamedItem( "ammo_357" );
		GiveNamedItem( "weapon_crossbow" );
		GiveAmmo(AMMO_CROSSBOWCLIP_GIVE, "bolts"); //GiveNamedItem( "ammo_crossbow" );
		GiveNamedItem( "weapon_egon" );
		GiveNamedItem( "weapon_gauss" );
		GiveAmmo(AMMO_URANIUMBOX_GIVE, "uranium"); //GiveNamedItem( "ammo_gaussclip" );
		GiveNamedItem( "weapon_rpg" );
		GiveAmmo(AMMO_RPGCLIP_GIVE, "rockets"); //GiveNamedItem( "ammo_rpgclip" );
		GiveNamedItem( "weapon_satchel" );
		GiveNamedItem( "weapon_snark" );
		GiveNamedItem( "weapon_hornetgun" );
#if FEATURE_MEDKIT
		if (g_modFeatures.IsWeaponEnabled(WEAPON_MEDKIT))
			GiveNamedItem( "weapon_medkit" );
#endif
#if FEATURE_DESERT_EAGLE
		if (g_modFeatures.IsWeaponEnabled(WEAPON_EAGLE))
			GiveNamedItem( "weapon_eagle" );
#endif
#if FEATURE_PIPEWRENCH
		if (g_modFeatures.IsWeaponEnabled(WEAPON_PIPEWRENCH))
			GiveNamedItem( "weapon_pipewrench" );
#endif
#if FEATURE_GRAPPLE
		if (g_modFeatures.IsWeaponEnabled(WEAPON_GRAPPLE))
			GiveNamedItem( "weapon_grapple" );
#endif
#if FEATURE_M249
		if (g_modFeatures.IsWeaponEnabled(WEAPON_M249))
		{
			GiveNamedItem( "weapon_m249" );
			GiveAmmo(AMMO_556CLIP_GIVE, "556");
		}
#endif
#if FEATURE_SNIPERRIFLE
		if (g_modFeatures.IsWeaponEnabled(WEAPON_SNIPERRIFLE))
		{
			GiveNamedItem( "weapon_sniperrifle" );
			GiveAmmo( AMMO_762BOX_GIVE, "762");
		}
#endif
#if FEATURE_DISPLACER
		if (g_modFeatures.IsWeaponEnabled(WEAPON_DISPLACER))
			GiveNamedItem( "weapon_displacer" );
#endif
#if FEATURE_SHOCKRIFLE
		if (g_modFeatures.IsWeaponEnabled(WEAPON_SHOCKRIFLE))
			GiveNamedItem( "weapon_shockrifle" );
#endif
#if FEATURE_SPORELAUNCHER
		if (g_modFeatures.IsWeaponEnabled(WEAPON_SPORELAUNCHER))
		{
			GiveNamedItem( "weapon_sporelauncher" );
			GiveAmmo(5, "spores");
		}
#endif
#if FEATURE_KNIFE
		if (g_modFeatures.IsWeaponEnabled(WEAPON_KNIFE))
			GiveNamedItem( "weapon_knife" );
#endif
#if FEATURE_PENGUIN
		if (g_modFeatures.IsWeaponEnabled(WEAPON_PENGUIN))
			GiveNamedItem( "weapon_penguin" );
#endif
#if FEATURE_UZI
		if (g_modFeatures.IsWeaponEnabled(WEAPON_UZI))
			GiveNamedItem( "weapon_uzi" );
#endif
		gEvilImpulse101 = FALSE;
		break;
	case 102:
		// Gibbage!!!
		CGib::SpawnHumanGibs( pev, 1 );
		break;
	case 103:
		// What the hell are you doing?
		pEntity = FindEntityForward( this );
		if( pEntity )
		{
			CBaseMonster *pMonster = pEntity->MyMonsterPointer();
			if( pMonster )
			{
				pMonster->ReportAIState(at_console);
				ALERT(at_console, "\n\n");
			}
		}
		break;
	case 104:
		// Dump all of the global state varaibles (and global entity names)
		gGlobalState.DumpGlobals();
		break;
	case 105:// player makes no sound for monsters to hear.
		if( m_fNoPlayerSound )
		{
			ALERT( at_console, "Player is audible\n" );
			m_fNoPlayerSound = FALSE;
		}
		else
		{
			ALERT( at_console, "Player is silent\n" );
			m_fNoPlayerSound = TRUE;
		}
		break;
	case 106:
		// Give me the classname and targetname of this entity.
		pEntity = FindEntityForward( this );
		if( pEntity )
		{
			ALERT( at_console, "Classname: %s", STRING( pEntity->pev->classname ) );

			if( !FStringNull( pEntity->pev->targetname ) )
			{
				ALERT( at_console, " - Targetname: %s\n", STRING( pEntity->pev->targetname ) );
			}
			else
			{
				ALERT( at_console, " - TargetName: No Targetname\n" );
			}

			ALERT( at_console, "Model: %s\n", STRING( pEntity->pev->model ) );
			if( pEntity->pev->globalname )
				ALERT( at_console, "Globalname: %s\n", STRING( pEntity->pev->globalname ) );
		}
		break;
	case 107:
		{
			//TraceResult tr;

			edict_t *pWorld = g_engfuncs.pfnPEntityOfEntIndex( 0 );

			Vector start = pev->origin + pev->view_ofs;
			Vector end = start + gpGlobals->v_forward * 1024;
			UTIL_TraceLine( start, end, ignore_monsters, edict(), &tr );
			if( tr.pHit )
				pWorld = tr.pHit;
			const char *pTextureName = TRACE_TEXTURE( pWorld, start, end );
			if( pTextureName )
				ALERT( at_console, "Texture: %s\n", pTextureName );
		}
		break;
	case 195:
		// show shortest paths for entire level to nearest node
		{
			Create( "node_viewer_fly", pev->origin, pev->angles );
		}
		break;
	case 196:
		// show shortest paths for entire level to nearest node
		{
			Create( "node_viewer_large", pev->origin, pev->angles );
		}
		break;
	case 197:
		// show shortest paths for entire level to nearest node
		{
			Create( "node_viewer_human", pev->origin, pev->angles );
		}
		break;
	case 199:
		// show nearest node and all connections
		{
			ALERT( at_console, "%d\n", WorldGraph.FindNearestNode( pev->origin, bits_NODE_GROUP_REALM ) );
			WorldGraph.ShowNodeConnections( WorldGraph.FindNearestNode( pev->origin, bits_NODE_GROUP_REALM ) );
		}
		break;
	case 202:
		// Random blood splatter
		UTIL_MakeVectors( pev->v_angle );
		UTIL_TraceLine( pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, ENT( pev ), &tr );

		if( tr.flFraction != 1.0f )
		{
			// line hit something, so paint a decal
			CBloodSplat *pBlood = GetClassPtr( (CBloodSplat *)NULL );
			pBlood->Spawn( pev );
		}
		break;
	case 203:
		// remove creature.
		pEntity = FindEntityForward( this );
		if( pEntity )
		{
			if( pEntity->pev->takedamage )
				pEntity->SetThink( &CBaseEntity::SUB_Remove );
		}
		break;
	}
}

//
// Add a weapon to the player (Item == Weapon == Selectable Object)
//
int CBasePlayer::AddPlayerItem( CBasePlayerWeapon *pItem )
{
	if( pev->flags & FL_SPECTATOR )
		return FALSE;

	CBasePlayerWeapon *pInsert = WeaponById(pItem->m_iId);
	if( pInsert )
	{
		if (pItem->AddDuplicate( pInsert ))
		{
			g_pGameRules->PlayerGotWeapon( this, pItem );
			pItem->CheckRespawn();

			// ugly hack to update clip w/o an update clip message
			pInsert->UpdateItemInfo();
			if( m_pActiveItem )
				m_pActiveItem->UpdateItemInfo();

			pItem->Kill();
		}
		else if( gEvilImpulse101 )
		{
			// FIXME: remove anyway for deathmatch testing
			pItem->Kill();
		}
		return GOT_DUP_ITEM;
	}

	if( pItem->AddToPlayer( this ) )
	{
		g_pGameRules->PlayerGotWeapon( this, pItem );
		pItem->CheckRespawn();

		InsertWeaponById(pItem);

		// should we switch to this item?
		if( g_pGameRules->FShouldSwitchWeapon( this, pItem ) )
		{
			SwitchWeapon( pItem );
		}

		pItem->AttachToPlayer(this);
		return GOT_NEW_ITEM;
	}
	else if( gEvilImpulse101 )
	{
		// FIXME: remove anyway for deathmatch testing
		pItem->Kill();
	}
	return DID_NOT_GET_ITEM;
}

BOOL CBasePlayer::RemovePlayerItem( CBasePlayerWeapon *pItem, bool bCallHolster )
{
	pItem->pev->nextthink = 0;// crowbar may be trying to swing again, etc.
	pItem->SetThink( NULL );

	if( m_pActiveItem == pItem )
	{
		ResetAutoaim();
		if( bCallHolster )
			pItem->Holster();
		m_pActiveItem = NULL;
		pev->viewmodel = 0;
		pev->weaponmodel = 0;
	}

	// In some cases an item can be both the active and last item, like for instance dropping all weapons and only having an exhaustible weapon left. - Solokiller
	if( m_pLastItem == pItem )
		m_pLastItem = NULL;

	if (WeaponById(pItem->m_iId))
	{
		m_rgpPlayerWeapons[pItem->m_iId-1] = NULL;
		return TRUE;
	}
	return FALSE;
}

//
// Returns the unique ID for the ammo, or -1 if error
//
int CBasePlayer::GiveAmmo(int iCount, const char *szName)
{
	const AmmoType* ammoType = CBasePlayerWeapon::GetAmmoType(szName);
	if (!ammoType)
		return -1;

	if( !g_pGameRules->CanHaveAmmo( this, ammoType->name ) )
	{
		// game rules say I can't have any more of this ammo type.
		return -1;
	}

	int i = ammoType->id;

	int iAdd = Q_min( iCount, ammoType->maxAmmo - m_rgAmmo[i] );
	if( iAdd < 1 )
		return i;

	// horrific HACK to give player an exhaustible weapon as a real weapon, not just ammo
	bool addedAsWeapon = false;
	if (ammoType->exhaustible)
	{
		for (int j=0; j<MAX_WEAPONS; ++j) {
			const ItemInfo& II = CBasePlayerWeapon::ItemInfoArray[j];
			if ((II.iFlags & ITEM_FLAG_EXHAUSTIBLE) && II.pszAmmo1 && FStrEq(szName, II.pszAmmo1)) {
				// we found a weapon that uses this ammo type

				int weaponId = II.iId;
				const char* weaponName = II.pszName;

				if ((pev->weapons & (1 << weaponId)) == 0) {
					// player does not have this weapon
					CBaseEntity* pCreated = Create(weaponName, pev->origin, pev->angles);
					if (pCreated)
					{
						CBasePlayerWeapon* weapon = pCreated->MyWeaponPointer();
						if (weapon) {
							weapon->pev->spawnflags |= SF_NORESPAWN;
							weapon->m_iDefaultAmmo = iAdd;
							if (AddPlayerItem(weapon)) {
								addedAsWeapon = true;
							}
							else
								UTIL_Remove(weapon);
						}
					}
					else
					{
						ALERT(at_console, "GiveAmmo: expected weapon, but created entity is not a weapon\n");
						UTIL_Remove(pCreated);
					}
				}

				break;
			}
		}
	}

	if (!addedAsWeapon)
	{
		m_rgAmmo[i] += iAdd;
		if( gmsgAmmoPickup )  // make sure the ammo messages have been linked first
		{
			// Send the message that ammo has been picked up
			MESSAGE_BEGIN( MSG_ONE, gmsgAmmoPickup, NULL, pev );
				WRITE_BYTE( i );		// ammo ID
				WRITE_SHORT( iAdd );		// amount
			MESSAGE_END();
		}
	}

	return i;
}

void CBasePlayer::RemoveAmmo(int iAmount, const char *szName)
{
	const AmmoType* ammoType = CBasePlayerWeapon::GetAmmoType(szName);
	if (!ammoType)
		return;

	int i = ammoType->id;
	m_rgAmmo[i] -= iAmount;
	if (m_rgAmmo[i] < 0)
		m_rgAmmo[i] = 0;

	if (m_rgAmmo[i] <= 0 && ammoType->exhaustible)
	{
		for (int j=0; j<MAX_WEAPONS; ++j)
		{
			const ItemInfo& II = CBasePlayerWeapon::ItemInfoArray[j];
			if ((II.iFlags & ITEM_FLAG_EXHAUSTIBLE) && II.pszAmmo1 && FStrEq(szName, II.pszAmmo1)) {
				// we found a weapon that uses this ammo type
				CBasePlayerWeapon* pWeapon = WeaponById(j);
				if (pWeapon)
					pWeapon->RetireWeapon();
			}
		}
	}
}

/*
============
ItemPreFrame

Called every frame by the player PreThink
============
*/
void CBasePlayer::ItemPreFrame()
{
#if CLIENT_WEAPONS
	if( m_flNextAttack > 0 )
#else
	if( gpGlobals->time < m_flNextAttack )
#endif
	{
		return;
	}

	if( !m_pActiveItem )
		return;

	m_pActiveItem->ItemPreFrame();
}

/*
============
ItemPostFrame

Called every frame by the player PostThink
============
*/
void CBasePlayer::ItemPostFrame()
{
	//static int fInSelect = FALSE;

	// check if the player is using a tank
	if( m_pTank != 0 )
		return;

	ImpulseCommands();

#if CLIENT_WEAPONS
	if( m_flNextAttack > 0 )
#else
	if( gpGlobals->time < m_flNextAttack )
#endif
	{
		return;
	}

	if( !m_pActiveItem )
		return;

	m_pActiveItem->ItemPostFrame();
}

int CBasePlayer::AmmoInventory( int iAmmoIndex )
{
	if( iAmmoIndex == -1 )
	{
		return -1;
	}

	return m_rgAmmo[iAmmoIndex];
}

int CBasePlayer::GetAmmoIndex( const char *psz )
{
	return g_AmmoRegistry.IndexOf(psz);
}

// Called from UpdateClientData
// makes sure the client has all the necessary ammo info,  if values have changed
void CBasePlayer::SendAmmoUpdate( void )
{
	for( int i = 0; i < MAX_AMMO_TYPES; i++ )
	{
		if( m_rgAmmo[i] != m_rgAmmoLast[i] )
		{
			m_rgAmmoLast[i] = m_rgAmmo[i];

			const int maxPossibleAmmoCount = (1 << 15) - 1;
			ASSERT( m_rgAmmo[i] >= 0 );
			ASSERT( m_rgAmmo[i] <= maxPossibleAmmoCount );

			// send "Ammo" update message
			MESSAGE_BEGIN( MSG_ONE, gmsgAmmoX, NULL, pev );
				WRITE_BYTE( i );
				WRITE_SHORT( Q_max( Q_min( m_rgAmmo[i], maxPossibleAmmoCount ), 0 ) );
			MESSAGE_END();
		}
	}
}

static void SendParseErrorsToClient(edict_t* client, const std::string& str)
{
	char chunk[121];
	const char* cstr = str.c_str();
	size_t offset = 0;
	while (offset <= str.size())
	{
		strncpy(chunk, cstr + offset, sizeof(chunk)-1);
		chunk[sizeof(chunk)-1] = '\0';
		offset += sizeof(chunk)-1;

		MESSAGE_BEGIN( MSG_ONE, gmsgParseErrors, NULL, client );
			WRITE_BYTE( offset >= str.size() ? TRUE : FALSE );
			WRITE_STRING( chunk );
		MESSAGE_END();
	}
}

/*
=========================================================
	UpdateClientData

resends any changed player HUD info to the client.
Called every frame by PlayerPreThink
Also called at start of demo recording and playback by
ForceClientDllUpdate to ensure the demo gets messages
reflecting all of the HUD state info.
=========================================================
*/
void CBasePlayer::UpdateClientData( void )
{
	if( m_fInitHUD )
	{
		m_fInitHUD = FALSE;
		gInitHUD = FALSE;

		MESSAGE_BEGIN( MSG_ONE, gmsgResetHUD, NULL, pev );
			WRITE_BYTE( 0 );
		MESSAGE_END();

		if( !m_fGameHUDInitialized )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgInitHUD, NULL, pev );
			MESSAGE_END();

			g_pGameRules->InitHUD( this );

			for (int i=0; i<ARRAYSIZE(m_inventoryItems); ++i)
			{
				if (!FStringNull(m_inventoryItems[i]))
				{
					MESSAGE_BEGIN(MSG_ONE, gmsgInventory, NULL, pev);
						WRITE_SHORT(m_inventoryItemCounts[i]);
						WRITE_STRING(STRING(m_inventoryItems[i]));
						WRITE_BYTE(INVENTORY_DONT_SHOW_IN_HISTORY);
					MESSAGE_END();
				}
			}

			if ( g_psv_developer->value > 0 && !g_pGameRules->IsMultiplayer() || (entindex() == 1 && !IS_DEDICATED_SERVER()) )
			{
				if (g_errorCollector.HasErrors())
				{
					std::string fullErrorString = g_errorCollector.GetFullString();
					SendParseErrorsToClient(edict(), fullErrorString);
				}
			}

			m_fGameHUDInitialized = TRUE;

			m_iObserverLastMode = OBS_ROAMING;

			if( g_pGameRules->IsMultiplayer() )
			{
				FireTargets( "game_playerjoin", this, this );
			}
		}

		if( g_pGameRules->IsMultiplayer() )
			FireTargets( "game_playerspawn", this, this );

		// Send flashlight status
		MESSAGE_BEGIN( MSG_ONE, gmsgFlashlight, NULL, pev );
			WRITE_BYTE( SuitLightIsOn() ? 1 : 0 );
			WRITE_BYTE( m_iFlashBattery );
		MESSAGE_END();

		if (HasNVG())
		{
			// Send Nightvision Off message.
			MESSAGE_BEGIN( MSG_ONE, gmsgNightvision, NULL, pev );
				WRITE_BYTE( NVGIsOn() ? 1 : 0 );
			MESSAGE_END();
		}

		// Vit_amiN: the geiger state could run out of sync, too
		MESSAGE_BEGIN( MSG_ONE, gmsgGeigerRange, NULL, pev );
			WRITE_BYTE( 0 );
		MESSAGE_END();

		InitStatusBar();
	}

	if( m_iHideHUD != m_iClientHideHUD )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgHideWeapon, NULL, pev );
			WRITE_BYTE( m_iHideHUD );
		MESSAGE_END();

		m_iClientHideHUD = m_iHideHUD;
	}

	if( m_iFOV != m_iClientFOV )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
			WRITE_BYTE( m_iFOV );
		MESSAGE_END();

		// cache FOV change at end of function, so weapon updates can see that FOV has changed
	}

	// HACKHACK -- send the message to display the game title
	if( gDisplayTitle )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgShowGameTitle, NULL, pev );
		WRITE_BYTE( 0 );
		MESSAGE_END();
		gDisplayTitle = 0;
	}

	if( pev->health != m_iClientHealth || (int)pev->max_health != m_iClientMaxHealth )
	{
#define clamp( val, min, max ) ( ((val) > (max)) ? (max) : ( ((val) < (min)) ? (min) : (val) ) )
		int iHealth = clamp( pev->health, 0, 9999 ); // make sure that no negative health values are sent
		if( pev->health > 0.0f && pev->health <= 1.0f )
			iHealth = 1;

		// send "health" update message
		MESSAGE_BEGIN( MSG_ONE, gmsgHealth, NULL, pev );
			WRITE_SHORT( iHealth );
			WRITE_SHORT( (int)pev->max_health );
		MESSAGE_END();

		m_iClientHealth = (int)pev->health;
		m_iClientMaxHealth = (int)pev->max_health;
	}

	const int maxArmor = MaxArmor();
	if( (int)pev->armorvalue != m_iClientBattery || maxArmor != m_iClientMaxBattery )
	{
		m_iClientBattery = (int)pev->armorvalue;
		m_iClientMaxBattery = maxArmor;

		ASSERT( gmsgBattery > 0 );

		// send "health" update message
		MESSAGE_BEGIN( MSG_ONE, gmsgBattery, NULL, pev );
			WRITE_SHORT( (int)pev->armorvalue );
			WRITE_SHORT( maxArmor );
		MESSAGE_END();
	}

	if (m_iItemsBits != m_iClientItemsBits)
	{
		m_iClientItemsBits = m_iItemsBits;
		MESSAGE_BEGIN( MSG_ONE, gmsgItems, NULL, pev );
		WRITE_LONG( m_iItemsBits );
		MESSAGE_END();
	}

	if( pev->dmg_take || pev->dmg_save || m_bitsHUDDamage != m_bitsDamageType )
	{
		// Comes from inside me if not set
		Vector damageOrigin = pev->origin;
		// send "damage" message
		// causes screen to flash, and pain compass to show direction of damage
		edict_t *other = pev->dmg_inflictor;
		if( other )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( other );
			if( pEntity )
				damageOrigin = pEntity->Center();
		}

		// only send down damage type that have hud art
		int visibleDamageBits = m_bitsDamageType & DMG_SHOWNHUD;

		MESSAGE_BEGIN( MSG_ONE, gmsgDamage, NULL, pev );
			WRITE_BYTE( (int)pev->dmg_save );
			WRITE_BYTE( (int)pev->dmg_take );
			WRITE_LONG( visibleDamageBits );
			WRITE_VECTOR( damageOrigin );
		MESSAGE_END();

		pev->dmg_take = 0;
		pev->dmg_save = 0;
		m_bitsHUDDamage = m_bitsDamageType;

		// Clear off non-time-based damage indicators
		m_bitsDamageType &= DMG_TIMEBASED;
	}

	// Update Flashlight
	if( ( m_flFlashLightTime ) && ( m_flFlashLightTime <= gpGlobals->time ) )
	{
		if( SuitLightIsOn() )
		{
			if( m_iFlashBattery )
			{
				m_flFlashLightTime = gSkillData.flashlightDrainTime/100 + gpGlobals->time;
				m_iFlashBattery--;

				if( !m_iFlashBattery )
					SuitLightTurnOff();
			}
		}
		else
		{
			if( m_iFlashBattery < 100 )
			{
				m_flFlashLightTime = gSkillData.flashlightChargeTime/100 + gpGlobals->time;
				m_iFlashBattery++;
			}
			else
				m_flFlashLightTime = 0;
		}

		MESSAGE_BEGIN( MSG_ONE, gmsgFlashBattery, NULL, pev );
			WRITE_BYTE( m_iFlashBattery );
		MESSAGE_END();
	}

	if( m_iTrain & TRAIN_NEW )
	{
		ASSERT( gmsgTrain > 0 );

		// send "health" update message
		MESSAGE_BEGIN( MSG_ONE, gmsgTrain, NULL, pev );
			WRITE_BYTE( m_iTrain & 0xF );
		MESSAGE_END();

		m_iTrain &= ~TRAIN_NEW;
	}

	//
	// New Weapon?
	//
	if( !m_fKnownItem )
	{
		m_fKnownItem = TRUE;

	// WeaponInit Message
	// byte  = # of weapons
	//
	// for each weapon:
	// byte		name str length (not including null)
	// bytes... name
	// byte		Ammo Type
	// byte		Ammo2 Type
	// byte		bucket
	// byte		bucket pos
	// byte		flags
	// ????		Icons

		// Send ALL the weapon info now
		int i;

		for (i = 1; i < MAX_AMMO_TYPES; ++i)
		{
			const AmmoType* ammoType = g_AmmoRegistry.GetByIndex(i);
			if (ammoType)
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgAmmoList, NULL, pev );
					WRITE_STRING( ammoType->name );
					WRITE_SHORT( ammoType->maxAmmo );
					WRITE_BYTE( ammoType->id | (ammoType->exhaustible ? AMMO_EXHAUSTIBLE_NETWORK_BIT : 0) );
				MESSAGE_END();
			}
		}

		for( i = 0; i < MAX_WEAPONS; i++ )
		{
			ItemInfo& II = CBasePlayerWeapon::ItemInfoArray[i];

			if( !II.iId )
				continue;

			const char *pszName;
			if( !II.pszName )
				pszName = "Empty";
			else
				pszName = II.pszName;

			MESSAGE_BEGIN( MSG_ONE, gmsgWeaponList, NULL, pev );  
				WRITE_STRING( pszName );			// string	weapon name
				WRITE_BYTE( GetAmmoIndex( II.pszAmmo1 ) );	// byte		Ammo Type
				WRITE_BYTE( GetAmmoIndex( II.pszAmmo2 ) );	// byte		Ammo2 Type
				WRITE_BYTE( II.iSlot );					// byte		bucket
				WRITE_BYTE( II.iPosition );				// byte		bucket pos
				WRITE_BYTE( II.iId );						// byte		id (bit index into pev->weapons)
				WRITE_BYTE( II.iFlags );					// byte		Flags
			MESSAGE_END();
		}
	}

	SendAmmoUpdate();

	// Update all the items
	for( int i = 0; i < MAX_WEAPONS; i++ )
	{
		if( m_rgpPlayerWeapons[i] )  // each item updates it's successors
			m_rgpPlayerWeapons[i]->UpdateClientData( this );
	}

	// Cache and client weapon change
	m_pClientActiveItem = m_pActiveItem;
	m_iClientFOV = m_iFOV;

	// Update Status Bar
	if( m_flNextSBarUpdateTime < gpGlobals->time )
	{
		UpdateStatusBar();
		m_flNextSBarUpdateTime = gpGlobals->time + 0.2f;
	}

	// Send new room type to client.
	if (m_ClientSndRoomtype != m_SndRoomtype)
	{
		m_ClientSndRoomtype = m_SndRoomtype;

		MESSAGE_BEGIN(MSG_ONE, SVC_ROOMTYPE, NULL, edict());
		WRITE_SHORT((short)m_SndRoomtype); // sequence number
		MESSAGE_END();
	}

	if ( !m_bSentMessages && g_PlayerFullyInitialized[ENTINDEX(edict())-1] )
	{
		if (!FStringNull(m_loopedMp3) && gmsgPlayMP3)
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgPlayMP3, NULL, pev );
				WRITE_STRING( STRING( m_loopedMp3 ) );
				WRITE_BYTE( 1 );
			MESSAGE_END();
		}

		m_bSentMessages = true;

		const int startingIndex = 1;
		edict_t *pEdict = g_engfuncs.pfnPEntityOfEntOffset( 0 ) + startingIndex;
		if (pEdict)
		{
			for( int i = startingIndex; i < gpGlobals->maxEntities; i++, pEdict++ )
			{
				if (FNullEnt(pEdict) || pEdict->free || FBitSet(pEdict->v.flags, FL_CLIENT | FL_KILLME) || FStringNull(pEdict->v.classname))
					continue;
				CBaseEntity* pEntity = CBaseEntity::Instance(pEdict);
				if (pEntity)
				{
					pEntity->SendMessages(this);
				}
			}
		}
	}
}

//=========================================================
// FBecomeProne - Overridden for the player to set the proper
// physics flags when a barnacle grabs player.
//=========================================================
BOOL CBasePlayer::FBecomeProne( void )
{
	m_afPhysicsFlags |= PFLAG_ONBARNACLE;

#if FEATURE_ROPE
	if( (m_afPhysicsFlags & PFLAG_ONROPE) )
	{
		LetGoRope();
	}
#endif

	return TRUE;
}

//=========================================================
// BarnacleVictimBitten - bad name for a function that is called
// by Barnacle victims when the barnacle pulls their head
// into its mouth. For the player, just die.
//=========================================================
void CBasePlayer::BarnacleVictimBitten( entvars_t *pevBarnacle )
{
	TakeDamage( pevBarnacle, pevBarnacle, pev->health + pev->armorvalue, DMG_SLASH | DMG_ALWAYSGIB );
}

//=========================================================
// BarnacleVictimReleased - overridden for player who has
// physics flags concerns. 
//=========================================================
void CBasePlayer::BarnacleVictimReleased( void )
{
	m_afPhysicsFlags &= ~PFLAG_ONBARNACLE;
}

//=========================================================
// Illumination 
// return player light level plus virtual muzzle flash
//=========================================================
int CBasePlayer::Illumination( void )
{
	int iIllum = CBaseEntity::Illumination();

	iIllum += m_iWeaponFlash;
	if( iIllum > 255 )
		return 255;
	return iIllum;
}

void CBasePlayer::SetPrefsFromUserinfo( char *infobuffer )
{
	const char *pszKeyVal;

	pszKeyVal = g_engfuncs.pfnInfoKeyValue( infobuffer, "cl_autowepswitch" );

	if( pszKeyVal[0] != '\0' )
		m_iAutoWepSwitch = atoi( pszKeyVal );
	else
		m_iAutoWepSwitch = 1;

	pszKeyVal = g_engfuncs.pfnInfoKeyValue( infobuffer, "_satctrl" );
	if( pszKeyVal[0] != '\0' )
		m_iSatchelControl = atoi( pszKeyVal );
	else
		m_iSatchelControl = 0;

	pszKeyVal = g_engfuncs.pfnInfoKeyValue( infobuffer, "_grenphys" );
	if( pszKeyVal[0] != '\0' )
		m_iPreferNewGrenadePhysics = atoi( pszKeyVal );
	else
		m_iPreferNewGrenadePhysics = 0;
}

void CBasePlayer::EnableControl( BOOL fControl )
{
	if( !fControl )
		pev->flags |= FL_FROZEN;
	else
		pev->flags &= ~FL_FROZEN;
}

#define DOT_1DEGREE   0.9998476951564
#define DOT_2DEGREE   0.9993908270191
#define DOT_3DEGREE   0.9986295347546
#define DOT_4DEGREE   0.9975640502598
#define DOT_5DEGREE   0.9961946980917
#define DOT_6DEGREE   0.9945218953683
#define DOT_7DEGREE   0.9925461516413
#define DOT_8DEGREE   0.9902680687416
#define DOT_9DEGREE   0.9876883405951
#define DOT_10DEGREE  0.9848077530122
#define DOT_15DEGREE  0.9659258262891
#define DOT_20DEGREE  0.9396926207859
#define DOT_25DEGREE  0.9063077870367

//=========================================================
// Autoaim
// set crosshair position to point to enemey
//=========================================================
Vector CBasePlayer::GetAutoaimVector(float flDelta)
{
	return GetAutoaimVectorFromPoint(GetGunPosition(), flDelta);
}

Vector CBasePlayer::GetAutoaimVectorFromPoint( const Vector& vecSrc, float flDelta )
{
	if( g_iSkillLevel == SKILL_HARD )
	{
		UTIL_MakeVectors( pev->v_angle + pev->punchangle );
		return gpGlobals->v_forward;
	}

	float flDist = 8192.0f;

	// always use non-sticky autoaim
	// UNDONE: use sever variable to chose!
	if( 1 || g_iSkillLevel == SKILL_MEDIUM )
	{
		m_vecAutoAim = Vector( 0, 0, 0 );
		// flDelta *= 0.5;
	}

	BOOL m_fOldTargeting = m_fOnTarget;
	Vector angles = AutoaimDeflection(vecSrc, flDist, flDelta );

	// update ontarget if changed
	if( !g_pGameRules->AllowAutoTargetCrosshair() )
		m_fOnTarget = 0;
	else if( m_fOldTargeting != m_fOnTarget )
	{
		m_pActiveItem->UpdateItemInfo();
	}

	if( angles.x > 180 )
		angles.x -= 360;
	if( angles.x < -180 )
		angles.x += 360;
	if( angles.y > 180 )
		angles.y -= 360;
	if( angles.y < -180 )
		angles.y += 360;

	if( angles.x > 25 )
		angles.x = 25;
	if( angles.x < -25 )
		angles.x = -25;
	if( angles.y > 12 )
		angles.y = 12;
	if( angles.y < -12 )
		angles.y = -12;

	// always use non-sticky autoaim
	// UNDONE: use sever variable to chose!
	if( 0 || g_iSkillLevel == SKILL_EASY )
	{
		m_vecAutoAim = m_vecAutoAim * 0.67f + angles * 0.33f;
	}
	else
	{
		m_vecAutoAim = angles * 0.9f;
	}

	// m_vecAutoAim = m_vecAutoAim * 0.99;

	// Don't send across network if sv_aim is 0
	if( g_psv_aim->value != 0 )
	{
		if( m_vecAutoAim.x != m_lastx || m_vecAutoAim.y != m_lasty )
		{
			SET_CROSSHAIRANGLE( edict(), -m_vecAutoAim.x, m_vecAutoAim.y );

			m_lastx = (int)m_vecAutoAim.x;
			m_lasty = (int)m_vecAutoAim.y;
		}
	}

	// ALERT( at_console, "%f %f\n", angles.x, angles.y );

	UTIL_MakeVectors( pev->v_angle + pev->punchangle + m_vecAutoAim );
	return gpGlobals->v_forward;
}

Vector CBasePlayer::AutoaimDeflection( const Vector &vecSrc, float flDist, float flDelta )
{
	edict_t *pEdict = g_engfuncs.pfnPEntityOfEntIndex( 1 );
	CBaseEntity *pEntity;
	float bestdot;
	Vector bestdir;
	edict_t *bestent;
	TraceResult tr;

	if( g_psv_aim->value == 0 )
	{
		m_fOnTarget = FALSE;
		return g_vecZero;
	}

	UTIL_MakeVectors( pev->v_angle + pev->punchangle + m_vecAutoAim );

	// try all possible entities
	bestdir = gpGlobals->v_forward;
	bestdot = flDelta; // +- 10 degrees
	bestent = NULL;

	m_fOnTarget = FALSE;

	UTIL_TraceLine( vecSrc, vecSrc + bestdir * flDist, dont_ignore_monsters, edict(), &tr );

	if( tr.pHit && tr.pHit->v.takedamage != DAMAGE_NO )
	{
		// don't look through water
		if( !( ( pev->waterlevel != 3 && tr.pHit->v.waterlevel == 3 ) || ( pev->waterlevel == 3 && tr.pHit->v.waterlevel == 0 ) ) )
		{
			if( tr.pHit->v.takedamage == DAMAGE_AIM )
				m_fOnTarget = TRUE;

			return m_vecAutoAim;
		}
	}

	for( int i = 1; i < gpGlobals->maxEntities; i++, pEdict++ )
	{
		Vector center;
		Vector dir;
		float dot;

		if( pEdict->free )	// Not in use
			continue;

		if( pEdict->v.takedamage != DAMAGE_AIM )
			continue;
		if( pEdict == edict() )
			continue;
		//if( pev->team > 0 && pEdict->v.team == pev->team )
		//	continue;	// don't aim at teammate
		if( !g_pGameRules->ShouldAutoAim( this, pEdict ) )
			continue;

		pEntity = Instance( pEdict );
		if( pEntity == NULL )
			continue;

		if( !pEntity->IsAlive() )
			continue;

		// don't look through water
		if( ( pev->waterlevel != 3 && pEntity->pev->waterlevel == 3 ) || ( pev->waterlevel == 3 && pEntity->pev->waterlevel == 0 ) )
			continue;

		center = pEntity->BodyTarget( vecSrc );

		dir = ( center - vecSrc ).Normalize();

		// make sure it's in front of the player
		if( DotProduct( dir, gpGlobals->v_forward ) < 0 )
			continue;

		dot = fabs( DotProduct( dir, gpGlobals->v_right ) ) + fabs( DotProduct( dir, gpGlobals->v_up ) ) * 0.5f;

		// tweek for distance
		dot *= 1.0f + 0.2f * ( ( center - vecSrc ).Length() / flDist );

		if( dot > bestdot )
			continue;	// to far to turn

		UTIL_TraceLine( vecSrc, center, dont_ignore_monsters, edict(), &tr );
		if( tr.flFraction != 1.0f && tr.pHit != pEdict )
		{
			// ALERT( at_console, "hit %s, can't see %s\n", STRING( tr.pHit->v.classname ), STRING( pEdict->v.classname ) );
			continue;
		}

		// don't shoot at friends
		if( IRelationship( pEntity ) < 0 )
		{
			if( !pEntity->IsPlayer() && !g_pGameRules->IsDeathmatch() )
				// ALERT( at_console, "friend\n" );
				continue;
		}

		// can shoot at this one
		bestdot = dot;
		bestent = pEdict;
		bestdir = dir;
	}

	if( bestent )
	{
		bestdir = UTIL_VecToAngles( bestdir );
		bestdir.x = -bestdir.x;
		bestdir = bestdir - pev->v_angle - pev->punchangle;

		if( bestent->v.takedamage == DAMAGE_AIM )
			m_fOnTarget = TRUE;

		return bestdir;
	}

	return Vector( 0, 0, 0 );
}

void CBasePlayer::ResetAutoaim()
{
	if( m_vecAutoAim.x != 0 || m_vecAutoAim.y != 0 )
	{
		m_vecAutoAim = Vector( 0, 0, 0 );
		SET_CROSSHAIRANGLE( edict(), 0, 0 );
	}
	m_fOnTarget = FALSE;
}

/*
=============
SetCustomDecalFrames

  UNDONE:  Determine real frame limit, 8 is a placeholder.
  Note:  -1 means no custom frames present.
=============
*/
void CBasePlayer::SetCustomDecalFrames( int nFrames )
{
	if( nFrames > 0 && nFrames < 8 )
		m_nCustomSprayFrames = nFrames;
	else
		m_nCustomSprayFrames = -1;
}

/*
=============
GetCustomDecalFrames

  Returns the # of custom frames this player's custom clan logo contains.
=============
*/
int CBasePlayer::GetCustomDecalFrames( void )
{
	return m_nCustomSprayFrames;
}

static bool ItemDropIsProhibited()
{
	return !g_pGameRules->IsMultiplayer() || ( weaponstay.value > 0 );
}

//=========================================================
// DropPlayerItem - drop the named item, or if no name,
// the active item. 
//=========================================================
void CBasePlayer::DropPlayerItem( const char* pszItemName )
{
	if( !g_pGameRules->PlayerCanDropWeapon(this) ) {
		return;
	}

	if( pszItemName[0] == '\0' )
	{
		// if this string has no length, the client didn't type a name!
		// assume player wants to drop the active item.
		// make the string null to make future operations in this function easier
		pszItemName = NULL;
	}

	for( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CBasePlayerWeapon *pWeapon = m_rgpPlayerWeapons[i];
		// if we land here with a valid pWeapon pointer, that's because we found the
		// item we want to drop and hit a BREAK;  pWeapon is the item.
		if( pWeapon && ((pszItemName && !strcmp( pszItemName, STRING( pWeapon->pev->classname ) )) || pWeapon == m_pActiveItem) )
		{
			DropPlayerItemImpl(pWeapon);
			return;// we're done, so stop searching with the FOR loop.
		}
	}
}

void CBasePlayer::DropPlayerItemById(int iId)
{
	if( !g_pGameRules->PlayerCanDropWeapon(this) ) {
		return;
	}

	CBasePlayerWeapon *pWeapon = WeaponById(iId);
	if ( pWeapon )
	{
		DropPlayerItemImpl(pWeapon);
	}
}

void CBasePlayer::DropPlayerItemImpl(CBasePlayerWeapon *pWeapon, int dropType, float speed)
{
	if (!pWeapon->CanBeDropped())
		return;

	if (!g_pGameRules->GetNextBestWeapon( this, pWeapon ))
		return;

	UTIL_MakeVectors( pev->angles );

	pev->weapons &= ~( 1 << pWeapon->m_iId );// take item off hud

	CWeaponBox *pWeaponBox = (CWeaponBox *)CBaseEntity::Create( "weaponbox", pev->origin + gpGlobals->v_forward * 10, pev->angles, edict() );

	pWeaponBox->SetWeaponModel(pWeapon);
	pWeaponBox->pev->angles.x = 0;
	pWeaponBox->pev->angles.z = 0;
	pWeaponBox->pev->velocity = gpGlobals->v_forward * speed;

	// drop ammo for this weapon.
	int iAmmoIndex = GetAmmoIndex( pWeapon->pszAmmo1() ); // ???
	if( iAmmoIndex != -1 )
	{
		// this weapon weapon uses ammo, so pack an appropriate amount.
		if( pWeapon->iFlags() & ITEM_FLAG_EXHAUSTIBLE )
		{
			// pack up all the ammo, this weapon is its own ammo type
			pWeaponBox->PackAmmo( MAKE_STRING( pWeapon->pszAmmo1() ), m_rgAmmo[iAmmoIndex] );
			m_rgAmmo[iAmmoIndex] = 0;
		}
		else
		{
			if (dropType == DropAmmoFair) {
				int weaponCount = 1; // number of weapons that use the same ammo
				for( int j = 0; j < MAX_WEAPONS; j++ )
				{
					CBasePlayerWeapon* otherWeapon = m_rgpPlayerWeapons[j];
					if( otherWeapon )
					{
						if ( otherWeapon != pWeapon && otherWeapon->pszAmmo1() && FStrEq(otherWeapon->pszAmmo1(), pWeapon->pszAmmo1())) {
							weaponCount++;
						}
					}
				}

				int toPack = m_rgAmmo[iAmmoIndex] / weaponCount;
				pWeaponBox->PackAmmo( MAKE_STRING( pWeapon->pszAmmo1() ), toPack );
				m_rgAmmo[iAmmoIndex] -= toPack;
			} else if (dropType == DropAllAmmo) {
				pWeaponBox->PackAmmo( MAKE_STRING( pWeapon->pszAmmo1() ), m_rgAmmo[iAmmoIndex] );
				m_rgAmmo[iAmmoIndex] = 0;
			}
		}
	}

	// same for ammo2
	iAmmoIndex = GetAmmoIndex( pWeapon->pszAmmo2() );
	if( iAmmoIndex != -1 )
	{
		if (dropType == DropAmmoFair) {
			int weaponCount = 1; // number of weapons that use the same ammo
			for( int j = 0; j < MAX_WEAPONS; j++ )
			{
				CBasePlayerWeapon* otherWeapon = m_rgpPlayerWeapons[j];
				if( otherWeapon )
				{
					if ( otherWeapon != pWeapon && otherWeapon->pszAmmo2() && FStrEq(otherWeapon->pszAmmo2(), pWeapon->pszAmmo2())) {
						weaponCount++;
					}
				}
			}

			int toPack = m_rgAmmo[iAmmoIndex] / weaponCount;
			pWeaponBox->PackAmmo( MAKE_STRING( pWeapon->pszAmmo2() ), toPack );
			m_rgAmmo[iAmmoIndex] -= toPack;
		} else if (dropType == DropAllAmmo) {
			pWeaponBox->PackAmmo( MAKE_STRING( pWeapon->pszAmmo2() ), m_rgAmmo[iAmmoIndex] );
			m_rgAmmo[iAmmoIndex] = 0;
		}
	}
	pWeaponBox->PackWeapon( pWeapon );
}

void CBasePlayer::DropAmmo()
{
	if( !g_pGameRules->IsMultiplayer() )
	{
		// no dropping in single player.
		return;
	}

	const char* pszItemName = "";

	if( !*pszItemName )
	{
		// if this string has no length, the client didn't type a name!
		// assume player wants to drop the active item.
		// make the string null to make future operations in this function easier
		pszItemName = NULL;
	}

	int i;

	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		CBasePlayerWeapon *pWeapon = m_rgpPlayerWeapons[i];

		if( pWeapon && ((pszItemName && FStrEq( pszItemName, STRING( pWeapon->pev->classname ) )) || pWeapon == m_pActiveItem) )
		{
			const char* entName = pWeapon->pszAmmoEntity();
			int ammoCount = pWeapon->iDropAmmo();
			if (entName && ammoCount) {
				int iAmmoIndex = GetAmmoIndex( pWeapon->pszAmmo1() );
				if( iAmmoIndex != -1 )
				{
					if (m_rgAmmo[iAmmoIndex] >= ammoCount) {
						UTIL_MakeVectors( pev->angles );

						CBaseEntity* ammoEnt = CBaseEntity::Create( entName, pev->origin + gpGlobals->v_forward * 10, pev->angles, edict() );
						if (ammoEnt) {
							ammoEnt->pev->spawnflags |= SF_NORESPAWN;
							//UTIL_SetSize( ammoEnt->pev, g_vecZero, g_vecZero );
							//UTIL_SetOrigin( ammoEnt->pev, ammoEnt->pev->origin );
							ammoEnt->pev->bInDuck = 1;
							ammoEnt->pev->angles.x = 0;
							ammoEnt->pev->angles.z = 0;
							ammoEnt->pev->velocity = gpGlobals->v_forward * 400;
							m_rgAmmo[iAmmoIndex] -= ammoCount;
						}
					}
				}
			}

			return;
		}
	}
}

//=========================================================
// HasPlayerItem Does the player already have this item?
//=========================================================
BOOL CBasePlayer::HasPlayerItem( CBasePlayerWeapon *pCheckItem )
{
	return WeaponById(pCheckItem->m_iId) ? TRUE : FALSE;
}

//=========================================================
// HasNamedPlayerItem Does the player already have this item?
//=========================================================
BOOL CBasePlayer::HasNamedPlayerItem( const char *pszItemName )
{
	return GetWeaponByName(pszItemName) != NULL;
}

CBasePlayerWeapon* CBasePlayer::GetWeaponByName(const char *pszItemName)
{
	for( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CBasePlayerWeapon* pWeapon = m_rgpPlayerWeapons[i];

		if( pWeapon )
		{
			if( strcmp( pszItemName, STRING( pWeapon->pev->classname ) ) == 0 )
			{
				return pWeapon;
			}
		}
	}
	return NULL;
}

//=========================================================
// 
//=========================================================
BOOL CBasePlayer::SwitchWeapon(CBasePlayerWeapon *pWeapon )
{
	if( !pWeapon->CanDeploy() )
	{
		return FALSE;
	}
	
	ResetAutoaim();

	if( m_pActiveItem )
	{
		m_pActiveItem->Holster();
	}

	m_pActiveItem = pWeapon;

	pWeapon->m_ForceSendAnimations = true;
	pWeapon->Deploy();
	pWeapon->m_ForceSendAnimations = false;

	return TRUE;
}

BOOL CBasePlayer::SwitchToBestWeapon()
{
	CBasePlayerWeapon *pBest = m_pActiveItem;

	for( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CBasePlayerWeapon *pCheck = m_rgpPlayerWeapons[i];

		if ( pCheck )
		{
			const int bestWeight = pBest ? pBest->iWeight() : -1;
			if( pCheck->iWeight() > bestWeight && pCheck != pBest )
			{
				if( pCheck->CanDeploy() )
				{
					pBest = pCheck;
				}
			}
		}
	}

	if (pBest)
	{
		if (pBest != m_pActiveItem)
			SwitchWeapon(pBest);
		return TRUE;
	}
	return FALSE;
}

void CBasePlayer::InsertWeaponById(CBasePlayerWeapon *pItem)
{
	if (pItem && pItem->m_iId && pItem->m_iId <= MAX_WEAPONS) {
		m_rgpPlayerWeapons[pItem->m_iId-1] = pItem;
	}
}

CBasePlayerWeapon* CBasePlayer::WeaponById(int id)
{
	if (id && id <= MAX_WEAPONS) {
		return m_rgpPlayerWeapons[id-1];
	}
	return NULL;
}

void CBasePlayer::SetFlashlightOnly()
{
	RemoveNVG();
	SetFlashlight();
}

void CBasePlayer::RemoveFlashlight()
{
	FlashlightTurnOff(false);
	m_iItemsBits &= ~(PLAYER_ITEM_FLASHLIGHT);
}

void CBasePlayer::SetNVGOnly()
{
	RemoveFlashlight();
	SetNVG();
}

void CBasePlayer::RemoveNVG()
{
	NVGTurnOff(false);
	m_iItemsBits &= ~(PLAYER_ITEM_NIGHTVISION);
}

void CBasePlayer::RemoveSuitLight() {
	RemoveFlashlight();
	RemoveNVG();
}

void CBasePlayer::SetSuitAndDefaultLight()
{
	SetJustSuit();
	SetDefaultLight();
}

void CBasePlayer::SetDefaultLight()
{
	switch (g_modFeatures.suit_light) {
	case ModFeatures::SUIT_LIGHT_FLASHLIGHT:
		SetFlashlight();
		break;
	case ModFeatures::SUIT_LIGHT_NVG:
		SetNVG();
		break;
	default:
		break;
	}
}

void CBasePlayer::SetLongjump(bool enabled)
{
	m_fLongJump = enabled;
	if (enabled)
		g_engfuncs.pfnSetPhysicsKeyValue( edict(), "slj", "1" );
	else
		g_engfuncs.pfnSetPhysicsKeyValue( edict(), "slj", "0" );
}

void CBasePlayer::SetLoopedMp3(string_t loopedMp3)
{
	m_loopedMp3 = loopedMp3;
}

int CBasePlayer::GiveInventoryItem(string_t item, int count, bool allowOverflow)
{
	int freeSlot = -1;
	int i;
	for (i=0; i<ARRAYSIZE(m_inventoryItems); ++i)
	{
		if (freeSlot < 0 && FStringNull(m_inventoryItems[i]))
		{
			freeSlot = i;
		}
		if (!FStringNull(m_inventoryItems[i]) && FStrEq(STRING(item), STRING(m_inventoryItems[i])))
		{
			break;
		}
	}

	if (i == ARRAYSIZE(m_inventoryItems))
	{
		if (freeSlot >= 0)
		{
			i = freeSlot;
		}
		else
		{
			return INVENTORY_ITEM_NONE_GIVEN_MAXITEMS;
		}
	}

	const InventoryItemSpec* spec = GetInventoryItemSpec(STRING(item));
	if (!allowOverflow && spec && spec->maxCount > 0)
	{
		if (m_inventoryItemCounts[i] >= spec->maxCount)
			return INVENTORY_ITEM_NONE_GIVEN_MAXCOUNT;
	}

	int oldCount = m_inventoryItemCounts[i];
	m_inventoryItems[i] = item;
	m_inventoryItemCounts[i] += count;

	int result = INVENTORY_ITEM_GIVEN;

	if (spec && spec->maxCount > 0)
	{
		if (oldCount <= spec->maxCount && m_inventoryItemCounts[i] > spec->maxCount)
		{
			if (!allowOverflow)
				m_inventoryItemCounts[i] = spec->maxCount;
			result = INVENTORY_ITEM_GIVEN_OVERFLOW;
		}
	}

	MESSAGE_BEGIN(MSG_ONE, gmsgInventory, NULL, pev);
		WRITE_SHORT(m_inventoryItemCounts[i]);
		WRITE_STRING(STRING(item));
		WRITE_BYTE(0);
	MESSAGE_END();

	return result;
}

int CBasePlayer::SetInventoryItem(string_t item, int count, bool allowOverflow)
{
	int i = InventoryItemIndex(item);
	if (i != -1)
	{
		int oldCount = m_inventoryItemCounts[i];
		if (count <= 0)
		{
			m_inventoryItems[i] = iStringNull;
			m_inventoryItemCounts[i] = 0;
		}
		else
		{
			m_inventoryItemCounts[i] = count;
		}
		if (oldCount != count)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgInventory, NULL, pev);
				WRITE_SHORT(m_inventoryItemCounts[i]);
				WRITE_STRING(STRING(item));
				WRITE_BYTE(0);
			MESSAGE_END();

			int result = INVENTORY_ITEM_COUNT_CHANGED;
			const InventoryItemSpec* spec = GetInventoryItemSpec(STRING(item));
			if (spec && spec->maxCount > 0)
			{
				if (oldCount <= spec->maxCount && count > spec->maxCount)
				{
					if (!allowOverflow)
						m_inventoryItemCounts[i] = spec->maxCount;
					result = INVENTORY_ITEM_GIVEN_OVERFLOW;
				}
			}
			return result;
		}
		return INVENTORY_ITEM_NO_CHANGE;
	}
	else
	{
		return GiveInventoryItem(item, count, allowOverflow);
	}
}

bool CBasePlayer::RemoveInventoryItem(string_t item, int count)
{
	int i = InventoryItemIndex(item);
	if (i != -1)
	{
		m_inventoryItemCounts[i] -= count;
		if (m_inventoryItemCounts[i] <= 0)
		{
			m_inventoryItemCounts[i] = 0;
			m_inventoryItems[i] = iStringNull;
		}

		MESSAGE_BEGIN(MSG_ONE, gmsgInventory, NULL, pev);
			WRITE_SHORT(m_inventoryItemCounts[i]);
			WRITE_STRING(STRING(item));
			WRITE_BYTE(0);
		MESSAGE_END();
		return true;
	}
	return false;
}

void CBasePlayer::RemoveAllInventoryItems()
{
	for (int i=0; i<ARRAYSIZE(m_inventoryItems); ++i)
	{
		if (!FStringNull(m_inventoryItems[i]))
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgInventory, NULL, pev);
				WRITE_SHORT(0);
				WRITE_STRING(STRING(m_inventoryItems[i]));
				WRITE_BYTE(0);
			MESSAGE_END();

			m_inventoryItems[i] = iStringNull;
			m_inventoryItemCounts[i] = 0;
		}
	}
}

bool CBasePlayer::HasInventoryItem(string_t item)
{
	return InventoryItemIndex(item) != -1;
}

int CBasePlayer::InventoryItemIndex(string_t item)
{
	if (FStringNull(item))
		return -1;
	for (int i=0; i<ARRAYSIZE(m_inventoryItems); ++i)
	{
		if (!FStringNull(m_inventoryItems[i]))
		{
			if (FStrEq(STRING(item), STRING(m_inventoryItems[i])))
				return i;
		}
	}
	return -1;
}

void CBasePlayer::RecruitFollowers()
{
	const float maxRange = g_FollowersDescription.FastRecruitRange();
	Vector vecStart = pev->origin;
	vecStart.z = pev->absmax.z;

	bool saySentence = true;
	for (auto it = g_FollowersDescription.RecruitsBegin(); it != g_FollowersDescription.RecruitsEnd(); ++it)
	{
		CBaseEntity *pFriend = NULL;
		while( ( pFriend = UTIL_FindEntityByClassname( pFriend, it->c_str()) ) != NULL )
		{
			CFollowingMonster *pMonster = CanRecruit(pFriend, this); pFriend->MyMonsterPointer();
			if (!pMonster)
				continue;
			Vector vecCheck = pFriend->pev->origin;
			vecCheck.z = pFriend->pev->absmax.z;
			if ((vecCheck - vecStart).Length() <= maxRange)
			{
				TraceResult tr;
				UTIL_TraceLine( vecStart, vecCheck, ignore_monsters, ENT( pev ), &tr );
				if( tr.flFraction == 1.0 )
				{
					CFollowingMonster* followingMonster = pMonster->MyFollowingMonsterPointer();
					if (followingMonster && followingMonster->CanFollow())
					{
						int result = followingMonster->DoFollowerUse(this, saySentence, USE_ON);
						if (result == FOLLOWING_STARTED)
						{
							saySentence = false;
						}
						else if (result == FOLLOWING_NOTREADY)
						{
							followingMonster->DoFollowerUse(this, false, USE_ON, true);
						}
					}
				}
			}
		}
	}
}

void CBasePlayer::DisbandFollowers()
{
	bool saySentence = true;
	for (auto it = g_FollowersDescription.RecruitsBegin(); it != g_FollowersDescription.RecruitsEnd(); ++it)
	{
		CBaseEntity *pFriend = NULL;
		while( ( pFriend = UTIL_FindEntityByClassname( pFriend, it->c_str() ) ) )
		{
			CBaseMonster *pMonster = pFriend->MyMonsterPointer();
			if (pMonster)
			{
				CFollowingMonster* talkMonster = pMonster->MyFollowingMonsterPointer();
				if (talkMonster && !talkMonster->ShouldDeclineFollowing())
				{
					int result = talkMonster->DoFollowerUse(this, saySentence, USE_OFF);
					if (result == FOLLOWING_STOPPED)
					{
						saySentence = false;
					}
					else if (result == FOLLOWING_NOTREADY)
					{
						talkMonster->DoFollowerUse(this, false, USE_OFF, true);
					}
				}
			}
		}
	}
}

//=========================================================
// Dead HEV suit prop
//=========================================================
class CDeadHEV : public CDeadMonster
{
public:
	void Spawn();
	const char* DefaultModel() { return g_modFeatures.DeadHazModel(); }
	int	DefaultClassify() { return	CLASS_HUMAN_MILITARY; }

	const char* getPos(int pos) const;
	static const char *m_szPoses[4];
};

const char *CDeadHEV::m_szPoses[] = { "deadback", "deadsitting", "deadstomach", "deadtable" };

const char* CDeadHEV::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

LINK_ENTITY_TO_CLASS( monster_hevsuit_dead, CDeadHEV )

//=========================================================
// ********** DeadHEV SPAWN **********
//=========================================================
void CDeadHEV::Spawn()
{
	SpawnHelper();
	pev->body			= 1;
	MonsterInitDead();
}

class CStripWeapons : public CPointEntity
{
public:
	void Precache()
	{
		if (!FStringNull(pev->noise))
			PRECACHE_SOUND( STRING(pev->noise) );
	}

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

private:
};

LINK_ENTITY_TO_CLASS( player_weaponstrip, CStripWeapons )

void CStripWeapons::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = g_pGameRules->EffectivePlayer(pActivator);

	if( pPlayer ) {
		int stripFlags = pev->spawnflags;
		if (FBitSet(pev->spawnflags, STRIP_SUIT) && g_modFeatures.suit_light != ModFeatures::SUIT_LIGHT_NOTHING)
			stripFlags |= STRIP_SUITLIGHT;
		pPlayer->RemoveAllItems( stripFlags );
		if (!FStringNull(pev->noise))
			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, STRING(pev->noise), 1, ATTN_NORM );
	}
}

enum
{
	PLAYER_CALC_PARAM_HEALTH,
	PLAYER_CALC_PARAM_ARMOR,
	PLAYER_CALC_PARAM_AMMO,
	PLAYER_CALC_PARAM_AMMO_INCLUDE_CLIP,
	PLAYER_CALC_INVENTORY_ITEM,
};

enum
{
	PLAYER_CALC_ABSOLUTE,
	PLAYER_CALC_FRACTION,
};

class CPlayerCalcRatio : public CPointEntity
{
public:
	void KeyValue(KeyValueData *pkvd)
	{
		if (FStrEq(pkvd->szKeyName, "calc_param"))
		{
			pev->impulse = atoi(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "value_notion"))
		{
			pev->button = atoi(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "ammo_name"))
		{
			pev->message = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "item_name"))
		{
			pev->netname = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else
			CPointEntity::KeyValue(pkvd);
	}
	bool CalcRatio(CBaseEntity *pLocus, float *outResult)
	{
		CBasePlayer* pPlayer = g_pGameRules->EffectivePlayer(pLocus);
		if (!pPlayer) {
			return false;
		}
		bool success = true;
		*outResult = CalcParam(pev->impulse, pPlayer, pev->button == PLAYER_CALC_FRACTION, success);
		return success;
	}
private:
	float CalcParam(int paramType, CBasePlayer* pPlayer, bool isFraction, bool& success)
	{
		success = true;
		switch (paramType) {
		case PLAYER_CALC_PARAM_HEALTH:
		{
			if (pPlayer->pev->health <= 0.0f)
				return 0.0f;
			if (isFraction)
			{
				return pPlayer->pev->health / pPlayer->pev->max_health;
			}
			else
			{
				return pPlayer->pev->health;
			}
		}
		case PLAYER_CALC_PARAM_ARMOR:
		{
			if (isFraction)
			{
				return pPlayer->pev->armorvalue / pPlayer->MaxArmor();
			}
			else
			{
				return pPlayer->pev->armorvalue;
			}
		}
		case PLAYER_CALC_PARAM_AMMO:
		case PLAYER_CALC_PARAM_AMMO_INCLUDE_CLIP:
		{
			if (FStringNull(pev->message))
			{
				ALERT(at_warning, "Requesting player's ammo amount via %s, but the ammo name is not specified\n", STRING(pev->classname));
				success = false;
				return 0.0f;
			}
			const char* ammoName = STRING(pev->message);
			const AmmoType* ammoType = CBasePlayerWeapon::GetAmmoType(ammoName);
			if (!ammoType)
			{
				success = false;
				return 0.0f;
			}
			int ammoAmount = pPlayer->AmmoInventory(ammoType->id);
			int ammoMax = ammoType->maxAmmo;

			if (paramType == PLAYER_CALC_PARAM_AMMO_INCLUDE_CLIP)
			{
				for (int i=1; i<MAX_WEAPONS; ++i)
				{
					CBasePlayerWeapon* pWeapon = pPlayer->WeaponById(i);
					if (!pWeapon)
						continue;
					if (pWeapon->iMaxClip() == WEAPON_NOCLIP)
						continue;
					const char* ammo1 = pWeapon->pszAmmo1();
					if (ammo1 && strcmp(ammo1, ammoName) == 0)
					{
						ammoAmount += pWeapon->m_iClip;
						ammoMax += pWeapon->iMaxClip();
					}
				}
			}
			if (isFraction)
			{
				return (float)ammoAmount / ammoMax;
			}
			else
			{
				return ammoAmount;
			}
		}
		case PLAYER_CALC_INVENTORY_ITEM:
		{
			if (FStringNull(pev->netname))
			{
				ALERT(at_warning, "Requesting player's inventory item via %s, but the item name is not specified\n", STRING(pev->classname));
				success = false;
				return 0.0f;
			}
			int index = pPlayer->InventoryItemIndex(pev->netname);
			if (index == -1)
			{
				return 0.0f;
			}
			else
			{
				if (isFraction)
				{
					const InventoryItemSpec* spec = GetInventoryItemSpec(STRING(pPlayer->m_inventoryItems[index]));
					if (spec && spec->maxCount > 0)
					{
						return pPlayer->m_inventoryItemCounts[index] / (float)spec->maxCount;
					}
					return pPlayer->m_inventoryItemCounts[index] > 0 ? 1.0f : 0.0f;
				}
				else
					return pPlayer->m_inventoryItemCounts[index];
			}
		}
		default:
			ALERT(at_warning, "Unknown player calc parameter %d in %s!\n", paramType, STRING(pev->classname));
			success = false;
			return 0.0f;
		}
	}
};

LINK_ENTITY_TO_CLASS( player_calc_ratio, CPlayerCalcRatio )

class CPlayerHasThing : public CPointEntity
{
public:
	void KeyValue( KeyValueData *pkvd )
	{
		if (FStrEq(pkvd->szKeyName, "pass_target"))
		{
			m_PassTarget = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "fail_target"))
		{
			m_FailTarget = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else
			CPointEntity::KeyValue(pkvd);
	}

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		CBasePlayer* pPlayer = g_pGameRules->EffectivePlayer(pActivator);
		if (!pPlayer) {
			return;
		}
		const bool success = HasThing(pPlayer);
		if (success && !FStringNull(m_PassTarget)) {
			FireTargets(STRING(m_PassTarget), pActivator, this);
		}
		if (!success && !FStringNull(m_FailTarget)) {
			FireTargets(STRING(m_FailTarget), pActivator, this);
		}
	}

	int	ObjectCaps() { return CPointEntity::ObjectCaps() | FCAP_MASTER; }

	bool IsTriggered(CBaseEntity *pActivator) {
		CBasePlayer* pPlayer = g_pGameRules->EffectivePlayer(pActivator);
		if (!pPlayer) {
			return false;
		}
		return HasThing(pPlayer);
	}

	virtual bool HasThing(CBasePlayer* pPlayer) = 0;

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
protected:
	string_t m_PassTarget;
	string_t m_FailTarget;
};

TYPEDESCRIPTION	CPlayerHasThing::m_SaveData[] =
{
	DEFINE_FIELD( CPlayerHasThing, m_PassTarget, FIELD_STRING ),
	DEFINE_FIELD( CPlayerHasThing, m_FailTarget, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CPlayerHasThing, CPointEntity )

enum
{
	PLAYER_HAS_SUIT = 0,
	PLAYER_HAS_FLASHLIGHT = 1,
	PLAYER_HAS_NVG = 2,
	PLAYER_HAS_ANYLIGHT = 3,
	PLAYER_HAS_LONGJUMP = 4
};

class CPlayerHasItem : public CPlayerHasThing
{
public:
	void KeyValue( KeyValueData *pkvd )
	{
		if (FStrEq(pkvd->szKeyName, "item_type"))
		{
			pev->impulse = atoi(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else
			CPlayerHasThing::KeyValue(pkvd);
	}

	bool HasThing(CBasePlayer* pPlayer)
	{
		return HasItem(pPlayer, ItemType());
	}
private:
	int ItemType() const { return pev->impulse; }
	bool HasItem(CBasePlayer* pPlayer, int itemType) {
		switch (itemType) {
		case PLAYER_HAS_SUIT:
			return pPlayer->HasSuit();
		case PLAYER_HAS_FLASHLIGHT:
			return pPlayer->HasFlashlight();
		case PLAYER_HAS_NVG:
			return pPlayer->HasNVG();
		case PLAYER_HAS_ANYLIGHT:
			return pPlayer->HasSuitLight();
		case PLAYER_HAS_LONGJUMP:
			return pPlayer->m_fLongJump;
		default:
			return false;
		}
	}
};

LINK_ENTITY_TO_CLASS( player_hasitem, CPlayerHasItem )

class CPlayerHasWeapon : public CPlayerHasThing
{
public:
	void KeyValue( KeyValueData *pkvd )
	{
		if (FStrEq(pkvd->szKeyName, "weapon_name"))
		{
			m_WeaponName = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else
			CPlayerHasThing::KeyValue(pkvd);
	}

	bool HasThing(CBasePlayer* pPlayer) {
		return HasWeapon(pPlayer, m_WeaponName);
	}

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
private:
	bool HasWeapon(CBasePlayer* pPlayer, string_t weaponName) {
		if (FStringNull(weaponName)) {
			return pPlayer->HasWeapons();
		} else {
			return pPlayer->HasNamedPlayerItem(STRING(weaponName));
		}
	}

	string_t m_WeaponName;
};

TYPEDESCRIPTION	CPlayerHasWeapon::m_SaveData[] =
{
	DEFINE_FIELD( CPlayerHasWeapon, m_WeaponName, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CPlayerHasWeapon, CPlayerHasThing )

LINK_ENTITY_TO_CLASS( player_hasweapon, CPlayerHasWeapon )

#define SF_PLAYER_INVENTORY_REMOVE_ON_FIRE (1 << 0)
#define SF_PLAYER_INVENTORY_ALLOW_MAXCOUNT_OVERFLOW (1 << 1)

#define PLAYER_INVENTORY_OP_REMOVE -1
#define PLAYER_INVENTORY_OP_INPUT 0
#define PLAYER_INVENTORY_OP_GIVE 1
#define PLAYER_INVENTORY_OP_SET 2

class CPlayerInventory : public CPointEntity
{
public:
	void KeyValue( KeyValueData *pkvd )
	{
		if (FStrEq(pkvd->szKeyName, "item_name"))
		{
			pev->message = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "operation"))
		{
			pev->weapons = atoi(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "count"))
		{
			pev->impulse = atoi(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "on_count_change"))
		{
			m_fireOnCountChange = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "on_item_limit"))
		{
			m_fireOnItemLimit = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "on_max_count_limit"))
		{
			m_fireOnMaxCountLimit = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else if (FStrEq(pkvd->szKeyName, "on_count_overflow"))
		{
			m_fireOnCountOverflow = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else
			CPointEntity::KeyValue(pkvd);
	}
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
	{
		CBasePlayer* pPlayer = g_pGameRules->EffectivePlayer(pActivator);
		if (!pPlayer)
			return;

		string_t item = pev->message;
		int operation = pev->weapons;
		int count = pev->impulse;

		if (operation == PLAYER_INVENTORY_OP_INPUT)
		{
			if (useType == USE_OFF)
			{
				operation = PLAYER_INVENTORY_OP_REMOVE;
			}
			else if (useType == USE_SET)
			{
				operation = PLAYER_INVENTORY_OP_SET;
				count = (int)value;
			}
			else
			{
				operation = PLAYER_INVENTORY_OP_GIVE;
			}
		}

		if (FStringNull(item))
		{
			if (count <= 0 && (operation == PLAYER_INVENTORY_OP_SET || operation == PLAYER_INVENTORY_OP_REMOVE))
				pPlayer->RemoveAllInventoryItems();
			return;
		}

		int oldCount = 0;
		if (!FStringNull(m_fireOnCountChange))
		{
			int itemIndex = pPlayer->InventoryItemIndex(item);
			if (itemIndex != -1)
			{
				oldCount = pPlayer->m_inventoryItemCounts[itemIndex];
			}
		}

		int result = INVENTORY_ITEM_NO_CHANGE;
		switch (operation) {
		case PLAYER_INVENTORY_OP_REMOVE:
			result = pPlayer->RemoveInventoryItem(item, count) ? INVENTORY_ITEM_COUNT_CHANGED : INVENTORY_ITEM_NO_CHANGE;
			break;
		case PLAYER_INVENTORY_OP_SET:
			result = pPlayer->SetInventoryItem(item, count, AllowOverflow());
			break;
		case PLAYER_INVENTORY_OP_GIVE:
			result = pPlayer->GiveInventoryItem(item, count, AllowOverflow());
			break;
		default:
			break;
		}

		if (!FStringNull(m_fireOnCountChange))
		{
			int newCount = 0;
			int itemIndex = pPlayer->InventoryItemIndex(item);
			if (itemIndex != -1)
			{
				newCount = pPlayer->m_inventoryItemCounts[itemIndex];
			}
			if (oldCount != newCount)
			{
				FireTargets(STRING(m_fireOnCountChange), pActivator, this);
			}
		}

		switch (result) {
		case INVENTORY_ITEM_NONE_GIVEN_MAXITEMS:
			if (!FStringNull(m_fireOnItemLimit))
			{
				FireTargets(STRING(m_fireOnItemLimit), pActivator, this);
			}
			break;
		case INVENTORY_ITEM_NONE_GIVEN_MAXCOUNT:
			if (!FStringNull(m_fireOnMaxCountLimit))
			{
				FireTargets(STRING(m_fireOnMaxCountLimit), pActivator, this);
			}
			break;
		case INVENTORY_ITEM_GIVEN_OVERFLOW:
			if (!FStringNull(m_fireOnCountOverflow))
			{
				FireTargets(STRING(m_fireOnCountOverflow), pActivator, this);
			}
			break;
		default:
			break;
		}

		if (FBitSet(pev->spawnflags, SF_PLAYER_INVENTORY_REMOVE_ON_FIRE))
			UTIL_Remove(this);
	}

	bool AllowOverflow()
	{
		return FBitSet(pev->spawnflags, SF_PLAYER_INVENTORY_ALLOW_MAXCOUNT_OVERFLOW);
	}

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	string_t m_fireOnCountChange;
	string_t m_fireOnItemLimit;
	string_t m_fireOnMaxCountLimit;
	string_t m_fireOnCountOverflow;
};

LINK_ENTITY_TO_CLASS( player_inventory, CPlayerInventory )

TYPEDESCRIPTION	CPlayerInventory::m_SaveData[] =
{
	DEFINE_FIELD( CPlayerInventory, m_fireOnCountChange, FIELD_STRING ),
	DEFINE_FIELD( CPlayerInventory, m_fireOnItemLimit, FIELD_STRING ),
	DEFINE_FIELD( CPlayerInventory, m_fireOnMaxCountLimit, FIELD_STRING ),
	DEFINE_FIELD( CPlayerInventory, m_fireOnCountOverflow, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CPlayerInventory, CPointEntity )

class CPlayerHasInventory : public CPlayerHasThing
{
public:
	void KeyValue( KeyValueData *pkvd )
	{
		if (FStrEq(pkvd->szKeyName, "item_name"))
		{
			pev->message = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else
			CPlayerHasThing::KeyValue(pkvd);
	}
	bool HasThing(CBasePlayer* pPlayer) {
		string_t item = pev->message;
		if (FStringNull(item))
			return false;
		return pPlayer->HasInventoryItem(item);
	}
};

LINK_ENTITY_TO_CLASS( player_hasinventory, CPlayerHasInventory )

class CPlayerDeployWeapon : public CPointEntity
{
public:
	void KeyValue( KeyValueData *pkvd )
	{
		if (FStrEq(pkvd->szKeyName, "weapon_name"))
		{
			pev->netname = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else
			CPointEntity::KeyValue(pkvd);
	}
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
	{
		CBasePlayer* pPlayer = g_pGameRules->EffectivePlayer(pActivator);
		if (pPlayer && !FStringNull(pev->netname))
		{
			pPlayer->SelectItem(STRING(pev->netname));
		}
	}
};

LINK_ENTITY_TO_CLASS( player_deployweapon, CPlayerDeployWeapon )

enum
{
	PLAYER_ABILITY_DISABLE = -1,
	PLAYER_ABILITY_NOCHANGE = 0,
	PLAYER_ABILITY_ENABLE = 1,
	PLAYER_ABILITY_TOGGLE = 2,
	PLAYER_ABILITY_COPYINPUT = 3,
};

class CPlayerCapabilities : public CPointEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		CBasePlayer* pPlayer = g_pGameRules->EffectivePlayer(pActivator);
		if (!pPlayer)
			return;
		ConfigurePlayerCapability(pPlayer, PLAYER_SUPPRESS_ATTACK, m_attackCapability, useType);
		ConfigurePlayerCapability(pPlayer, PLAYER_SUPPRESS_JUMP, m_jumpCapability, useType);
		ConfigurePlayerCapability(pPlayer, PLAYER_SUPPRESS_DUCK, m_duckCapability, useType);
		ConfigurePlayerCapability(pPlayer, PLAYER_SUPPRESS_USE, m_useCapability, useType);
		ConfigurePlayerCapability(pPlayer, PLAYER_SUPPRESS_STEP_SOUND, m_stepSoundCapability, useType);

		pPlayer->SetPhysicsKeyValues();
	}

	void KeyValue( KeyValueData *pkvd )
	{
		if( FStrEq( pkvd->szKeyName, "attack_capability" ) )
		{
			m_attackCapability = atoi( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
		else if( FStrEq( pkvd->szKeyName, "jump_capability" ) )
		{
			m_jumpCapability = atoi( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
		else if( FStrEq( pkvd->szKeyName, "duck_capability" ) )
		{
			m_duckCapability = atoi( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
		else if( FStrEq( pkvd->szKeyName, "use_capability" ) )
		{
			m_useCapability = atoi( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
		else if( FStrEq( pkvd->szKeyName, "stepsound_capability" ) )
		{
			m_stepSoundCapability = atoi( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
		else
			CPointEntity::KeyValue(pkvd);
	}

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

private:
	void ConfigurePlayerCapability(CBasePlayer* pPlayer, int suppressCapability, short setting, USE_TYPE useType)
	{
		if (setting == PLAYER_ABILITY_COPYINPUT)
		{
			if (useType == USE_OFF)
				setting = PLAYER_ABILITY_DISABLE;
			else if (useType == USE_ON)
				setting = PLAYER_ABILITY_ENABLE;
			else
				setting = PLAYER_ABILITY_TOGGLE;
		}

		if (setting == PLAYER_ABILITY_DISABLE)
		{
			SetBits(pPlayer->m_suppressedCapabilities, suppressCapability);
		}
		else if (setting == PLAYER_ABILITY_ENABLE)
		{
			ClearBits(pPlayer->m_suppressedCapabilities, suppressCapability);
		}
		else if (setting == PLAYER_ABILITY_TOGGLE)
		{
			if (FBitSet(pPlayer->m_suppressedCapabilities, suppressCapability))
				ClearBits(pPlayer->m_suppressedCapabilities, suppressCapability);
			else
				SetBits(pPlayer->m_suppressedCapabilities, suppressCapability);
		}
	}

	short m_attackCapability;
	short m_jumpCapability;
	short m_duckCapability;
	short m_useCapability;
	short m_stepSoundCapability;
};

LINK_ENTITY_TO_CLASS( player_capabilities, CPlayerCapabilities )

TYPEDESCRIPTION	CPlayerCapabilities::m_SaveData[] =
{
	DEFINE_FIELD( CPlayerCapabilities, m_attackCapability, FIELD_SHORT ),
	DEFINE_FIELD( CPlayerCapabilities, m_jumpCapability, FIELD_SHORT ),
	DEFINE_FIELD( CPlayerCapabilities, m_duckCapability, FIELD_SHORT ),
	DEFINE_FIELD( CPlayerCapabilities, m_useCapability, FIELD_SHORT ),
	DEFINE_FIELD( CPlayerCapabilities, m_stepSoundCapability, FIELD_SHORT ),
};

IMPLEMENT_SAVERESTORE( CPlayerCapabilities, CPointEntity )

class CPlayerSpeed : public CPointEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		CBasePlayer* pPlayer = g_pGameRules->EffectivePlayer(pActivator);
		if (!pPlayer)
			return;

		pPlayer->m_maxSpeedFraction = pev->health;
	}

	void KeyValue( KeyValueData *pkvd )
	{
		if( FStrEq( pkvd->szKeyName, "maxspeed" ) )
		{
			// pev->maxspeed is not saved by default. Use some other float variable
			pev->health = atof( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
		else
			CPointEntity::KeyValue(pkvd);
	}
};

LINK_ENTITY_TO_CLASS( player_speed, CPlayerSpeed )

#define SF_PLAYER_LOAD_SAVED_FREEZE 1
#define SF_PLAYER_LOAD_SAVED_WEAPONSTRIP 2
#define SF_PLAYER_LOAD_SAVED_RETURN_TO_MENU 128

class CRevertSaved : public CPointEntity
{
public:
	void Spawn();
	void EXPORT LoadSavedUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT MessageThink( void );
	void EXPORT LoadThink( void );
	void KeyValue( KeyValueData *pkvd );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	inline float Duration( void ) { return pev->dmg_take; }
	inline float HoldTime( void ) { return pev->dmg_save; }
	inline float MessageTime( void ) { return m_messageTime; }
	inline float LoadTime( void ) { return m_loadTime; }

	inline void SetDuration( float duration ) { pev->dmg_take = duration; }
	inline void SetHoldTime( float hold ) { pev->dmg_save = hold; }
	inline void SetMessageTime( float time ) { m_messageTime = time; }
	inline void SetLoadTime( float time ) { m_loadTime = time; }

private:
	float m_messageTime;
	float m_loadTime;
};

LINK_ENTITY_TO_CLASS( player_loadsaved, CRevertSaved )

TYPEDESCRIPTION	CRevertSaved::m_SaveData[] =
{
	DEFINE_FIELD( CRevertSaved, m_messageTime, FIELD_FLOAT ),	// These are not actual times, but durations, so save as floats
	DEFINE_FIELD( CRevertSaved, m_loadTime, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CRevertSaved, CPointEntity )

void CRevertSaved::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "duration" ) )
	{
		SetDuration( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "holdtime" ) )
	{
		SetHoldTime( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "messagetime" ) )
	{
		SetMessageTime( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "loadtime" ) )
	{
		SetLoadTime( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CRevertSaved::Spawn()
{
	CPointEntity::Spawn();
	SetUse(&CRevertSaved::LoadSavedUse);
}

void CRevertSaved::LoadSavedUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (FBitSet(pev->spawnflags, SF_PLAYER_LOAD_SAVED_WEAPONSTRIP|SF_PLAYER_LOAD_SAVED_FREEZE))
	{
		CBasePlayer *pPlayer = g_pGameRules->EffectivePlayer(pActivator);
		if (pPlayer) {
			if (FBitSet(pev->spawnflags, SF_PLAYER_LOAD_SAVED_WEAPONSTRIP)) {
				pPlayer->RemoveAllItems(STRIP_WEAPONS_ONLY);
			}

			if (FBitSet(pev->spawnflags, SF_PLAYER_LOAD_SAVED_FREEZE)) {
				pPlayer->EnableControl(FALSE);
			}
		}
	}

	UTIL_ScreenFadeAll( pev->rendercolor, Duration(), HoldTime(), (int)pev->renderamt, FFADE_OUT );
	pev->nextthink = gpGlobals->time + MessageTime();
	SetUse( NULL );
	SetThink( &CRevertSaved::MessageThink );
}

void CRevertSaved::MessageThink( void )
{
	UTIL_ShowMessageAll( STRING( pev->message ) );
	float nextThink = LoadTime() - MessageTime();
	if( nextThink > 0 ) 
	{
		pev->nextthink = gpGlobals->time + nextThink;
		SetThink( &CRevertSaved::LoadThink );
	}
	else
		LoadThink();
}

void CRevertSaved::LoadThink( void )
{
	if( !g_pGameRules->IsMultiplayer() )
	{
		if (FBitSet(pev->spawnflags, SF_PLAYER_LOAD_SAVED_RETURN_TO_MENU))
		{
			g_engfuncs.pfnEndSection( "_oem_end_training" );
		} else
		{
			SERVER_COMMAND( "reload\n" );
		}
	}
}

//=========================================================
// Multiplayer intermission spots.
//=========================================================
class CInfoIntermission:public CPointEntity
{
	void Spawn( void );
	void Think( void );
};

void CInfoIntermission::Spawn( void )
{
	UTIL_SetOrigin( pev, pev->origin );
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->v_angle = g_vecZero;

	pev->nextthink = gpGlobals->time + 2.0f;// let targets spawn!
}

void CInfoIntermission::Think( void )
{
	edict_t *pTarget;

	// find my target
	pTarget = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->target ) );

	if( !FNullEnt( pTarget ) )
	{
		pev->v_angle = UTIL_VecToAngles( ( pTarget->v.origin - pev->origin ).Normalize() );
		pev->v_angle.x = -pev->v_angle.x;
	}
}

LINK_ENTITY_TO_CLASS( info_intermission, CInfoIntermission )
