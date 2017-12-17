/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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

enum medkit_e {
	MEDKIT_IDLE = 0,
	MEDKIT_LONGIDLE,
	MEDKIT_LONGUSE,
	MEDKIT_SHORTUSE,
	MEDKIT_HOLSTER,
	MEDKIT_DRAW,
};

LINK_ENTITY_TO_CLASS(weapon_medkit, CMedkit)

CBaseEntity* CMedkit::FindHealTarget()
{
#ifndef CLIENT_DLL
	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecSrc = m_pPlayer->EyePosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 64;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, m_pPlayer->edict(), &tr );

	if (tr.flFraction != 1.0 && !FNullEnt( tr.pHit )) {
		CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
		if( pHit && pHit->IsAlive() && pHit->pev->health < pHit->pev->max_health) {
			if (pHit->IsPlayer()) {
				return pHit;
			}
			CBaseMonster* monster = pHit->MyMonsterPointer();
			if (monster && (monster->Classify() == CLASS_HUMAN_PASSIVE || monster->Classify() == CLASS_PLAYER_ALLY)) {
				return pHit;
			}
		}
	}
#endif
	return NULL;
}

void CMedkit::Spawn()
{
	Precache();
	m_iId = WEAPON_MEDKIT;
	SET_MODEL(ENT(pev), "models/w_medkit.mdl");

	m_iDefaultAmmo = MEDKIT_DEFAULT_GIVE;
	m_secondaryAttack = FALSE;
	m_flSoundDelay = 0;

	FallInit();// get ready to fall down.
}


void CMedkit::Precache(void)
{
	PRECACHE_MODEL("models/v_tfc_medkit.mdl");
	PRECACHE_MODEL("models/w_medkit.mdl");
	PRECACHE_MODEL("models/p_medkit.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("items/medshot4.wav");
	PRECACHE_SOUND("items/medshot5.wav");
	PRECACHE_SOUND("items/medshotno1.wav");

	m_usMedkitFire = PRECACHE_EVENT(1, "events/medkit.sc");
}

int CMedkit::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Medicine";
	p->iMaxAmmo1 = MEDKIT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_MEDKIT;
	p->iWeight = MEDKIT_WEIGHT;
	
	if (gSkillData.plrMedkitTime != 0) {
		p->iFlags = ITEM_FLAG_NOAUTOSWITCHEMPTY | ITEM_FLAG_NOAUTORELOAD;
	} else {
		p->iFlags = ITEM_FLAG_NOAUTORELOAD;
	}

	return 1;
}

int CMedkit::AddToPlayer(CBasePlayer *pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev);
		WRITE_BYTE(m_iId);
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CMedkit::Deploy()
{
	m_flSoundDelay = 0;
	return DefaultDeploy("models/v_tfc_medkit.mdl", "models/p_medkit.mdl", MEDKIT_DRAW, "trip");
}

void CMedkit::Holster(int skiplocal /*= 0*/)
{
	m_flSoundDelay = 0;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	
	//HACKHACK - can't select medkit if it's empty! no way to get ammo for it, either
	if( gSkillData.plrMedkitTime != 0 && !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] ) {
		m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = 1;
	}
	
	SendWeaponAnim(MEDKIT_HOLSTER);
}

void CMedkit::PrimaryAttack(void)
{
	Reload();

	CBaseEntity* healTarget;
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= gSkillData.plrDmgMedkit && (healTarget = FindHealTarget()) ) {
//		if (healTarget->IsPlayer()) {
//			m_pPlayer->TryToSayHealing();
//		}
	} else {
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay(0.8);
		return;
	}

	m_secondaryAttack = FALSE;
	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usMedkitFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0.0, 0, 0.0);
	float delay = GetNextAttackDelay(2);
	if (delay < UTIL_WeaponTimeBase())
		delay = UTIL_WeaponTimeBase() + 2;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = delay;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 5, 10);
	m_flSoundDelay = gpGlobals->time + 0.8;
}

void CMedkit::SecondaryAttack()
{
	Reload();

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < gSkillData.plrDmgMedkit || m_pPlayer->pev->health >= m_pPlayer->pev->max_health) {
		PlayEmptySound();
		m_flNextSecondaryAttack = GetNextAttackDelay(0.8);
		return;
	}

	m_secondaryAttack = TRUE;

	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usMedkitFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 1, 0.0, 0, 0.0);
	float delay = GetNextAttackDelay(3);
	if (delay < UTIL_WeaponTimeBase())
		delay = UTIL_WeaponTimeBase() + 3;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = delay;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 5, 10);
	m_flSoundDelay = gpGlobals->time + 1;
}

void CMedkit::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= MEDKIT_MAX_CARRY )
		return;
	if( gSkillData.plrMedkitTime != 0 && m_flRechargeTime < gpGlobals->time )
	{
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]++;
		m_flRechargeTime = gpGlobals->time + gSkillData.plrMedkitTime;
	}
}

void CMedkit::WeaponIdle(void)
{
	Reload();
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= gSkillData.plrDmgMedkit && m_flSoundDelay != 0 && m_flSoundDelay <= gpGlobals->time)
	{
		if (m_secondaryAttack) {
			const float diff = m_pPlayer->pev->max_health - m_pPlayer->pev->health;
			const int toHeal = (int)(Q_min(diff, gSkillData.plrDmgMedkit));
			if ( m_pPlayer->TakeHealth(toHeal, DMG_GENERIC) ) {
				m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= toHeal;
			}
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "items/medshot5.wav", 1.0, ATTN_NORM, 0, 100);
		} else {
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);
			
			CBaseEntity* healTarget = FindHealTarget();
	
			if (healTarget) {
				const float diff = healTarget->pev->max_health - healTarget->pev->health;
				const int toHeal = (int)(Q_min(diff, gSkillData.plrDmgMedkit));
				if ( healTarget->TakeHealth(toHeal, DMG_GENERIC) ) {
					m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= toHeal;
#ifndef CLIENT_DLL
					CBaseMonster* monster = healTarget->MyMonsterPointer();
					if (monster) {
						monster->Forget(bits_MEMORY_PROVOKED|bits_MEMORY_SUSPICIOUS);
						if (monster->m_hEnemy.Get() && monster->m_hEnemy->IsPlayer()) {
							monster->m_hEnemy = NULL;
						}
					}
#endif
				}
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "items/medshot4.wav", 1.0, ATTN_NORM, 0, 100);
			}
		}
		m_flSoundDelay = 0;
	}

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0.0, 1.0);

	if (flRand <= 0.75)
	{
		iAnim = MEDKIT_LONGIDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 72.0 / 30.0;
	}
	else
	{
		iAnim = MEDKIT_IDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 36.0 / 30.0;
	}

	SendWeaponAnim(iAnim);
}

BOOL CMedkit::PlayEmptySound(void)
{
	if (m_iPlayEmptySound)
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "items/medshotno1.wav", 1.0, ATTN_NORM);
		m_iPlayEmptySound = 0;
		return 0;
	}
	return 0;
}
