/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== combat.cpp ========================================================

  functions dealing with damage infliction & death

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "game.h"
#include "monsters.h"
#include "soundent.h"
#include "decals.h"
#include "animation.h"
#include "bullet_types.h"
#include "combat.h"
#include "func_break.h"
#include "player.h"
#include "gamerules.h"
#include "scripted.h"
#include "game.h"
#include "common_soundscripts.h"
#include "visuals_utils.h"
#include "ent_templates.h"

extern DLL_GLOBAL Vector		g_vecAttackDir;
extern DLL_GLOBAL int			g_iSkillLevel;

#define GERMAN_GIB_COUNT		4
#define	HUMAN_GIB_COUNT			6
#define ALIEN_GIB_COUNT			4


// HACKHACK -- The gib velocity equations don't work
void CGib::LimitVelocity( void )
{
	float length = pev->velocity.Length();

	// ceiling at 1500.  The gib velocity equation is not bounded properly.  Rather than tune it
	// in 3 separate places again, I'll just limit it here.
	if( length > 1500.0f )
		pev->velocity = pev->velocity.Normalize() * 1500.0f;		// This should really be sv_maxvelocity * 0.75 or something
}


void CGib::SpawnStickyGibs( entvars_t *pevVictim, Vector vecOrigin, int cGibs )
{
	int i;

	for( i = 0; i < cGibs; i++ )
	{
		CGib *pGib = GetClassPtr( (CGib *)NULL );

		pGib->Spawn( "models/stickygib.mdl" );
		pGib->pev->body = RANDOM_LONG( 0, 2 );

		if( pevVictim )
		{
			pGib->pev->origin.x = vecOrigin.x + RANDOM_FLOAT( -3.0f, 3.0f );
			pGib->pev->origin.y = vecOrigin.y + RANDOM_FLOAT( -3.0f, 3.0f );
			pGib->pev->origin.z = vecOrigin.z + RANDOM_FLOAT( -3.0f, 3.0f );

			/*
			pGib->pev->origin.x = pevVictim->absmin.x + pevVictim->size.x * ( RANDOM_FLOAT( 0, 1 ) );
			pGib->pev->origin.y = pevVictim->absmin.y + pevVictim->size.y * ( RANDOM_FLOAT( 0, 1 ) );
			pGib->pev->origin.z = pevVictim->absmin.z + pevVictim->size.z * ( RANDOM_FLOAT( 0, 1 ) );
			*/

			// make the gib fly away from the attack vector
			pGib->pev->velocity = g_vecAttackDir * -1.0f;

			// mix in some noise
			pGib->pev->velocity.x += RANDOM_FLOAT( -0.15f, 0.15f );
			pGib->pev->velocity.y += RANDOM_FLOAT( -0.15f, 0.15f );
			pGib->pev->velocity.z += RANDOM_FLOAT( -0.15f, 0.15f );

			pGib->pev->velocity = pGib->pev->velocity * 900.0f;

			pGib->pev->avelocity.x = RANDOM_FLOAT( 250.0f, 400.0f );
			pGib->pev->avelocity.y = RANDOM_FLOAT( 250.0f, 400.0f );

			// copy owner's blood color
			pGib->m_bloodColor = ( CBaseEntity::Instance( pevVictim ) )->BloodColor();

			if( pevVictim->health > -50 )
			{
				pGib->pev->velocity = pGib->pev->velocity * 0.7f;
			}
			else if( pevVictim->health > -200 )
			{
				pGib->pev->velocity = pGib->pev->velocity * 2.0f;
			}
			else
			{
				pGib->pev->velocity = pGib->pev->velocity * 4.0f;
			}

			pGib->pev->movetype = MOVETYPE_TOSS;
			pGib->pev->solid = SOLID_BBOX;
			UTIL_SetSize( pGib->pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
			pGib->SetTouch( &CGib::StickyGibTouch );
			pGib->SetThink( NULL );
		}
		pGib->LimitVelocity();
	}
}

void CGib::SpawnHeadGib( entvars_t *pevVictim, const Visual* visual )
{
	CGib *pGib = GetClassPtr( (CGib *)NULL );

	pGib->Spawn( "models/hgibs.mdl", visual );// throw one head

	pGib->pev->body = 0;

	if( pevVictim )
	{
		pGib->pev->origin = pevVictim->origin + pevVictim->view_ofs;

		edict_t *pentPlayer = FIND_CLIENT_IN_PVS( pGib->edict() );

		if( RANDOM_LONG( 0, 100 ) <= 5 && pentPlayer )
		{
			// 5% chance head will be thrown at player's face.
			entvars_t *pevPlayer;

			pevPlayer = VARS( pentPlayer );
			pGib->pev->velocity = ( ( pevPlayer->origin + pevPlayer->view_ofs ) - pGib->pev->origin ).Normalize() * 300.0f;
			pGib->pev->velocity.z += 100.0f;
		}
		else
		{
			pGib->pev->velocity = Vector( RANDOM_FLOAT( -100.0f, 100.0f ), RANDOM_FLOAT( -100.0f, 100.0f ), RANDOM_FLOAT( 200.0f, 300.0f ) );
		}

		pGib->pev->avelocity.x = RANDOM_FLOAT( 100.0f, 200.0f );
		pGib->pev->avelocity.y = RANDOM_FLOAT( 100.0f, 300.0f );

		// copy owner's blood color
		pGib->m_bloodColor = ( CBaseEntity::Instance( pevVictim ) )->BloodColor();

		if( pevVictim->health > -50 )
		{
			pGib->pev->velocity = pGib->pev->velocity * 0.7f;
		}
		else if( pevVictim->health > -200 )
		{
			pGib->pev->velocity = pGib->pev->velocity * 2.0f;
		}
		else
		{
			pGib->pev->velocity = pGib->pev->velocity * 4.0f;
		}
	}
	pGib->LimitVelocity();
}

void CGib::SpawnHumanGibs(entvars_t *pevVictim, int cGibs, const Visual* visual)
{
	SpawnRandomGibs( pevVictim, cGibs, "models/hgibs.mdl", HUMAN_GIB_COUNT, 1, visual ); // start at one to avoid throwing random amounts of skulls (0th gib)
}

void CGib::SpawnRandomGibs(entvars_t *pevVictim, int cGibs, const char* gibModel, int gibBodiesNum , int startGibIndex, const Visual* visual)
{
	int cSplat;

	for( cSplat = 0; cSplat < cGibs; cSplat++ )
	{
		CGib *pGib = GetClassPtr( (CGib *)NULL );
		pGib->Spawn( gibModel, visual );
		if (gibBodiesNum <= 0)
		{
			gibBodiesNum = MODEL_FRAMES(pGib->pev->modelindex);
			if (gibBodiesNum == 0)
				gibBodiesNum = startGibIndex + 1;
			startGibIndex = startGibIndex > gibBodiesNum - 1 ? gibBodiesNum - 1 : startGibIndex;
		}
		pGib->pev->body = RANDOM_LONG( startGibIndex, gibBodiesNum - 1 );

		if( pevVictim )
		{
			// spawn the gib somewhere in the monster's bounding volume
			pGib->pev->origin.x = pevVictim->absmin.x + pevVictim->size.x * ( RANDOM_FLOAT( 0.0f, 1.0f ) );
			pGib->pev->origin.y = pevVictim->absmin.y + pevVictim->size.y * ( RANDOM_FLOAT( 0.0f, 1.0f ) );
			pGib->pev->origin.z = pevVictim->absmin.z + pevVictim->size.z * ( RANDOM_FLOAT( 0.0f, 1.0f ) ) + 1.0f;	// absmin.z is in the floor because the engine subtracts 1 to enlarge the box

			// make the gib fly away from the attack vector
			pGib->pev->velocity = g_vecAttackDir * -1.0f;

			// mix in some noise
			pGib->pev->velocity.x += RANDOM_FLOAT( -0.25f, 0.25f );
			pGib->pev->velocity.y += RANDOM_FLOAT( -0.25f, 0.25f );
			pGib->pev->velocity.z += RANDOM_FLOAT( -0.25f, 0.25f );

			pGib->pev->velocity = pGib->pev->velocity * RANDOM_FLOAT( 300.0f, 400.0f );

			pGib->pev->avelocity.x = RANDOM_FLOAT( 100.0f, 200.0f );
			pGib->pev->avelocity.y = RANDOM_FLOAT( 100.0f, 300.0f );

			// copy owner's blood color
			pGib->m_bloodColor = ( CBaseEntity::Instance( pevVictim ) )->BloodColor();

			if( pevVictim->health > -50 )
			{
				pGib->pev->velocity = pGib->pev->velocity * 0.7f;
			}
			else if( pevVictim->health > -200 )
			{
				pGib->pev->velocity = pGib->pev->velocity * 2.0f;
			}
			else
			{
				pGib->pev->velocity = pGib->pev->velocity * 4.0f;
			}

			pGib->pev->solid = SOLID_BBOX;
			UTIL_SetSize( pGib->pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
		}
		pGib->LimitVelocity();
	}
}

void CGib::SpawnRandomGibs(entvars_t *pevVictim, int cGibs, const char* gibModel, const Visual* visual)
{
	SpawnRandomGibs(pevVictim, cGibs, gibModel, 0, 0, visual);
}

extern int gmsgRandomGibs;

void CGib::SpawnRandomClientGibs(entvars_t *pevVictim, int cGibs, const char *gibModel, int gibBodiesNum, int startGibIndex)
{
	if (!pevVictim)
		return;

	Vector direction = g_vecAttackDir * -1;
	int modelIndex = MODEL_INDEX(gibModel);

	byte bloodType;

	CBaseEntity* pEntity = CBaseEntity::Instance(pevVictim);
	int bloodColor = pEntity->BloodColor();
	switch (bloodColor) {
	case BLOOD_COLOR_RED:
		bloodType = 1;
		break;
	case BLOOD_COLOR_YELLOW:
		bloodType = 2;
		break;
	default:
		bloodType = 0;
		break;
	}

	int velocityMultiplier = 10;
	if( pevVictim->health > -50 )
	{
		velocityMultiplier = 7;
	}
	else if( pevVictim->health > -200 )
	{
		velocityMultiplier = 20;
	}
	else
	{
		velocityMultiplier = 40;
	}

	if (gmsgRandomGibs)
	{
		MESSAGE_BEGIN( MSG_PVS, gmsgRandomGibs, pevVictim->origin );
			// position
			WRITE_VECTOR( pevVictim->absmin );

			// size
			WRITE_VECTOR( pevVictim->size );

			// velocity
			WRITE_VECTOR( direction );

			// randomization
			WRITE_BYTE( 25 );

			// Model
			WRITE_SHORT( modelIndex );

			// # of gibs
			WRITE_BYTE( cGibs );

			// lifetime
			WRITE_BYTE( 25 );

			// blood type
			WRITE_BYTE( bloodType );

			WRITE_BYTE( gibBodiesNum );
			WRITE_BYTE( startGibIndex );

			WRITE_BYTE( velocityMultiplier );
		MESSAGE_END();
	}
	else
	{
		ALERT(at_warning, "gmsgRandomGibs is not registered\n");
	}
}

enum
{
	GIBTYPE_UNKNOWN,
	GIBTYPE_HUMAN,
	GIBTYPE_ALIEN,
};
int GibType(CBaseMonster* monster)
{
	switch (monster->DefaultClassify()) {
	case CLASS_HUMAN_MILITARY:
	case CLASS_PLAYER_ALLY:
	case CLASS_HUMAN_PASSIVE:
	case CLASS_PLAYER:
	case CLASS_PLAYER_ALLY_MILITARY:
	case CLASS_HUMAN_BLACKOPS:
	case CLASS_ALIEN_MILITARY:
	case CLASS_ALIEN_MONSTER:
	case CLASS_ALIEN_PASSIVE:
	case CLASS_INSECT:
	case CLASS_ALIEN_PREDATOR:
	case CLASS_ALIEN_PREY:
	case CLASS_RACEX_PREDATOR:
	case CLASS_RACEX_SHOCK:
	case CLASS_GARGANTUA:
	{
		int bloodColor = monster->BloodColor();
		if (bloodColor == BLOOD_COLOR_RED)
			return GIBTYPE_HUMAN;
		else if (bloodColor == BLOOD_COLOR_YELLOW)
			return GIBTYPE_ALIEN;
	}
	default:
		return GIBTYPE_UNKNOWN;
	}
}

BOOL CBaseMonster::HasHumanGibs( void )
{
	return GibType(this) == GIBTYPE_HUMAN;
}

BOOL CBaseMonster::HasAlienGibs( void )
{
	return GibType(this) == GIBTYPE_ALIEN;
}

const char* CBaseMonster::DefaultGibModel()
{
	if (HasHumanGibs()) {
		return "models/hgibs.mdl";
	} else if (HasAlienGibs()) {
		return "models/agibs.mdl";
	}
	return NULL;
}

const char* CBaseMonster::GibModel()
{
	const char* nonDefaultModel = MyNonDefaultGibModel();
	if (nonDefaultModel)
		return nonDefaultModel;

	return DefaultGibModel();
}

int CBaseMonster::DefaultGibCount()
{
	return 4;
}

int CBaseMonster::GibCount()
{
	return FStringNull(m_gibModel) ? DefaultGibCount() : 4;
}

bool CBaseMonster::IsAlienMonster()
{
	switch (DefaultClassify()) {
	case CLASS_ALIEN_MILITARY:
	case CLASS_ALIEN_PASSIVE:
	case CLASS_ALIEN_MONSTER:
	case CLASS_ALIEN_PREY:
	case CLASS_ALIEN_PREDATOR:
	case CLASS_RACEX_PREDATOR:
	case CLASS_RACEX_SHOCK:
	case CLASS_GARGANTUA:
		return true;
	default:
		return false;
	}
}

void CBaseMonster::FadeMonster( void )
{
	StopAnimation();
	pev->velocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
	pev->avelocity = g_vecZero;
	pev->animtime = gpGlobals->time;
	pev->effects |= EF_NOINTERP;
	SUB_StartFadeOut();
}

//=========================================================
// GibMonster - create some gore and get rid of a monster's
// model.
//=========================================================
void CBaseMonster::GibMonster( void )
{
	BOOL		gibbed = FALSE;

	EmitSoundScript(NPC::bodySplatSoundScript);

	const char* gibModel = GibModel();
	const Visual* gibVisual = MyGibVisual();
	if (gibModel)
	{
		if (HasHumanGibs())
		{
			if( violence_hgibs->value != 0 )
			{
				if (FStrEq(gibModel, "models/hgibs.mdl"))
				{
					CGib::SpawnHeadGib(pev, gibVisual);
					CGib::SpawnHumanGibs(pev, 4, gibVisual);
				}
				else
				{
					CGib::SpawnRandomGibs( pev, GibCount(), gibModel, gibVisual );
				}
			}
			gibbed = TRUE;
		}
		else if (HasAlienGibs())
		{
			if( violence_agibs->value != 0 )
			{
				CGib::SpawnRandomGibs( pev, GibCount(), gibModel, gibVisual );
			}
			gibbed = TRUE;
		}
		else
		{
			CGib::SpawnRandomGibs( pev, GibCount(), gibModel, gibVisual );
			gibbed = TRUE;
		}
	}

	if( !IsPlayer() )
	{
		if( gibbed )
		{
			// don't remove players!
			SetThink( &CBaseEntity::SUB_Remove );
			pev->nextthink = gpGlobals->time;
		}
		else
		{
			FadeMonster();
		}
	}
}

//=========================================================
// GetDeathActivity - determines the best type of death
// anim to play.
//=========================================================
Activity CBaseMonster::GetDeathActivity( void )
{
	Activity	deathActivity;
	BOOL		fTriedDirection;
	float		flDot;
	TraceResult	tr;
	Vector		vecSrc;

	if( pev->deadflag != DEAD_NO )
	{
		// don't run this while dying.
		return m_IdealActivity;
	}

	vecSrc = Center();

	fTriedDirection = FALSE;
	deathActivity = ACT_DIESIMPLE;// in case we can't find any special deaths to do.

	UTIL_MakeVectors( pev->angles );
	flDot = DotProduct( gpGlobals->v_forward, g_vecAttackDir * -1.0f );

	switch( m_LastHitGroup )
	{
		// try to pick a region-specific death.
	case HITGROUP_HEAD:
		deathActivity = ACT_DIE_HEADSHOT;
		break;
	case HITGROUP_STOMACH:
		deathActivity = ACT_DIE_GUTSHOT;
		break;
	case HITGROUP_GENERIC:
		// try to pick a death based on attack direction
		fTriedDirection = TRUE;
		if( flDot > 0.3f )
		{
			deathActivity = ACT_DIEFORWARD;
		}
		else if( flDot <= -0.3f )
		{
			deathActivity = ACT_DIEBACKWARD;
		}
		break;
	default:
		// try to pick a death based on attack direction
		fTriedDirection = TRUE;

		if( flDot > 0.3f )
		{
			deathActivity = ACT_DIEFORWARD;
		}
		else if( flDot <= -0.3f )
		{
			deathActivity = ACT_DIEBACKWARD;
		}
		break;
	}

	// can we perform the prescribed death?
	if( LookupActivity( deathActivity ) == ACTIVITY_NOT_AVAILABLE )
	{
		// no! did we fail to perform a directional death? 
		if( fTriedDirection )
		{
			// if yes, we're out of options. Go simple.
			deathActivity = ACT_DIESIMPLE;
		}
		else
		{
			// cannot perform the ideal region-specific death, so try a direction.
			if( flDot > 0.3f )
			{
				deathActivity = ACT_DIEFORWARD;
			}
			else if( flDot <= -0.3f )
			{
				deathActivity = ACT_DIEBACKWARD;
			}
		}
	}

	if( LookupActivity( deathActivity ) == ACTIVITY_NOT_AVAILABLE )
	{
		// if we're still invalid, simple is our only option.
		deathActivity = ACT_DIESIMPLE;
	}

	if( deathActivity == ACT_DIEFORWARD )
	{
		// make sure there's room to fall forward
		UTIL_TraceHull( vecSrc, vecSrc + gpGlobals->v_forward * 64.0f, dont_ignore_monsters, head_hull, edict(), &tr );

		if( tr.flFraction != 1.0f )
		{
			deathActivity = ACT_DIESIMPLE;
		}
	}

	if( deathActivity == ACT_DIEBACKWARD )
	{
		// make sure there's room to fall backward
		UTIL_TraceHull( vecSrc, vecSrc - gpGlobals->v_forward * 64.0f, dont_ignore_monsters, head_hull, edict(), &tr );

		if( tr.flFraction != 1.0f )
		{
			deathActivity = ACT_DIESIMPLE;
		}
	}

	return deathActivity;
}

//=========================================================
// GetSmallFlinchActivity - determines the best type of flinch
// anim to play.
//=========================================================
Activity CBaseMonster::GetSmallFlinchActivity( void )
{
	Activity	flinchActivity;
	// BOOL		fTriedDirection;
	//float		flDot;

	// fTriedDirection = FALSE;
	UTIL_MakeVectors( pev->angles );
	//flDot = DotProduct( gpGlobals->v_forward, g_vecAttackDir * -1.0f );

	switch( m_LastHitGroup )
	{
		// pick a region-specific flinch
	case HITGROUP_HEAD:
		flinchActivity = ACT_FLINCH_HEAD;
		break;
	case HITGROUP_STOMACH:
		flinchActivity = ACT_FLINCH_STOMACH;
		break;
	case HITGROUP_LEFTARM:
		flinchActivity = ACT_FLINCH_LEFTARM;
		break;
	case HITGROUP_RIGHTARM:
		flinchActivity = ACT_FLINCH_RIGHTARM;
		break;
	case HITGROUP_LEFTLEG:
		flinchActivity = ACT_FLINCH_LEFTLEG;
		break;
	case HITGROUP_RIGHTLEG:
		flinchActivity = ACT_FLINCH_RIGHTLEG;
		break;
	case HITGROUP_GENERIC:
	default:
		// just get a generic flinch.
		flinchActivity = ACT_SMALL_FLINCH;
		break;
	}

	// do we have a sequence for the ideal activity?
	if( LookupActivity( flinchActivity ) == ACTIVITY_NOT_AVAILABLE )
	{
		flinchActivity = ACT_SMALL_FLINCH;
	}

	return flinchActivity;
}


void CBaseMonster::BecomeDead( void )
{
	pev->takedamage = DAMAGE_YES;// don't let autoaim aim at corpses.

	// give the corpse half of the monster's original maximum health. 
	pev->health = pev->max_health / 2;
	pev->max_health = 5; // max_health now becomes a counter for how many blood decals the corpse can place.

	// make the corpse fly away from the attack vector
	pev->movetype = MOVETYPE_TOSS;
	if (corpsephysics.value &&
			// affect only dying monsters, not initially dead ones
			m_IdealMonsterState == MONSTERSTATE_DEAD)
	{
		pev->flags &= ~FL_ONGROUND;
		pev->origin.z += 2.0f;
		pev->velocity = g_vecAttackDir * -1.0f;
		pev->velocity = pev->velocity * RANDOM_FLOAT( 300.0f, 400.0f );
	}

}

BOOL CBaseMonster::ShouldGibMonster( int iGib )
{
	if ( iGib != GIB_NEVER && m_gibPolicy == GIBBING_POLICY_PREFER_GIB )
		return TRUE;
	if ( iGib != GIB_ALWAYS && m_gibPolicy == GIBBING_POLICY_PREFER_NOGIB )
		return FALSE;

	if( ( iGib == GIB_NORMAL && pev->health < GIB_HEALTH_VALUE ) || ( iGib == GIB_ALWAYS ) )
		return TRUE;

	return FALSE;
}

void CBaseMonster::CallGibMonster( void )
{
	BOOL fade = FALSE;

	if( HasHumanGibs() )
	{
		if( violence_hgibs->value == 0.0f )
			fade = TRUE;
	}
	else if( HasAlienGibs() )
	{
		if( violence_agibs->value == 0.0f )
			fade = TRUE;
	}

	pev->takedamage = DAMAGE_NO;
	pev->solid = SOLID_NOT;// do something with the body. while monster blows up

	if( fade )
	{
		FadeMonster();
	}
	else
	{
		pev->effects = EF_NODRAW; // make the model invisible.
		GibMonster();
	}

	pev->deadflag = DEAD_DEAD;
	FCheckAITrigger();

	// don't let the status bar glitch for players.with <0 health.
	if( pev->health < -99 )
	{
		pev->health = 0;
	}

	// No need for this. Entity will be removed either by GibMonster or upon fading
	//if( ShouldFadeOnDeath() && !fade )
	//	UTIL_Remove( this );
}

/*
============
Killed
============
*/
void CBaseMonster::Killed( entvars_t *pevInflictor, entvars_t *pevAttacker, int iGib )
{
	//unsigned int	cCount = 0;
	//BOOL		fDone = FALSE;

	if( HasMemory( bits_MEMORY_KILLED ) )
	{
		if( ShouldGibMonster( iGib ) )
			CallGibMonster();
		return;
	}

	// clear the deceased's sound channels.(may have been firing or reloading when killed)
	EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "common/null.wav", 1, ATTN_NORM );
	m_IdealMonsterState = MONSTERSTATE_DEAD;
	// Make sure this condition is fired too (TakeDamage breaks out before this happens on death)
	SetConditions( bits_COND_LIGHT_DAMAGE );

	OnDying();

	if( ShouldGibMonster( iGib ) )
	{
		CallGibMonster();
		return;
	}
	else if( pev->flags & FL_MONSTER )
	{
		SetTouch( NULL );
		BecomeDead();
	}

	// don't let the status bar glitch for players.with <0 health.
	if( pev->health < -99 )
	{
		pev->health = 0;
	}

	//pev->enemy = ENT( pevAttacker );//why? (sjb)

	m_IdealMonsterState = MONSTERSTATE_DEAD;

	pev->solid = SOLID_NOT;
}

void CBaseMonster::OnDying()
{
	if (!g_modFeatures.dying_monsters_block_player)
		pev->iuser3 = -1;
	Remember( bits_MEMORY_KILLED );
	// tell owner ( if any ) that we're dead.This is mostly for MonsterMaker functionality.
	CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
	if( pOwner )
	{
		pOwner->DeathNotice( pev );
	}
}

void CBaseMonster::UpdateOnRemove()
{
	if (!HasMemory(bits_MEMORY_KILLED))
	{
		// Only notice if did not die before removing.
		// If monster died they already reported their death.
		CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
		if( pOwner )
		{
			pOwner->DeathNotice( pev );
		}
	}
	CBaseToggle::UpdateOnRemove();
}

//
// fade out - slowly fades a entity out, then removes it.
//
// DON'T USE ME FOR GIBS AND STUFF IN MULTIPLAYER! 
// SET A FUTURE THINK AND A RENDERMODE!!
void CBaseEntity::SUB_StartFadeOut( void )
{
	if( pev->rendermode == kRenderNormal )
	{
		pev->renderamt = 255;
		pev->rendermode = kRenderTransTexture;
	}

	pev->solid = SOLID_NOT;
	pev->avelocity = g_vecZero;

	pev->nextthink = gpGlobals->time + 0.1f;
	SetThink( &CBaseEntity::SUB_FadeOut );
}

void CBaseEntity::SUB_FadeOut( void )
{
	if( pev->renderamt > 7 )
	{
		pev->renderamt -= 7;
		pev->nextthink = gpGlobals->time + 0.1f;
	}
	else 
	{
		pev->renderamt = 0;
		pev->nextthink = gpGlobals->time + 0.2f;
		SetThink( &CBaseEntity::SUB_Remove );
	}
}

//=========================================================
// WaitTillLand - in order to emit their meaty scent from
// the proper location, gibs should wait until they stop 
// bouncing to emit their scent. That's what this function
// does.
//=========================================================
void CGib::WaitTillLand( void )
{
	if( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}

	if( pev->velocity == g_vecZero ||
			(m_bornTime + m_lifeTime + 10 <= gpGlobals->time) ) // start fading even if gib had not stopped moving at this time. This is to prevent gibs endlessly rotating on edges
	{
		SetThink( &CBaseEntity::SUB_StartFadeOut );
		if (pev->velocity == g_vecZero)
			pev->nextthink = gpGlobals->time + m_lifeTime;
		else
			pev->nextthink = gpGlobals->time;

		// If you bleed, you stink!
		if( m_bloodColor != DONT_BLEED )
		{
			// ok, start stinkin!
			CSoundEnt::InsertSound( bits_SOUND_MEAT, pev->origin, 384, 25 );
		}
	}
	else
	{
		// wait and check again in another half second.
		pev->nextthink = gpGlobals->time + 0.5f;
	}
}

//
// Gib bounces on the ground or wall, sponges some blood down, too!
//
void CGib::BounceGibTouch( CBaseEntity *pOther )
{
	Vector	vecSpot;
	TraceResult	tr;

	//if( RANDOM_LONG( 0, 1 ) )
	//	return;// don't bleed everytime

	if( pev->flags & FL_ONGROUND )
	{
		pev->velocity = pev->velocity * 0.9f;
		pev->angles.x = 0.0f;
		pev->angles.z = 0.0f;
		pev->avelocity.x = 0.0f;
		pev->avelocity.z = 0.0f;
	}
	else
	{
		if( m_cBloodDecals > 0 && m_bloodColor != DONT_BLEED )
		{
			vecSpot = pev->origin + Vector( 0.0f, 0.0f, 8.0f );//move up a bit, and trace down.
			UTIL_TraceLine( vecSpot, vecSpot + Vector( 0.0f, 0.0f, -24.0f ), ignore_monsters, ENT( pev ), &tr );

			UTIL_BloodDecalTrace( &tr, m_bloodColor );

			m_cBloodDecals--; 
		}

		if( m_material != matNone && RANDOM_LONG( 0, 2 ) == 0 )
		{
			float volume;
			float zvel = fabs( pev->velocity.z );

			volume = 0.8f * Q_min( 1.0f, zvel / 450.0f );

			CBreakable::MaterialSoundRandom( edict(), (Materials)m_material, volume );
		}
	}
}

//
// Sticky gib puts blood on the wall and stays put. 
//
void CGib::StickyGibTouch( CBaseEntity *pOther )
{
	Vector	vecSpot;
	TraceResult	tr;

	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 10.0f;

	if( !FClassnameIs( pOther->pev, "worldspawn" ) )
	{
		pev->nextthink = gpGlobals->time;
		return;
	}

	UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 32.0f,  ignore_monsters, ENT( pev ), &tr );

	UTIL_BloodDecalTrace( &tr, m_bloodColor );

	pev->velocity = tr.vecPlaneNormal * -1.0f;
	pev->angles = UTIL_VecToAngles( pev->velocity );
	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
}

//
// Throw a chunk
//
void CGib::Spawn( const char *szGibModel, const Visual* visual )
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->friction = 0.55f; // deading the bounce a bit

	// sometimes an entity inherits the edict from a former piece of glass,
	// and will spawn using the same render FX or rendermode! bad!
	pev->renderamt = 255;
	pev->rendermode = kRenderNormal;
	pev->renderfx = kRenderFxNone;
	pev->solid = SOLID_TRIGGER; //LRC - so that they don't get in each other's way when we fire lots
	//pev->solid = SOLID_SLIDEBOX;/// hopefully this will fix the VELOCITY TOO LOW crap
	pev->classname = MAKE_STRING( "gib" );

	ApplyVisual(visual, szGibModel);

	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	pev->nextthink = gpGlobals->time + 4.0f;
	m_lifeTime = 25;
	m_bornTime = gpGlobals->time;
	SetThink( &CGib::WaitTillLand );
	SetTouch( &CGib::BounceGibTouch );

	m_material = matNone;
	m_cBloodDecals = 5;// how many blood decals this gib can place (1 per bounce until none remain). 
}

void CGib::StartFadeOut()
{
	if( pev->rendermode == kRenderNormal )
	{
		pev->renderamt = 255;
		pev->rendermode = kRenderTransTexture;
	}

	pev->avelocity = g_vecZero;

	pev->nextthink = gpGlobals->time + 0.1f;
	SetThink( &CBaseEntity::SUB_FadeOut );
}

// take health
int CBaseMonster::TakeHealth(CBaseEntity *pHealer, float flHealth, int bitsDamageType )
{
	if( !pev->takedamage )
		return 0;

	// clear out any damage types we healed.
	// UNDONE: generic health should not heal any
	// UNDONE: time-based damage

	m_bitsDamageType &= ~( bitsDamageType & ~DMG_TIMEBASED );

	return CBaseEntity::TakeHealth( pHealer, flHealth, bitsDamageType );
}

void AddScoreForDamage(entvars_t *pevAttacker, CBaseEntity* victim, const float damage)
{
	if (!g_pGameRules->IsCoOp() || !dmgperscore.value) {
		return;
	}
	CBaseEntity *attacker = CBaseEntity::Instance( pevAttacker );
	if (attacker && attacker->IsPlayer()) {
		const float dmg = damage > victim->pev->health ? victim->pev->health : damage;
		const float score = dmg / dmgperscore.value;

		if (victim->IsPlayer()) {
			if (victim != attacker && g_pGameRules->PlayerRelationship(attacker, victim) == GR_TEAMMATE) {
				attacker->AddFloatPoints(-score * allydmgpenalty.value, true);
			}
		} else {
			CBaseMonster* monster = victim->MyMonsterPointer();
			if (monster)
			{
				if (monster->IDefaultRelationship(CLASS_PLAYER) == R_AL) {
					attacker->AddFloatPoints(-score * allydmgpenalty.value, true);
				} else {
					attacker->AddFloatPoints(score, true);
				}
			}
		}
	}
}

/*
============
TakeDamage

The damage is coming from inflictor, but get mad at attacker
This should be the only function that ever reduces health.
bitsDamageType indicates the type of damage sustained, ie: DMG_SHOCK

Time-based damage: only occurs while the monster is within the trigger_hurt.
When a monster is poisoned via an arrow etc it takes all the poison damage at once.

GLOBALS ASSUMED SET:  g_iSkillLevel
============
*/
int CBaseMonster::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	float	flTake;
	Vector	vecDir;

	if( !pev->takedamage )
		return 0;

	if (!g_pGameRules->FMonsterCanTakeDamage(this, CBaseEntity::Instance(pevAttacker)))
		return 0;

	if( !IsAlive() )
	{
		return DeadTakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
	}

	const short takeDamagePolicy = m_pCine ? m_pCine->m_takeDamagePolicy : 0;
	if (takeDamagePolicy == SCRIPT_TAKE_DAMAGE_POLICY_INVULNERABLE)
		return 0;

	if( pev->deadflag == DEAD_NO && flDamage > 0 )
	{
		// no pain sound during death animation.
		PainSound();// "Ouch!"
	}

	//!!!LATER - make armor consideration here!
	flTake = flDamage;

	// set damage type sustained
	m_bitsDamageType |= bitsDamageType;

	// grab the vector of the incoming attack. ( pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	vecDir = Vector( 0, 0, 0 );
	if( !FNullEnt( pevInflictor ) )
	{
		CBaseEntity *pInflictor = CBaseEntity::Instance( pevInflictor );
		if( pInflictor )
		{
			vecDir = ( pInflictor->Center() - Vector ( 0, 0, 10 ) - Center() ).Normalize();
			vecDir = g_vecAttackDir = vecDir.Normalize();
		}
	}

	// add to the damage total for clients, which will be sent as a single
	// message at the end of the frame
	// todo: remove after combining shotgun blasts?
	if( IsPlayer() )
	{
		if( pevInflictor )
			pev->dmg_inflictor = ENT( pevInflictor );

		pev->dmg_take += flTake;

		// check for godmode or invincibility
		if( pev->flags & FL_GODMODE )
		{
			return 0;
		}

		// if this is a player, move him around!
		if( ( !FNullEnt( pevInflictor ) ) && ( pev->movetype == MOVETYPE_WALK ) && ( !pevAttacker || pevAttacker->solid != SOLID_TRIGGER ) && !FBitSet(bitsDamageType, DMG_NO_PLAYER_PUSH) )
		{
			Vector velocityAdd = vecDir * -DamageForce( flDamage );
			if (!AllowGrenadeJump())
			{
				velocityAdd.z = 0;
			}
			pev->velocity = pev->velocity + velocityAdd;
		}
	}

	AddScoreForDamage(pevAttacker, this, flTake);

	ApplyDamageToHealth(flTake);

	// HACKHACK Don't kill monsters in a script.  Let them break their scripts first
	if( m_MonsterState == MONSTERSTATE_SCRIPT )
	{
		if (takeDamagePolicy == SCRIPT_TAKE_DAMAGE_POLICY_NONLETHAL && pev->health <= 0.0f)
			pev->health = 1.0f;

		if ( m_pCine && m_pCine->m_interruptionPolicy == SCRIPT_INTERRUPTION_POLICY_ONLY_DEATH )
		{
			if (pev->health <= 0.0f)
				SetConditions( bits_COND_HEAVY_DAMAGE );
		}
		else if (flDamage > 0)
			SetConditions( bits_COND_LIGHT_DAMAGE );
		return 0;
	}

	if( pev->health <= 0 )
	{
		if (FBitSet(bitsDamageType, DMG_NONLETHAL))
		{
			pev->health = 1;
		}
		else
		{
			int gibType = GIB_NORMAL;
			if( bitsDamageType & DMG_ALWAYSGIB )
			{
				gibType = GIB_ALWAYS;
			}
			else if( bitsDamageType & DMG_NEVERGIB )
			{
				gibType = GIB_NEVER;
			}
			Killed( pevInflictor, pevAttacker, gibType );
			return 0;
		}
	}

	// react to the damage (get mad)
	if( ( pev->flags & FL_MONSTER ) && !FNullEnt( pevAttacker ) )
	{
		if( pevAttacker->flags & ( FL_MONSTER | FL_CLIENT ) )
		{
			// only if the attack was a monster or client!
			// enemy's last known position is somewhere down the vector that the attack came from.
			if( pevInflictor )
			{
				if( m_hEnemy == 0 || pevInflictor == m_hEnemy->pev || !HasConditions( bits_COND_SEE_ENEMY ) )
				{
					m_vecEnemyLKP = pevInflictor->origin;
				}
			}
			else
			{
				m_vecEnemyLKP = pev->origin + ( g_vecAttackDir * 64.0f );
			}

			MakeIdealYaw( m_vecEnemyLKP );

			// add pain to the conditions
			if( flDamage > 0.0f )
			{
				SetConditions( bits_COND_LIGHT_DAMAGE );
			}

			const float heavyDamageValue = Q_min(60.0f, Q_max(20.0f, pev->max_health/3));
			if( flDamage >= heavyDamageValue )
			{
				SetConditions( bits_COND_HEAVY_DAMAGE );
			}

			m_bForceConditionsGather = TRUE;
		}
	}

	return 1;
}

//=========================================================
// DeadTakeDamage - takedamage function called when a monster's
// corpse is damaged.
//=========================================================
int CBaseMonster::DeadTakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	Vector vecDir;

	// grab the vector of the incoming attack. ( pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	vecDir = Vector( 0, 0, 0 );
	if( !FNullEnt( pevInflictor ) )
	{
		CBaseEntity *pInflictor = CBaseEntity::Instance( pevInflictor );
		if( pInflictor )
		{
			vecDir = ( pInflictor->Center() - Vector ( 0.0f, 0.0f, 10.0f ) - Center() ).Normalize();
			vecDir = g_vecAttackDir = vecDir.Normalize();
		}
	}

#if 0// turn this back on when the bounding box issues are resolved.

	pev->flags &= ~FL_ONGROUND;
	pev->origin.z += 1.0f;

	// let the damage scoot the corpse around a bit.
	if( !FNullEnt( pevInflictor ) && ( pevAttacker->solid != SOLID_TRIGGER ) )
	{
		pev->velocity = pev->velocity + vecDir * -DamageForce( flDamage );
	}
#endif
	// kill the corpse if enough damage was done to destroy the corpse and the damage is of a type that is allowed to destroy the corpse.
	if( bitsDamageType & DMG_GIB_CORPSE )
	{
		if( pev->health <= flDamage )
		{
			pev->health = -50;
			Killed( pevInflictor, pevAttacker, GIB_ALWAYS );
			return 0;
		}
		// Accumulate corpse gibbing damage, so you can gib with multiple hits
		pev->health -= flDamage * 0.1f;
	}

	return 1;
}

float CBaseMonster::DamageForce( float damage )
{ 
	float force = damage * ( ( 32.0f * 32.0f * 72.0f ) / ( pev->size.x * pev->size.y * pev->size.z ) ) * 5.0f;

	if( force > 1000.0f ) 
	{
		force = 1000.0f;
	}

	return force;
}

//
// RadiusDamage - this entity is exploding, or otherwise needs to inflict damage upon entities within a certain range.
// 
// only damage ents that can clearly be seen by the explosion!
void RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType )
{
	CBaseEntity *pEntity = NULL;
	TraceResult	tr;
	float		flAdjustedDamage, falloff;
	Vector		vecSpot;

	if( flRadius )
		falloff = flDamage / flRadius;
	else
		falloff = 1.0f;

	int bInWater = ( UTIL_PointContents( vecSrc ) == CONTENTS_WATER );

	vecSrc.z += 1.0f;// in case grenade is lying on the ground

	if( !pevAttacker )
		pevAttacker = pevInflictor;

	// iterate on all entities in the vicinity.
	while( ( pEntity = UTIL_FindEntityInSphere( pEntity, vecSrc, flRadius ) ) != NULL )
	{
		if( pEntity->pev->takedamage != DAMAGE_NO )
		{
			// UNDONE: this should check a damage mask, not an ignore
			if( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
			{
				// houndeyes don't hurt other houndeyes with their attack
				continue;
			}

			// blast's don't tavel into or out of water
			if( bInWater && pEntity->pev->waterlevel == 0 )
				continue;
			if( !bInWater && pEntity->pev->waterlevel == 3 )
				continue;

			vecSpot = pEntity->BodyTarget( vecSrc );

			UTIL_TraceLine( vecSrc, vecSpot, dont_ignore_monsters, ENT( pevInflictor ), &tr );

			if( tr.flFraction == 1.0f || tr.pHit == pEntity->edict() )
			{
				// the explosion can 'see' this entity, so hurt them!
				if( tr.fStartSolid )
				{
					// if we're stuck inside them, fixup the position and distance
					tr.vecEndPos = vecSrc;
					tr.flFraction = 0.0f;
				}

				// decrease damage for an ent that's farther from the bomb.
				flAdjustedDamage = ( vecSrc - tr.vecEndPos ).Length() * falloff;
				flAdjustedDamage = flDamage - flAdjustedDamage;

				if( flAdjustedDamage < 0.0f )
				{
					flAdjustedDamage = 0.0f;
				}

				// ALERT( at_console, "hit %s\n", STRING( pEntity->pev->classname ) );
				if( tr.flFraction != 1.0f )
				{
					pEntity->ApplyTraceAttack( pevInflictor, pevAttacker, flAdjustedDamage, ( tr.vecEndPos - vecSrc ).Normalize(), &tr, bitsDamageType );
				}
				else
				{
					pEntity->TakeDamage ( pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType );
				}
			}
		}
	}
}

void CBaseMonster::RadiusDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType )
{
	::RadiusDamage( pev->origin, pevInflictor, pevAttacker, flDamage, flDamage * DEFAULT_EXPLOSION_RADIUS_MULTIPLIER, iClassIgnore, bitsDamageType );
}

void CBaseMonster::RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType )
{
	::RadiusDamage( vecSrc, pevInflictor, pevAttacker, flDamage, flDamage * DEFAULT_EXPLOSION_RADIUS_MULTIPLIER, iClassIgnore, bitsDamageType );
}

//=========================================================
// CheckTraceHullAttack - expects a length to trace, amount 
// of damage to do, and damage type. Returns a pointer to
// the damaged entity in case the monster wishes to do
// other stuff to the victim (punchangle, etc)
//
// Used for many contact-range melee attacks. Bites, claws, etc.
//=========================================================
CBaseEntity* CBaseMonster::CheckTraceHullAttack( float flDist, int iDamage, int iDmgType )
{
	return CheckTraceHullAttack( flDist, iDamage, iDmgType, pev->size.z * 0.5f );
}

CBaseEntity* CBaseMonster::CheckTraceHullAttack( float flDist, int iDamage, int iDmgType, float height )
{
	TraceResult tr;

	if( IsPlayer() )
		UTIL_MakeVectors( pev->angles );
	else
		UTIL_MakeAimVectors( pev->angles );

	Vector vecStart = pev->origin;
	vecStart.z += height;
	Vector vecEnd = vecStart + ( gpGlobals->v_forward * flDist );

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT( pev ), &tr );

	if( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		if( iDamage > 0 )
		{
			pEntity->TakeDamage( pev, pev, iDamage, iDmgType );
		}

		return pEntity;
	}

	return NULL;
}

//=========================================================
// FInViewCone - returns true is the passed ent is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//=========================================================
BOOL CBaseMonster::FInViewCone( CBaseEntity *pEntity )
{
	Vector2D	vec2LOS;
	float	flDot;

	UTIL_MakeVectors( pev->angles );

	vec2LOS = ( pEntity->pev->origin - pev->origin ).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct( vec2LOS, gpGlobals->v_forward.Make2D() );

	if( flDot > m_flFieldOfView )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//=========================================================
// FInViewCone - returns true is the passed vector is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//=========================================================
BOOL CBaseMonster::FInViewCone( Vector *pOrigin )
{
	Vector2D	vec2LOS;
	float		flDot;

	UTIL_MakeVectors( pev->angles );

	vec2LOS = ( *pOrigin - pev->origin ).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct( vec2LOS, gpGlobals->v_forward.Make2D() );

	if( flDot > m_flFieldOfView )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//=========================================================
// FVisible - returns true if a line can be traced from
// the caller's eyes to the target
//=========================================================
BOOL CBaseEntity::FVisible( CBaseEntity *pEntity )
{
	TraceResult tr;
	Vector		vecLookerOrigin;
	Vector		vecTargetOrigin;

	if( !pEntity )
		return FALSE;
	if( !pEntity->pev )
		return FALSE;

	if( FBitSet( pEntity->pev->flags, FL_NOTARGET ) )
		return FALSE;

	// don't look through water
	if( ( pev->waterlevel != 3 && pEntity->pev->waterlevel == 3 ) 
		|| ( pev->waterlevel == 3 && pEntity->pev->waterlevel == 0 ) )
		return FALSE;

	vecLookerOrigin = pev->origin + pev->view_ofs;//look through the caller's 'eyes'
	vecTargetOrigin = pEntity->EyePosition();

	UTIL_TraceLine( vecLookerOrigin, vecTargetOrigin, ignore_monsters, ignore_glass, ENT( pev )/*pentIgnore*/, &tr );

	if( tr.flFraction != 1.0f )
	{
		return FALSE;// Line of sight is not established
	}
	else
	{
		return TRUE;// line of sight is valid.
	}
}

//=========================================================
// FVisible - returns true if a line can be traced from
// the caller's eyes to the target vector
//=========================================================
BOOL CBaseEntity::FVisible( const Vector &vecOrigin )
{
	TraceResult tr;
	Vector		vecLookerOrigin;

	vecLookerOrigin = EyePosition();//look through the caller's 'eyes'

	UTIL_TraceLine( vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, ENT( pev )/*pentIgnore*/, &tr );

	if( tr.flFraction != 1.0f )
	{
		return FALSE;// Line of sight is not established
	}
	else
	{
		return TRUE;// line of sight is valid.
	}
}

/*
================
TraceAttack
================
*/
void CBaseEntity::TraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	Vector vecOrigin = ptr->vecEndPos - vecDir * 4.0f;

	if( pev->takedamage )
	{
		AddMultiDamage( pevInflictor, pevAttacker, this, flDamage, bitsDamageType );

		int blood = BloodColor();

		if( blood != DONT_BLEED )
		{
			SpawnBlood( vecOrigin, blood, flDamage );// a little surface blood.
			TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
		}
	}
}

void CBaseEntity::ApplyTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	ClearMultiDamage();
	TraceAttack(pevInflictor, pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
	ApplyMultiDamage(pevInflictor, pevAttacker);
}

/*
//=========================================================
// TraceAttack
//=========================================================
void CBaseMonster::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	Vector vecOrigin = ptr->vecEndPos - vecDir * 4.0f;

	ALERT( at_console, "%d\n", ptr->iHitgroup );

	if( pev->takedamage )
	{
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );

		int blood = BloodColor();

		if( blood != DONT_BLEED )
		{
			SpawnBlood( vecOrigin, blood, flDamage );// a little surface blood.
		}
	}
}
*/

//=========================================================
// TraceAttack
//=========================================================
float CBaseMonster::HeadHitGroupDamageMultiplier()
{
	return gSkillData.monHead;
}

void CBaseMonster::TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if( pev->takedamage )
	{
		m_LastHitGroup = ptr->iHitgroup;

		switch( ptr->iHitgroup )
		{
		case HITGROUP_GENERIC:
			break;
		case HITGROUP_HEAD:
			flDamage *= HeadHitGroupDamageMultiplier();
			break;
		case HITGROUP_CHEST:
			flDamage *= gSkillData.monChest;
			break;
		case HITGROUP_STOMACH:
			flDamage *= gSkillData.monStomach;
			break;
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			flDamage *= gSkillData.monArm;
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			flDamage *= gSkillData.monLeg;
			break;
		default:
			break;
		}

		if (!FBitSet(bitsDamageType, DMG_DONTBLEED))
		{
			SpawnBlood( ptr->vecEndPos, BloodColor(), flDamage );// a little surface blood.
			TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
		}
		AddMultiDamage( pevInflictor, pevAttacker, this, flDamage, bitsDamageType );
	}
}

static float DamageByBulletType(int bulletType, float defaultDamage)
{
	switch (bulletType) {
	case BULLET_PLAYER_9MM:
		return gSkillData.plrDmg9MM;
	case BULLET_PLAYER_MP5:
		return gSkillData.plrDmgMP5;
	case BULLET_PLAYER_357:
		return gSkillData.plrDmg357;
	case BULLET_PLAYER_BUCKSHOT:
		return gSkillData.plrDmgBuckshot;
#if FEATURE_M249
	case BULLET_PLAYER_556:
		return gSkillData.plrDmg556;
#endif
#if FEATURE_SNIPERRIFLE
	case BULLET_PLAYER_762:
		return gSkillData.plrDmg762;
#endif
#if FEATURE_DESERT_EAGLE
	case BULLET_PLAYER_EAGLE:
		return gSkillData.plrDmgEagle;
#endif
#if FEATURE_UZI
	case BULLET_PLAYER_UZI:
		return gSkillData.plrDmgUzi;
#endif
	case BULLET_MONSTER_9MM:
		return gSkillData.monDmg9MM;
	case BULLET_MONSTER_MP5:
		return gSkillData.monDmgMP5;
	case BULLET_MONSTER_12MM:
		return gSkillData.monDmg12MM;
	case BULLET_MONSTER_357:
		return gSkillData.monDmg357;
	case BULLET_MONSTER_556:
		return gSkillData.monDmg556;
	case BULLET_MONSTER_762:
		return gSkillData.monDmg762;
	default:
		return defaultDamage;
	}
}

static void DoBulletTraceAttack(entvars_t *pevInflictor, entvars_t *pevAttacker, TraceResult& tr, const Vector& vecDir, const Vector& vecSrc, const Vector& vecEnd, int iBulletType, int iDamage, float defaultDamage, bool decalsPredicted = false)
{
	CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

	if( iDamage )
	{
		pEntity->TraceAttack( pevInflictor, pevAttacker, iDamage, vecDir, &tr, DMG_BULLET | ( ( iDamage > 16 ) ? DMG_ALWAYSGIB : DMG_NEVERGIB ) );

		TEXTURETYPE_PlaySound( &tr, vecSrc, vecEnd, iBulletType );
		DecalGunshot( &tr, iBulletType );
	}
	else
	{
		if (iBulletType == BULLET_NONE)
		{
			pEntity->TraceAttack( pevInflictor, pevAttacker, 50, vecDir, &tr, DMG_CLUB );
			TEXTURETYPE_PlaySound( &tr, vecSrc, vecEnd, iBulletType );
			// only decal glass
			if( !FNullEnt( tr.pHit ) && VARS( tr.pHit )->rendermode != 0 )
			{
				UTIL_DecalTrace( &tr, DECAL_GLASSBREAK1 + RANDOM_LONG( 0, 2 ) );
			}
		}
		else
		{
			pEntity->TraceAttack( pevInflictor, pevAttacker, DamageByBulletType(iBulletType, defaultDamage), vecDir, &tr, DMG_BULLET );

			if (!decalsPredicted)
			{
				TEXTURETYPE_PlaySound( &tr, vecSrc, vecEnd, iBulletType );
				DecalGunshot( &tr, iBulletType );
			}
		}
	}
}

/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.

This version is used by Monsters.
================
*/
void CBaseEntity::FireBullets( ULONG cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, int iBulletType, int iTracerFreq, int iDamage, entvars_t *pevAttacker )
{
	static int tracerCount;
	TraceResult tr;
	Vector vecRight = gpGlobals->v_right;
	Vector vecUp = gpGlobals->v_up;

	if( pevAttacker == NULL )
		pevAttacker = pev;  // the default attacker is ourselves

	ClearMultiDamage();
	gMultiDamage.type = DMG_BULLET | DMG_NEVERGIB;

	UTIL_MuzzleLight(vecSrc);

	for( ULONG iShot = 1; iShot <= cShots; iShot++ )
	{
		// get circular gaussian spread
		float x, y, z;
		do {
			x = RANDOM_FLOAT( -0.5f, 0.5f ) + RANDOM_FLOAT( -0.5f, 0.5f );
			y = RANDOM_FLOAT( -0.5f, 0.5f ) + RANDOM_FLOAT( -0.5f, 0.5f );
			z = x * x + y * y;
		} while (z > 1);

		Vector vecDir = vecDirShooting +
						x * vecSpread.x * vecRight +
						y * vecSpread.y * vecUp;
		Vector vecEnd;

		vecEnd = vecSrc + vecDir * flDistance;
		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev )/*pentIgnore*/, &tr );

		if( iTracerFreq != 0 && ( tracerCount++ % iTracerFreq ) == 0 )
		{
			Vector vecTracerSrc;

			if( IsPlayer() )
			{
				// adjust tracer position for player
				vecTracerSrc = vecSrc + Vector( 0.0f, 0.0f, -4.0f ) + gpGlobals->v_right * 2.0f + gpGlobals->v_forward * 16.0f;
			}
			else
			{
				vecTracerSrc = vecSrc;
			}

			MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, vecTracerSrc );
				WRITE_BYTE( TE_TRACER );
				WRITE_VECTOR( vecTracerSrc );
				WRITE_VECTOR( tr.vecEndPos );
			MESSAGE_END();
		}
		// do damage, paint decals
		if( tr.flFraction != 1.0f )
		{
			DoBulletTraceAttack(pev, pevAttacker, tr, vecDir, vecSrc, vecEnd, iBulletType, iDamage, gSkillData.monDmg9MM);
		}
		// make bullet trails
		UTIL_BubbleTrail( vecSrc, tr.vecEndPos, (int)( ( flDistance * tr.flFraction ) / 64.0f ) );
	}
	ApplyMultiDamage( pev, pevAttacker );
}

/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.

This version is used by Players, uses the random seed generator to sync client and server side shots.
================
*/
Vector CBaseEntity::FireBulletsPlayer( ULONG cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, int iBulletType, int iTracerFreq, int iDamage, entvars_t *pevAttacker, int shared_rand )
{
	TraceResult tr;
	Vector vecRight = gpGlobals->v_right;
	Vector vecUp = gpGlobals->v_up;
	float x = 0.0f, y = 0.0f;
	//float z;

	if( pevAttacker == NULL )
		pevAttacker = pev;  // the default attacker is ourselves

	ClearMultiDamage();
	gMultiDamage.type = DMG_BULLET | DMG_NEVERGIB;

	for( ULONG iShot = 1; iShot <= cShots; iShot++ )
	{
		//Use player's random seed.
		// get circular gaussian spread
		x = UTIL_SharedRandomFloat( shared_rand + iShot, -0.5f, 0.5f ) + UTIL_SharedRandomFloat( shared_rand + ( 1 + iShot ) , -0.5f, 0.5f );
		y = UTIL_SharedRandomFloat( shared_rand + ( 2 + iShot ), -0.5f, 0.5f ) + UTIL_SharedRandomFloat( shared_rand + ( 3 + iShot ), -0.5f, 0.5f );
		//z = x * x + y * y;

		Vector vecDir = vecDirShooting +
						x * vecSpread.x * vecRight +
						y * vecSpread.y * vecUp;
		Vector vecEnd;

		vecEnd = vecSrc + vecDir * flDistance;
		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev )/*pentIgnore*/, &tr );

		// do damage, paint decals
		if( tr.flFraction != 1.0f )
		{
			DoBulletTraceAttack(pev, pevAttacker, tr, vecDir, vecSrc, vecEnd, iBulletType, iDamage, gSkillData.plrDmg9MM, true);
		}
		// make bullet trails
		UTIL_BubbleTrail( vecSrc, tr.vecEndPos, (int)( ( flDistance * tr.flFraction ) / 64.0f ) );
	}
	ApplyMultiDamage( pev, pevAttacker );

	return Vector( x * vecSpread.x, y * vecSpread.y, 0.0 );
}

void CBaseEntity::TraceBleed( float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if( BloodColor() == DONT_BLEED )
		return;

	if( (int)flDamage == 0 )
		return;

	if( !( bitsDamageType & ( DMG_CRUSH | DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB | DMG_MORTAR ) ) )
		return;

	// make blood decal on the wall! 
	TraceResult Bloodtr;
	Vector vecTraceDir; 
	float flNoise;
	int cCount;
	int i;

/*
	if( !IsAlive() )
	{
		// dealing with a dead monster. 
		if( pev->max_health <= 0 )
		{
			// no blood decal for a monster that has already decalled its limit.
			return; 
		}
		else
		{
			pev->max_health--;
		}
	}
*/
	if( flDamage < 10.0f )
	{
		flNoise = 0.1f;
		cCount = 1;
	}
	else if( flDamage < 25.0f )
	{
		flNoise = 0.2f;
		cCount = 2;
	}
	else
	{
		flNoise = 0.3f;
		cCount = 4;
	}

	for( i = 0; i < cCount; i++ )
	{
		vecTraceDir = vecDir * -1.0f;// trace in the opposite direction the shot came from (the direction the shot is going)

		vecTraceDir.x += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.y += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.z += RANDOM_FLOAT( -flNoise, flNoise );

		UTIL_TraceLine( ptr->vecEndPos, ptr->vecEndPos + vecTraceDir * -172.0f, ignore_monsters, ENT( pev ), &Bloodtr );

		if( Bloodtr.flFraction != 1.0f )
		{
			UTIL_BloodDecalTrace( &Bloodtr, BloodColor() );
		}
	}
}

//=========================================================
//=========================================================
void CBaseMonster::MakeDamageBloodDecal( int cCount, float flNoise, TraceResult *ptr, const Vector &vecDir )
{
	// make blood decal on the wall! 
	TraceResult Bloodtr;
	Vector vecTraceDir; 
	int i;

	if( !IsAlive() )
	{
		// dealing with a dead monster. 
		if( pev->max_health <= 0 )
		{
			// no blood decal for a monster that has already decalled its limit.
			return; 
		}
		else
		{
			pev->max_health--;
		}
	}

	for( i = 0; i < cCount; i++ )
	{
		vecTraceDir = vecDir;

		vecTraceDir.x += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.y += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.z += RANDOM_FLOAT( -flNoise, flNoise );

		UTIL_TraceLine( ptr->vecEndPos, ptr->vecEndPos + vecTraceDir * 172.0f, ignore_monsters, ENT( pev ), &Bloodtr );

/*
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_SHOWLINE);
			WRITE_VECTOR( ptr->vecEndPos );

			WRITE_VECTOR( Bloodtr.vecEndPos );
		MESSAGE_END();
*/

		if( Bloodtr.flFraction != 1.0f )
		{
			UTIL_BloodDecalTrace( &Bloodtr, BloodColor() );
		}
	}
}
