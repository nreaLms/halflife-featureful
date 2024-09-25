#pragma once
#ifndef VISUALS_UTILS_H
#define VISUALS_UTILS_H

#include "extdll.h"
#include "util.h"
#include "effects.h"
#include "visuals.h"

CSprite* CreateSpriteFromVisual(const Visual& visual, const Vector& origin, int spawnFlags = 0);
CSprite* CreateSpriteFromVisual(const Visual* visual, const Vector& origin, int spawnFlags = 0);

CBeam* CreateBeamFromVisual(const Visual& visual);
CBeam* CreateBeamFromVisual(const Visual* visual);

void WriteBeamVisual(const Visual& visual);
void WriteBeamVisual(const Visual* visual);

void WriteBeamFollowVisual(const Visual& visual);
void WriteBeamFollowVisual(const Visual* visual);

void WriteSpriteVisual(const Visual& visual);
void WriteSpriteVisual(const Visual* visual);

void WriteDynLightVisual(const Visual& visual);
void WriteDynLightVisual(const Visual* visual);

void WriteEntLightVisual(const Visual& visual);
void WriteEntLightVisual(const Visual* visual);

void ApplyVisualToEntity(CBaseEntity* pEntity, const Visual& visual);
void ApplyVisualToEntity(CBaseEntity* pEntity, const Visual* visual);

float AnimateWithFramerate(float frame, float maxFrame, float framerate, float* pLastTime = nullptr);

inline Vector VectorFromColor(const Color& color) {
	return Vector(color.r, color.g, color.b);
}

#endif
