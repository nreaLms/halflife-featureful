#pragma once
#ifndef FEATURES_H
#define FEATURES_H

/*
 * 0 to disable feature
 * 1 to enable feature
 */

// New monsters
#define FEATURE_ZOMBIE_BARNEY 1
#define FEATURE_ZOMBIE_SOLDIER 1
#define FEATURE_GONOME 1
#define FEATURE_PITDRONE 1
#define FEATURE_SHOCKTROOPER 1
#define FEATURE_VOLTIFORE 1
#define FEATURE_MASSN 1
#define FEATURE_BLACK_OSPREY 1
#define FEATURE_BLACK_APACHE 1
#define FEATURE_OTIS 1
#define FEATURE_CLEANSUIT_SCIENTIST 1
#define FEATURE_BABYGARG 1
#define FEATURE_OPFOR_GRUNT 1

// whether fgrunts and black mesa personnel are enemies like in Opposing Force
#define FEATURE_OPFOR_ALLY_RELATIONSHIP 1

// New weapons
#define FEATURE_PIPEWRENCH 1
#define FEATURE_DESERT_EAGLE 1
#define FEATURE_SNIPERRIFLE 1
#define FEATURE_SHOCKRIFLE 0
#define FEATURE_SPORELAUNCHER 0

// Dependent features
#define FEATURE_SHOCKBEAM (FEATURE_SHOCKTROOPER || FEATURE_SHOCKRIFLE)
#define FEATURE_SPOREGRENADE (FEATURE_SHOCKTROOPER || FEATURE_SPORELAUNCHER)

// Replacement stuff
#if FEATURE_DESERT_EAGLE
#define DESERT_EAGLE_DROP_NAME "weapon_eagle"
#else
#define DESERT_EAGLE_DROP_NAME "ammo_357"
#endif

#endif
