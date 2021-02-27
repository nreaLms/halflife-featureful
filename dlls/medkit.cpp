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

#if FEATURE_MEDKIT

LINK_ENTITY_TO_CLASS(weapon_medkit, CMedkit)

void FindHullIntersection(const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity);

CBaseEntity* CMedkit::FindHealTarget(bool increasedRadius)
{
#ifndef CLIENT_DLL
	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * (increasedRadius ? 48.0f : 32.0f);

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

	if( tr.flFraction >= 1.0f )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if( tr.flFraction < 1.0f )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if( !pHit || pHit->IsBSPModel() )
			{
				FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			}
		}
	}

	if (!FNullEnt( tr.pHit )) {
		CBaseEntity* pEntity = CBaseEntity::Instance( tr.pHit );
		if( pEntity && pEntity->IsAlive() && pEntity->pev->health < pEntity->pev->max_health) {
			CBaseEntity* foundTarget = NULL;
			if (pEntity->IsPlayer()) {
				foundTarget = pEntity;
			}
			else
			{
				CBaseMonster* monster = pEntity->MyMonsterPointer();
				if (monster && monster->IDefaultRelationship(m_pPlayer) == R_AL) {
					foundTarget = pEntity;
				}
			}
			return foundTarget;
		}
	}

#endif
	return NULL;
}

void CMedkit::Spawn()
{
	Precache();
	m_iId = WEAPON_MEDKIT;
	SET_MODEL(ENT(pev), MyWModel());

	m_iDefaultAmmo = MEDKIT_DEFAULT_GIVE;
	m_secondaryAttack = FALSE;
	m_flSoundDelay = 0;

	FallInit();// get ready to fall down.
}


void CMedkit::Precache(void)
{
	PRECACHE_MODEL("models/v_tfc_medkit.mdl");
	PRECACHE_MODEL(MyWModel());
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
	p->iPosition = 4;
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
	return AddToPlayerDefault(pPlayer);
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
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 && (healTarget = FindHealTarget()) ) {
		//
	} else {
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.8;
		return;
	}

	m_secondaryAttack = FALSE;
	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usMedkitFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0);
	float delay = GetNextAttackDelay(2);
	if (delay < UTIL_WeaponTimeBase())
		delay = UTIL_WeaponTimeBase() + 2;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = delay;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 5, 10);
	m_flSoundDelay = gpGlobals->time + 0.4;
}

void CMedkit::SecondaryAttack()
{
	Reload();

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 0 || m_pPlayer->pev->health >= m_pPlayer->pev->max_health) {
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

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 && m_flSoundDelay != 0 && m_flSoundDelay <= gpGlobals->time)
	{
		const int maxHeal = Q_min((int)gSkillData.plrDmgMedkit, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);
		if (m_secondaryAttack) {
			const int diff = (int)ceil(m_pPlayer->pev->max_health - m_pPlayer->pev->health);
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= m_pPlayer->TakeHealth(m_pPlayer, Q_min(maxHeal, diff), DMG_GENERIC);
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "items/medshot5.wav", 1.0, ATTN_NORM, 0, 100);
		} else {
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);
			
			CBaseEntity* healTarget = FindHealTarget(true);
	
			if (healTarget) {
				const int diff = (int)ceil(healTarget->pev->max_health - healTarget->pev->health);
				const int healResult = healTarget->TakeHealth(m_pPlayer, Q_min(maxHeal, diff), DMG_GENERIC);
				m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= healResult;
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
#endif
