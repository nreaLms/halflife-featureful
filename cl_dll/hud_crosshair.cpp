#include "hud.h"
#include "cl_util.h"
#include "hud_crosshair.h"

float g_flCrosshairX = 0.f;
float g_flCrosshairY = 0.f;

int CHudCrosshair::Init()
{
	gHUD.AddHudElem( this );
	m_iFlags = HUD_ACTIVE;
	return 1;
}

int CHudCrosshair::VidInit()
{
	// recentre le viseur à chaque changement de résolution / niveau
	g_flCrosshairX = ScreenWidth * 0.5f;
	g_flCrosshairY = ScreenHeight * 0.5f;
	return 1;
}

void CHudCrosshair::Reset()
{
	g_flCrosshairX = ScreenWidth * 0.5f;
	g_flCrosshairY = ScreenHeight * 0.5f;
}

int CHudCrosshair::Draw( float flTime )
{
	int x = (int)g_flCrosshairX;
	int y = (int)g_flCrosshairY;
	int size = 6;

	FillRGBA( x - size, y - 1, size * 2, 2, 255, 255, 255, 255 );
	FillRGBA( x - 1, y - size, 2, size * 2, 255, 255, 255, 255 );

	return 1;
}