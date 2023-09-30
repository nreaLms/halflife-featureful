#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"

BOOL CBasePlayerWeapon::CanDeploy( void )
{
	BOOL bHasAmmo = 0;

	if( !pszAmmo1() )
	{
		// this weapon doesn't use ammo, can always deploy.
		return TRUE;
	}

	if( pszAmmo1() )
	{
		bHasAmmo |= ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] != 0 );
	}
	if( pszAmmo2() )
	{
		bHasAmmo |= ( m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] != 0 );
	}
	if( m_iClip > 0 )
	{
		bHasAmmo |= 1;
	}
	if( !bHasAmmo )
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CBasePlayerWeapon::DefaultReload( int iClipSize, int iAnim, float fDelay, int body )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		return FALSE;

	int j = Q_min( iClipSize - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] );

	if( j == 0 )
		return FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + fDelay;

	//!!UNDONE -- reload sound goes here !!!
	SendWeaponAnim( iAnim, body );

	m_fInReload = TRUE;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
	return TRUE;
}

void CBasePlayerWeapon::ResetEmptySound( void )
{
	m_iPlayEmptySound = 1;
}

BOOL CanAttack( float attack_time, float curtime, BOOL isPredicted )
{
#ifdef CLIENT_DLL
	return attack_time <= 0.0f;
#else
#if CLIENT_WEAPONS
	if( !isPredicted )
#else
	if( 1 )
#endif
	{
		return ( attack_time <= curtime ) ? TRUE : FALSE;
	}
	else
	{
		return ( (static_cast<int>(::floor(attack_time * 1000.0f)) * 1000.0f) <= 0.0f) ? TRUE : FALSE;
	}
#endif
}

void CBasePlayerWeapon::ItemPostFrame( void )
{
	if( ( m_fInReload ) && ( m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase() ) )
	{
		int maxClip = iMaxClip();
		// complete the reload.
		int j = Q_min( maxClip - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);

		// Add them to the clip
		m_iClip += j;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= j;

		m_fInReload = FALSE;
	}

#ifndef CLIENT_DLL
	if( !(m_pPlayer->pev->button & IN_ATTACK ) )
	{
		m_flLastFireTime = 0.0f;
	}
#endif

	if( ( m_pPlayer->pev->button & IN_ATTACK2 ) && CanAttack( m_flNextSecondaryAttack, gpGlobals->time, UseDecrement() ) )
	{
		if( pszAmmo2() && !m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] )
		{
			m_fFireOnEmpty = TRUE;
		}

		SecondaryAttack();
		m_pPlayer->pev->button &= ~IN_ATTACK2;
	}
	else if( ( m_pPlayer->pev->button & IN_ATTACK ) && CanAttack( m_flNextPrimaryAttack, gpGlobals->time, UseDecrement() ) )
	{
		if( ( m_iClip == 0 && pszAmmo1() ) || ( iMaxClip() == -1 && !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] ) )
		{
			m_fFireOnEmpty = TRUE;
		}

		PrimaryAttack();
	}
	else if( m_pPlayer->pev->button & IN_RELOAD && iMaxClip() != WEAPON_NOCLIP && !m_fInReload )
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
	}
	else if( !( m_pPlayer->pev->button & ( IN_ATTACK | IN_ATTACK2 ) ) )
	{
		// no fire buttons down
		m_fFireOnEmpty = FALSE;

#ifndef CLIENT_DLL
		if( !IsUseable() && m_flNextPrimaryAttack < ( UseDecrement() ? 0.0f : gpGlobals->time ) )
		{
			// weapon isn't useable, switch.
			if( !( iFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY ) && g_pGameRules->GetNextBestWeapon( m_pPlayer, this ) )
			{
				m_flNextPrimaryAttack = ( UseDecrement() ? 0.0f : gpGlobals->time ) + 0.3f;
				return;
			}
		}
		else
#endif
		{
			// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
			if( m_iClip == 0 && !(iFlags() & ITEM_FLAG_NOAUTORELOAD ) &&
#ifdef CLIENT_DLL
					m_flNextPrimaryAttack <= 0.0f
#else
					m_flNextPrimaryAttack < ( UseDecrement() ? 0.0f : gpGlobals->time )
#endif
					)
			{
				Reload();
				return;
			}
		}

		WeaponIdle();
		return;
	}

	// catch all
	if( ShouldWeaponIdle() )
	{
		WeaponIdle();
	}
}

int CBasePlayerWeapon::iMaxClip()
{
	int maxClip;
#ifdef CLIENT_DLL
		ItemInfo itemInfo = {0};
		GetItemInfo( &itemInfo );
		maxClip = itemInfo.iMaxClip;
#else
	maxClip = ItemInfoArray[ m_iId ].iMaxClip;
#endif
	return maxClip;
}

bool CBasePlayerWeapon::InZoom()
{
	return m_pPlayer->pev->fov != 0;
}
