#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include "r_efx.h"
#include "customentity.h"
#include "r_studioint.h"
#include "event_api.h"
#include "com_model.h"
#include "studio.h"
#include "pmtrace.h"
#include "cl_msg.h"

#include "r_studioint.h"

#include "fx_flags.h"

#if USE_PARTICLEMAN
#include "particleman.h"
#endif

extern engine_studio_api_t IEngineStudio;

void GibHitCallback( TEMPENTITY* ent, pmtrace_t* pmtrace )
{
	static const char* redBloodDecals[] = {"{blood1", "{blood2", "{blood3", "{blood4", "{blood5", "{blood6"};
	static const char* yellowBloodDecals[] = {"{yblood1", "{yblood2", "{yblood3", "{yblood4", "{yblood5", "{yblood6"};

	const char* decalName = NULL;
	if (ent->entity.curstate.iuser1 == 1)
	{
		decalName = redBloodDecals[gEngfuncs.pfnRandomLong(0, 5)];
	}
	else if (ent->entity.curstate.iuser1 > 1)
	{
		decalName = yellowBloodDecals[gEngfuncs.pfnRandomLong(0, 5)];
	}

	if (ent->entity.curstate.onground)
	{
		ent->entity.baseline.origin = ent->entity.baseline.origin * 0.9;
		ent->entity.curstate.angles.x = 0;
		ent->entity.curstate.angles.z = 0;
		ent->entity.baseline.angles.x = 0;
		ent->entity.baseline.angles.z = 0;
	}
	else
	{
		ent->entity.curstate.origin = ent->entity.curstate.origin + Vector(0, 0, 8);

		if (ent->entity.curstate.iuser1 > 0 && ent->entity.curstate.iuser2 > 0 && decalName != NULL)
		{
			int decalIndex = gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( (char*)decalName );
			int textureIndex = gEngfuncs.pEfxAPI->Draw_DecalIndex( decalIndex );
			int traceEntIndex = gEngfuncs.pEventAPI->EV_IndexFromTrace( pmtrace );
			gEngfuncs.pEfxAPI->R_DecalShoot(textureIndex, traceEntIndex, 0, pmtrace->endpos, 0 );
			ent->entity.curstate.iuser2--;
		}
	}
}

int __MsgFunc_RandomGibs( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	Vector absmin;
	Vector size;
	Vector direction;

	absmin[0] = READ_COORD();
	absmin[1] = READ_COORD();
	absmin[2] = READ_COORD();

	size[0] = READ_COORD();
	size[1] = READ_COORD();
	size[2] = READ_COORD();

	direction[0] = READ_COORD();
	direction[1] = READ_COORD();
	direction[2] = READ_COORD();

	float randomization = READ_BYTE() / 100.0f;
	int modelIndex = READ_SHORT();
	int gibCount = READ_BYTE();
	int lifeTime = READ_BYTE();
	int bloodType = READ_BYTE();
	int gibBodiesNum = READ_BYTE();
	int startGibIndex = READ_BYTE();

	float velocityMultiplier = READ_BYTE() / 10.0f;

	struct model_s* model = IEngineStudio.GetModelByIndex(modelIndex);

	if (gibBodiesNum == 0)
	{
		studiohdr_t* pstudiohdr = (studiohdr_t *)IEngineStudio.Mod_Extradata(model);
		if (pstudiohdr)
		{
			mstudiobodyparts_t *pbodypart = (mstudiobodyparts_t *)( (byte *)pstudiohdr + pstudiohdr->bodypartindex );

			gibBodiesNum = 1;
			for (int j=0; j<pstudiohdr->numbodyparts; ++j)
			{
				gibBodiesNum = gibBodiesNum * pbodypart[j].nummodels;
			}
		}
	}

	if (gibBodiesNum == 0)
		gibBodiesNum = startGibIndex + 1;
	startGibIndex = startGibIndex > gibBodiesNum - 1 ? gibBodiesNum - 1 : startGibIndex;

	const float clientTime = gEngfuncs.GetClientTime();

	for (int i=0; i<gibCount; ++i)
	{
		Vector gibPos;
		gibPos.x = absmin.x + size.x * gEngfuncs.pfnRandomFloat(0, 1);
		gibPos.y = absmin.y + size.y * gEngfuncs.pfnRandomFloat(0, 1);
		gibPos.z = absmin.z + size.z * gEngfuncs.pfnRandomFloat(0, 1) + 1;

		Vector gibVelocity = direction;
		gibVelocity.x += gEngfuncs.pfnRandomFloat(-randomization, randomization);
		gibVelocity.y += gEngfuncs.pfnRandomFloat(-randomization, randomization);
		gibVelocity.z += gEngfuncs.pfnRandomFloat(-randomization, randomization);

		gibVelocity = gibVelocity * gEngfuncs.pfnRandomFloat( 300, 400 ) * velocityMultiplier;

		if (gibVelocity.Length() > 1500)
		{
			gibVelocity = gibVelocity.Normalize() * 1500;
		}

		TEMPENTITY* pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc(gibPos, model);
		if (!pTemp)
			break;

		pTemp->entity.curstate.body = gEngfuncs.pfnRandomLong(startGibIndex, gibBodiesNum - 1);
		pTemp->flags |= FTENT_COLLIDEWORLD | FTENT_FADEOUT | FTENT_GRAVITY | FTENT_ROTATE | FTENT_PERSIST;

		pTemp->entity.curstate.iuser1 = bloodType;
		pTemp->entity.curstate.iuser2 = 5;
		pTemp->entity.curstate.solid = SOLID_SLIDEBOX;
		pTemp->entity.curstate.movetype = MOVETYPE_BOUNCE;
		pTemp->entity.curstate.friction = 0.55;
		pTemp->entity.curstate.rendermode = kRenderNormal;
		pTemp->entity.curstate.renderamt = pTemp->entity.baseline.renderamt = 255;
		pTemp->hitcallback = &GibHitCallback;

		pTemp->entity.baseline.angles.x = gEngfuncs.pfnRandomFloat(-256, 255);
		pTemp->entity.baseline.angles.z = gEngfuncs.pfnRandomFloat(-256, 255);
		pTemp->entity.baseline.origin = gibVelocity;
		pTemp->die = clientTime + lifeTime;
	}

	return 1;
}

int __MsgFunc_MuzzleLight( const char *pszName, int iSize, void *pbuf )
{
	Vector vecSrc;

	BEGIN_READ( pbuf, iSize );
	vecSrc[0] = READ_COORD();
	vecSrc[1] = READ_COORD();
	vecSrc[2] = READ_COORD();

	if (cl_muzzlelight_monsters && cl_muzzlelight_monsters->value)
	{
		dlight_t* dl = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );
		dl->origin[0] = vecSrc.x;
		dl->origin[1] = vecSrc.y;
		dl->origin[2] = vecSrc.z;
		dl->radius = 144;
		dl->color.r = 255;
		dl->color.g = 208;
		dl->color.b = 128;
		dl->die = gEngfuncs.GetClientTime() + 0.01f;
	}

	return 1;
}

int __MsgFunc_CustomBeam( const char* pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int beamType = READ_BYTE();

	vec3_t	start, end;
	int	modelIndex, startFrame;
	float	frameRate, life, width;
	int	startEnt, endEnt;
	float	noise, speed;
	float	r, g, b, a;
	BEAM* beam;
	int flags;

	switch (beamType) {
	case TE_BEAMPOINTS:
	case TE_BEAMENTPOINT:
	case TE_BEAMENTS:
	case TE_BEAMRING:
	{
		if( beamType == TE_BEAMENTS || beamType == TE_BEAMRING )
		{
			startEnt = READ_SHORT();
			endEnt = READ_SHORT();
		}
		else
		{
			if( beamType == TE_BEAMENTPOINT )
			{
				startEnt = READ_SHORT();
			}
			else
			{
				start[0] = READ_COORD();
				start[1] = READ_COORD();
				start[2] = READ_COORD();
			}
			end[0] = READ_COORD();
			end[1] = READ_COORD();
			end[2] = READ_COORD();
		}
		modelIndex = READ_SHORT();
		startFrame = READ_BYTE();
		frameRate = (float)READ_BYTE() * 0.1f;
		life = (float)READ_BYTE() * 0.1f;
		width = (float)READ_BYTE() * 0.1f;
		noise = (float)READ_BYTE() * 0.01f;
		r = (float)READ_BYTE() / 255.0f;
		g = (float)READ_BYTE() / 255.0f;
		b = (float)READ_BYTE() / 255.0f;
		a = (float)READ_BYTE() / 255.0f;
		speed = (float)READ_BYTE() * 0.1f;
		flags = READ_BYTE();
		if ( beamType == TE_BEAMRING )
			beam = gEngfuncs.pEfxAPI->R_BeamRing( startEnt, endEnt, modelIndex, life, width, noise, a, speed, startFrame, frameRate, r, g, b );
		if( beamType == TE_BEAMENTS )
			beam = gEngfuncs.pEfxAPI->R_BeamEnts( startEnt, endEnt, modelIndex, life, width, noise, a, speed, startFrame, frameRate, r, g, b );
		else if( beamType == TE_BEAMENTPOINT )
			beam = gEngfuncs.pEfxAPI->R_BeamEntPoint( startEnt, end, modelIndex, life, width, noise, a, speed, startFrame, frameRate, r, g, b );
		else
			beam = gEngfuncs.pEfxAPI->R_BeamPoints( start, end, modelIndex, life, width, noise, a, speed, startFrame, frameRate, r, g, b );
		if (beam)
		{
			if (flags & BEAM_FSINE)
				beam->flags |= FBEAM_SINENOISE;
			if (flags & BEAM_FSOLID)
				beam->flags |= FBEAM_SOLID;
			if (flags & BEAM_FSHADEIN)
				beam->flags |= FBEAM_SHADEIN;
			if (flags & BEAM_FSHADEOUT)
				beam->flags |= FBEAM_SHADEOUT;
		}
	}
		break;
	default:
		gEngfuncs.Con_DPrintf("%s: got the unknown beam type %d\n", pszName, beamType);
		break;
	}

	return 1;
}

void FX_Sprite_Trail( Vector start, Vector end, int modelIndex, int count, float life, float size, float amp, int renderamt, float speed, int r = 0, int g = 0, int b = 0, float extraLifeMax = 0.0f )
{
	Vector delta, dir;
	model_t *pmodel;

	if(( pmodel = gEngfuncs.pfnGetModelByIndex( modelIndex )) == NULL )
		return;

	delta = end - start;
	dir = delta.Normalize();

	amp /= 256.0f;

	const float clientTime = gEngfuncs.GetClientTime();

	for( int i = 0; i < count; i++ )
	{
		Vector pos, vel;

		if( i == 0 )
			pos = start;
		else
			VectorMA( start, ( i / ( count - 1.0f )), delta, pos );

		TEMPENTITY *pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc( pos, pmodel );
		if( !pTemp ) return;

		pTemp->flags = (FTENT_COLLIDEWORLD|FTENT_FADEOUT|FTENT_SLOWGRAVITY);
		pTemp->frameMax = pmodel->numframes;
		if (pmodel->numframes > 1)
			pTemp->flags |= FTENT_SPRCYCLE;

		VectorScale( dir, speed, vel );
		vel[0] += Com_RandomFloat( -127.0f, 128.0f ) * amp;
		vel[1] += Com_RandomFloat( -127.0f, 128.0f ) * amp;
		vel[2] += Com_RandomFloat( -127.0f, 128.0f ) * amp;
		VectorCopy( vel, pTemp->entity.baseline.origin );
		VectorCopy( pos, pTemp->entity.origin );

		pTemp->entity.curstate.scale = size;
		pTemp->entity.curstate.rendermode = kRenderGlow;
		pTemp->entity.curstate.renderfx = kRenderFxNoDissipation;
		pTemp->entity.curstate.renderamt = pTemp->entity.baseline.renderamt = renderamt;
		pTemp->entity.curstate.rendercolor.r = r;
		pTemp->entity.curstate.rendercolor.g = g;
		pTemp->entity.curstate.rendercolor.b = b;

		if (pmodel->numframes > 1)
			pTemp->entity.curstate.frame = Com_RandomLong( 0, pmodel->numframes-1 );
		pTemp->die = clientTime + life;
		if (extraLifeMax)
			pTemp->die += Com_RandomFloat( 0.0f, extraLifeMax );
	}
}

int __MsgFunc_SpriteTrail( const char* pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	Vector pos, pos2;

	pos[0] = READ_COORD();
	pos[1] = READ_COORD();
	pos[2] = READ_COORD();
	pos2[0] = READ_COORD();
	pos2[1] = READ_COORD();
	pos2[2] = READ_COORD();
	int modelIndex = READ_SHORT();
	int count = READ_BYTE();
	float life = (float)READ_BYTE() * 0.1f;
	float scale = (float)READ_BYTE();
	if( !scale )
		scale = 1.0f;
	else
		scale *= 0.1f;
	float vel = (float)READ_BYTE() * 10;
	float random = (float)READ_BYTE() * 10;
	int r = READ_BYTE();
	int g = READ_BYTE();
	int b = READ_BYTE();
	int a = READ_BYTE();
	float extraLifeMax = READ_BYTE() * 0.1f;
	FX_Sprite_Trail( pos, pos2, modelIndex, count, life, scale, random, a, vel, r, g, b, extraLifeMax );

	return 1;
}

void FX_Streaks( Vector pos, Vector dir, int color, int count, float speed, int velocityMin, int velocityMax, float minLife = 0.1f, float maxLife = 0.5f, ptype_t particleType = pt_grav, float length = 1.0f )
{
	if (maxLife < minLife)
		maxLife = minLife;

	Vector vel;
	VectorScale( dir, speed, vel );

	for( int i = 0; i < count; i++ )
	{
		vel.x += Com_RandomFloat( velocityMin, velocityMax );
		vel.y += Com_RandomFloat( velocityMin, velocityMax );
		vel.z += Com_RandomFloat( velocityMin, velocityMax );

		particle_t *p = gEngfuncs.pEfxAPI->R_TracerParticles( pos, vel, Com_RandomFloat( minLife, maxLife ));
		if( !p ) return;

		p->type = particleType;
		p->color = color;
		p->ramp = length;
	}
}

int __MsgFunc_Streaks( const char* pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	Vector pos, dir;
	int color, count;
	ptype_t particleType;
	float minLife, maxLife, speed, velRandomness, length;

	pos[0] = READ_COORD();
	pos[1] = READ_COORD();
	pos[2] = READ_COORD();
	dir[0] = READ_COORD();
	dir[1] = READ_COORD();
	dir[2] = READ_COORD();
	color = READ_BYTE();
	count = READ_SHORT();
	speed = READ_SHORT();
	velRandomness = READ_SHORT();
	minLife = READ_BYTE() * 0.1f;
	maxLife = READ_BYTE() * 0.1f;
	particleType = (ptype_t)READ_BYTE();
	length = READ_BYTE() * 0.1f;

	FX_Streaks(pos, dir, color, count, speed, -velRandomness, velRandomness, minLife, maxLife, particleType, length);

	return 1;
}

void ExpandCallback(TEMPENTITY *ent, float frametime, float currenttime)
{
	const float minScale = 0.0001;
	const float timeCreated = ent->entity.curstate.fuser1;
	const float originalScale = ent->entity.curstate.fuser2;
	const float scaleSpeed = ent->entity.curstate.fuser3;
	const bool fade = ent->entity.curstate.iuser1;

	if (fade)
	{
		const int originalRenderamt = ent->entity.curstate.iuser2;
		ent->entity.curstate.renderamt = (ent->frameMax - ent->entity.curstate.frame)/ent->frameMax * originalRenderamt;
		if (ent->entity.curstate.renderamt == 0)
		{
			ent->die = currenttime;
		}
	}

	if (scaleSpeed)
	{
		ent->entity.curstate.scale = scaleSpeed * originalScale * (currenttime - timeCreated) + originalScale;
		if (ent->entity.curstate.scale < minScale)
		{
			ent->entity.curstate.scale = minScale;
			ent->die = currenttime;
		}
	}
}

void FX_Smoke(TEMPENTITY* pTemp, float scale, float speed, float zOffset, int rendermode, int renderamt, int r, int g, int b )
{
	pTemp->entity.curstate.rendermode = rendermode;
	pTemp->entity.curstate.renderfx = kRenderFxNone;
	pTemp->entity.baseline.origin[2] = speed;
	pTemp->entity.curstate.rendercolor.r = r;
	pTemp->entity.curstate.rendercolor.g = g;
	pTemp->entity.curstate.rendercolor.b = b;
	pTemp->entity.curstate.renderamt = renderamt;
	pTemp->entity.origin[2] += zOffset;
	pTemp->entity.curstate.scale = scale;
}

int __MsgFunc_Smoke( const char* pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	Vector pos, dir;
	int modelIndex;
	float scale, frameRate, speed, zOffset, scaleSpeed;
	int rendermode, renderamt, r, g, b;

	const int flags = READ_BYTE();
	const bool directed = (flags & 1) != 0;
	const bool fade = (flags & 2) != 0;
	pos[0] = READ_COORD();
	pos[1] = READ_COORD();
	pos[2] = READ_COORD();
	modelIndex = READ_SHORT();
	scale = (float)(READ_BYTE() * 0.1f);
	frameRate = READ_BYTE();
	speed = READ_SHORT();
	zOffset = READ_SHORT();
	rendermode = READ_BYTE();
	renderamt = READ_BYTE();
	r = READ_BYTE();
	g = READ_BYTE();
	b = READ_BYTE();
	scaleSpeed = READ_SHORT() / 10.0f;

	if (directed)
	{
		dir[0] = READ_COORD();
		dir[1] = READ_COORD();
		dir[2] = READ_COORD();
	}

	// Original hard-coded TE_SMOKE values
	if (speed == 0.0f && zOffset == 0.0f)
	{
		speed = 30;
		zOffset = 20;
	}
	if (renderamt == 0)
		renderamt = 255;
	if (rendermode == 0)
		rendermode = kRenderTransAlpha;
	if (r + g + b == 0)
		r = g = b = Com_RandomLong( 20, 35 );

	TEMPENTITY* pTemp = gEngfuncs.pEfxAPI->R_DefaultSprite( pos, modelIndex, frameRate );

	if (pTemp)
	{
		FX_Smoke(pTemp, scale, speed, zOffset, rendermode, renderamt, r, g, b);
		if (directed)
		{
			pTemp->entity.baseline.origin = dir * speed;
		}
		if (scaleSpeed != 0 || fade)
		{
			pTemp->entity.curstate.fuser1 = gEngfuncs.GetClientTime();
			pTemp->entity.curstate.fuser2 = scale;
			pTemp->entity.curstate.fuser3 = scaleSpeed;
			pTemp->entity.curstate.iuser1 = 1;
			pTemp->entity.curstate.iuser2 = renderamt;
			pTemp->flags |= FTENT_CLIENTCUSTOM;
			pTemp->callback = &ExpandCallback;
		}
	}

	return 1;
}

int __MsgFunc_Particle( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	const float x = READ_COORD();
	const float y = READ_COORD();
	const float z = READ_COORD();

	const float velX = READ_COORD();
	const float velY = READ_COORD();
	const float velZ = READ_COORD();

	const int renderMode = READ_BYTE();
	const float particleLife = READ_BYTE() / 10.0f;
	const float particleSize = READ_SHORT() / 10.0f;
	const int red = READ_BYTE();
	const int green = READ_BYTE();
	const int blue = READ_BYTE();
	const int brightness = READ_BYTE();
	const float gravity = READ_BYTE() / 100.0f;
	const float fadeSpeed = READ_BYTE() / 10.0f;
	const float scaleSpeed = READ_BYTE() / 10.0f;
	const int frameRate = READ_BYTE();
	const int flags = READ_BYTE();
	const int modelIndex = READ_SHORT();

#if USE_PARTICLEMAN
	if (g_pParticleMan)
	{
		const float clTime = gEngfuncs.GetClientTime();

		model_t* sprite = gEngfuncs.pfnGetModelByIndex(modelIndex);
		if (sprite)
		{
			CBaseParticle *particle = g_pParticleMan->CreateParticle(Vector(x,y,z), Vector(0.0f, 0.0f, 0.0f), sprite, particleSize > 0 ? particleSize : 16, brightness, "particle");
			if (particle)
			{
				particle->SetLightFlag(LIGHT_NONE);
				particle->SetCullFlag(CULL_PVS);
				particle->SetRenderFlag(RENDER_FACEPLAYER);

				particle->m_vVelocity = Vector(velX, velY, velZ);
				particle->m_iRendermode = renderMode;
				particle->m_vColor = Vector(red, green, blue);
				particle->m_flGravity = gravity;
				if (fadeSpeed > 0)
					particle->m_flFadeSpeed = fadeSpeed;
				particle->m_flScaleSpeed = scaleSpeed;
				if (flags & SF_PARTICLESHOOTER_ANIMATED)
				{
					particle->m_iFramerate = frameRate > 0 ? frameRate : 10;
					particle->m_iNumFrames = sprite->numframes;
				}

				if (flags & SF_PARTICLESHOOTER_SPIRAL)
					particle->SetCollisionFlags(TRI_SPIRAL);
				if (flags & SF_PARTICLESHOOTER_COLLIDE_WITH_WORLD)
					particle->SetCollisionFlags(TRI_COLLIDEWORLD);
				if (flags & SF_PARTICLESHOOTER_AFFECTED_BY_FORCE)
					particle->m_bAffectedByForce = true;
				if (flags & SF_PARTICLESHOOTER_KILLED_ON_COLLIDE)
					particle->SetCollisionFlags(TRI_COLLIDEKILL);

				particle->m_flDieTime = clTime + particleLife;
			}
		}
	}
#endif

	return 1;
}

void HookFXMessages()
{
	HOOK_MESSAGE( RandomGibs );
	HOOK_MESSAGE( MuzzleLight );
	HOOK_MESSAGE( CustomBeam );
	HOOK_MESSAGE( SpriteTrail );
	HOOK_MESSAGE( Streaks );
	HOOK_MESSAGE( Smoke );
	HOOK_MESSAGE( Particle );
}
