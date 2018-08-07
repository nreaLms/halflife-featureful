#pragma once
#ifndef MOD_FEATURES_H
#define MOD_FEATURES_H

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
#define FEATURE_PITWORM 1
#define FEATURE_GENEWORM 1
#define FEATURE_DRILLSERGEANT 1
#define FEATURE_RECRUIT 1

#define FEATURE_ISLAVE_DEAD 1
#define FEATURE_HOUNDEYE_DEAD 1
#define FEATURE_OPFOR_DEADHAZ 0
#define FEATURE_SKELETON 1

// whether fgrunts and black mesa personnel are enemies like in Opposing Force
#define FEATURE_OPFOR_ALLY_RELATIONSHIP 1

// whether race x and alien military are enemies
#define FEATURE_RACEX_AND_AMIL_ENEMIES 1

// whether monsters get health from eating meat or enemy corpses
#define FEATURE_EAT_FOR_HEALTH 1

// New weapons
#define FEATURE_PIPEWRENCH 1
#define FEATURE_KNIFE 1
#define FEATURE_GRAPPLE 1
#define FEATURE_DESERT_EAGLE 1
#define FEATURE_PENGUIN 1
#define FEATURE_M249 1
#define FEATURE_SNIPERRIFLE 1
#define FEATURE_DISPLACER 1
#define FEATURE_SHOCKRIFLE 1
#define FEATURE_SPORELAUNCHER 1
#define FEATURE_MEDKIT 0

// Weapon features
#define FEATURE_CROWBAR_IDLE_ANIM 1
#define FEATURE_TRIPMINE_OPFOR_SIZE 0

// Dependent features
#define FEATURE_SHOCKBEAM (FEATURE_SHOCKTROOPER || FEATURE_SHOCKRIFLE)
#define FEATURE_SPOREGRENADE (FEATURE_SHOCKTROOPER || FEATURE_SPORELAUNCHER)

// Replacement stuff
#if FEATURE_DESERT_EAGLE
#define DESERT_EAGLE_DROP_NAME "weapon_eagle"
#else
#define DESERT_EAGLE_DROP_NAME "ammo_357"
#endif

#if FEATURE_M249
#define M249_DROP_NAME "weapon_m249"
#else
#define M249_DROP_NAME "ammo_9mmAR"
#endif

#if FEATURE_OPFOR_DEADHAZ
#define DEADHAZMODEL "models/deadhaz.mdl"
#else
#define DEADHAZMODEL "models/player.mdl"
#endif

// Misc features
#define FEATURE_OPFOR_DECALS 0
#define FEATURE_OPFOR_NIGHTVISION 0
#define FEATURE_CS_NIGHTVISION 0

#define FEATURE_NIGHTVISION (FEATURE_OPFOR_NIGHTVISION || FEATURE_CS_NIGHTVISION)

#endif
