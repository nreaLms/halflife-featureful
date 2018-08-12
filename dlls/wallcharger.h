#pragma once
#ifndef WALLCHARGER_H
#define WALLCHARGER_H

class CWallCharger : public CBaseToggle
{
public:
	void Spawn();
	void Precache( void );
	void EXPORT Off( void );
	void EXPORT Recharge( void );
	virtual const char* LoopingSound() = 0;
	virtual int RechargeTime() = 0;
	virtual const char* RechargeSound() = 0;
	virtual int ChargerCapacity() = 0;
	virtual const char* DenySound() = 0;
	virtual const char* ChargeStartSound() = 0;
	virtual float SoundVolume() = 0;
	virtual bool GiveCharge(CBaseEntity* pActivator) = 0;

	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int ObjectCaps( void );
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	float m_flNextCharge;
	int m_iReactivate ; // DeathMatch Delay until reactvated
	int m_iJuice;
	int m_iOn;			// 0 = off, 1 = startup, 2 = going
	float m_flSoundTime;
	string_t m_triggerOnFirstUse;
	string_t m_triggerOnEmpty;
	string_t m_triggerOnRecharged;
};
#endif
