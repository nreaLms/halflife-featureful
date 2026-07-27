#include <cmath>

#include "hud.h"
#include "cl_util.h"
#include "hud_crosshair.h"

float g_flCrosshairX = 0.f;
float g_flCrosshairY = 0.f;

namespace
{
	constexpr float CROSSHAIR_BASE_SIZE = 6.f;        // length of each bar at rest
	constexpr float CROSSHAIR_BASE_GAP = 2.f;         // gap from center at rest
	constexpr float CROSSHAIR_MAX_FIRE_SPREAD = 14.f;  // max extra pixels added by firing
	constexpr float CROSSHAIR_DECAY_SPEED = 45.f;      // pixels/sec the fire-spread shrinks back
	constexpr float CROSSHAIR_MOVE_SPREAD_SCALE = 0.05f; // pixels of spread per unit of player speed
	constexpr float CROSSHAIR_MOVE_SPREAD_MAX = 6.f;   // cap on movement-based spread
}

int CHudCrosshair::Init()
{
	gHUD.AddHudElem(this);
	m_iFlags = HUD_ACTIVE;
	m_flSpread = 0.f;
	m_flLastDrawTime = 0.f;
	return 1;
}

int CHudCrosshair::VidInit()
{
	// Centers the crosshair on each resolution / level change
	g_flCrosshairX = ScreenWidth * 0.5f;
	g_flCrosshairY = ScreenHeight * 0.5f;
	m_flSpread = 0.f;
	m_flLastDrawTime = 0.f;
	return 1;
}

void CHudCrosshair::Reset()
{
	g_flCrosshairX = ScreenWidth * 0.5f;
	g_flCrosshairY = ScreenHeight * 0.5f;
	m_flSpread = 0.f;
	m_flLastDrawTime = 0.f;
}

void CHudCrosshair::NotifyWeaponFired(float flKick)
{
	m_flSpread += flKick;
	if(m_flSpread > CROSSHAIR_MAX_FIRE_SPREAD)
		m_flSpread = CROSSHAIR_MAX_FIRE_SPREAD;
}

int CHudCrosshair::Draw(float flTime)
{
	// dt since last frame, used to decay the fire-spread back down smoothly
	float dt = (m_flLastDrawTime > 0.f) ? (flTime - m_flLastDrawTime) : 0.f;
	if(dt < 0.f || dt > 0.25f) // clamp: negative on time-reset, huge after level load/pause
		dt = 0.f;
	m_flLastDrawTime = flTime;

	if(m_flSpread > 0.f)
	{
		m_flSpread -= CROSSHAIR_DECAY_SPEED * dt;
		if(m_flSpread < 0.f)
			m_flSpread = 0.f;
	}

	// Passive spread from player movement speed (feet on the ground moving = less precision)
	float flMoveSpread = 0.f;
	cl_entity_t *pPlayer = gEngfuncs.GetLocalPlayer();
	if(pPlayer)
	{
		float vx = pPlayer->curstate.velocity[0];
		float vy = pPlayer->curstate.velocity[1];
		float flSpeed = sqrtf(vx * vx + vy * vy);
		flMoveSpread = flSpeed * CROSSHAIR_MOVE_SPREAD_SCALE;
		if(flMoveSpread > CROSSHAIR_MOVE_SPREAD_MAX)
			flMoveSpread = CROSSHAIR_MOVE_SPREAD_MAX;
	}

	float flTotalSpread = m_flSpread + flMoveSpread;

	int x = (int)g_flCrosshairX;
	int y = (int)g_flCrosshairY;
	int size = (int)(CROSSHAIR_BASE_SIZE + flTotalSpread * 0.5f);
	int gap = (int)(CROSSHAIR_BASE_GAP + flTotalSpread);

	// Four separate bars (N/S/E/W) instead of one solid cross, so the gap
	// itself animates outward on fire/movement instead of the bars just growing in place.
	FillRGBA(x - gap - size, y - 1, size, 2, 255, 255, 255, 255); // left
	FillRGBA(x + gap, y - 1, size, 2, 255, 255, 255, 255); // right
	FillRGBA(x - 1, y - gap - size, 2, size, 255, 255, 255, 255); // top
	FillRGBA(x - 1, y + gap, 2, size, 255, 255, 255, 255); // bottom

	return 1;
}