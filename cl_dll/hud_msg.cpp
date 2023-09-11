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
//  hud_msg.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"

#define MAX_CLIENTS 32

extern BEAM *pBeam;
extern BEAM *pBeam2;
extern TEMPENTITY *pFlare;	// Vit_amiN

extern float g_lastFOV;			// Vit_amiN

/// USER-DEFINED SERVER MESSAGE HANDLERS

int CHud::MsgFunc_ResetHUD( const char *pszName, int iSize, void *pbuf )
{
	ASSERT( iSize == 0 );

	// clear all hud data
	HUDLIST *pList = m_pHudList;

	while( pList )
	{
		if( pList->p )
			pList->p->Reset();
		pList = pList->pNext;
	}
	m_Nightvision.Reset();

	m_iItemBits = 0;

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;

	// Vit_amiN: reset the FOV
	m_iFOV = 0;	// default_fov
	g_lastFOV = 0.0f;

	return 1;
}

void CAM_ToFirstPerson( void );

void CHud::MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf )
{
	CAM_ToFirstPerson();
}

void CHud::MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf )
{
	// prepare all hud data
	HUDLIST *pList = m_pHudList;

	while( pList )
	{
		if( pList->p )
			pList->p->InitHUDData();
		pList = pList->pNext;
	}

	//Probably not a good place to put this.
	pBeam = pBeam2 = NULL;
	pFlare = NULL;	// Vit_amiN: clear egon's beam flare
}

int CHud::MsgFunc_SetFog( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	fog.r = READ_BYTE();
	fog.g = READ_BYTE();
	fog.b = READ_BYTE();

	fog.fadeDuration = READ_SHORT();
	fog.startDist = READ_SHORT();

	if (fog.fadeDuration > 0)
	{
//		// fading in
		fog.finalEndDist = READ_SHORT();
		fog.endDist = FOG_LIMIT;
	}
	else if (fog.fadeDuration < 0)
	{
//		// fading out
		fog.finalEndDist = fog.endDist = READ_SHORT();
	}
	else
	{
		fog.endDist = READ_SHORT();
	}

	fog.density = READ_LONG() / 10000.0f;
	fog.type = READ_BYTE();
	fog.affectSkybox = READ_BYTE();

	return 1;
}

//LRC
int CHud::MsgFunc_KeyedDLight( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int iKey = READ_BYTE();
	dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight( iKey );

	int bActive = READ_BYTE();
	if (!bActive)
	{
		// die instantly
		dl->die = gEngfuncs.GetClientTime();
	}
	else
	{
		// never die
		dl->die = gEngfuncs.GetClientTime() + (float)1E6;

		dl->origin[0] = READ_COORD();
		dl->origin[1] = READ_COORD();
		dl->origin[2] = READ_COORD();
		dl->radius = READ_SHORT();
		dl->color.r = READ_BYTE();
		dl->color.g = READ_BYTE();
		dl->color.b = READ_BYTE();
	}
	return 1;
}

int CHud::MsgFunc_WallPuffs(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	wallPuffs[0] = READ_SHORT();
	wallPuffs[1] = READ_SHORT();
	wallPuffs[2] = READ_SHORT();
	wallPuffs[3] = READ_SHORT();

	wallPuffCount = 0;
	for (int i=0; i<sizeof(wallPuffs)/sizeof(wallPuffs[0]); ++i)
	{
		if (wallPuffs[i])
			wallPuffCount++;
		else
			break;
	}
	return 1;
}

int CHud::MsgFunc_GameMode( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_Teamplay = READ_BYTE();

	return 1;
}

int CHud::MsgFunc_Damage( const char *pszName, int iSize, void *pbuf )
{
	int		armor, blood;
	Vector	from;
	int		i;
	float	count;

	BEGIN_READ( pbuf, iSize );
	armor = READ_BYTE();
	blood = READ_BYTE();

	for( i = 0; i < 3; i++)
		from[i] = READ_COORD();

	count = ( blood * 0.5 ) + ( armor * 0.5 );

	if( count < 10 )
		count = 10;

	// TODO: kick viewangles,  show damage visually
	return 1;
}

int CHud::MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf )
{
	int r, g, b;
	BEGIN_READ( pbuf, iSize );
	m_iConcussionEffect = READ_BYTE();
	if( m_iConcussionEffect )
	{
		UnpackRGB( r, g, b, gHUD.HUDColor() );	// Vit_amiN: fixed
		this->m_StatusIcons.EnableIcon( "dmg_concuss", r, g, b );
	}
	else
		this->m_StatusIcons.DisableIcon( "dmg_concuss" );
	return 1;
}
