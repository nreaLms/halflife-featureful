#pragma once
#ifndef WARPBALL_H
#define WARPBALL_H

#include <string>
#include "extdll.h"

#define WARPBALL_RED_DEFAULT 77
#define WARPBALL_GREEN_DEFAULT 210
#define WARPBALL_BLUE_DEFAULT 130

#define WARPBALL_BEAM_RED_DEFAULT 20
#define WARPBALL_BEAM_GREEN_DEFAULT 243
#define WARPBALL_BEAM_BLUE_DEFAULT 20

#define ALIEN_TELEPORT_SOUND "debris/alien_teleport.wav"

#define WARPBALL_SPRITE "sprites/fexplo1.spr"
#define WARPBALL_SPRITE2 "sprites/xflare1.spr"
#define WARPBALL_BEAM "sprites/lgtning.spr"
#define WARPBALL_SOUND1 "debris/beamstart2.wav"
#define WARPBALL_SOUND2 "debris/beamstart7.wav"

struct Color
{
	Color(): r(0), g(0), b(0) {}
	Color(int red, int green, int blue): r(red), g(green), b(blue) {}
	int r;
	int g;
	int b;
};

template <typename N>
struct NumberRange
{
	NumberRange(): min(), max() {}
	NumberRange(N mini, N maxi): min(mini), max(maxi) {}
	NumberRange(N val): min(val), max(val) {}
	N min;
	N max;
};

typedef NumberRange<float> FloatRange;
typedef NumberRange<int> IntRange;

struct WarpballSound
{
	WarpballSound(): sound(), soundName(0), volume(1.0f), attenuation(0.8f), pitch(100) {}
	std::string sound;
	string_t soundName;
	float volume;
	float attenuation;
	IntRange pitch;
};

struct WarpballSprite
{
	WarpballSprite():
		sprite(),
		spriteName(0),
		color(),
		alpha(255),
		scale(1.0f),
		framerate(12.0f),
		rendermode(kRenderGlow),
		renderfx(kRenderFxNoDissipation)
	{}
	std::string sprite;
	string_t spriteName;
	Color color;
	int alpha;
	float scale;
	float framerate;
	int rendermode;
	int renderfx;
};

struct WarpballBeam
{
	WarpballBeam():
		sprite(),
		spriteName(0),
		texture(0),
		color(),
		alpha(220),
		width(30),
		noise(65),
		life(0.5f, 1.6f) {}
	std::string sprite;
	string_t spriteName;
	int texture;
	Color color;
	int alpha;
	int width;
	int noise;
	FloatRange life;
};

struct WarpballLight
{
	WarpballLight():
		color(),
		radius(192),
		life(1.5f) {}
	Color color;
	int radius;
	float life;
	inline bool IsDefined() const {
		return life > 0.0 && radius > 0;
	}
};

struct WarpballShake
{
	WarpballShake():
		radius(192),
		duration(0.0f),
		frequency(160.0f),
		amplitude(6) {}
	int radius;
	float duration;
	float frequency;
	int amplitude;
	inline bool IsDefined() const {
		return duration > 0;
	}
};

struct WarpballAiSound
{
	WarpballAiSound(): type(0), radius(192), duration(0.3f) {}
	int type;
	int radius;
	float duration;
	inline bool IsDefined() const {
		return type != 0 && duration > 0.0f && radius > 0;
	}
};

struct WarpballPosition
{
	WarpballPosition(): verticalShift(0.0f), defined(false) {}
	float verticalShift;
	bool defined;
	inline bool IsDefined() const {
		return defined;
	}
};

struct WarpballTemplate
{
	WarpballTemplate():
		beamRadius(192),
		beamCount(10, 20),
		spawnDelay(0.0f) {}
	WarpballSound sound1;
	WarpballSound sound2;

	WarpballSprite sprite1;
	WarpballSprite sprite2;

	WarpballBeam beam;
	int beamRadius;
	IntRange beamCount;

	WarpballLight light;
	WarpballShake shake;

	WarpballAiSound aiSound;
	float spawnDelay;
	WarpballPosition position;
};

void LoadWarpballTemplates();
void PrecacheWarpballTemplate(const char* name, const char* entityClassname);
const WarpballTemplate* FindWarpballTemplate(const char* warpballName, const char* entityClassname);
void PlayWarpballEffect(const WarpballTemplate& warpballTemplate, const Vector& vecOrigin, edict_t* playSoundEnt);

#endif
