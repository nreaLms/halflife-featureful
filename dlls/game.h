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
#if !defined(GAME_H)
#define GAME_H

#include "cvardef.h"

extern void GameDLLInit( void );

struct ModFeatures
{
	ModFeatures();
	bool SetValue(const char* key, const char* value);
	bool EnableWeapon(const char* name);
	void EnableAllWeapons();

	bool IsWeaponEnabled(int weaponId) const;

	const char* DesertEagleDropName() const;
	const char* M249DropName() const;
	const char* DeadHazModel() const;

	bool DisplacerBallEnabled() const;
	bool ShockBeamEnabled() const;
	bool SporesEnabled() const;

	void EnableMonster(const char* name);
	bool IsMonsterEnabled(const char* name);

	bool items_instant_drop;
	bool tripmines_solid;
	bool satchels_pickable;

	bool monsters_stop_attacking_dying_monsters;
	bool monsters_delegate_squad_leadership;
	bool monsters_eat_for_health;

	bool blackops_classify;
	bool opfor_grunts_dislike_civilians;

	bool racex_dislike_alien_military;
	bool racex_dislike_gargs;
	bool racex_dislike_alien_monsters;

	bool skill_opfor;
	bool opfor_decals;
	bool opfor_deadhaz;
	bool tentacle_opfor_height;

private:
	bool UpdateBoolean(const char* value, bool& result, const char* key);
	bool UpdateInteger(const char* value, int& result, const char* key);
	bool UpdateColor(const char* value, int& result, const char* key);

	bool weapons[64];
	char monsters[64][64];
	unsigned int monstersCount;
};

extern ModFeatures g_modFeatures;

extern cvar_t displaysoundlist;

// multiplayer server rules
extern cvar_t teamplay;
extern cvar_t fraglimit;
extern cvar_t timelimit;
extern cvar_t friendlyfire;
extern cvar_t falldamage;
extern cvar_t weaponstay;
extern cvar_t dropweapons;

extern cvar_t weapon_respawndelay;
extern cvar_t ammo_respawndelay;
extern cvar_t item_respawndelay;
extern cvar_t healthcharger_rechargetime;
extern cvar_t hevcharger_rechargetime;

extern cvar_t selfgauss;
extern cvar_t satchelfix;
extern cvar_t explosionfix;
extern cvar_t monsteryawspeedfix;
extern cvar_t corpsephysics;
extern cvar_t pushablemode;
extern cvar_t forcerespawn;
extern cvar_t respawndelay;
extern cvar_t flashlight;
extern cvar_t aimcrosshair;
extern cvar_t decalfrequency;
extern cvar_t teamlist;
extern cvar_t teamoverride;
extern cvar_t defaultteam;

extern cvar_t allowmonsters;
extern cvar_t allowmonsterinfo;
extern cvar_t npc_dropweapons;
extern cvar_t dmgperscore;
extern cvar_t allydmgpenalty;
extern cvar_t npckill;

extern cvar_t keepinventory;

// Engine Cvars
extern cvar_t *g_psv_gravity;
extern cvar_t *g_psv_maxspeed;
extern cvar_t *g_psv_aim;
extern cvar_t *g_footsteps;
extern cvar_t *g_enable_cheats;

extern cvar_t *g_psv_developer;

#endif // GAME_H
