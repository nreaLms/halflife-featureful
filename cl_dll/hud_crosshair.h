#pragma once
#if !defined(HUD_CROSSHAIR_H)
#define HUD_CROSSHAIR_H

#include "hud.h"

class CHudCrosshair : public CHudBase
{
public:
	int Init();
	int VidInit();
	int Draw( float flTime );
	void Reset();
};

// position du viseur en pixels écran, mise à jour depuis le code d'input
extern float g_flCrosshairX;
extern float g_flCrosshairY;

#endif