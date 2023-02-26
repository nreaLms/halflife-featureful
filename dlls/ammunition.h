#pragma once
#ifndef AMMUNITION_H
#define AMMUNITION_H

#include "items.h"

class CBasePlayerAmmo : public CPickup
{
public:
	virtual void Spawn( void );
	void Precache();
	void EXPORT DefaultTouch( CBaseEntity *pOther ); // default weapon touch
	virtual BOOL AddAmmo( CBaseEntity *pOther );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void TouchOrUse( CBaseEntity* other );

	virtual const char* MyModel() = 0;
	virtual int MyAmount() = 0;
	virtual const char* AmmoName() = 0;

	Vector MyRespawnSpot();
	virtual float MyRespawnTime();
	void OnMaterialize();
};

#endif
