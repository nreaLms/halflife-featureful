#pragma once
#if !defined(WEAPON_IDS_H)
#define WEAPON_IDS_H

#include "mod_features.h"

#define WEAPON_NONE				0
#define WEAPON_CROWBAR			1
#define	WEAPON_GLOCK			2
#define WEAPON_PYTHON			3
#define WEAPON_MP5				4
#define WEAPON_CHAINGUN			5
#define WEAPON_CROSSBOW			6
#define WEAPON_SHOTGUN			7
#define WEAPON_RPG				8
#define WEAPON_GAUSS			9
#define WEAPON_EGON				10
#define WEAPON_HORNETGUN		11
#define WEAPON_HANDGRENADE		12
#define WEAPON_TRIPMINE			13
#define	WEAPON_SATCHEL			14
#define	WEAPON_SNARK			15

#if FEATURE_GRAPPLE
#define WEAPON_GRAPPLE			16
#endif
#if FEATURE_DESERT_EAGLE
#define WEAPON_EAGLE			17
#endif
#if FEATURE_PIPEWRENCH
#define WEAPON_PIPEWRENCH		18
#endif
#if FEATURE_M249
#define	WEAPON_M249				19
#endif
#if FEATURE_DISPLACER
#define WEAPON_DISPLACER		20
#endif
#if FEATURE_MEDKIT
#define WEAPON_MEDKIT			21
#endif
#if FEATURE_SHOCKRIFLE
#define WEAPON_SHOCKRIFLE		22
#endif
#if FEATURE_SPORELAUNCHER
#define WEAPON_SPORELAUNCHER	23
#endif
#if FEATURE_SNIPERRIFLE
#define WEAPON_SNIPERRIFLE		24
#endif
#if FEATURE_KNIFE
#define WEAPON_KNIFE			25
#endif
#if FEATURE_PENGUIN
#define	WEAPON_PENGUIN			26
#endif
#if FEATURE_UZI
#define WEAPON_UZI				27
#endif

#endif