//=============================================================================
// info_camdist.cpp
//
// TMOD: server entity that changes the third person camera's target
// distance and blend speed for the player while inside a mapper-defined
// zone. Not solid by itself: a trigger volume (e.g. trigger_multiple)
// should target this entity's name and call Use() on it.
//
// Hammer keyvalues:
//   cam_dist    - target camera distance for the zone (default 100)
//   blend_speed - how fast the camera distance blends to cam_dist (default 2)
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"

extern int gmsgCamZone;

class CInfoCamDist: public CBaseEntity
{
public:
    void    Spawn()   override;
    void    Use(CBaseEntity *pActivator, CBaseEntity *pCaller,
                USE_TYPE useType, float value) override;
    void    KeyValue(KeyValueData *pkvd) override;

    virtual int Save(CSave &save);
    virtual int Restore(CRestore &restore);
    static  TYPEDESCRIPTION m_SaveData[];

    float m_flCamDist;
    float m_flBlendSpeed;
};

TYPEDESCRIPTION CInfoCamDist::m_SaveData[] =
{
    DEFINE_FIELD(CInfoCamDist, m_flCamDist,  FIELD_FLOAT),
    DEFINE_FIELD(CInfoCamDist, m_flBlendSpeed, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CInfoCamDist, CBaseEntity);
LINK_ENTITY_TO_CLASS(info_camdist, CInfoCamDist);

void CInfoCamDist::KeyValue(KeyValueData *pkvd)
{
    if(FStrEq(pkvd->szKeyName, "cam_dist"))
    {
        m_flCamDist = atof(pkvd->szValue);
        pkvd->fHandled = true;
    }
    else if(FStrEq(pkvd->szKeyName, "blend_speed"))
    {
        m_flBlendSpeed = atof(pkvd->szValue);
        pkvd->fHandled = true;
    }
    else
    {
        CBaseEntity::KeyValue(pkvd);
    }
}

void CInfoCamDist::Spawn()
{
    pev->solid = SOLID_NOT;
    pev->movetype = MOVETYPE_NONE;

    if(m_flCamDist == 0.0f) m_flCamDist = 100.0f;
    if(m_flBlendSpeed == 0.0f) m_flBlendSpeed = 2.0f;
}

void CInfoCamDist::Use(CBaseEntity *pActivator, CBaseEntity *pCaller,
                         USE_TYPE useType, float value)
{
    if(!pActivator || !pActivator->IsPlayer())
        return;

    CBasePlayer *pPlayer = (CBasePlayer *)pActivator;

    // WRITE_FLOAT doesn't exist server-side, so floats are sent as LONGs
    float heightToSend = (m_flCamDist == 0.0f) ? -1.0f : m_flCamDist;

    MESSAGE_BEGIN(MSG_ONE, gmsgCamZone, nullptr, pPlayer->edict());
    WRITE_LONG(*reinterpret_cast<int *>(&heightToSend));
    WRITE_LONG(*reinterpret_cast<int *>(&m_flBlendSpeed));
    MESSAGE_END();
}
