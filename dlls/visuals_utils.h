#pragma once
#ifndef VISUALS_UTILS_H
#define VISUALS_UTILS_H

#include "extdll.h"
#include "util.h"
#include "effects.h"
#include "visuals.h"

CSprite* CreateSpriteFromVisual(const Visual* visual, const Vector& origin, int spawnFlags = 0);

CBeam* CreateBeamFromVisual(const Visual* visual);

void WriteBeamVisual(const Visual* visual);

void WriteBeamFollowVisual(const Visual* visual);

void WriteSpriteVisual(const Visual* visual);

void WriteDynLightVisual(const Visual* visual);

void WriteEntLightVisual(const Visual* visual);

void SendDynLight(const Vector& vecOrigin, const Visual* visual, int decay = 0);
void SendSprite(const Vector& vecOrigin, const Visual* visual);
void SendSpray(const Vector& position, const Vector& direction, const Visual* visual, int count, int speed, int noise);
void SendSmoke(const Vector& position, const Visual* visual);
void SendBeamFollow(int entIndex, const Visual* visual);

float AnimateWithFramerate(float frame, float maxFrame, float framerate, float* pLastTime = nullptr);

#endif
