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
#include "extdll.h"
#include "eiface.h"
#include "util.h"
#include "game.h"
#include "mod_features.h"

BOOL		g_fIsXash3D;

cvar_t displaysoundlist = {"displaysoundlist","0"};

// multiplayer server rules
cvar_t fragsleft	= { "mp_fragsleft","0", FCVAR_SERVER | FCVAR_UNLOGGED };	  // Don't spam console/log files/users with this changing
cvar_t timeleft		= { "mp_timeleft","0" , FCVAR_SERVER | FCVAR_UNLOGGED };	  // "      "

// multiplayer server rules
cvar_t teamplay		= { "mp_teamplay","0", FCVAR_SERVER };
cvar_t fraglimit	= { "mp_fraglimit","0", FCVAR_SERVER };
cvar_t timelimit	= { "mp_timelimit","0", FCVAR_SERVER };
cvar_t friendlyfire	= { "mp_friendlyfire","0", FCVAR_SERVER };
cvar_t falldamage	= { "mp_falldamage","0", FCVAR_SERVER };
cvar_t weaponstay	= { "mp_weaponstay","0", FCVAR_SERVER };
cvar_t dropweapons	= { "mp_dropweapons","1", FCVAR_SERVER };

cvar_t weapon_respawndelay = { "mp_weapon_respawndelay","-2",FCVAR_SERVER };
cvar_t ammo_respawndelay = { "mp_ammo_respawndelay","-2",FCVAR_SERVER };
cvar_t item_respawndelay = { "mp_item_respawndelay","-2",FCVAR_SERVER };
cvar_t healthcharger_rechargetime = { "mp_healthcharger_rechargetime","-2",FCVAR_SERVER };
cvar_t hevcharger_rechargetime = { "mp_hevcharger_rechargetime","-2",FCVAR_SERVER };

cvar_t selfgauss	= { "selfgauss", "0", FCVAR_SERVER };
cvar_t satchelfix	= { "satchelfix", "1", FCVAR_SERVER };
cvar_t explosionfix	= { "explosionfix", "1", FCVAR_SERVER };
cvar_t monsteryawspeedfix	= { "monsteryawspeedfix", "1", FCVAR_SERVER };
cvar_t animeventfix = {"animeventfix", "0", FCVAR_SERVER };
cvar_t corpsephysics = { "corpsephysics", "0", FCVAR_SERVER };
cvar_t forcerespawn	= { "mp_forcerespawn","1", FCVAR_SERVER };
cvar_t respawndelay	= { "mp_respawndelay","0", FCVAR_SERVER };
cvar_t flashlight	= { "mp_flashlight","0", FCVAR_SERVER };
cvar_t aimcrosshair	= { "mp_autocrosshair","1", FCVAR_SERVER };
cvar_t decalfrequency	= { "decalfrequency","30", FCVAR_SERVER };
cvar_t teamlist		= { "mp_teamlist","hgrunt;scientist", FCVAR_SERVER };
cvar_t teamoverride	= { "mp_teamoverride","1" };
cvar_t defaultteam	= { "mp_defaultteam","0" };

cvar_t allowmonsters	= { "mp_allowmonsters","0", FCVAR_SERVER };
cvar_t allowmonsterinfo = { "mp_allowmonsterinfo","0", FCVAR_SERVER };
cvar_t npc_dropweapons = { "npc_dropweapons", "1", FCVAR_SERVER };
cvar_t dmgperscore = { "mp_dmgperscore", "0", FCVAR_SERVER };
cvar_t allydmgpenalty = { "mp_allydmgpenalty", "2", FCVAR_SERVER };
cvar_t npckill = { "mp_npckill", "1", FCVAR_SERVER };

cvar_t bhopcap		= { "mp_bhopcap", "1", FCVAR_SERVER };

cvar_t allow_spectators = { "allow_spectators", "0", FCVAR_SERVER };	// 0 prevents players from being spectators
cvar_t multibyte_only = { "mp_multibyte_only", "0", FCVAR_SERVER };

#if FEATURE_USE_THROUGH_WALLS_CVAR
cvar_t use_through_walls = { "use_through_walls", "1", FCVAR_SERVER };
#endif
#if FEATURE_TRIDEPTH_CVAR
cvar_t npc_tridepth = { "npc_tridepth", "1", FCVAR_SERVER };
#endif
#if FEATURE_TRIDEPTH_ALL_CVAR
cvar_t npc_tridepth_all = { "npc_tridepth_all", "0", FCVAR_SERVER };
#endif
#if FEATURE_NPC_NEAREST_CVAR
cvar_t npc_nearest = { "npc_nearest", "0", FCVAR_SERVER };
#endif
#if FEATURE_NPC_FORGET_ENEMY_CVAR
cvar_t npc_forget_enemy_time = { "npc_forget_enemy_time", "0", FCVAR_SERVER };
#endif
#if FEATURE_NPC_FIX_MELEE_DISTANCE_CVAR
cvar_t npc_fix_melee_distance = { "npc_fix_melee_distance", "0", FCVAR_SERVER };
#endif
#if FEATURE_NPC_ACTIVE_AFTER_COMBAT_CVAR
cvar_t npc_active_after_combat = { "npc_active_after_combat", "0", FCVAR_SERVER };
#endif
#if FEATURE_NPC_FOLLOW_OUT_OF_PVS_CVAR
cvar_t npc_follow_out_of_pvs = { "npc_follow_out_of_pvs", "0", FCVAR_SERVER };
#endif
cvar_t npc_patrol = { "npc_patrol", "1", FCVAR_SERVER };

cvar_t mp_chattime	= { "mp_chattime","10", FCVAR_SERVER };

#if FEATURE_USE_TO_TAKE_CVAR
cvar_t use_to_take = { "use_to_take","0", FCVAR_SERVER };
#endif
#if FEATURE_GRENADE_JUMP_CVAR
cvar_t grenade_jump = { "grenade_jump","1", FCVAR_SERVER };
#endif

cvar_t keepinventory	= { "mp_keepinventory","0", FCVAR_SERVER }; // keep inventory across level transitions in multiplayer coop

// Engine Cvars
cvar_t *g_psv_gravity = NULL;
cvar_t *g_psv_maxspeed = NULL;
cvar_t *g_psv_aim = NULL;
cvar_t *g_footsteps = NULL;
cvar_t *g_enable_cheats = NULL;

cvar_t *g_psv_developer;

#define DECLARE_SKILL_VALUE(name, defaultValue) \
cvar_t name ## 1 = { #name "1", defaultValue, 0, 0, 0 }; \
cvar_t name ## 2 = { #name "2", defaultValue, 0, 0, 0 }; \
cvar_t name ## 3 = { #name "3", defaultValue, 0, 0, 0 };

#define DECLARE_SKILL_VALUE3(name, defaultValue1, defaultValue2, defaultValue3) \
cvar_t name ## 1 = { #name "1", defaultValue1, 0, 0, 0 }; \
cvar_t name ## 2 = { #name "2", defaultValue2, 0, 0, 0 }; \
cvar_t name ## 3 = { #name "3", defaultValue3, 0, 0, 0 };

#define REGISTER_SKILL_CVARS(name) \
CVAR_REGISTER( &name ## 1 ); \
CVAR_REGISTER( &name ## 2 ); \
CVAR_REGISTER( &name ## 3 );

//CVARS FOR SKILL LEVEL SETTINGS
// Agrunt
DECLARE_SKILL_VALUE(sk_agrunt_health, "0")
DECLARE_SKILL_VALUE(sk_agrunt_dmg_punch, "0")

// Apache
DECLARE_SKILL_VALUE(sk_apache_health, "0")

#if FEATURE_BABYGARG
// Baby Gargantua
DECLARE_SKILL_VALUE(sk_babygargantua_health, "0")
DECLARE_SKILL_VALUE(sk_babygargantua_dmg_slash, "0")
DECLARE_SKILL_VALUE(sk_babygargantua_dmg_fire, "0")
DECLARE_SKILL_VALUE(sk_babygargantua_dmg_stomp, "0")
#endif

// Barnacle
DECLARE_SKILL_VALUE(sk_barnacle_health, "25")

// Barney
DECLARE_SKILL_VALUE(sk_barney_health, "0")

// Bullsquid
DECLARE_SKILL_VALUE(sk_bullsquid_health, "0")
DECLARE_SKILL_VALUE(sk_bullsquid_dmg_bite, "0")
DECLARE_SKILL_VALUE(sk_bullsquid_dmg_whip, "0")
DECLARE_SKILL_VALUE(sk_bullsquid_dmg_spit, "0")

// Big Momma
DECLARE_SKILL_VALUE(sk_bigmomma_health_factor, "1.0")
DECLARE_SKILL_VALUE(sk_bigmomma_dmg_slash, "50")
DECLARE_SKILL_VALUE(sk_bigmomma_dmg_blast, "100")
DECLARE_SKILL_VALUE(sk_bigmomma_radius_blast, "250")

#if FEATURE_CLEANSUIT_SCIENTIST
// Cleansuit Scientist
DECLARE_SKILL_VALUE(sk_cleansuit_scientist_health, "0")
#endif

// Gargantua
DECLARE_SKILL_VALUE(sk_gargantua_health, "0")
DECLARE_SKILL_VALUE(sk_gargantua_dmg_slash, "0")
DECLARE_SKILL_VALUE(sk_gargantua_dmg_fire, "0")
DECLARE_SKILL_VALUE(sk_gargantua_dmg_stomp, "0")

// Hassassin
DECLARE_SKILL_VALUE(sk_hassassin_health, "0")

// Headcrab
DECLARE_SKILL_VALUE(sk_headcrab_health, "0")
DECLARE_SKILL_VALUE(sk_headcrab_dmg_bite, "0")

#if FEATURE_OPFOR_GRUNT
// Opposing Force Hgrunt
DECLARE_SKILL_VALUE(sk_hgrunt_ally_health, "0")
DECLARE_SKILL_VALUE(sk_hgrunt_ally_kick, "0")
DECLARE_SKILL_VALUE(sk_hgrunt_ally_pellets, "0")
DECLARE_SKILL_VALUE(sk_hgrunt_ally_gspeed, "0")

// Medic
DECLARE_SKILL_VALUE(sk_medic_ally_health, "0")
DECLARE_SKILL_VALUE(sk_medic_ally_kick, "0")
DECLARE_SKILL_VALUE(sk_medic_ally_gspeed, "0")
DECLARE_SKILL_VALUE(sk_medic_ally_heal, "0")

// Torch
DECLARE_SKILL_VALUE(sk_torch_ally_health, "0")
DECLARE_SKILL_VALUE(sk_torch_ally_kick, "0")
DECLARE_SKILL_VALUE(sk_torch_ally_gspeed, "0")
#endif

// Hgrunt 
DECLARE_SKILL_VALUE(sk_hgrunt_health, "0")
DECLARE_SKILL_VALUE(sk_hgrunt_kick, "0")
DECLARE_SKILL_VALUE(sk_hgrunt_pellets, "0")
DECLARE_SKILL_VALUE(sk_hgrunt_gspeed, "0")

#if FEATURE_HWGRUNT
DECLARE_SKILL_VALUE(sk_hwgrunt_health, "0")
#endif

// Houndeye
DECLARE_SKILL_VALUE(sk_houndeye_health, "0")
DECLARE_SKILL_VALUE(sk_houndeye_dmg_blast, "0")

// ISlave
DECLARE_SKILL_VALUE(sk_islave_health, "0")
DECLARE_SKILL_VALUE(sk_islave_dmg_claw, "0")
DECLARE_SKILL_VALUE(sk_islave_dmg_clawrake, "0")
DECLARE_SKILL_VALUE(sk_islave_dmg_zap, "0")

DECLARE_SKILL_VALUE3(sk_islave_zap_rate, "1", "1", "1.5")
DECLARE_SKILL_VALUE3(sk_islave_revival, "0", "0", "1")

// Icthyosaur
DECLARE_SKILL_VALUE(sk_ichthyosaur_health, "0")
DECLARE_SKILL_VALUE(sk_ichthyosaur_shake, "0")

// Leech
DECLARE_SKILL_VALUE(sk_leech_health, "0")
DECLARE_SKILL_VALUE(sk_leech_dmg_bite, "0")

// Controller
DECLARE_SKILL_VALUE(sk_controller_health, "0")
DECLARE_SKILL_VALUE(sk_controller_dmgzap, "0")
DECLARE_SKILL_VALUE(sk_controller_speedball, "0")
DECLARE_SKILL_VALUE(sk_controller_dmgball, "0")

#if FEATURE_MASSN
// Massassin
DECLARE_SKILL_VALUE(sk_massassin_health, "0")
DECLARE_SKILL_VALUE(sk_massassin_kick, "0")
DECLARE_SKILL_VALUE(sk_massassin_gspeed, "0")
#endif

// Nihilanth
DECLARE_SKILL_VALUE(sk_nihilanth_health, "0")
DECLARE_SKILL_VALUE(sk_nihilanth_zap, "0")

// Osprey
DECLARE_SKILL_VALUE(sk_osprey, "400")

#if FEATURE_BLACK_OSPREY
// Blackops Osprey
DECLARE_SKILL_VALUE(sk_blkopsosprey, "0")
#endif

#if FEATURE_OTIS
// Otis
DECLARE_SKILL_VALUE(sk_otis_health, "0")
#endif

#if FEATURE_KATE
// Kate
DECLARE_SKILL_VALUE(sk_kate_health, "0")
#endif

#if FEATURE_PITDRONE
// Pitdrone
DECLARE_SKILL_VALUE(sk_pitdrone_health, "0")
DECLARE_SKILL_VALUE(sk_pitdrone_dmg_bite, "0")
DECLARE_SKILL_VALUE(sk_pitdrone_dmg_whip, "0")
DECLARE_SKILL_VALUE(sk_pitdrone_dmg_spit, "0")
#endif

#if FEATURE_PITWORM
// Pitworm
DECLARE_SKILL_VALUE(sk_pitworm_health, "0")
DECLARE_SKILL_VALUE(sk_pitworm_dmg_swipe, "0")
DECLARE_SKILL_VALUE(sk_pitworm_dmg_beam, "0")
#endif

#if FEATURE_GENEWORM
// Geneworm
DECLARE_SKILL_VALUE(sk_geneworm_health, "0")
DECLARE_SKILL_VALUE(sk_geneworm_dmg_spit, "0")
DECLARE_SKILL_VALUE(sk_geneworm_dmg_hit, "0")
#endif

// Scientist
DECLARE_SKILL_VALUE(sk_scientist_health, "0")

#if FEATURE_ROBOGRUNT
DECLARE_SKILL_VALUE(sk_rgrunt_explode, "0")
#endif

#if FEATURE_SHOCKTROOPER
// Shock Roach
DECLARE_SKILL_VALUE(sk_shockroach_health, "0")
DECLARE_SKILL_VALUE(sk_shockroach_dmg_bite, "0")
DECLARE_SKILL_VALUE(sk_shockroach_lifespan, "0")

// ShockTrooper
DECLARE_SKILL_VALUE(sk_shocktrooper_health, "0")
DECLARE_SKILL_VALUE(sk_shocktrooper_kick, "0")
DECLARE_SKILL_VALUE(sk_shocktrooper_gspeed, "0")
DECLARE_SKILL_VALUE(sk_shocktrooper_maxcharge, "0")
DECLARE_SKILL_VALUE(sk_shocktrooper_rchgspeed, "0")
#endif

// Snark
DECLARE_SKILL_VALUE(sk_snark_health, "0")
DECLARE_SKILL_VALUE(sk_snark_dmg_bite, "0")
DECLARE_SKILL_VALUE(sk_snark_dmg_pop, "0")

#if FEATURE_VOLTIFORE
// Voltigore
DECLARE_SKILL_VALUE(sk_voltigore_health, "0")
DECLARE_SKILL_VALUE(sk_voltigore_dmg_punch, "0")
DECLARE_SKILL_VALUE(sk_voltigore_dmg_beam, "0")
DECLARE_SKILL_VALUE(sk_voltigore_dmg_explode, "0")

// Baby Voltigore
DECLARE_SKILL_VALUE(sk_babyvoltigore_health, "0")
DECLARE_SKILL_VALUE(sk_babyvoltigore_dmg_punch, "0")
#endif

// Zombie
DECLARE_SKILL_VALUE(sk_zombie_health, "0")
DECLARE_SKILL_VALUE(sk_zombie_dmg_one_slash, "0")
DECLARE_SKILL_VALUE(sk_zombie_dmg_both_slash, "0")

#if FEATURE_ZOMBIE_BARNEY
// Zombie Barney
DECLARE_SKILL_VALUE(sk_zombie_barney_health, "0")
DECLARE_SKILL_VALUE(sk_zombie_barney_dmg_one_slash, "0")
DECLARE_SKILL_VALUE(sk_zombie_barney_dmg_both_slash, "0")
#endif

#if FEATURE_ZOMBIE_SOLDIER
// Zombie Soldier
DECLARE_SKILL_VALUE(sk_zombie_soldier_health, "0")
DECLARE_SKILL_VALUE(sk_zombie_soldier_dmg_one_slash, "0")
DECLARE_SKILL_VALUE(sk_zombie_soldier_dmg_both_slash, "0")
#endif

#if FEATURE_GONOME
// Gonome
DECLARE_SKILL_VALUE(sk_gonome_health, "0")
DECLARE_SKILL_VALUE(sk_gonome_dmg_one_slash, "0")
DECLARE_SKILL_VALUE(sk_gonome_dmg_guts, "0")
DECLARE_SKILL_VALUE(sk_gonome_dmg_one_bite, "0")
#endif

//Turret
DECLARE_SKILL_VALUE(sk_turret_health, "0")

// MiniTurret
DECLARE_SKILL_VALUE(sk_miniturret_health, "0")

// Sentry Turret
DECLARE_SKILL_VALUE(sk_sentry_health, "0")

// PLAYER WEAPONS

// Crowbar whack
DECLARE_SKILL_VALUE(sk_plr_crowbar, "0")

// Glock Round
DECLARE_SKILL_VALUE(sk_plr_9mm_bullet, "0")

// 357 Round
DECLARE_SKILL_VALUE(sk_plr_357_bullet, "0")

// MP5 Round
DECLARE_SKILL_VALUE(sk_plr_9mmAR_bullet, "0")

// M203 grenade
DECLARE_SKILL_VALUE(sk_plr_9mmAR_grenade, "0")

// Shotgun buckshot
DECLARE_SKILL_VALUE(sk_plr_buckshot, "0")

// Crossbow
DECLARE_SKILL_VALUE(sk_plr_xbow_bolt_client, "0")
DECLARE_SKILL_VALUE(sk_plr_xbow_bolt_monster, "0")

// RPG
DECLARE_SKILL_VALUE(sk_plr_rpg, "0")

// Tau Cannon
DECLARE_SKILL_VALUE(sk_plr_gauss, "0")

// Gluon Gun
DECLARE_SKILL_VALUE(sk_plr_egon_narrow, "0")
DECLARE_SKILL_VALUE(sk_plr_egon_wide, "0")

// Hand Grendade
DECLARE_SKILL_VALUE(sk_plr_hand_grenade, "0")

// Satchel Charge
DECLARE_SKILL_VALUE(sk_plr_satchel, "0")

// Tripmine
DECLARE_SKILL_VALUE(sk_plr_tripmine, "0")

#if FEATURE_DESERT_EAGLE
// Desert Eagle
DECLARE_SKILL_VALUE(sk_plr_eagle, "0")
#endif

#if FEATURE_PIPEWRENCH
// Pipe wrench
DECLARE_SKILL_VALUE(sk_plr_pipewrench, "0")
#endif

#if FEATURE_KNIFE
// Knife
DECLARE_SKILL_VALUE(sk_plr_knife, "0")
#endif

#if FEATURE_GRAPPLE
// Grapple
DECLARE_SKILL_VALUE(sk_plr_grapple, "0")
#endif

#if FEATURE_M249
// M249
DECLARE_SKILL_VALUE(sk_plr_556_bullet, "0")
#endif

#if FEATURE_SNIPERRIFLE
// Sniper rifle
DECLARE_SKILL_VALUE(sk_plr_762_bullet, "0")
#endif

#if FEATURE_MEDKIT
// Medkit
DECLARE_SKILL_VALUE(sk_plr_medkitshot, "10")

DECLARE_SKILL_VALUE3(sk_plr_medkittime, "3", "5", "0")
#endif

// WORLD WEAPONS
DECLARE_SKILL_VALUE(sk_12mm_bullet, "0")
DECLARE_SKILL_VALUE(sk_9mmAR_bullet, "0")
DECLARE_SKILL_VALUE(sk_9mm_bullet, "0")
DECLARE_SKILL_VALUE(sk_357_bullet, "34")
DECLARE_SKILL_VALUE(sk_556_bullet, "15")
DECLARE_SKILL_VALUE3(sk_762_bullet, "35", "40", "40")

#if FEATURE_SHOCKBEAM
DECLARE_SKILL_VALUE(sk_plr_shockroachs, "0")
DECLARE_SKILL_VALUE(sk_plr_shockroachm, "0")
#endif

#if FEATURE_SPOREGRENADE
DECLARE_SKILL_VALUE(sk_plr_spore, "0")
#endif

#if FEATURE_DISPLACER
DECLARE_SKILL_VALUE(sk_plr_displacer_other, "0")
DECLARE_SKILL_VALUE(sk_plr_displacer_radius, "0")
#endif

#if FEATURE_UZI
DECLARE_SKILL_VALUE(sk_plr_uzi, "6")
#endif

// HORNET
DECLARE_SKILL_VALUE(sk_hornet_dmg, "0")
DECLARE_SKILL_VALUE(sk_plr_hornet_dmg, "7")

// MORTAR
DECLARE_SKILL_VALUE(sk_mortar, "200")

// HEALTH/CHARGE
DECLARE_SKILL_VALUE(sk_suitcharger, "0")
DECLARE_SKILL_VALUE(sk_battery, "0")
DECLARE_SKILL_VALUE(sk_healthcharger, "0")
DECLARE_SKILL_VALUE(sk_healthkit, "0")
DECLARE_SKILL_VALUE(sk_scientist_heal, "0")
DECLARE_SKILL_VALUE(sk_scientist_heal_time, "60")
DECLARE_SKILL_VALUE(sk_soda, "1")

// monster damage adjusters
DECLARE_SKILL_VALUE(sk_monster_head, "2")
DECLARE_SKILL_VALUE(sk_monster_chest, "1")
DECLARE_SKILL_VALUE(sk_monster_stomach, "1")
DECLARE_SKILL_VALUE(sk_monster_arm, "1")
DECLARE_SKILL_VALUE(sk_monster_leg, "1")

// player damage adjusters
DECLARE_SKILL_VALUE(sk_player_head, "2")
DECLARE_SKILL_VALUE(sk_player_chest, "1")
DECLARE_SKILL_VALUE(sk_player_stomach, "1")
DECLARE_SKILL_VALUE(sk_player_arm, "1")
DECLARE_SKILL_VALUE(sk_player_leg, "1")

// flashlight settings
DECLARE_SKILL_VALUE(sk_flashlight_drain_time, "120")
DECLARE_SKILL_VALUE(sk_flashlight_charge_time, "20")

// END Cvars for Skill Level settings

void Cmd_ReportAIState()
{
	ReportAIStateByClassname(CMD_ARGV( 1 ));
}

// Register your console variables here
// This gets called one time when the game is initialied
void GameDLLInit( void )
{
	// Register cvars here:
	if( CVAR_GET_POINTER( "build" ) )
		g_fIsXash3D = TRUE;

	g_psv_gravity = CVAR_GET_POINTER( "sv_gravity" );
	g_psv_maxspeed = CVAR_GET_POINTER( "sv_maxspeed" );
	g_psv_aim = CVAR_GET_POINTER( "sv_aim" );
	g_footsteps = CVAR_GET_POINTER( "mp_footsteps" );

	g_psv_developer = CVAR_GET_POINTER( "developer" );

	g_enable_cheats = CVAR_GET_POINTER( "sv_cheats" );


	CVAR_REGISTER( &displaysoundlist );
	CVAR_REGISTER( &allow_spectators );
#if FEATURE_USE_THROUGH_WALLS_CVAR
	CVAR_REGISTER( &use_through_walls );
#endif
#if FEATURE_TRIDEPTH_CVAR
	CVAR_REGISTER( &npc_tridepth );
#endif
#if FEATURE_TRIDEPTH_ALL_CVAR
	CVAR_REGISTER( &npc_tridepth_all );
#endif
#if FEATURE_NPC_NEAREST_CVAR
	CVAR_REGISTER( &npc_nearest );
#endif
#if FEATURE_NPC_FORGET_ENEMY_CVAR
	CVAR_REGISTER( &npc_forget_enemy_time );
#endif
#if FEATURE_NPC_FIX_MELEE_DISTANCE_CVAR
	CVAR_REGISTER( &npc_fix_melee_distance );
#endif
#if FEATURE_NPC_ACTIVE_AFTER_COMBAT_CVAR
	CVAR_REGISTER( &npc_active_after_combat );
#endif
#if FEATURE_NPC_FOLLOW_OUT_OF_PVS_CVAR
	CVAR_REGISTER( &npc_follow_out_of_pvs );
#endif
	CVAR_REGISTER( &npc_patrol );

	CVAR_REGISTER( &teamplay );
	CVAR_REGISTER( &fraglimit );
	CVAR_REGISTER( &timelimit );

	CVAR_REGISTER( &fragsleft );
	CVAR_REGISTER( &timeleft );

	CVAR_REGISTER( &friendlyfire );
	CVAR_REGISTER( &falldamage );
	CVAR_REGISTER( &weaponstay );
	CVAR_REGISTER( &dropweapons );

	CVAR_REGISTER( &weapon_respawndelay );
	CVAR_REGISTER( &ammo_respawndelay );
	CVAR_REGISTER( &item_respawndelay );
	CVAR_REGISTER( &healthcharger_rechargetime );
	CVAR_REGISTER( &hevcharger_rechargetime );

	CVAR_REGISTER( &selfgauss );
	CVAR_REGISTER( &satchelfix );
	CVAR_REGISTER( &explosionfix );
	CVAR_REGISTER( &monsteryawspeedfix );
	CVAR_REGISTER( &animeventfix );
	CVAR_REGISTER( &corpsephysics );
	CVAR_REGISTER( &forcerespawn );
	CVAR_REGISTER( &respawndelay );
	CVAR_REGISTER( &flashlight );
	CVAR_REGISTER( &aimcrosshair );
	CVAR_REGISTER( &decalfrequency );
	CVAR_REGISTER( &teamlist );
	CVAR_REGISTER( &teamoverride );
	CVAR_REGISTER( &defaultteam );

	CVAR_REGISTER( &allowmonsters );
	CVAR_REGISTER( &allowmonsterinfo );
	CVAR_REGISTER( &npc_dropweapons );
	CVAR_REGISTER( &dmgperscore );
	CVAR_REGISTER( &allydmgpenalty );
	CVAR_REGISTER( &npckill );

	CVAR_REGISTER( &bhopcap );
	CVAR_REGISTER( &multibyte_only );

	CVAR_REGISTER( &mp_chattime );

#if FEATURE_USE_TO_TAKE_CVAR
	CVAR_REGISTER( &use_to_take );
#endif
#if FEATURE_GRENADE_JUMP_CVAR
	CVAR_REGISTER( &grenade_jump );
#endif

	CVAR_REGISTER( &keepinventory );

// REGISTER CVARS FOR SKILL LEVEL STUFF
	// Agrunt
	REGISTER_SKILL_CVARS(sk_agrunt_health);
	REGISTER_SKILL_CVARS(sk_agrunt_dmg_punch);

	// Apache
	REGISTER_SKILL_CVARS( sk_apache_health );
	
	// Barnacle
	REGISTER_SKILL_CVARS(sk_barnacle_health);

#if FEATURE_BABYGARG
	// Baby Gargantua
	REGISTER_SKILL_CVARS(sk_babygargantua_health);
	REGISTER_SKILL_CVARS(sk_babygargantua_dmg_slash);
	REGISTER_SKILL_CVARS(sk_babygargantua_dmg_fire);
	REGISTER_SKILL_CVARS(sk_babygargantua_dmg_stomp);
#endif

	// Barney
	REGISTER_SKILL_CVARS(sk_barney_health);

	// Bullsquid
	REGISTER_SKILL_CVARS(sk_bullsquid_health);
	REGISTER_SKILL_CVARS(sk_bullsquid_dmg_bite);
	REGISTER_SKILL_CVARS(sk_bullsquid_dmg_whip);
	REGISTER_SKILL_CVARS(sk_bullsquid_dmg_spit);

	// Big Momma
	REGISTER_SKILL_CVARS(sk_bigmomma_health_factor);
	REGISTER_SKILL_CVARS(sk_bigmomma_dmg_slash);
	REGISTER_SKILL_CVARS(sk_bigmomma_dmg_blast);
	REGISTER_SKILL_CVARS(sk_bigmomma_radius_blast);

#if FEATURE_CLEANSUIT_SCIENTIST
	// Cleansuit Scientist
	REGISTER_SKILL_CVARS(sk_cleansuit_scientist_health);
#endif

	// Gargantua
	REGISTER_SKILL_CVARS(sk_gargantua_health);
	REGISTER_SKILL_CVARS(sk_gargantua_dmg_slash);
	REGISTER_SKILL_CVARS(sk_gargantua_dmg_fire);
	REGISTER_SKILL_CVARS(sk_gargantua_dmg_stomp);

	// Hassassin
	REGISTER_SKILL_CVARS(sk_hassassin_health);

	// Headcrab
	REGISTER_SKILL_CVARS(sk_headcrab_health);
	REGISTER_SKILL_CVARS(sk_headcrab_dmg_bite);

#if FEATURE_OPFOR_GRUNT
	// Opposing Hgrunt
	REGISTER_SKILL_CVARS(sk_hgrunt_ally_health);
	REGISTER_SKILL_CVARS(sk_hgrunt_ally_kick);
	REGISTER_SKILL_CVARS(sk_hgrunt_ally_pellets);
	REGISTER_SKILL_CVARS(sk_hgrunt_ally_gspeed);

	// Medic
	REGISTER_SKILL_CVARS(sk_medic_ally_health);
	REGISTER_SKILL_CVARS(sk_medic_ally_kick);
	REGISTER_SKILL_CVARS(sk_medic_ally_gspeed);
	REGISTER_SKILL_CVARS(sk_medic_ally_heal);

	// Torch
	REGISTER_SKILL_CVARS(sk_torch_ally_health);
	REGISTER_SKILL_CVARS(sk_torch_ally_kick);
	REGISTER_SKILL_CVARS(sk_torch_ally_gspeed);
#endif

	// Hgrunt
	REGISTER_SKILL_CVARS(sk_hgrunt_health);
	REGISTER_SKILL_CVARS(sk_hgrunt_kick);
	REGISTER_SKILL_CVARS(sk_hgrunt_pellets);
	REGISTER_SKILL_CVARS(sk_hgrunt_gspeed);

#if FEATURE_HWGRUNT
	// HWgrunt
	REGISTER_SKILL_CVARS(sk_hwgrunt_health);
#endif

	// Houndeye
	REGISTER_SKILL_CVARS(sk_houndeye_health);
	REGISTER_SKILL_CVARS(sk_houndeye_dmg_blast);

	// ISlave
	REGISTER_SKILL_CVARS(sk_islave_health);
	REGISTER_SKILL_CVARS(sk_islave_dmg_claw);
	REGISTER_SKILL_CVARS(sk_islave_dmg_clawrake);
	REGISTER_SKILL_CVARS(sk_islave_dmg_zap);
	REGISTER_SKILL_CVARS(sk_islave_zap_rate);
	REGISTER_SKILL_CVARS(sk_islave_revival);

	// Icthyosaur
	REGISTER_SKILL_CVARS(sk_ichthyosaur_health);
	REGISTER_SKILL_CVARS(sk_ichthyosaur_shake);

	// Leech
	REGISTER_SKILL_CVARS(sk_leech_health);
	REGISTER_SKILL_CVARS(sk_leech_dmg_bite);

	// Controller
	REGISTER_SKILL_CVARS(sk_controller_health);
	REGISTER_SKILL_CVARS(sk_controller_dmgzap);
	REGISTER_SKILL_CVARS(sk_controller_speedball);
	REGISTER_SKILL_CVARS(sk_controller_dmgball);

#if FEATURE_MASSN
	// Massassin
	REGISTER_SKILL_CVARS(sk_massassin_health);
	REGISTER_SKILL_CVARS(sk_massassin_kick);
	REGISTER_SKILL_CVARS(sk_massassin_gspeed);
#endif

	// Nihilanth
	REGISTER_SKILL_CVARS(sk_nihilanth_health);
	REGISTER_SKILL_CVARS(sk_nihilanth_zap);

	// Osprey
	REGISTER_SKILL_CVARS(sk_osprey);

#if FEATURE_BLACK_OSPREY
	// Blackops Osprey
	REGISTER_SKILL_CVARS(sk_blkopsosprey);
#endif

#if FEATURE_OTIS
	// Otis
	REGISTER_SKILL_CVARS(sk_otis_health);
#endif

#if FEATURE_KATE
	// Kate
	REGISTER_SKILL_CVARS(sk_kate_health);
#endif

#if FEATURE_PITDRONE
	// Pitdrone
	REGISTER_SKILL_CVARS(sk_pitdrone_health);
	REGISTER_SKILL_CVARS(sk_pitdrone_dmg_bite);
	REGISTER_SKILL_CVARS(sk_pitdrone_dmg_whip);
	REGISTER_SKILL_CVARS(sk_pitdrone_dmg_spit);
#endif

#if FEATURE_PITWORM
	// Pitworm
	REGISTER_SKILL_CVARS(sk_pitworm_health);
	REGISTER_SKILL_CVARS(sk_pitworm_dmg_swipe);
	REGISTER_SKILL_CVARS(sk_pitworm_dmg_beam);
#endif

#if FEATURE_GENEWORM
	// Geneworm
	REGISTER_SKILL_CVARS(sk_geneworm_health);
	REGISTER_SKILL_CVARS(sk_geneworm_dmg_spit);
	REGISTER_SKILL_CVARS(sk_geneworm_dmg_hit);
#endif

#if FEATURE_ROBOGRUNT
	REGISTER_SKILL_CVARS(sk_rgrunt_explode);
#endif


#if FEATURE_SHOCKTROOPER
	// ShockTrooper
	REGISTER_SKILL_CVARS(sk_shocktrooper_health);
	REGISTER_SKILL_CVARS(sk_shocktrooper_kick);
	REGISTER_SKILL_CVARS(sk_shocktrooper_gspeed);
	REGISTER_SKILL_CVARS(sk_shocktrooper_maxcharge);
	REGISTER_SKILL_CVARS(sk_shocktrooper_rchgspeed);

	// Shock Roach
	REGISTER_SKILL_CVARS(sk_shockroach_health);
	REGISTER_SKILL_CVARS(sk_shockroach_dmg_bite);
	REGISTER_SKILL_CVARS(sk_shockroach_lifespan);
#endif

	// Scientist
	REGISTER_SKILL_CVARS(sk_scientist_health);

	// Snark
	REGISTER_SKILL_CVARS(sk_snark_health);
	REGISTER_SKILL_CVARS(sk_snark_dmg_bite);
	REGISTER_SKILL_CVARS(sk_snark_dmg_pop);

#if FEATURE_VOLTIFORE
	// Voltigore
	REGISTER_SKILL_CVARS(sk_voltigore_health);
	REGISTER_SKILL_CVARS(sk_voltigore_dmg_punch);
	REGISTER_SKILL_CVARS(sk_voltigore_dmg_beam);
	REGISTER_SKILL_CVARS(sk_voltigore_dmg_explode);

	// Baby Voltigore
	REGISTER_SKILL_CVARS(sk_babyvoltigore_health);
	REGISTER_SKILL_CVARS(sk_babyvoltigore_dmg_punch);
#endif

	// Zombie
	REGISTER_SKILL_CVARS(sk_zombie_health);
	REGISTER_SKILL_CVARS(sk_zombie_dmg_one_slash);
	REGISTER_SKILL_CVARS(sk_zombie_dmg_both_slash);

#if FEATURE_ZOMBIE_BARNEY
	// Zombie Barney
	REGISTER_SKILL_CVARS(sk_zombie_barney_health);
	REGISTER_SKILL_CVARS(sk_zombie_barney_dmg_one_slash);
	REGISTER_SKILL_CVARS(sk_zombie_barney_dmg_both_slash);
#endif

#if FEATURE_ZOMBIE_SOLDIER
	// Zombie Soldier
	REGISTER_SKILL_CVARS(sk_zombie_soldier_health);
	REGISTER_SKILL_CVARS(sk_zombie_soldier_dmg_one_slash);
	REGISTER_SKILL_CVARS(sk_zombie_soldier_dmg_both_slash);
#endif

#if FEATURE_GONOME
	// Gonome
	REGISTER_SKILL_CVARS(sk_gonome_health);
	REGISTER_SKILL_CVARS(sk_gonome_dmg_one_slash);
	REGISTER_SKILL_CVARS(sk_gonome_dmg_guts);
	REGISTER_SKILL_CVARS(sk_gonome_dmg_one_bite);
#endif

	//Turret
	REGISTER_SKILL_CVARS(sk_turret_health);

	// MiniTurret
	REGISTER_SKILL_CVARS(sk_miniturret_health);

	// Sentry Turret
	REGISTER_SKILL_CVARS(sk_sentry_health);

	// PLAYER WEAPONS

	// Crowbar whack
	REGISTER_SKILL_CVARS(sk_plr_crowbar);

	// Glock Round
	REGISTER_SKILL_CVARS(sk_plr_9mm_bullet);

	// 357 Round
	REGISTER_SKILL_CVARS(sk_plr_357_bullet);

	// MP5 Round
	REGISTER_SKILL_CVARS(sk_plr_9mmAR_bullet);

	// M203 grenade
	REGISTER_SKILL_CVARS(sk_plr_9mmAR_grenade);

	// Shotgun buckshot
	REGISTER_SKILL_CVARS(sk_plr_buckshot);

	// Crossbow
	REGISTER_SKILL_CVARS(sk_plr_xbow_bolt_monster);
	REGISTER_SKILL_CVARS(sk_plr_xbow_bolt_client);

	// RPG
	REGISTER_SKILL_CVARS(sk_plr_rpg);

	// Tau Cannon
	REGISTER_SKILL_CVARS(sk_plr_gauss);

	// Gluon Gun
	REGISTER_SKILL_CVARS(sk_plr_egon_narrow);
	REGISTER_SKILL_CVARS(sk_plr_egon_wide);

	// Hand Grendade
	REGISTER_SKILL_CVARS(sk_plr_hand_grenade);

	// Satchel Charge
	REGISTER_SKILL_CVARS(sk_plr_satchel);

	// Tripmine
	REGISTER_SKILL_CVARS(sk_plr_tripmine);

#if FEATURE_MEDKIT
	// Medkit
	REGISTER_SKILL_CVARS(sk_plr_medkitshot);
	REGISTER_SKILL_CVARS(sk_plr_medkittime);
#endif

#if FEATURE_DESERT_EAGLE
	// Desert Eagle
	REGISTER_SKILL_CVARS(sk_plr_eagle);
#endif

#if FEATURE_PIPEWRENCH
	// Pipe wrench
	REGISTER_SKILL_CVARS(sk_plr_pipewrench);
#endif

#if FEATURE_KNIFE
	// Knife
	REGISTER_SKILL_CVARS(sk_plr_knife);
#endif

#if FEATURE_GRAPPLE
	// Grapple
	REGISTER_SKILL_CVARS(sk_plr_grapple);
#endif

#if FEATURE_M249
	// M249
	REGISTER_SKILL_CVARS(sk_plr_556_bullet);
#endif

#if FEATURE_SNIPERRIFLE
	// Sniper rifle
	REGISTER_SKILL_CVARS(sk_plr_762_bullet);
#endif

#if FEATURE_SHOCKBEAM
	// Shock rifle
	REGISTER_SKILL_CVARS(sk_plr_shockroachs);
	REGISTER_SKILL_CVARS(sk_plr_shockroachm);
#endif

#if FEATURE_SPOREGRENADE
	REGISTER_SKILL_CVARS(sk_plr_spore);
#endif

#if FEATURE_DISPLACER
	REGISTER_SKILL_CVARS(sk_plr_displacer_other);
	REGISTER_SKILL_CVARS(sk_plr_displacer_radius);
#endif

#if FEATURE_UZI
	REGISTER_SKILL_CVARS(sk_plr_uzi);
#endif

	// WORLD WEAPONS
	REGISTER_SKILL_CVARS(sk_12mm_bullet);
	REGISTER_SKILL_CVARS(sk_9mmAR_bullet);
	REGISTER_SKILL_CVARS(sk_9mm_bullet);
	REGISTER_SKILL_CVARS(sk_357_bullet);
	REGISTER_SKILL_CVARS(sk_556_bullet);
	REGISTER_SKILL_CVARS(sk_762_bullet);

	// HORNET
	REGISTER_SKILL_CVARS(sk_hornet_dmg);
	REGISTER_SKILL_CVARS(sk_plr_hornet_dmg);

	// MORTAR
	REGISTER_SKILL_CVARS(sk_mortar);

	// HEALTH/SUIT CHARGE DISTRIBUTION
	REGISTER_SKILL_CVARS(sk_suitcharger);
	REGISTER_SKILL_CVARS(sk_battery);
	REGISTER_SKILL_CVARS(sk_healthcharger);
	REGISTER_SKILL_CVARS(sk_healthkit);
	REGISTER_SKILL_CVARS(sk_scientist_heal);
	REGISTER_SKILL_CVARS(sk_scientist_heal_time);
	REGISTER_SKILL_CVARS(sk_soda);

	// monster damage adjusters
	REGISTER_SKILL_CVARS(sk_monster_head);
	REGISTER_SKILL_CVARS(sk_monster_chest);
	REGISTER_SKILL_CVARS(sk_monster_stomach);
	REGISTER_SKILL_CVARS(sk_monster_arm);
	REGISTER_SKILL_CVARS(sk_monster_leg);

	// player damage adjusters
	REGISTER_SKILL_CVARS(sk_player_head);
	REGISTER_SKILL_CVARS(sk_player_chest);
	REGISTER_SKILL_CVARS(sk_player_stomach);
	REGISTER_SKILL_CVARS(sk_player_arm);
	REGISTER_SKILL_CVARS(sk_player_leg);

	// Flashlight
	REGISTER_SKILL_CVARS(sk_flashlight_drain_time);
	REGISTER_SKILL_CVARS(sk_flashlight_charge_time);
// END REGISTER CVARS FOR SKILL LEVEL STUFF

	SERVER_COMMAND( "exec skill.cfg\n" );
#if FEATURE_OPFOR_SKILL
	SERVER_COMMAND( "exec skillopfor.cfg\n" );
#endif

	// Register server commands
	g_engfuncs.pfnAddServerCommand("report_ai_state", Cmd_ReportAIState);
}

