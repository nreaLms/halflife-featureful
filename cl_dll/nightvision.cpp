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
#include "dlight.h"
#include "r_efx.h"
#include "mod_features.h"

#include <string.h>
#include <stdio.h>

#if FEATURE_NIGHTVISION_STYLES
extern cvar_t *cl_nvgstyle;
#endif

#if FEATURE_CS_NIGHTVISION
extern cvar_t *cl_nvgradius_cs;
#endif

#if FEATURE_OPFOR_NIGHTVISION_DLIGHT
extern cvar_t *cl_nvgradius_of;
#endif

#define OF_NVG_DLIGHT_RADIUS 400
#define CS_NVG_DLIGHT_RADIUS 775
#define NVG_DLIGHT_MIN_RADIUS 400
#define NVG_DLIGHT_MAX_RADIUS 1000

#if FEATURE_FILTER_NIGHTVISION
extern cvar_t *cl_nvgfilterbrightness;
#endif

DECLARE_MESSAGE( m_Nightvision, Nightvision )

#define NIGHTVISION_SPRITE_NAME "sprites/of_nv_b.spr"

int CHudNightvision::Init(void)
{
	m_fOn = 0;

	HOOK_MESSAGE(Nightvision);

	m_iFlags |= HUD_ACTIVE;

#if FEATURE_CS_NIGHTVISION
	m_pLightCS = 0;
#endif

#if FEATURE_OPFOR_NIGHTVISION_DLIGHT
	m_pLightOF = 0;
#endif

	//gHUD.AddHudElem(this);

	return 1;
}

void CHudNightvision::Reset(void)
{
	m_fOn = 0;
	ResetFilterMode();
}

int CHudNightvision::VidInit(void)
{
#if FEATURE_OPFOR_NIGHTVISION
	m_hSprite = LoadSprite(NIGHTVISION_SPRITE_NAME);

	// Get the number of frames available in this sprite.
	m_nFrameCount = SPR_Frames(m_hSprite);

	// current frame.
	m_iFrame = 0;
#endif
	return 1;
}


int CHudNightvision::MsgFunc_Nightvision(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	m_fOn = READ_BYTE();
	if (!m_fOn) {
		ResetFilterMode();
		RemoveCSdlight();
		RemoveOFdlight();
	}

	return 1;
}

int CHudNightvision::Draw(float flTime)
{
	if( gEngfuncs.IsSpectateOnly() )
	{
		return 1;
	}

	if (gHUD.m_iHideHUDDisplay & (HIDEHUD_FLASHLIGHT | HIDEHUD_ALL))
		return 1;

	// Only display this if the player is equipped with the suit.
	if (!(gHUD.m_iWeaponBits & (1 << (WEAPON_SUIT))))
		return 1;

	if (m_fOn) {
#if FEATURE_NIGHTVISION_STYLES
		if (cl_nvgstyle)
		{
			if (cl_nvgstyle->value < 1) {
#if FEATURE_OPFOR_NIGHTVISION
				RemoveCSdlight();
				ResetFilterMode();
				DrawOpforNVG(flTime);
				return 1;
#endif
			}
			if (cl_nvgstyle->value < 2) {
#if FEATURE_CS_NIGHTVISION
				RemoveOFdlight();
				ResetFilterMode();
				DrawCSNVG(flTime);
				return 1;
#endif
			}
#if FEATURE_FILTER_NIGHTVISION
			RemoveCSdlight();
			RemoveOFdlight();
			SetFilterMode();
			return 1;
#endif
		}
#endif
#if FEATURE_CS_NIGHTVISION
	DrawCSNVG(flTime);
#elif FEATURE_OPFOR_NIGHTVISION
	DrawOpforNVG(flTime);
#elif FEATURE_FILTER_NIGHTVISION
	SetFilterMode();
#endif
	}
	return 1;
}

void CHudNightvision::DrawCSNVG(float flTime)
{
#if FEATURE_CS_NIGHTVISION
	gEngfuncs.pfnFillRGBABlend(0, 0, ScreenWidth, ScreenHeight, 50, 225, 50, 110);

	if( !m_pLightCS || m_pLightCS->die < flTime )
	{
		m_pLightCS = MakeDynLight(flTime, 50, 255, 50);
	}
	UpdateDynLight( m_pLightCS, CSNvgRadius(), gHUD.m_vecOrigin );
#endif
}

void CHudNightvision::DrawOpforNVG(float flTime)
{
#if FEATURE_OPFOR_NIGHTVISION
	int r, g, b, x, y;
	int a = 255;

	// Get each color component from the main
	// hud color.
	UnpackRGB(r, g, b, RGB_GREENISH);

	ScaleColors(r, g, b, a);

	// Top left of the screen.
	x = y = 0;

	// Reset the number of frame if we are at last frame.
	if (m_iFrame >= m_nFrameCount)
		m_iFrame = 0;

	const int nvgSpriteWidth = SPR_Width(m_hSprite, 0);
	const int nvgSpriteHeight = SPR_Height(m_hSprite, 0);

	const int colCount = (int)ceil(ScreenWidth / (float)nvgSpriteWidth);
	const int rowCount = (int)ceil(ScreenHeight / (float)nvgSpriteHeight);

	//
	// draw nightvision scanlines sprite.
	//
	SPR_Set(m_hSprite, r, g, b);

	int i, j;
	for (i = 0; i < rowCount; ++i) // height
	{
		for (j = 0; j < colCount; ++j) // width
		{
			SPR_DrawAdditive(m_iFrame, x + (j * 256), y + (i * 256), NULL);
		}
	}

	// Increase sprite frame.
	m_iFrame++;

#if FEATURE_OPFOR_NIGHTVISION_DLIGHT
	if( !m_pLightOF || m_pLightOF->die < flTime )
	{
		m_pLightOF = MakeDynLight(flTime, 250, 250, 250);
	}
	UpdateDynLight( m_pLightOF, OpforNvgRadius(), gHUD.m_vecOrigin + Vector(0.0f, 0.0f, 32.0f ) );
#endif

#endif
}

dlight_t* CHudNightvision::MakeDynLight(float flTime, int r, int g, int b)
{
	dlight_t* dLight = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );

	// I hope no one is crazy so much to keep NVG for 9999 seconds
	dLight->die = flTime + 9999.0f;
	dLight->color.r = r;
	dLight->color.g = g;
	dLight->color.b = b;

	return dLight;
}

void CHudNightvision::UpdateDynLight(dlight_t *dynLight, float radius, const Vector &origin)
{
	if( dynLight )
	{
		dynLight->origin = origin;
		dynLight->radius = radius;
	}
}

void CHudNightvision::RemoveCSdlight()
{
#if FEATURE_CS_NIGHTVISION
	if( m_pLightCS )
	{
		m_pLightCS->die = 0;
		m_pLightCS = NULL;
	}
#endif
}

void CHudNightvision::RemoveOFdlight()
{
#if FEATURE_OPFOR_NIGHTVISION_DLIGHT
	if( m_pLightOF )
	{
		m_pLightOF->die = 0;
		m_pLightOF = NULL;
	}
#endif
}

void CHudNightvision::SetFilterMode()
{
#if FEATURE_FILTER_NIGHTVISION
	if (!m_filterModeSet) {
		m_filterModeSet = true;

		gEngfuncs.pfnSetFilterMode(1);
		gEngfuncs.pfnSetFilterColor(0.2f, 0.9f, 0.2f);
	}
	gEngfuncs.pfnSetFilterBrightness(FilterBrightness());
#endif
}

void CHudNightvision::ResetFilterMode()
{
#if FEATURE_FILTER_NIGHTVISION
	if (m_filterModeSet) {
		m_filterModeSet = false;
		gEngfuncs.pfnSetFilterMode(0);
	}
#endif
}

#if FEATURE_CS_NIGHTVISION
float CHudNightvision::CSNvgRadius()
{
	float radius = cl_nvgradius_cs && cl_nvgradius_cs->value > 0.0f ? cl_nvgradius_cs->value : CS_NVG_DLIGHT_RADIUS;
	if (radius < NVG_DLIGHT_MIN_RADIUS)
		return NVG_DLIGHT_MIN_RADIUS;
	else if (radius > NVG_DLIGHT_MAX_RADIUS)
		return NVG_DLIGHT_MAX_RADIUS;
	return radius;
}
#endif

#if FEATURE_OPFOR_NIGHTVISION_DLIGHT
float CHudNightvision::OpforNvgRadius()
{
	float radius = cl_nvgradius_of && cl_nvgradius_of->value > 0.0f ? cl_nvgradius_of->value : OF_NVG_DLIGHT_RADIUS;
	if (radius < NVG_DLIGHT_MIN_RADIUS)
		return NVG_DLIGHT_MIN_RADIUS;
	else if (radius > NVG_DLIGHT_MAX_RADIUS)
		return NVG_DLIGHT_MAX_RADIUS;
	return radius;
}
#endif

bool CHudNightvision::IsOn()
{
	return m_fOn;
}

#if FEATURE_FILTER_NIGHTVISION
float CHudNightvision::FilterBrightness()
{
	if (cl_nvgfilterbrightness) {
		float brightness = cl_nvgfilterbrightness->value;
		if (brightness >= 0.1f || brightness <= 1.0f) {
			return brightness;
		}
	}
	return 0.6f;
}
#endif
