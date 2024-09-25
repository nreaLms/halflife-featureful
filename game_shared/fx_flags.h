#pragma once
#ifndef FX_FLAGS_H
#define FX_FLAGS_H

#define SF_PARTICLESHOOTER_REPEATABLE 1
#define SF_PARTICLESHOOTER_SPIRAL 2
#define SF_PARTICLESHOOTER_COLLIDE_WITH_WORLD 4
#define SF_PARTICLESHOOTER_AFFECTED_BY_FORCE 8
#define SF_PARTICLESHOOTER_ANIMATED 16
#define SF_PARTICLESHOOTER_STARTON 32
#define SF_PARTICLESHOOTER_KILLED_ON_COLLIDE 64
#define SF_PARTICLESHOOTER_RIPPLE_WHEN_HITTING_WATER 128

#define SMOKER_FLAG_DIRECTED (1 << 0)
#define SMOKER_FLAG_FADE_SPRITE (1 << 1)
#define SMOKER_FLAG_SCALE_VALUE_IS_NORMAL (1 << 2)

#define SF_WEATHER_ACTIVE (1 << 0)
#define SF_WEATHER_ALLOW_INDOOR (1 << 1)
#define SF_WEATHER_NOT_AFFECTED_BY_WIND (1 << 2)
#define SF_WEATHER_LOCALIZED (1 << 3)
#define SF_WEATHER_DISTANCE_IS_RADIUS (1 << 4)
#define WEATHER_BRUSH_ENTITY (1 << 12)

#define SF_RAIN_ACTIVE SF_WEATHER_ACTIVE
#define SF_RAIN_ALLOW_INDOOR SF_WEATHER_ALLOW_INDOOR
#define SF_RAIN_NOT_AFFECTED_BY_WIND SF_WEATHER_NOT_AFFECTED_BY_WIND
#define SF_RAIN_LOCALIZED SF_WEATHER_LOCALIZED
#define SF_RAIN_DISTANCE_IS_RADIUS SF_WEATHER_DISTANCE_IS_RADIUS
#define SF_RAIN_NO_WIND (1 << 5)
#define SF_RAIN_NO_SPLASHES (1 << 6)
#define SF_RAIN_NO_RIPPLES (1 << 7)
#define RAIN_BRUSH_ENTITY WEATHER_BRUSH_ENTITY

#define SF_SNOW_ACTIVE SF_WEATHER_ACTIVE
#define SF_SNOW_ALLOW_INDOOR SF_WEATHER_ALLOW_INDOOR
#define SF_SNOW_NOT_AFFECTED_BY_WIND SF_WEATHER_NOT_AFFECTED_BY_WIND
#define SF_SNOW_LOCALIZED SF_WEATHER_LOCALIZED
#define SF_SNOW_DISTANCE_IS_RADIUS SF_WEATHER_DISTANCE_IS_RADIUS
#define SNOW_BRUSH_ENTITY WEATHER_BRUSH_ENTITY

enum
{
	PARTICLE_LIGHT_DEFAULT = 0,
	PARTICLE_LIGHT_NONE,
	PARTICLE_LIGHT_COLOR,
	PARTICLE_LIGHT_INTENSITY,
};

#define SPRAY_FLAG_COLLIDEWORLD (1 << 0)
#define SPRAY_FLAG_ANIMATE (1 << 1)
#define SPRAY_FLAG_FADEOUT (1 << 2)

#endif
