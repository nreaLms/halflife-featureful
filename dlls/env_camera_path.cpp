//=============================================================================
// env_camera_path.cpp
//
// TMOD: camera that positions itself along a path_track chain based on the
// player's position (projection onto the closest segment).
// Bidirectional: works when the player travels the path in either direction.
//
// Hammer wiring:
//   trigger_inout
//     target         -> env_camera_path  (USE_TOGGLE on enter)
//     m_iszAltTarget -> env_camera_path  (USE_TOGGLE on exit)
//
// Keyvalues:
//   firstnode   - targetname of the first path_track of the chain
//   blend_speed - transition speed (minimum 10)
//   look_at     - targetname of an entity to look at (e.g. info_target).
//                 When set, every path_track's angles are ignored and the
//                 camera always looks at this entity instead.
//
// Angles are read from each path_track's "angles" field and linearly
// interpolated between nodes, unless look_at overrides them.
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "trains.h"      // CPathTrack
#include "saverestore.h"

extern int gmsgCamFixed;

class CEnvCameraPath: public CPointEntity
{
public:
	void Spawn() override;
	void Activate() override;
	void KeyValue(KeyValueData *pkvd) override;
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller,
			 USE_TYPE useType, float value) override;

	void EXPORT ThinkWrapper();
	void DoThink();

	int  Save(CSave &save) override;
	int  Restore(CRestore &restore) override;
	static TYPEDESCRIPTION m_SaveData[];

private:
	// Projects the player onto segment [A, B].
	// Returns t in [0,1] and the projected position.
	float ProjectOnSegment(const Vector &playerPos,
						   const Vector &a, const Vector &b,
						   Vector &outPos) const;

	// Walks every segment of the path and returns the closest global
	// projection point, along with the surrounding nodes A and B.
	bool FindClosestProjection(const Vector &playerPos,
							   CPathTrack *pFirst,
							   Vector &outPos,
							   Vector &outAngles) const;

	void SendCameraMsg(CBasePlayer *pPlayer, bool bEnable,
					   const Vector &pos, const Vector &ang);

	string_t     m_iszFirstNode;
	float        m_flBlendSpeed;
	string_t     m_iszLookAt;
	bool         m_bActive;
	EHANDLE      m_hPlayer;       // player currently being followed
	EHANDLE      m_hLookAt;       // look_at entity, resolved in Activate()
};

LINK_ENTITY_TO_CLASS(env_camera_path, CEnvCameraPath)

TYPEDESCRIPTION CEnvCameraPath::m_SaveData[] =
{
	DEFINE_FIELD(CEnvCameraPath, m_iszFirstNode,  FIELD_STRING),
	DEFINE_FIELD(CEnvCameraPath, m_flBlendSpeed,  FIELD_FLOAT),
	DEFINE_FIELD(CEnvCameraPath, m_iszLookAt,     FIELD_STRING),
	DEFINE_FIELD(CEnvCameraPath, m_bActive,       FIELD_BOOLEAN),
	DEFINE_FIELD(CEnvCameraPath, m_hPlayer,       FIELD_EHANDLE),
	DEFINE_FIELD(CEnvCameraPath, m_hLookAt,       FIELD_EHANDLE),
};
IMPLEMENT_SAVERESTORE(CEnvCameraPath, CPointEntity)

void CEnvCameraPath::Spawn()
{
	if(m_flBlendSpeed < 10.0f)
		m_flBlendSpeed = 10.0f;
	m_bActive = false;
	SetThink(NULL);
}

void CEnvCameraPath::Activate()
{
	CPointEntity::Activate();

	// Resolve the look_at entity once all entities have spawned
	if(!FStringNull(m_iszLookAt))
	{
		CBaseEntity *pTarget = UTIL_FindEntityByTargetname(NULL, STRING(m_iszLookAt));
		if(pTarget)
			m_hLookAt = pTarget;
	}
}

void CEnvCameraPath::KeyValue(KeyValueData *pkvd)
{
	if(FStrEq(pkvd->szKeyName, "firstnode"))
	{
		m_iszFirstNode = ALLOC_STRING(pkvd->szValue); pkvd->fHandled = TRUE;
	}
	else if(FStrEq(pkvd->szKeyName, "blend_speed"))
	{
		m_flBlendSpeed = atof(pkvd->szValue); pkvd->fHandled = TRUE;
	}
	else if(FStrEq(pkvd->szKeyName, "look_at"))
	{
		m_iszLookAt = ALLOC_STRING(pkvd->szValue); pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

void CEnvCameraPath::Use(CBaseEntity *pActivator, CBaseEntity *pCaller,
						 USE_TYPE useType, float value)
{
	CBasePlayer *pPlayer = NULL;
	if(pActivator && pActivator->IsPlayer())
		pPlayer = static_cast<CBasePlayer *>(pActivator);
	else
		pPlayer = static_cast<CBasePlayer *>(UTIL_FindEntityByClassname(NULL, "player"));

	if(!pPlayer) return;

	bool bEnable;
	if(useType == USE_ON)  bEnable = true;
	else if(useType == USE_OFF) bEnable = false;
	else                         bEnable = !m_bActive;

	if(bEnable == m_bActive) return;

	m_bActive = bEnable;

	if(bEnable)
	{
		m_hPlayer = pPlayer;
		SetThink(&CEnvCameraPath::ThinkWrapper);
		pev->nextthink = gpGlobals->time;
	}
	else
	{
		// Disabling: send a disable message with a dummy position (the
		// client handles the return blend on its own).
		Vector dummyPos(0, 0, 0);
		Vector dummyAng(0, 0, 0);
		SendCameraMsg(pPlayer, false, dummyPos, dummyAng);
		SetThink(NULL);
		pev->nextthink = -1;
		m_hPlayer = NULL;
	}
}

void EXPORT CEnvCameraPath::ThinkWrapper()
{
	if(!m_bActive)
	{
		SetThink(NULL);
		return;
	}
	DoThink();
}

void CEnvCameraPath::DoThink()
{
	if(!m_bActive)
	{
		SetThink(NULL);
		return;
	}

	CBasePlayer *pPlayer = static_cast<CBasePlayer *>((CBaseEntity *)m_hPlayer);
	if(!pPlayer || !pPlayer->IsPlayer())
	{
		m_bActive = false;
		SetThink(NULL);
		return;
	}

	edict_t *pEdict = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(m_iszFirstNode));
	if(FNullEnt(pEdict))
	{
		pev->nextthink = gpGlobals->time + 0.1f;
		return;
	}
	CPathTrack *pFirst = CPathTrack::Instance(pEdict);
	if(!pFirst)
	{
		pev->nextthink = gpGlobals->time + 0.1f;
		return;
	}

	Vector camPos, camAngles;
	if(FindClosestProjection(pPlayer->pev->origin, pFirst, camPos, camAngles))
	{
		// If look_at is set, override the interpolated angles with a look
		// vector toward the target instead.
		CBaseEntity *pLookAt = m_hLookAt;
		if(pLookAt)
		{
			Vector dir = pLookAt->pev->origin - camPos;
			Vector angles = UTIL_VecToAngles(dir);
			camAngles.x = -angles.x;  // inverted pitch, like trigger_camera
			camAngles.y = angles.y;
			camAngles.z = 0.0f;
		}

		SendCameraMsg(pPlayer, true, camPos, camAngles);
	}

	pev->nextthink = gpGlobals->time;
}

float CEnvCameraPath::ProjectOnSegment(const Vector &playerPos,
									   const Vector &a, const Vector &b,
									   Vector &outPos) const
{
	Vector ab = b - a;
	float len2 = DotProduct(ab, ab);

	if(len2 < 0.0001f)
	{
		outPos = a;
		return 0.0f;
	}

	float t = DotProduct(playerPos - a, ab) / len2;
	if(t < 0.0f) t = 0.0f;
	if(t > 1.0f) t = 1.0f;

	outPos = a + ab * t;
	return t;
}

bool CEnvCameraPath::FindClosestProjection(const Vector &playerPos,
										   CPathTrack *pFirst,
										   Vector &outPos,
										   Vector &outAngles) const
{
	float    bestDist = -1.0f;
	Vector   bestPos(0, 0, 0);
	Vector   bestAngA(0, 0, 0);
	Vector   bestAngB(0, 0, 0);
	float    bestT = 0.0f;

	// Safety net against infinite loops on malformed/cyclic chains
	int      deadCount = 0;

	CPathTrack *pNode = pFirst;

	while(pNode && deadCount < 512)
	{
		deadCount++;

		CPathTrack *pNext = pNode->GetNext();
		if(!pNext)
			break;  // end of the chain

		Vector projPos;
		float t = ProjectOnSegment(playerPos,
								   pNode->pev->origin,
								   pNext->pev->origin,
								   projPos);

		float dist = (playerPos - projPos).Length();

		if(bestDist < 0.0f || dist < bestDist)
		{
			bestDist = dist;
			bestPos = projPos;
			bestT = t;
			bestAngA = pNode->pev->angles;
			bestAngB = pNext->pev->angles;
		}

		pNode = pNext;
		if(pNode == pFirst)
			break;  // avoid looping forever on a closed circuit
	}

	if(bestDist < 0.0f)
		return false;

	// Interpolate angles between the two surrounding nodes
	for(int i = 0; i < 3; i++)
	{
		float a = bestAngA[i];
		float b = bestAngB[i];

		// Normalize to avoid angle jumps (e.g. 350 deg -> 10 deg)
		float d = b - a;
		if(d > 180.0f) d -= 360.0f;
		if(d < -180.0f) d += 360.0f;

		outAngles[i] = a + d * bestT;
	}

	outPos = bestPos;
	return true;
}

void CEnvCameraPath::SendCameraMsg(CBasePlayer *pPlayer, bool bEnable,
								   const Vector &pos, const Vector &ang)
{
	float posX = pos.x;
	float posY = pos.y;
	float posZ = pos.z;
	float pitch = ang.x;
	float yaw = ang.y;
	float roll = ang.z;

	MESSAGE_BEGIN(MSG_ONE, gmsgCamFixed, NULL, pPlayer->edict());
	WRITE_BYTE(bEnable ? 1 : 0);
	WRITE_LONG(*reinterpret_cast<int *>(&posX));
	WRITE_LONG(*reinterpret_cast<int *>(&posY));
	WRITE_LONG(*reinterpret_cast<int *>(&posZ));
	WRITE_LONG(*reinterpret_cast<int *>(&pitch));
	WRITE_LONG(*reinterpret_cast<int *>(&yaw));
	WRITE_LONG(*reinterpret_cast<int *>(&roll));
	WRITE_LONG(*reinterpret_cast<int *>(&m_flBlendSpeed));
	MESSAGE_END();
}
