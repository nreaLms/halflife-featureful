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
#pragma once
#if !defined(WEAPONS_H)
#define WEAPONS_H

#include "weapon_ids.h"
#include "mod_features.h"
#include "bullet_types.h"
#include "weapon_animations.h"
#include "weaponinfo.h"
#include "player_items.h"
#include "ammoregistry.h"
#include "cone_degrees.h"

#if !CLIENT_DLL
#include "combat.h"
#include "effects.h"
#include "ggrenade.h"
#include "global_models.h"
#endif

class CBasePlayer;
extern int gmsgWeapPickup;

void DeactivateSatchels( CBasePlayer *pOwner );

// weapon weight factors (for auto-switching)   (-1 = noswitch)
#define CROWBAR_WEIGHT		0
#define GLOCK_WEIGHT		10
#define PYTHON_WEIGHT		15
#define MP5_WEIGHT			15
#define SHOTGUN_WEIGHT		15
#define CROSSBOW_WEIGHT		10
#define RPG_WEIGHT			20
#define GAUSS_WEIGHT		20
#define EGON_WEIGHT			20
#define HORNETGUN_WEIGHT	15
#define HANDGRENADE_WEIGHT	5
#define SNARK_WEIGHT		5
#define SATCHEL_WEIGHT		-10
#define TRIPMINE_WEIGHT		-10
#define EAGLE_WEIGHT		15
#define PIPEWRENCH_WEIGHT		0
#define M249_WEIGHT			15
#define SNIPERRIFLE_WEIGHT		10
#define DISPLACER_WEIGHT		20
#define SHOCKRIFLE_WEIGHT		15
#define SPORELAUNCHER_WEIGHT		20
#define KNIFE_WEIGHT			0
#define GRAPPLE_WEIGHT			21
#define MEDKIT_WEIGHT		-1
#define UZI_WEIGHT			15

// weapon clip/carry ammo capacities
#define URANIUM_MAX_CARRY		100
#define	_9MM_MAX_CARRY			250
#define _357_MAX_CARRY			36
#define BUCKSHOT_MAX_CARRY		125
#define BOLT_MAX_CARRY			50
#define ROCKET_MAX_CARRY		5
#define HANDGRENADE_MAX_CARRY	10
#define SATCHEL_MAX_CARRY		5
#define TRIPMINE_MAX_CARRY		5
#define SNARK_MAX_CARRY			15
#define HORNET_MAX_CARRY		8
#define M203_GRENADE_MAX_CARRY	10
#define PENGUIN_MAX_CARRY		9
#define	_556_MAX_CARRY			200
#define _762_MAX_CARRY			15
#define SHOCK_MAX_CARRY			10
#define SPORE_MAX_CARRY			20
#define MEDKIT_MAX_CARRY		100

// the maximum amount of ammo each weapon's clip can hold
#define WEAPON_NOCLIP			-1

//#define CROWBAR_MAX_CLIP		WEAPON_NOCLIP
#define GLOCK_MAX_CLIP			17
#define PYTHON_MAX_CLIP			6
#define MP5_MAX_CLIP			50
#define SHOTGUN_MAX_CLIP		8
#define CROSSBOW_MAX_CLIP		5
#define RPG_MAX_CLIP			1
#define GAUSS_MAX_CLIP			WEAPON_NOCLIP
#define EGON_MAX_CLIP			WEAPON_NOCLIP
#define HORNETGUN_MAX_CLIP		WEAPON_NOCLIP
#define HANDGRENADE_MAX_CLIP	WEAPON_NOCLIP
#define SATCHEL_MAX_CLIP		WEAPON_NOCLIP
#define TRIPMINE_MAX_CLIP		WEAPON_NOCLIP
#define SNARK_MAX_CLIP			WEAPON_NOCLIP
#define EAGLE_MAX_CLIP			7
#define M249_MAX_CLIP			50
#define SNIPERRIFLE_MAX_CLIP	5
#define SHOCKRIFLE_MAX_CLIP		10
#define SPORELAUNCHER_MAX_CLIP		5
#define UZI_MAX_CLIP			32

// the default amount of ammo that comes with each gun when it spawns
#define GLOCK_DEFAULT_GIVE			17
#define PYTHON_DEFAULT_GIVE			6
#if FEATURE_OPFOR_SPECIFIC
#define MP5_DEFAULT_GIVE			50
#else
#define MP5_DEFAULT_GIVE			25
#endif
#define MP5_DEFAULT_GIVE_MP			MP5_MAX_CLIP
#define MP5_M203_DEFAULT_GIVE		0
#define SHOTGUN_DEFAULT_GIVE		12
#define CROSSBOW_DEFAULT_GIVE		5
#define RPG_DEFAULT_GIVE			1
#define GAUSS_DEFAULT_GIVE			20
#define EGON_DEFAULT_GIVE			20
#define HANDGRENADE_DEFAULT_GIVE	5
#define SATCHEL_DEFAULT_GIVE		1
#define TRIPMINE_DEFAULT_GIVE		1
#define SNARK_DEFAULT_GIVE			5
#define HIVEHAND_DEFAULT_GIVE		8
#define EAGLE_DEFAULT_GIVE			7
#define PENGUIN_DEFAULT_GIVE		3
#define M249_DEFAULT_GIVE			50
#define SNIPERRIFLE_DEFAULT_GIVE		5
#define DISPLACER_DEFAULT_GIVE		40
#define SHOCKRIFLE_DEFAULT_GIVE		10
#define SPORELAUNCHER_DEFAULT_GIVE	5
#define MEDKIT_DEFAULT_GIVE			50
#define UZI_DEFAULT_GIVE			UZI_MAX_CLIP

// The amount of ammo given to a player by an ammo item.
#define AMMO_URANIUMBOX_GIVE	20
#define AMMO_GLOCKCLIP_GIVE		GLOCK_MAX_CLIP
#define AMMO_357BOX_GIVE		PYTHON_MAX_CLIP
#define AMMO_MP5CLIP_GIVE		MP5_MAX_CLIP
#define AMMO_CHAINBOX_GIVE		200
#define AMMO_M203BOX_GIVE		2
#define AMMO_BUCKSHOTBOX_GIVE	12
#define AMMO_CROSSBOWCLIP_GIVE	CROSSBOW_MAX_CLIP
#define AMMO_RPGCLIP_GIVE		RPG_MAX_CLIP
#define AMMO_URANIUMBOX_GIVE	20
#define AMMO_SNARKBOX_GIVE		5
#define AMMO_PENGUINBOX_GIVE		3
#define AMMO_556CLIP_GIVE			50
#define AMMO_762BOX_GIVE		5
#define AMMO_SPORE_GIVE			1

#define ITEM_FLAG_SELECTONEMPTY		1
#define ITEM_FLAG_NOAUTORELOAD		2
#define ITEM_FLAG_NOAUTOSWITCHEMPTY	4
#define ITEM_FLAG_LIMITINWORLD		8
#define ITEM_FLAG_EXHAUSTIBLE		16 // A player can totally exhaust their ammo supply and lose this weapon
#define ITEM_FLAG_NOAUTOSWITCHTO	32

#define WEAPON_IS_ONTARGET 0x40

typedef struct
{
	int		iSlot;
	int		iPosition;
	const char	*pszAmmo1;	// ammo 1 type
	const char	*pszAmmo2;	// ammo 2 type
	const char	*pszName;
	int		iMaxClip;
	int		iId;
	int		iFlags;
	int		iWeight;// this value used to determine this weapon's importance in autoselection.
	const char* pszAmmoEntity;
	int iDropAmmo;
} ItemInfo;

class CBasePlayerWeapon : public CBaseAnimating
{
public:
	virtual void SetObjectCollisionBox( void );
	virtual void KeyValue( KeyValueData *pkvd );

#ifndef CLIENT_DLL
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	virtual int AddToPlayer( CBasePlayer *pPlayer );	// return TRUE if the item you want the item added to the player inventory
	void EXPORT DestroyItem( void );
	void EXPORT DefaultTouch( CBaseEntity *pOther );	// default weapon touch
	void EXPORT FallThink ( void );// when an item is first spawned, this think is run to determine when the object has hit the ground.
	void EXPORT Materialize( void );// make a weapon visible and tangible
	void EXPORT AttemptToMaterialize( void );  // the weapon desires to become visible and tangible, if the game rules allow for it
	CBaseEntity* Respawn ( void );// copy a weapon
	void FallInit( void );
	void CheckRespawn( void );
	virtual int GetItemInfo(ItemInfo *p) { return 0; }	// returns 0 if struct not filled out
	virtual BOOL CanDeploy( void );
	virtual BOOL Deploy( )								// returns is deploy was successful
		 { return TRUE; }

	virtual BOOL CanHolster( void ) { return TRUE; }// can this weapon be put away right now?

	virtual void ItemPreFrame( void )	{ return; }		// called each frame by the player PreThink

	virtual void Drop( void );
	virtual void Kill( void );
	virtual void AttachToPlayer ( CBasePlayer *pPlayer );

	int ObjectCaps();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void TouchOrUse( CBaseEntity* other );

	static const AmmoType* GetAmmoType( const char* name );

	static ItemInfo ItemInfoArray[ MAX_WEAPONS ];

	CBasePlayer	*m_pPlayer;
	int		m_iId;												// WEAPON_???

	int			iItemPosition( void ) { return ItemInfoArray[ m_iId ].iPosition; }
	const char	*pszAmmo1( void ) const { return ItemInfoArray[ m_iId ].pszAmmo1; }
	int			iMaxAmmo1( void )	{
		if (m_iPrimaryAmmoType > 0)
			return g_AmmoRegistry.GetMaxAmmo(m_iPrimaryAmmoType);
		return g_AmmoRegistry.GetMaxAmmo(pszAmmo1());
	}
	bool UsesAmmo() const {
		return m_iPrimaryAmmoType > 0 || pszAmmo1() != NULL;
	}
	const char	*pszAmmo2( void ) const { return ItemInfoArray[ m_iId ].pszAmmo2; }
	int			iMaxAmmo2( void )	{
		if (m_iSecondaryAmmoType > 0)
			return g_AmmoRegistry.GetMaxAmmo(m_iSecondaryAmmoType);
		return g_AmmoRegistry.GetMaxAmmo(pszAmmo2());
	}
	bool UsesSecondaryAmmo() const {
		return m_iSecondaryAmmoType > 0 || pszAmmo2() != NULL;
	}

	const char	*pszName( void )	{ return ItemInfoArray[ m_iId ].pszName; }
	int			iMaxClip( void );
	int			iWeight( void )		{ return ItemInfoArray[ m_iId ].iWeight; }
	int			iFlags( void )		{ return ItemInfoArray[ m_iId ].iFlags; }
	const char* pszAmmoEntity( void ) { return ItemInfoArray[ m_iId ].pszAmmoEntity; }
	int			iDropAmmo( void )	{ return ItemInfoArray[ m_iId ].iDropAmmo; }

	virtual const char* MyWModel() { return 0; }

	int AddToPlayerDefault( CBasePlayer *pPlayer );
	virtual int AddDuplicate( CBasePlayerWeapon *pItem );

	virtual int ExtractAmmo( CBasePlayerWeapon *pWeapon ); //{ return TRUE; };			// Return TRUE if you can add ammo to yourself when picked up
	virtual int ExtractClipAmmo( CBasePlayerWeapon *pWeapon );// { return TRUE; };			// Return TRUE if you can add ammo to yourself when picked up

	virtual int AddWeapon( void ) { ExtractAmmo( this ); return TRUE; }	// Return TRUE if you want to add yourself to the player

	// generic "shared" ammo handlers
	BOOL AddPrimaryAmmo(int iCount);
	BOOL AddSecondaryAmmo(int iCount);

	virtual void UpdateItemInfo( void ) {}	// updates HUD state

	//Special stuff for grenades and satchels.
	float m_flStartThrow;
	float m_flReleaseThrow;
	int m_chargeReady;
	int m_fInAttack;

	enum EGON_FIRESTATE { FIRE_OFF, FIRE_CHARGE };
	int m_fireState;

	int m_iPlayEmptySound;
	int m_fFireOnEmpty;		// True when the gun is empty and the player is still holding down the
							// attack key(s)
	virtual BOOL PlayEmptySound( void );
	virtual void ResetEmptySound( void );

	virtual void SendWeaponAnim( int iAnim, int body = 0 );  // skiplocal is 1 if client is predicting weapon animations

	virtual BOOL IsUseable( void );
	BOOL DefaultDeploy( const char *szViewModel, const char *szWeaponModel, int iAnim, const char *szAnimExt, int body = 0 );
	BOOL DefaultReload( int iClipSize, int iAnim, float fDelay, int body = 0 );
	void PrecachePModel(const char* name);

	virtual void ItemPostFrame( void );	// called each frame by the player PostThink
	// called by CBasePlayerWeapons ItemPostFrame()
	virtual void PrimaryAttack( void ) { return; }				// do "+ATTACK"
	virtual void SecondaryAttack( void ) { return; }			// do "+ATTACK2"
	virtual void Reload( void ) { return; }						// do "+RELOAD"
	virtual void WeaponIdle( void ) { return; }					// called when no buttons pressed
	virtual int UpdateClientData( CBasePlayer *pPlayer );		// sends hud info to client dll, if things have changed
	virtual void GetWeaponData(weapon_data_t& data) {}
	virtual void SetWeaponData(const weapon_data_t& data) {}

	virtual void RetireWeapon( void );
	virtual BOOL ShouldWeaponIdle( void ) {return FALSE; }
	virtual void Holster();
	virtual BOOL UseDecrement( void ) { return FALSE; }

	int	PrimaryAmmoIndex();
	int	SecondaryAmmoIndex();

	void PrintState( void );

	virtual CBasePlayerWeapon *MyWeaponPointer( void ) { return this; }
	virtual bool CanBeDropped() { return true; }
	float GetNextAttackDelay( float delay );
	bool InZoom();

	int		m_fInSpecialReload;									// Are we in the middle of a reload for the shotguns
	float	m_flNextPrimaryAttack;								// soonest time ItemPostFrame will call PrimaryAttack
	float	m_flNextSecondaryAttack;							// soonest time ItemPostFrame will call SecondaryAttack
	float	m_flTimeWeaponIdle;									// soonest time ItemPostFrame will call WeaponIdle
	int		m_iPrimaryAmmoType;									// "primary" ammo index into players m_rgAmmo[]
	int		m_iSecondaryAmmoType;								// "secondary" ammo index into players m_rgAmmo[]
	int		m_iClip;											// number of shots left in the primary weapon clip, -1 it not used
	int		m_iClientClip;										// the last version of m_iClip sent to hud dll
	int		m_iClientWeaponState;								// the last version of the weapon state sent to hud dll (is current weapon, is on target)
	int		m_fInReload;										// Are we in the middle of a reload;

	void	InitDefaultAmmo(int defaultGive);
	int		m_iDefaultAmmo;// how much ammo you get when you pick up this weapon as placed by a level designer.

	// hle time creep vars
	float	m_flPrevPrimaryAttack;
	float	m_flLastFireTime;

	//Hack so deploy animations work when weapon prediction is enabled.
	bool m_ForceSendAnimations;
};

#define LOUD_GUN_VOLUME			1000
#define NORMAL_GUN_VOLUME		600
#define QUIET_GUN_VOLUME		200

#define	BRIGHT_GUN_FLASH		512
#define NORMAL_GUN_FLASH		256
#define	DIM_GUN_FLASH			128

#define	WEAPON_ACTIVITY_VOLUME	64

//=========================================================
// CWeaponBox - a single entity that can store weapons
// and ammo.
//=========================================================
class CWeaponBox : public CBaseDelay
{
public:
	void Precache( void );
	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	void KeyValue( KeyValueData *pkvd );
	BOOL IsEmpty( void );
	void SetObjectCollisionBox( void );

	int ObjectCaps();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void TouchOrUse( CBaseEntity* other );

	void EXPORT Kill ( void );
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL HasWeapon( CBasePlayerWeapon *pCheckItem );
	BOOL PackWeapon( CBasePlayerWeapon *pWeapon );
	BOOL PackAmmo( string_t iszName, int iCount );

	void SetWeaponModel( CBasePlayerWeapon* pItem );

	void InsertWeaponById( CBasePlayerWeapon* pItem );
	CBasePlayerWeapon* WeaponById( int id );

	CBasePlayerWeapon	*m_rgpPlayerWeapons[MAX_WEAPONS];// one slot for each

	string_t m_rgiszAmmo[MAX_AMMO_TYPES];// ammo names
	int	m_rgAmmo[MAX_AMMO_TYPES];// ammo quantities

	int m_cAmmoTypes;// how many ammo types packed into this box (if packed by a level designer)
};

bool bIsMultiplayer ( void );

#if CLIENT_DLL
void LoadVModel ( const char *szViewModel, CBasePlayer *m_pPlayer );
#endif

class CGlock : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int GetItemInfo( ItemInfo *p );
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void GlockFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/w_9mmhandgun.mdl"; }

private:
	int m_iShell;

	unsigned short m_usFireGlock1;
	unsigned short m_usFireGlock2;
};

class CCrowbar : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT SwingAgain( void );
	void EXPORT Smack( void );
	int GetItemInfo( ItemInfo *p );
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	int Swing( int fFirst );
	BOOL Deploy( void );
	void Holster();
	void WeaponIdle();
	int m_iSwing;
	TraceResult m_trHit;

	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/w_crowbar.mdl"; }

private:
	unsigned short m_usCrowbar;
};

class CPython : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	void Holster();
	void Reload( void );
	void WeaponIdle( void );
	float m_flSoundDelay;

	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/w_357.mdl"; }
private:
	unsigned short m_usFirePython;
};

class CMP5 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	float m_flNextAnimTime;
	int m_iShell;

	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/w_9mmAR.mdl"; }

private:
	unsigned short m_usMP5;
	unsigned short m_usMP52;
};

class CCrossbow : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int GetItemInfo(ItemInfo *p);

	void FireBolt( void );
	void FireSniperBolt( void );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	int AddToPlayer( CBasePlayer *pPlayer );
	BOOL Deploy( );
	void Holster();
	void Reload( void );
	void WeaponIdle( void );

	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/w_crossbow.mdl"; }

private:
	unsigned short m_usCrossbow;
	unsigned short m_usCrossbow2;
};

class CShotgun : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( );
	void Reload( void );
	void WeaponIdle( void );

	void ItemPostFrame( void );
	float m_flPumpTime;
	float m_flNextReload;
	int m_iShell;

	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/w_shotgun.mdl"; }

private:
	unsigned short m_usDoubleFire;
	unsigned short m_usSingleFire;
};

class CLaserSpot : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );

	int	ObjectCaps( void ) { return FCAP_DONT_SAVE; }

	void Suspend( float flSuspendTime );
	void EXPORT Revive( void );
	void Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib );

	static CLaserSpot *CreateSpot( edict_t* pOwner = 0 );
};

class CRpg : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	void Reload( void );
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	BOOL CanHolster( void );
	void Holster();

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );

	void UpdateSpot( void );
	BOOL ShouldWeaponIdle( void ) { return TRUE; };

	CLaserSpot *m_pSpot;
	int m_fSpotActive;
	int m_cActiveRockets;// how many missiles in flight from this launcher right now?

	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/w_rpg.mdl"; }

	void GetWeaponData(weapon_data_t& data);
	void SetWeaponData(const weapon_data_t& data);

private:
	unsigned short m_usRpg;
};

class CGauss : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	void Holster();

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );

	void StartFire( void );
	void Fire( Vector vecOrigSrc, Vector vecDirShooting, float flDamage );
	float GetFullChargeTime( void );
	int m_iBalls;
	int m_iGlow;
	int m_iBeam;
	int m_iSoundState; // don't save this

	// was this weapon just fired primary or secondary?
	// we need to know so we can pick the right set of effects.
	BOOL m_fPrimaryFire;

	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/w_gauss.mdl"; }

	void GetWeaponData(weapon_data_t& data);
	void SetWeaponData(const weapon_data_t& data);

private:
	unsigned short m_usGaussFire;
	unsigned short m_usGaussSpin;
};

class CEgon : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	BOOL CanHolster( void );
	void Holster();

	void UpdateEffect( const Vector &startPoint, const Vector &endPoint, float timeBlend );

	void CreateEffect ( void );
	void DestroyEffect ( void );
	void EndAttack( void );
	void Attack( void );
	void PrimaryAttack( void );
	void WeaponIdle( void );

	float m_flAmmoUseTime;// since we use < 1 point of ammo per update, we subtract ammo on a timer.

	float GetPulseInterval( void );
	float GetDischargeInterval( void );

	void Fire( const Vector &vecOrigSrc, const Vector &vecDir );

	BOOL HasAmmo( void );

	void UseAmmo( int count );

	enum EGON_FIREMODE { FIRE_NARROW, FIRE_WIDE};

#if !CLIENT_DLL
	CBeam				*m_pBeam;
	CBeam				*m_pNoise;
	CSprite				*m_pSprite;
#endif

	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/w_egon.mdl"; }

	void GetWeaponData(weapon_data_t& data);
	void SetWeaponData(const weapon_data_t& data);

	unsigned short m_usEgonStop;

private:
#if !CLIENT_DLL
	float				m_shootTime;
#endif
	EGON_FIREMODE		m_fireMode;
	float				m_shakeTime;
	BOOL				m_deployed;

	unsigned short m_usEgonFire;
};

class CHgun : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	BOOL IsUseable( void );
	void Holster();
	void Reload( void );
	void WeaponIdle( void );
	float m_flNextAnimTime;

	float m_flRechargeTime;

	int m_iFirePhase;

	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/w_hgun.mdl"; }
private:
	unsigned short m_usHornetFire;
};

class CHandGrenade : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	BOOL Deploy( void );
	BOOL CanHolster( void );
	void Holster();
	void WeaponIdle( void );
	bool PreferNewPhysics();

	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/w_grenade.mdl"; }

	void GetWeaponData(weapon_data_t& data);
	void SetWeaponData(const weapon_data_t& data);
};

class CSatchel : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	int AddDuplicate(CBasePlayerWeapon *pOriginal );
	BOOL CanDeploy( void );
	BOOL Deploy( void );
	BOOL IsUseable( void );
	bool CanBeDropped();

	void Holster();
	void WeaponIdle( void );
	void Throw( void );
	void Detonate(bool allowThrow);
	int ControlBehavior();
	void DrawSatchel( void );
	void DrawRadio();

	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/w_satchel.mdl"; }

	void GetWeaponData(weapon_data_t& data);
	void SetWeaponData(const weapon_data_t& data);
};

class CTripmine : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int GetItemInfo(ItemInfo *p);
	void SetObjectCollisionBox( void )
	{
		//!!!BUGBUG - fix the model!
		pev->absmin = pev->origin + Vector(-16, -16, -5);
		pev->absmax = pev->origin + Vector(16, 16, 28);
	}

	void PrimaryAttack( void );
	BOOL Deploy( void );
	void Holster();
	void WeaponIdle( void );

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/v_tripmine.mdl"; }
private:
	unsigned short m_usTripFire;
};

class CSqueak : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	void Holster();
	void WeaponIdle( void );
	int m_fJustThrown;

	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	virtual const char* GrenadeName() const;
	virtual int WeaponId() const;
	virtual const char* NestModel() const;
	virtual const char* PModel() const;
	virtual const char* VModel() const;
	virtual int PositionInSlot() const;
	virtual int DefaultGive() const;
	virtual const char* AmmoName() const;
	virtual const char* EventsFile() const;

	const char* MyWModel() { return NestModel(); }
private:
	unsigned short m_usSnarkFire;
};

#if FEATURE_DESERT_EAGLE
class CEagle : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	bool IsEnabledInMod();
	int GetItemInfo( ItemInfo *p );
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	void Holster();
	void Reload( void );
	void ItemPostFrame();
	void WeaponIdle( void );

	void UpdateSpot( void );
	CLaserSpot *m_pEagleLaser;
	int m_fEagleLaserActive;
	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/w_desert_eagle.mdl"; }

	void GetWeaponData(weapon_data_t& data);
	void SetWeaponData(const weapon_data_t& data);
private:
	int m_iShell;

	unsigned short m_usEagle;
};
#endif

#if FEATURE_PIPEWRENCH
class CPipeWrench : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	int		Save(CSave &save);
	int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn(void);
	void Precache(void);
	bool IsEnabledInMod();
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer(CBasePlayer *pPlayer);

	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void EXPORT SwingAgain(void);
	void EXPORT Smack(void);

	int Swing(int fFirst);
	BOOL Deploy(void);
	void WeaponIdle(void);
	void Holster();
	void BigSwing(void);

	int m_iSwing;
	TraceResult m_trHit;
	int m_iSwingMode;
	float m_flBigSwingStart;

	virtual BOOL UseDecrement(void)
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/w_pipe_wrench.mdl"; }

	void GetWeaponData(weapon_data_t& data);
	void SetWeaponData(const weapon_data_t& data);
private:

	unsigned short m_usPWrench;
};
#endif

#if FEATURE_MEDKIT
class CMedkit : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn(void);
	void Precache(void);
	bool IsEnabledInMod();
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer(CBasePlayer *pPlayer);

	void PrimaryAttack(void);
	void SecondaryAttack(void);
	BOOL Deploy(void);
	void Holster();
	void Reload( void );
	void WeaponIdle(void);
	BOOL PlayEmptySound(void);
	BOOL ShouldWeaponIdle(void) { return TRUE; }
	CBaseEntity* FindHealTarget(bool increasedRadius = false);

	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}
	const char* MyWModel() { return "models/w_medkit.mdl"; }

	float	m_flSoundDelay;
	float	m_flRechargeTime;
	BOOL	m_secondaryAttack;

protected:
	bool CanRecharge();
private:
	unsigned short m_usMedkitFire;
};
#endif

#if FEATURE_GRAPPLE
class CBarnacleGrappleTip;

class CBarnacleGrapple : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	enum FireState
	{
		OFF		= 0,
		CHARGE	= 1
	};

	void Precache( void );
	void Spawn( void );
	bool IsEnabledInMod();
	void EndAttack( void );

	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer* pPlayer );
	BOOL Deploy();
	void Holster();
	void WeaponIdle( void );
	void PrimaryAttack( void );

	void Fire( Vector vecOrigin, Vector vecDir );

	void CreateEffect( void );
	void UpdateEffect( void );
	void DestroyEffect( void );
	virtual BOOL UseDecrement(void)
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/w_bgrap.mdl"; }

private:
	CBarnacleGrappleTip* m_pTip;
#if !CLIENT_DLL
	CBeam* m_pBeam;
#endif

	float m_flShootTime;
	float m_flDamageTime;

	bool m_bGrappling;
	bool m_bMissed;
	bool m_bMomentaryStuck;
};
#endif

#if FEATURE_M249
class CM249 : public CBasePlayerWeapon
{
public:

#ifndef CLIENT_DLL
	int		Save(CSave &save);
	int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn(void);
	void Precache(void);
	bool IsEnabledInMod();
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer(CBasePlayer *pPlayer);

	void PrimaryAttack(void);
	BOOL Deploy(void);
	void Holster();
	void Reload(void);
	void ItemPostFrame();
	void WeaponIdle(void);

	int m_iShell;
	int m_iLink;
	bool m_bAlternatingEject;

	virtual BOOL UseDecrement(void)
	{
#if defined( CLIENT_WEAPONS )
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/w_saw.mdl"; }

	void GetWeaponData(weapon_data_t& data);
	void SetWeaponData(const weapon_data_t& data);

	void UpdateTape();
	void UpdateTape(int clip);
	int BodyFromClip();
	int BodyFromClip(int clip);

	int m_iVisibleClip;

private:
	bool m_IsNotFirstDraw;

	unsigned short m_usM249;
};
#endif

#if FEATURE_SNIPERRIFLE
class CSniperrifle : public CBasePlayerWeapon
{
public:

#ifndef CLIENT_DLL
	int		Save(CSave &save);
	int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn(void);
	void Precache(void);
	bool IsEnabledInMod();

	int GetItemInfo(ItemInfo *p);
	int AddToPlayer(CBasePlayer *pPlayer);
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	BOOL Deploy(void);
	void Holster();
	void Reload(void);
	void WeaponIdle(void);
	//void ItemPostFrame(void);

	virtual BOOL UseDecrement(void)
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/w_m40a1.mdl"; }

private:
	unsigned short m_usSniper;
};
#endif

#if FEATURE_DISPLACER
class CDisplacer : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	bool IsEnabledInMod();

	int GetItemInfo( ItemInfo *p );
	int AddToPlayer( CBasePlayer *pPlayer );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	void Holster();
	void WeaponIdle( void );

	BOOL PlayEmptySound( void );

	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/w_displacer.mdl"; }

	BOOL CanFireDisplacer( int count ) const;

	enum DISPLACER_FIREMODE { FIREMODE_FORWARD = 1, FIREMODE_BACKWARD };

	void ClearSpin( void );
	void EXPORT SpinUp( void );
	void EXPORT Teleport( void );
	void EXPORT Displace( void );
	void LightningEffect( void );
	void ClearBeams( void );
private:
#if !CLIENT_DLL
	CBeam *m_pBeam[3];
#endif
	int m_iFireMode;
	unsigned short m_usDisplacer;
};
#endif

#if FEATURE_SHOCKRIFLE
class CShockrifle : public CHgun
{
public:
	void Spawn(void);
	void Precache(void);
	bool IsEnabledInMod();

	int GetItemInfo(ItemInfo *p);
	int AddToPlayer(CBasePlayer *pPlayer);

	void PrimaryAttack(void);
	void SecondaryAttack(void);
	BOOL Deploy(void);
	void Holster();
	void Reload(void);
	void WeaponIdle(void);
	void CreateChargeEffect(void);
	void EXPORT ClearBeams(void);
	virtual BOOL UseDecrement(void)
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/w_shock_rifle.mdl"; }
private:
	unsigned short m_usShockFire;
#if !CLIENT_DLL
	CBeam* m_pBeam[4];
#endif
};
#endif

#if FEATURE_KNIFE
class CKnife : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	int		Save(CSave &save);
	int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn(void);
	void Precache(void);
	bool IsEnabledInMod();
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void EXPORT SwingAgain(void);
	void EXPORT Smack(void);

	int Swing(int fFirst);
	BOOL Deploy(void);
	void WeaponIdle();
	void Holster();
	void Stab();

	int m_iSwing;
	TraceResult m_trHit;
	int m_iSwingMode;
	float m_flStabStart;

	virtual BOOL UseDecrement(void)
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/w_knife.mdl"; }

	void GetWeaponData(weapon_data_t& data);
	void SetWeaponData(const weapon_data_t& data);
private:
	unsigned short m_usKnife;
};
#endif

#if FEATURE_PENGUIN
class CPenguin : public CSqueak
{
public:
	bool IsEnabledInMod();
	virtual const char* GrenadeName() const;
	virtual int WeaponId() const;
	virtual const char* NestModel() const;
	virtual const char* PModel() const;
	virtual const char* VModel() const;
	virtual int PositionInSlot() const;
	virtual int DefaultGive() const;
	virtual const char* AmmoName() const;
	virtual const char* EventsFile() const;
};
#endif

#if FEATURE_SPORELAUNCHER
class CSporelauncher : public CShotgun
{
public:
	void Spawn( void );
	void Precache( void );
	bool IsEnabledInMod();

	int GetItemInfo( ItemInfo *p );
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy();
	void Reload( void );
	void WeaponIdle( void );

	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/w_spore_launcher.mdl"; }

	int m_iSquidSpitSprite;
private:
	unsigned short m_usSporeFire;
};
#endif

#if FEATURE_UZI

class CUzi : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	bool IsEnabledInMod();
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	int m_iShell;

	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	const char* MyWModel() { return "models/w_uzi.mdl"; }

private:
	unsigned short m_usUzi;
};

#endif

#endif // WEAPONS_H
