#include "visuals_utils.h"

CSprite* CreateSpriteFromVisual(const Visual& visual, const Vector& origin, int spawnFlags)
{
	CSprite *sprite = CSprite::SpriteCreate( visual.model, origin, visual.framerate > 0.0f, spawnFlags );
	if (sprite)
	{
		sprite->pev->framerate = visual.framerate;
		sprite->SetTransparency(visual.rendermode, visual.rendercolor.r, visual.rendercolor.g, visual.rendercolor.b, visual.renderamt, visual.renderfx);
		sprite->SetScale(visual.scale);
	}
	return sprite;
}

CSprite* CreateSpriteFromVisual(const Visual* visual, const Vector& origin, int spawnFlags)
{
	return CreateSpriteFromVisual(*visual, origin, spawnFlags);
}

CBeam* CreateBeamFromVisual(const Visual& visual)
{
	CBeam* beam = CBeam::BeamCreate(visual.model, visual.beamWidth);
	if (beam)
	{
		beam->SetColor(visual.rendercolor.r, visual.rendercolor.g, visual.rendercolor.b);
		beam->SetBrightness(visual.renderamt);
		beam->SetWidth(visual.beamWidth);
		beam->SetNoise(visual.beamNoise);
		if (visual.beamScrollRate)
			beam->SetScrollRate(visual.beamScrollRate);
	}
	return beam;
}

CBeam* CreateBeamFromVisual(const Visual* visual)
{
	if (visual)
		return CreateBeamFromVisual(*visual);
	return nullptr;
}

void WriteBeamVisual(const Visual &visual)
{
	WRITE_SHORT( visual.modelIndex );
	WRITE_BYTE( 0 ); // framestart
	WRITE_BYTE( visual.framerate ); // framerate
	WRITE_BYTE( (int)(10*RandomizeNumberFromRange(visual.life)) ); // life
	WRITE_BYTE( visual.beamWidth );  // width
	WRITE_BYTE( visual.beamNoise );   // noise
	WRITE_COLOR( visual.rendercolor );
	WRITE_BYTE( visual.renderamt );	// brightness
	WRITE_BYTE( visual.beamScrollRate );		// speed
}

void WriteBeamVisual(const Visual *visual)
{
	WriteBeamVisual(*visual);
}

void WriteBeamFollowVisual(const Visual &visual)
{
	WRITE_SHORT( visual.modelIndex );
	WRITE_BYTE( (int)(10*RandomizeNumberFromRange(visual.life)) ); // life
	WRITE_BYTE( visual.beamWidth );  // width
	WRITE_COLOR( visual.rendercolor ); // r, g, b
	WRITE_BYTE( visual.renderamt );	// brightness
}

void WriteBeamFollowVisual(const Visual *visual)
{
	WriteBeamFollowVisual(*visual);
}

void WriteSpriteVisual(const Visual &visual)
{
	WRITE_SHORT( visual.modelIndex );		// model
	WRITE_BYTE( visual.scale * 10 );				// size * 10
	WRITE_BYTE( visual.rendermode );
	WRITE_COLOR( visual.rendercolor );
	WRITE_BYTE( visual.renderamt );			// brightness
	WRITE_BYTE( visual.renderfx );
	WRITE_SHORT( visual.framerate * 10 );
	WRITE_BYTE( RandomizeNumberFromRange(visual.life)*10 );
}

void WriteSpriteVisual(const Visual *visual)
{
	WriteSpriteVisual(*visual);
}

void WriteDynLightVisual(const Visual &visual)
{
	WRITE_BYTE( RandomizeNumberFromRange(visual.radius) * 0.1f );	// radius * 0.1
	WRITE_COLOR( visual.rendercolor );
	WRITE_BYTE( RandomizeNumberFromRange(visual.life)*10 );	// time * 10
}

void WriteDynLightVisual(const Visual *visual)
{
	WriteDynLightVisual(*visual);
}

void WriteEntLightVisual(const Visual &visual)
{
	WRITE_COORD( RandomizeNumberFromRange(visual.radius) );	// radius
	WRITE_COLOR( visual.rendercolor );
	WRITE_BYTE( RandomizeNumberFromRange(visual.life)*10 );	// life * 10
}

void WriteEntLightVisual(const Visual *visual)
{
	WriteEntLightVisual(*visual);
}

static bool CheckVisualDefine(const Visual& visual, int param, int ignored)
{
	return visual.HasDefined(param) && (ignored & param) == 0;
}

float AnimateWithFramerate(float frame, float maxFrame, float framerate, float* pLastTime)
{
	if (maxFrame == 0 || framerate == 0.0f)
		return frame;

	const float timeBetween = pLastTime ? (gpGlobals->time - *pLastTime) : 0.1f;
	const float frames = framerate * timeBetween;

	frame += frames;
	if (frame > maxFrame)
	{
		if (maxFrame > 0)
			frame = fmod(frame, maxFrame);
	}

	if (pLastTime)
		*pLastTime = gpGlobals->time;

	return frame;
}
