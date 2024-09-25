#pragma once
#ifndef UTIL_SHARED_H
#define UTIL_SHARED_H

int UTIL_SharedRandomLong( unsigned int seed, int low, int high );
float UTIL_SharedRandomFloat( unsigned int seed, float low, float high );

extern float		UTIL_AngleMod			(float a);
extern float		UTIL_AngleDiff			( float destAngle, float srcAngle );

extern float		UTIL_Approach( float target, float value, float speed );
extern float		UTIL_ApproachAngle( float target, float value, float speed );
extern float		UTIL_AngleDistance( float next, float cur );

extern void			UTIL_StringToVector( float *pVector, const char *pString, int* componentsRead = nullptr );

#endif
