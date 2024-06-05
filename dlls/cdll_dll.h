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
//
//  cdll_dll.h

// this file is included by both the game-dll and the client-dll,
#pragma once
#if !defined(CDLL_DLL_H)
#define CDLL_DLL_H
#include "player_items.h"
#include "mod_features.h"

#define MAX_WEAPONS		32

#define MAX_ITEMS				5	// hard coded item types

#define	HIDEHUD_WEAPONS		( 1<<0 )
#define	HIDEHUD_FLASHLIGHT	( 1<<1 )
#define	HIDEHUD_ALL		( 1<<2 )
#define HIDEHUD_HEALTH		( 1<<3 )

#define	MAX_AMMO_TYPES		32
#define AMMO_EXHAUSTIBLE_NETWORK_BIT ( 1 << 6 )

#define HUD_PRINTNOTIFY		1
#define HUD_PRINTCONSOLE	2
#define HUD_PRINTTALK		3
#define HUD_PRINTCENTER		4


#define MAX_INVENTORY_ITEMS 8
#define MAX_ICONSPRITES 6

#define PLAYER_STATUS_ICON_ENABLE (1 << 0)
#define PLAYER_STATUS_ICON_ALLOW_DUPLICATE (1 << 1)

#endif
