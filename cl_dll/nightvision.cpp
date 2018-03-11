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
// flashlight.cpp
//
// implementation of CHudFlashlight class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "mod_features.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE( m_Nightvision, Nightvision )
#if FEATURE_NIGHTVISION
DECLARE_MESSAGE( m_Nightvision, Flashlight )
#endif

#define NIGHTVISION_SPRITE_NAME "sprites/of_nv_b.spr"

int CHudNightvision::Init(void)
{
	m_fOn = 0;

	HOOK_MESSAGE(Nightvision);
#if FEATURE_NIGHTVISION
	HOOK_MESSAGE(Flashlight);
#endif

	m_iFlags |= HUD_ACTIVE;

	gHUD.AddHudElem(this);

	return 1;
}

void CHudNightvision::Reset(void)
{
	m_fOn = 0;
}

int CHudNightvision::VidInit(void)
{
	m_hSprite = LoadSprite(NIGHTVISION_SPRITE_NAME);

	m_prc = &gHUD.GetSpriteRect(m_hSprite);

	// Get the number of frames available in this sprite.
	m_nFrameCount = SPR_Frames(m_hSprite);

	// current frame.
	m_iFrame = 0;

	return 1;
}


int CHudNightvision::MsgFunc_Nightvision(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_fOn = READ_BYTE();

	return 1;
}

int CHudNightvision::MsgFunc_Flashlight( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_fOn = READ_BYTE();

	return 1;
}

int CHudNightvision::Draw(float flTime)
{
	if (gHUD.m_iHideHUDDisplay & (HIDEHUD_FLASHLIGHT | HIDEHUD_ALL))
		return 1;

	int r, g, b, x, y, a;

	// Only display this if the player is equipped with the suit.
	if (!(gHUD.m_iWeaponBits & (1 << (WEAPON_SUIT))))
		return 1;

	if (m_fOn)
		a = 225;
	else
		a = MIN_ALPHA;

	// Get each color component from the main
	// hud color.
	UnpackRGB(r, g, b, RGB_GREENISH);

	ScaleColors(r, g, b, a);

	// Top left of the screen.
	x = y = 0;

	// Reset the number of frame if we are at last frame.
	if (m_iFrame >= m_nFrameCount)
		m_iFrame = 0;

	if (m_fOn)
	{
		//
		// draw nightvision scanlines sprite.
		//
		SPR_Set(m_hSprite, r, g, b);

		int i, j;
		for (i = 0; i < 8; ++i) // height
		{
			for (j = 0; j < 16; ++j) // width
			{
				// Nightvision sprites are 256 x 256. So draw 128 -> (8 * 16) instances to cover
				// the entire screen. It's cheap but does the work.
				//
				// Keep in mind this is used until we find a better solution.
				SPR_DrawAdditive(m_iFrame, x + (j * 256), y + (i * 256), NULL);
			}
		}
	}

	// Increase sprite frame.
	m_iFrame++;

	return 1;
}
