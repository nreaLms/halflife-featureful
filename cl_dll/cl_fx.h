#pragma once
#ifndef CL_FX_H
#define CL_FX_H

#include "cl_dll.h"
#include "com_model.h"
#include "fx_types.h"

void LoadDefaultSprites();

void FX_Streaks(Vector pos, Vector dir, const StreakParams& streakParams);
void FX_RicochetSprite(Vector pos, model_t *pmodel, float duration, float scale);
void FX_SparkEffect(Vector pos, const SparkEffectParams& params);
void FX_SparkShower(Vector pos, const SparkEffectParams& params);

#endif
