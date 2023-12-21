#pragma once
#ifndef AMMUNITION_H
#define AMMUNITION_H

#include "items.h"

class CBasePlayerAmmo : public CPickup
{
public:
	virtual void Spawn( void );
	void KeyValue(KeyValueData* pkvd);
	void Precache();
	void EXPORT DefaultTouch( CBaseEntity *pOther ); // default weapon touch
	virtual bool AddAmmo( CBaseEntity *pOther );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void TouchOrUse( CBaseEntity* other );

	virtual const char* MyModel() = 0;
	virtual int DefaultAmount() = 0;
	int MyAmount();
	void SetCustomAmount(int amount);
	virtual const char* AmmoName() = 0;

	Vector MyRespawnSpot();
	virtual float MyRespawnTime();
	void OnMaterialize();

	CBasePlayerAmmo* MyAmmoPointer() {return this;}
};

#endif
