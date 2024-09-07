#include "tex_materials.h"
#include "pm_materials.h"
#include "bullet_types.h"
#include <cstring>

int GetTextureMaterialProperties(char chTextureType, float* fvol, float* fvolbar,
								  const char *rgsz[], int* cnt, float* fattn, int iBulletType)
{
	switch (chTextureType)
	{
	default:
	case CHAR_TEX_CONCRETE:
		*fvol = 0.9f;
		*fvolbar = 0.6f;
		rgsz[0] = "player/pl_step1.wav";
		rgsz[1] = "player/pl_step2.wav";
		*cnt = 2;
		break;
	case CHAR_TEX_METAL:
		*fvol = 0.9f;
		*fvolbar = 0.3f;
		rgsz[0] = "player/pl_metal1.wav";
		rgsz[1] = "player/pl_metal2.wav";
		*cnt = 2;
		break;
	case CHAR_TEX_DIRT:
		*fvol = 0.9f;
		*fvolbar = 0.1f;
		rgsz[0] = "player/pl_dirt1.wav";
		rgsz[1] = "player/pl_dirt2.wav";
		rgsz[2] = "player/pl_dirt3.wav";
		*cnt = 3;
		break;
	case CHAR_TEX_VENT:
		*fvol = 0.5f;
		*fvolbar = 0.3f;
		rgsz[0] = "player/pl_duct1.wav";
		rgsz[1] = "player/pl_duct1.wav";
		*cnt = 2;
		break;
	case CHAR_TEX_GRATE:
		*fvol = 0.9f;
		*fvolbar = 0.5f;
		rgsz[0] = "player/pl_grate1.wav";
		rgsz[1] = "player/pl_grate4.wav";
		*cnt = 2;
		break;
	case CHAR_TEX_TILE:
		*fvol = 0.8f;
		*fvolbar = 0.2f;
		rgsz[0] = "player/pl_tile1.wav";
		rgsz[1] = "player/pl_tile3.wav";
		rgsz[2] = "player/pl_tile2.wav";
		rgsz[3] = "player/pl_tile4.wav";
		*cnt = 4;
		break;
	case CHAR_TEX_SLOSH:
		*fvol = 0.9f;
		*fvolbar = 0.0;
		rgsz[0] = "player/pl_slosh1.wav";
		rgsz[1] = "player/pl_slosh3.wav";
		rgsz[2] = "player/pl_slosh2.wav";
		rgsz[3] = "player/pl_slosh4.wav";
		*cnt = 4;
		break;
	case CHAR_TEX_WOOD:
		*fvol = 0.9f;
		*fvolbar = 0.2f;
		rgsz[0] = "debris/wood1.wav";
		rgsz[1] = "debris/wood2.wav";
		rgsz[2] = "debris/wood3.wav";
		*cnt = 3;
		break;
	case CHAR_TEX_GLASS:
	case CHAR_TEX_COMPUTER:
		*fvol = 0.8f;
		*fvolbar = 0.2f;
		rgsz[0] = "debris/glass1.wav";
		rgsz[1] = "debris/glass2.wav";
		rgsz[2] = "debris/glass3.wav";
		*cnt = 3;
		break;
	case CHAR_TEX_FLESH:
		if( iBulletType == BULLET_PLAYER_CROWBAR )
			return 0; // crowbar already makes this sound
		*fvol = 1.0f;
		*fvolbar = 0.2f;
		rgsz[0] = "weapons/bullet_hit1.wav";
		rgsz[1] = "weapons/bullet_hit2.wav";
		*fattn = 1.0f;
		*cnt = 2;
		break;
	case CHAR_TEX_SNOW:
	case CHAR_TEX_SNOW_OPFOR:
		*fvol = 0.9f;
		*fvolbar = 0.1f;
		rgsz[0] = "player/pl_snow1.wav";
		rgsz[1] = "player/pl_snow2.wav";
		rgsz[2] = "player/pl_snow3.wav";
		*cnt = 3;
		break;
	}
	return 1;
}

void GetStrippedTextureName(char *szbuffer, const char *pTextureName)
{
	// strip leading '-0' or '+0~' or '{' or '!'
	if( *pTextureName == '-' || *pTextureName == '+' )
		pTextureName += 2;

	if( *pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ' )
		pTextureName++;
	// '}}'
	strcpy( szbuffer, pTextureName );
	szbuffer[CBTEXTURENAMEMAX - 1] = 0;
}
