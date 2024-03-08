#ifndef AMMOREGISTRY_H
#define AMMOREGISTRY_H

#include "cdll_dll.h"

struct AmmoType
{
	int id;
#if CLIENT_DLL
	char name[32];
#else
	const char* name;
#endif
	void SetName(const char* ammoName);
	bool IsValid() const;

	int maxAmmo;
	bool exhaustible;
};

class AmmoRegistry
{
public:
	AmmoRegistry() : lastAmmoIndex(0) {}
	int Register(const char* name, int maxAmmo, bool exhaustible = false);
	void RegisterOnClient(const char* name, int maxAmmo, int index, bool exhaustible);
	const AmmoType *GetByName(const char* name) const;
	const AmmoType *GetByIndex(int id) const;
	int IndexOf(const char* name) const;
	int GetMaxAmmo(const char* name) const;
	int GetMaxAmmo(int index) const;
private:
	void ReportRegisteredType(const AmmoType &ammoType);

	AmmoType ammoTypes[MAX_AMMO_TYPES-1];

	int lastAmmoIndex;
};

extern AmmoRegistry g_AmmoRegistry;

#endif // AMMOREGISTRY_H
