//=============================================================================
// env_camera_fixed.cpp
//
// TMOD: server entity that forces a fixed camera on the player while they
// are inside an associated trigger_inout volume.
//
// Hammer wiring:
//   trigger_inout
//     target          -> env_camera_fixed   (USE_TOGGLE on enter)
//     m_iszAltTarget  -> env_camera_fixed   (USE_TOGGLE on exit)
//
// The entity tracks its own ON/OFF state through m_bActive and USE_TOGGLE.
//
// Hammer keyvalues:
//   pitch       -> X angle (ignored if look_at is set)
//   yaw         -> Y angle (ignored if look_at is set)
//   roll        -> Z angle
//   blend_speed -> transition speed (minimum 10)
//   look_at     -> targetname of an entity to look at (e.g. info_target);
//                  pitch/yaw are computed automatically when set
//
// The camera position is this entity's origin, as placed in Hammer.
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "saverestore.h"

// Declared and registered in player.cpp
extern int gmsgCamFixed;

class CEnvCameraFixed: public CPointEntity
{
public:
	void Spawn() override;
	void Activate() override;
	void KeyValue(KeyValueData *pkvd) override;
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller,
			 USE_TYPE useType, float value) override;

	void EXPORT ThinkWrapper();

	int  Save(CSave &save) override;
	int  Restore(CRestore &restore) override;
	static TYPEDESCRIPTION m_SaveData[];

private:
	void SendCameraMsg(CBasePlayer *pPlayer, bool bEnable);

	float m_flPitch;       // X angle
	float m_flYaw;         // Y angle
	float m_flRoll;        // Z angle
	float m_flBlendSpeed;  // transition duration
	string_t m_iszLookAt;
	bool  m_bActive;       // current state, for USE_TOGGLE handling
};

LINK_ENTITY_TO_CLASS(env_camera_fixed, CEnvCameraFixed)

TYPEDESCRIPTION CEnvCameraFixed::m_SaveData[] =
{
	DEFINE_FIELD(CEnvCameraFixed, m_flPitch,     FIELD_FLOAT),
	DEFINE_FIELD(CEnvCameraFixed, m_flYaw,       FIELD_FLOAT),
	DEFINE_FIELD(CEnvCameraFixed, m_flRoll,      FIELD_FLOAT),
	DEFINE_FIELD(CEnvCameraFixed, m_flBlendSpeed, FIELD_FLOAT),
	DEFINE_FIELD(CEnvCameraFixed, m_iszLookAt,    FIELD_STRING),
	DEFINE_FIELD(CEnvCameraFixed, m_bActive,     FIELD_BOOLEAN),
};
IMPLEMENT_SAVERESTORE(CEnvCameraFixed, CPointEntity)

void CEnvCameraFixed::Spawn()
{
	if(m_flBlendSpeed < 10.0f)
		m_flBlendSpeed = 10.0f;
	m_bActive = false;
}

void CEnvCameraFixed::Activate()
{
	CPointEntity::Activate();

	// Resolve the look_at target once all entities have spawned
	if(!FStringNull(m_iszLookAt))
	{
		CBaseEntity *pTarget = UTIL_FindEntityByTargetname(NULL, STRING(m_iszLookAt));
		if(pTarget)
		{
			Vector dir = pTarget->pev->origin - pev->origin;
			Vector angles = UTIL_VecToAngles(dir);
			m_flPitch = -angles.x;  // inverted, like trigger_camera
			m_flYaw = angles.y;
		}
	}
}

void CEnvCameraFixed::KeyValue(KeyValueData *pkvd)
{
	if(FStrEq(pkvd->szKeyName, "pitch"))
	{
		m_flPitch = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if(FStrEq(pkvd->szKeyName, "yaw"))
	{
		m_flYaw = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if(FStrEq(pkvd->szKeyName, "roll"))
	{
		m_flRoll = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if(FStrEq(pkvd->szKeyName, "blend_speed"))
	{
		m_flBlendSpeed = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if(FStrEq(pkvd->szKeyName, "look_at"))
	{
		m_iszLookAt = ALLOC_STRING(pkvd->szValue); pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

void CEnvCameraFixed::Use(CBaseEntity *pActivator, CBaseEntity *pCaller,
						  USE_TYPE useType, float value)
{
	CBasePlayer *pPlayer = NULL;
	if(pActivator && pActivator->IsPlayer())
		pPlayer = static_cast<CBasePlayer *>(pActivator);
	else
		pPlayer = static_cast<CBasePlayer *>(UTIL_FindEntityByClassname(NULL, "player"));

	if(!pPlayer)
		return;

	// trigger_inout always sends USE_TOGGLE, so we track the state ourselves
	bool bEnable;
	if(useType == USE_ON)
		bEnable = true;
	else if(useType == USE_OFF)
		bEnable = false;
	else // USE_TOGGLE
		bEnable = !m_bActive;

	if(bEnable)
	{
		SetThink(&CEnvCameraFixed::ThinkWrapper);
		pev->nextthink = gpGlobals->time;
	}
	else
	{
		SetThink(NULL);
		pev->nextthink = -1;
		SendCameraMsg(pPlayer, false);
	}

	m_bActive = bEnable;
	SendCameraMsg(pPlayer, bEnable);
}

void EXPORT CEnvCameraFixed::ThinkWrapper()
{
	if(!m_bActive)
	{
		SetThink(NULL);
		pev->nextthink = -1;
		return;
	}

	CBasePlayer *pPlayer = static_cast<CBasePlayer *>(
		UTIL_FindEntityByClassname(NULL, "player"));
	if(pPlayer)
		SendCameraMsg(pPlayer, true);

	pev->nextthink = gpGlobals->time;
}

void CEnvCameraFixed::SendCameraMsg(CBasePlayer *pPlayer, bool bEnable)
{
	// Camera position = this entity's origin as placed in Hammer
	float posX = pev->origin.x;
	float posY = pev->origin.y;
	float posZ = pev->origin.z;

	// Floats are sent as LONGs via reinterpret_cast, same pattern as
	// info_camdist / gmsgCamZone.
	MESSAGE_BEGIN(MSG_ONE, gmsgCamFixed, NULL, pPlayer->edict());
	WRITE_BYTE(bEnable ? 1 : 0);
	WRITE_LONG(*reinterpret_cast<int *>(&posX));
	WRITE_LONG(*reinterpret_cast<int *>(&posY));
	WRITE_LONG(*reinterpret_cast<int *>(&posZ));
	WRITE_LONG(*reinterpret_cast<int *>(&m_flPitch));
	WRITE_LONG(*reinterpret_cast<int *>(&m_flYaw));
	WRITE_LONG(*reinterpret_cast<int *>(&m_flRoll));
	WRITE_LONG(*reinterpret_cast<int *>(&m_flBlendSpeed));
	MESSAGE_END();
}
