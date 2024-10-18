#include "visuals_utils.h"
#include "fx_flags.h"

extern int gmsgSprite;
extern int gmsgSpray;
extern int gmsgSmoke;

CSprite* CreateSpriteFromVisual(const Visual* visual, const Vector& origin, int spawnFlags)
{
	if (!visual || !visual->modelIndex)
		return nullptr;

	const float framerate = RandomizeNumberFromRange(visual->framerate);
	CSprite *sprite = CSprite::SpriteCreate(visual->model, origin, framerate > 0.0f, spawnFlags);
	if (sprite)
	{
		sprite->pev->framerate = framerate;
		sprite->SetTransparency(visual->rendermode, visual->rendercolor.r, visual->rendercolor.g, visual->rendercolor.b, visual->renderamt, visual->renderfx);
		sprite->SetScale(RandomizeNumberFromRange(visual->scale));
	}
	return sprite;
}

CBeam* CreateBeamFromVisual(const Visual* visual)
{
	if (!visual || !visual->modelIndex)
		return nullptr;

	CBeam* beam = CBeam::BeamCreate(visual->model, visual->beamWidth);
	if (beam)
	{
		beam->SetColor(visual->rendercolor.r, visual->rendercolor.g, visual->rendercolor.b);
		beam->SetBrightness(visual->renderamt);
		beam->SetWidth(visual->beamWidth);
		beam->SetNoise(visual->beamNoise);
		if (visual->beamScrollRate)
			beam->SetScrollRate(visual->beamScrollRate);
		beam->SetFlags(visual->beamFlags);
	}
	return beam;
}

void WriteBeamVisual(const Visual *visual)
{
	WRITE_SHORT( visual->modelIndex );
	WRITE_BYTE( 0 ); // framestart
	WRITE_BYTE( (int)RandomizeNumberFromRange(visual->framerate) ); // framerate
	WRITE_BYTE( (int)(10*RandomizeNumberFromRange(visual->life)) ); // life
	WRITE_BYTE( visual->beamWidth );  // width
	WRITE_BYTE( visual->beamNoise );   // noise
	WRITE_COLOR( visual->rendercolor );
	WRITE_BYTE( visual->renderamt );	// brightness
	WRITE_BYTE( visual->beamScrollRate );		// speed
}

void WriteBeamFollowVisual(const Visual *visual)
{
	WRITE_SHORT( visual->modelIndex );
	WRITE_BYTE( (int)(10*RandomizeNumberFromRange(visual->life)) ); // life
	WRITE_BYTE( visual->beamWidth );  // width
	WRITE_COLOR( visual->rendercolor ); // r, g, b
	WRITE_BYTE( visual->renderamt );	// brightness
}

void WriteSpriteVisual(const Visual *visual)
{
	WRITE_SHORT( visual->modelIndex );		// model
	WRITE_BYTE( RandomizeNumberFromRange(visual->scale) * 10 );				// size * 10
	WRITE_BYTE( visual->rendermode );
	WRITE_COLOR( visual->rendercolor );
	WRITE_BYTE( visual->renderamt );			// brightness
	WRITE_BYTE( visual->renderfx );
	WRITE_SHORT( (int)RandomizeNumberFromRange(visual->framerate) * 10 );
	WRITE_BYTE( RandomizeNumberFromRange(visual->life)*10 );
}

void WriteDynLightVisual(const Visual *visual)
{
	WRITE_BYTE( RandomizeNumberFromRange(visual->radius) * 0.1f );	// radius * 0.1
	WRITE_COLOR( visual->rendercolor );
	WRITE_BYTE( RandomizeNumberFromRange(visual->life)*10 );	// time * 10
}

void WriteEntLightVisual(const Visual *visual)
{
	WRITE_COORD( RandomizeNumberFromRange(visual->radius) );	// radius
	WRITE_COLOR( visual->rendercolor );
	WRITE_BYTE( RandomizeNumberFromRange(visual->life)*10 );	// life * 10
}

void SendDynLight(const Vector& vecOrigin, const Visual* visual, int decay)
{
	if (!visual)
		return;
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecOrigin);
		WRITE_BYTE(TE_DLIGHT);
		WRITE_VECTOR(vecOrigin);
		WriteDynLightVisual(visual);
		WRITE_BYTE(decay);
	MESSAGE_END();
}

void SendSprite(const Vector& vecOrigin, const Visual* visual)
{
	if (!visual || !visual->modelIndex)
		return;
	MESSAGE_BEGIN(MSG_PVS, gmsgSprite, vecOrigin);
		WRITE_VECTOR(vecOrigin);
		WriteSpriteVisual(visual);
	MESSAGE_END();
}

void SendSpray(const Vector& position, const Vector& direction, const Visual* visual, int count, int speed, int noise)
{
	if (!visual || !visual->modelIndex)
		return;
	MESSAGE_BEGIN( MSG_PVS, gmsgSpray, position );
		WRITE_VECTOR( position );	// pos
		WRITE_VECTOR( direction );	// dir
		WRITE_SHORT( visual->modelIndex );	// model
		WRITE_BYTE ( count );			// count
		WRITE_BYTE ( speed );			// speed
		WRITE_BYTE ( noise );			// noise ( client will divide by 100 )
		WRITE_BYTE( visual->rendermode );
		WRITE_COLOR( visual->rendercolor );
		WRITE_BYTE( visual->renderamt );
		WRITE_BYTE( visual->renderfx );
		WRITE_BYTE( (int)(RandomizeNumberFromRange(visual->scale) * 10) );
		WRITE_SHORT( (int)(RandomizeNumberFromRange(visual->framerate) * 10) );
		WRITE_BYTE( SPRAY_FLAG_FADEOUT );
	MESSAGE_END();
}

void SendSmoke(const Vector& position, const Visual* visual)
{
	if (!visual || !visual->modelIndex)
		return;

	const float framerate = RandomizeNumberFromRange(visual->framerate);
	const int minFramerate = Q_max(framerate - 1, 1);
	const int maxFramerate = framerate + 1;

	const float scale = RandomizeNumberFromRange(visual->scale);

	MESSAGE_BEGIN( MSG_PVS, gmsgSmoke, position );
		WRITE_BYTE( SMOKER_FLAG_SCALE_VALUE_IS_NORMAL );
		WRITE_VECTOR( position );
		WRITE_SHORT( visual->modelIndex );
		WRITE_COORD( RANDOM_FLOAT(scale, scale * 1.1f) );
		WRITE_BYTE( RANDOM_LONG( minFramerate, maxFramerate ) );
		WRITE_SHORT( 0 );
		WRITE_SHORT( 0 );
		WRITE_BYTE( visual->rendermode );
		WRITE_BYTE( visual->renderamt );
		WRITE_COLOR( visual->rendercolor );
		WRITE_SHORT( 0 );
	MESSAGE_END();
}

void SendBeamFollow(int entIndex, const Visual* visual)
{
	if (!visual || !visual->modelIndex)
		return;

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entIndex );	// entity
		WriteBeamFollowVisual( visual );
	MESSAGE_END();
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
