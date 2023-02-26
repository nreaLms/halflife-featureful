#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "ammunition.h"
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "game.h"
#include "gamerules.h"

void CBasePlayerAmmo::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), MyModel() );

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;

	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( &CBasePlayerAmmo::DefaultTouch );
}

void CBasePlayerAmmo::Precache()
{
	PRECACHE_MODEL( MyModel() );
	PRECACHE_SOUND( AMMO_PICKUP_SOUND );
}

Vector CBasePlayerAmmo::MyRespawnSpot()
{
	return g_pGameRules->VecAmmoRespawnSpot( this );
}

float CBasePlayerAmmo::MyRespawnTime()
{
	return g_pGameRules->FlAmmoRespawnTime( this );
}

void CBasePlayerAmmo::OnMaterialize()
{
	SetTouch( &CBasePlayerAmmo::DefaultTouch );
	SetThink( NULL );
}

void CBasePlayerAmmo::DefaultTouch( CBaseEntity *pOther )
{
	if (IsPickableByTouch()) {
		//Prevent dropped ammo from touching at the same time
		if( pev->bInDuck && !( pev->flags & FL_ONGROUND ) )
		{
			return;
		}
		pev->bInDuck = 0;
		TouchOrUse(pOther);
	}
}

void CBasePlayerAmmo::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (IsPickableByUse() && !(pev->effects & EF_NODRAW) ) {
		TouchOrUse(pCaller);
	}
}

extern int gEvilImpulse101;

void CBasePlayerAmmo::TouchOrUse( CBaseEntity *pOther )
{
	if( !pOther->IsPlayer() || !pOther->IsAlive() )
	{
		return;
	}

	if( AddAmmo( pOther ) )
	{
		if( g_pGameRules->AmmoShouldRespawn( this ) == GR_AMMO_RESPAWN_YES )
		{
			Respawn();
		}
		else
		{
			SetTouch( NULL );
			SetThink( &CBaseEntity::SUB_Remove );
			pev->nextthink = gpGlobals->time + 0.1f;
		}
	}
	else if( gEvilImpulse101 )
	{
		// evil impulse 101 hack, kill always
		SetTouch( NULL );
		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.1f;
	}
}

BOOL CBasePlayerAmmo::AddAmmo(CBaseEntity *pOther)
{
	int amount = MyAmount();
	const char* ammoName = AmmoName();

	if (!ammoName)
		return FALSE;

	if ( pOther->GiveAmmo( amount, ammoName ) != -1 )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, AMMO_PICKUP_SOUND, 1, ATTN_NORM );
		return TRUE;
	}
	return FALSE;
}

// Ammo entities


class CGlockAmmo : public CBasePlayerAmmo
{
	const char* MyModel() {
		return "models/w_9mmclip.mdl";
	}
	int MyAmount() {
		return AMMO_GLOCKCLIP_GIVE;
	}
	const char* AmmoName() {
		return "9mm";
	}
};

LINK_ENTITY_TO_CLASS( ammo_glockclip, CGlockAmmo )
LINK_ENTITY_TO_CLASS( ammo_9mmclip, CGlockAmmo )

class CPythonAmmo : public CBasePlayerAmmo
{
	const char* MyModel() {
		return "models/w_357ammobox.mdl";
	}
	int MyAmount() {
		return AMMO_357BOX_GIVE;
	}
	const char* AmmoName() {
		return "357";
	}
};

LINK_ENTITY_TO_CLASS( ammo_357, CPythonAmmo )

class CMP5AmmoClip : public CBasePlayerAmmo
{
	const char* MyModel() {
		return "models/w_9mmARclip.mdl";
	}
	int MyAmount() {
		return AMMO_MP5CLIP_GIVE;
	}
	const char* AmmoName() {
		return "9mm";
	}
};

LINK_ENTITY_TO_CLASS( ammo_mp5clip, CMP5AmmoClip )
LINK_ENTITY_TO_CLASS( ammo_9mmAR, CMP5AmmoClip )

class CMP5Chainammo : public CBasePlayerAmmo
{
	const char* MyModel() {
		return "models/w_chainammo.mdl";
	}
	int MyAmount() {
		return AMMO_CHAINBOX_GIVE;
	}
	const char* AmmoName() {
		return "9mm";
	}
};

LINK_ENTITY_TO_CLASS( ammo_9mmbox, CMP5Chainammo )

class CMP5AmmoGrenade : public CBasePlayerAmmo
{
	const char* MyModel() {
		return "models/w_ARgrenade.mdl";
	}
	int MyAmount() {
		return AMMO_M203BOX_GIVE;
	}
	const char* AmmoName() {
		return "ARgrenades";
	}
};

LINK_ENTITY_TO_CLASS( ammo_mp5grenades, CMP5AmmoGrenade )
LINK_ENTITY_TO_CLASS( ammo_ARgrenades, CMP5AmmoGrenade )

class CShotgunAmmo : public CBasePlayerAmmo
{
	const char* MyModel() {
		return "models/w_shotbox.mdl";
	}
	int MyAmount() {
		return AMMO_BUCKSHOTBOX_GIVE;
	}
	const char* AmmoName() {
		return "buckshot";
	}
};

LINK_ENTITY_TO_CLASS( ammo_buckshot, CShotgunAmmo )

class CCrossbowAmmo : public CBasePlayerAmmo
{
	const char* MyModel() {
		return "models/w_crossbow_clip.mdl";
	}
	int MyAmount() {
		return AMMO_CROSSBOWCLIP_GIVE;
	}
	const char* AmmoName() {
		return "bolts";
	}
};

LINK_ENTITY_TO_CLASS( ammo_crossbow, CCrossbowAmmo )

class CRpgAmmo : public CBasePlayerAmmo
{
	const char* MyModel() {
		return "models/w_rpgammo.mdl";
	}
	int MyAmount() {
		int iGive;
#ifdef CLIENT_DLL
	if( bIsMultiplayer() )
#else
	if( g_pGameRules->IsMultiplayer() )
#endif
		{
			// hand out more ammo per rocket in multiplayer.
			iGive = AMMO_RPGCLIP_GIVE * 2;
		}
		else
		{
			iGive = AMMO_RPGCLIP_GIVE;
		}
		return iGive;
	}
	const char* AmmoName() {
		return "rockets";
	}
};

LINK_ENTITY_TO_CLASS( ammo_rpgclip, CRpgAmmo )

class CGaussAmmo : public CBasePlayerAmmo
{
	const char* MyModel() {
		return "models/w_gaussammo.mdl";
	}
	int MyAmount() {
		return AMMO_URANIUMBOX_GIVE;
	}
	const char* AmmoName() {
		return "uranium";
	}
};

class CEgonAmmo : public CBasePlayerAmmo
{
	const char* MyModel() {
		return "models/w_chainammo.mdl";
	}
	int MyAmount() {
		return AMMO_URANIUMBOX_GIVE;
	}
	const char* AmmoName() {
		return "uranium";
	}
};

LINK_ENTITY_TO_CLASS( ammo_egonclip, CEgonAmmo )

LINK_ENTITY_TO_CLASS( ammo_gaussclip, CGaussAmmo )

#if FEATURE_SNIPERRIFLE
class CSniperrifleAmmo : public CBasePlayerAmmo
{
	bool IsEnabledInMod() {
		return g_modFeatures.IsWeaponEnabled(WEAPON_SNIPERRIFLE);
	}
	const char* MyModel() {
		return "models/w_m40a1clip.mdl";
	}
	int MyAmount() {
		return AMMO_762BOX_GIVE;
	}
	const char* AmmoName() {
		return "762";
	}
};
LINK_ENTITY_TO_CLASS( ammo_762, CSniperrifleAmmo )
#endif

#if FEATURE_M249
class CM249AmmoClip : public CBasePlayerAmmo
{
	bool IsEnabledInMod() {
		return g_modFeatures.IsWeaponEnabled(WEAPON_M249);
	}
	const char* MyModel() {
		return "models/w_saw_clip.mdl";
	}
	int MyAmount() {
		return AMMO_556CLIP_GIVE;
	}
	const char* AmmoName() {
		return "556";
	}
};

LINK_ENTITY_TO_CLASS(ammo_556, CM249AmmoClip)
#endif
