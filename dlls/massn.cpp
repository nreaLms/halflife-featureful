#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"schedule.h"
#include	"animation.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"
#include	"decals.h"
#include	"gamerules.h"
#include	"hgrunt.h"
#include	"mod_features.h"

#if FEATURE_MASSN

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	MASSN_CLIP_SIZE				36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!

// Weapon flags
#define MASSN_9MMAR					(1 << 0)
#define MASSN_HANDGRENADE			(1 << 1)
#define MASSN_GRENADELAUNCHER		(1 << 2)
#define MASSN_SNIPERRIFLE			(1 << 3)

// Body groups.
#define MASSN_HEAD_GROUP					1
#define MASSN_GUN_GROUP					2

// Head values
enum
{
	MASSN_HEAD_WHITE,
	MASSN_HEAD_BLACK,
	MASSN_HEAD_GOOGLES,
	MASSN_HEAD_COUNT,
};

// Gun values
#define MASSN_GUN_MP5				0
#define MASSN_GUN_SNIPERRIFLE				1
#define MASSN_GUN_NONE					2

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		MASSN_AE_KICK			( 3 )
#define		MASSN_AE_BURST1			( 4 )
#define		MASSN_AE_CAUGHT_ENEMY	( 10 ) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define		MASSN_AE_DROP_GUN		( 11 ) // grunt (probably dead) is dropping his mp5.

//=========================================================
// Purpose:
//=========================================================
class CMassn : public CHGrunt
{
public:
	void KeyValue(KeyValueData* pkvd);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	BOOL CheckRangeAttack2(float flDot, float flDist);
	void Sniperrifle(void);
	void GibMonster();

	BOOL FOkToSpeak(void);

	void Spawn( void );
	void Precache( void );

	void DeathSound(void);
	void PainSound(void);
	void IdleSound(void);

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	void DropMyItems(BOOL isGibbed);

	int head;
};

LINK_ENTITY_TO_CLASS(monster_male_assassin, CMassn)

//=========================================================
// Purpose:
//=========================================================
BOOL CMassn::FOkToSpeak(void)
{
	return FALSE;
}

//=========================================================
// Purpose:
//=========================================================
void CMassn::IdleSound(void)
{
}

//=========================================================
// Shoot
//=========================================================
void CMassn::Sniperrifle(void)
{
	if (m_hEnemy == 0)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_1DEGREES, 2048, BULLET_MONSTER_762, 0);

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CMassn::GibMonster( void )
{
	if( GetBodygroup( MASSN_GUN_GROUP ) != MASSN_GUN_NONE )
	{
		DropMyItems(TRUE);
	}

	CBaseMonster::GibMonster();
}

void CMassn::DropMyItems(BOOL isGibbed)
{
	Vector vecGunPos;
	Vector vecGunAngles;
	GetAttachment( 0, vecGunPos, vecGunAngles );

	if (!isGibbed) {
		SetBodygroup( MASSN_GUN_GROUP, MASSN_GUN_NONE );
	}

	if( FBitSet( pev->weapons, MASSN_SNIPERRIFLE ) ) {
		DropMyItem( "weapon_sniperrifle", vecGunPos, vecGunAngles, isGibbed );
	} else if ( FBitSet( pev->weapons, MASSN_9MMAR ) ) {
		DropMyItem( "weapon_9mmAR", vecGunPos, vecGunAngles, isGibbed );
	}
	if( FBitSet( pev->weapons, MASSN_GRENADELAUNCHER ) ) {
		DropMyItem( "ammo_ARgrenades", isGibbed ? vecGunPos : BodyTarget( pev->origin ), vecGunAngles, isGibbed );
	}
	pev->weapons = 0;
}

void CMassn::KeyValue(KeyValueData *pkvd)
{
	if( FStrEq(pkvd->szKeyName, "head" ) )
	{
		head = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CSquadMonster::KeyValue( pkvd );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMassn::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case MASSN_AE_DROP_GUN:
	{
		DropMyItems(FALSE);
	}
	break;


	case MASSN_AE_BURST1:
	{
		if (FBitSet(pev->weapons, MASSN_9MMAR))
		{
			Shoot();
			PlayFirstBurstSounds();
		}
		else if (FBitSet(pev->weapons, MASSN_SNIPERRIFLE))
		{
			Sniperrifle();
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sniper_fire.wav", 1, ATTN_NORM);
		}

		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
	}
	break;

	case MASSN_AE_KICK:
	{
		KickImpl(gSkillData.massnDmgKick);
	}
	break;

	case MASSN_AE_CAUGHT_ENEMY:
		break;

	default:
		CHGrunt::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack.
//=========================================================
BOOL CMassn::CheckRangeAttack2( float flDot, float flDist )
{
	if( !FBitSet( pev->weapons, ( MASSN_HANDGRENADE | MASSN_GRENADELAUNCHER ) ) )
	{
		return FALSE;
	}
	return CheckRangeAttack2Impl(gSkillData.massnGrenadeSpeed, flDot, flDist);
}

//=========================================================
// Spawn
//=========================================================
void CMassn::Spawn()
{
	SpawnHelper("models/massn.mdl", gSkillData.massnHealth);

	if (pev->weapons == 0)
	{
		// initialize to original values
		pev->weapons = MASSN_9MMAR | MASSN_HANDGRENADE;
	}

	if (FBitSet(pev->weapons, MASSN_SNIPERRIFLE))
	{
		SetBodygroup(MASSN_GUN_GROUP, MASSN_GUN_SNIPERRIFLE);
		m_cClipSize = 1;
	}
	else
	{
		m_cClipSize = MASSN_CLIP_SIZE;
	}
	m_cAmmoLoaded = m_cClipSize;

	if (head == -1 || head >= MASSN_HEAD_COUNT) {
		head = RANDOM_LONG(MASSN_HEAD_WHITE, MASSN_HEAD_BLACK); // never random night googles
	}
	SetBodygroup(MASSN_HEAD_GROUP, head);

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMassn::Precache()
{
	PrecacheHelper("models/massn.mdl");

	PRECACHE_SOUND("weapons/sniper_fire.wav");

	m_iBrassShell = PRECACHE_MODEL("models/shell.mdl");// brass shell
}

//=========================================================
// PainSound
//=========================================================
void CMassn::PainSound(void)
{
}

//=========================================================
// DeathSound
//=========================================================
void CMassn::DeathSound(void)
{
}

//=========================================================
// TraceAttack - reimplemented in male assassin because they never have helmets
//=========================================================
void CMassn::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CSquadMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

//=========================================================
// CAssassinRepel - when triggered, spawns a monster_male_assassin
// repelling down a line.
//=========================================================

class CAssassinRepel : public CHGruntRepel
{
public:
	void KeyValue(KeyValueData* pkvd);
	const char* TrooperName() {
		return "monster_male_assassin";
	}
	void PrepareBeforeSpawn(CBaseEntity* pEntity);

	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	int head;
};

LINK_ENTITY_TO_CLASS(monster_assassin_repel, CAssassinRepel)

TYPEDESCRIPTION	CAssassinRepel::m_SaveData[] =
{
	DEFINE_FIELD( CAssassinRepel, head, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CAssassinRepel, CHGruntRepel )

void CAssassinRepel::KeyValue(KeyValueData *pkvd)
{
	if( FStrEq(pkvd->szKeyName, "head" ) )
	{
		head = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CHGruntRepel::KeyValue( pkvd );
}

void CAssassinRepel::PrepareBeforeSpawn(CBaseEntity *pEntity)
{
	CMassn* massn = (CMassn*)pEntity;
	massn->head = head;
}

class CDeadMassn : public CDeadMonster
{
public:
	void Spawn( void );
	int	DefaultClassify ( void ) { return	CLASS_HUMAN_MILITARY; }

	void KeyValue( KeyValueData *pkvd );
	const char* getPos(int pos) const;

	int	m_iHead;
	static const char *m_szPoses[3];
};

const char *CDeadMassn::m_szPoses[] = { "deadstomach", "deadside", "deadsitting" };

const char* CDeadMassn::getPos(int pos) const
{
	return m_szPoses[pos % ARRAYSIZE(m_szPoses)];
}

void CDeadMassn::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "head"))
	{
		m_iHead = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CDeadMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_male_assassin_dead, CDeadMassn )

void CDeadMassn::Spawn( )
{
	SpawnHelper("models/massn.mdl");

	if ( pev->weapons <= 0 )
	{
		SetBodygroup( MASSN_GUN_GROUP, MASSN_GUN_NONE );
	}
	if (FBitSet( pev->weapons, MASSN_9MMAR ))
	{
		SetBodygroup(MASSN_GUN_GROUP, MASSN_GUN_MP5);
	}
	if (FBitSet(pev->weapons, MASSN_SNIPERRIFLE))
	{
		SetBodygroup(MASSN_GUN_GROUP, MASSN_GUN_SNIPERRIFLE);
	}

	if ( m_iHead < 0 || m_iHead >= MASSN_HEAD_COUNT ) {
		m_iHead = RANDOM_LONG(MASSN_HEAD_WHITE, MASSN_HEAD_BLACK);  // never random night googles
	}

	SetBodygroup( MASSN_HEAD_GROUP, m_iHead );

	MonsterInitDead();
}

#endif
