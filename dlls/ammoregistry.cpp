#include <cstring>

#include "ammoregistry.h"

#if CLIENT_DLL
#include "cl_dll.h"
#define AMMO_LOG gEngfuncs.Con_DPrintf
#define AMMO_ERROR gEngfuncs.Con_Printf
#else
#include "util.h"
#define AMMO_LOG(...) ALERT(at_aiconsole, ##__VA_ARGS__ )
#define AMMO_ERROR(...) ALERT(at_error, ##__VA_ARGS__ )
#endif
#include "arraysize.h"


void AmmoType::SetName(const char *ammoName)
{
#if CLIENT_DLL
	strncpy(name, ammoName, sizeof(name));
	name[sizeof(name)-1] = '\0';
#else
	name = ammoName;
#endif
}

bool AmmoType::IsValid() const
{
	if (!name || *name == '\0')
		return false;
	if (id <= 0)
		return false;
	return true;
}

int AmmoRegistry::Register(const char *name, int maxAmmo, bool exhaustible)
{
	if (!name)
		return -1;
	// make sure it's not already in the registry
	const int index = IndexOf(name);
	if (index != -1)
	{
		const AmmoType* type = GetByIndex(index);
		if (type->maxAmmo != maxAmmo || type->exhaustible != exhaustible)
		{
			AMMO_ERROR("Trying to re-register ammo '%s' with different parameters\n", name);
		}
		return index;
	}

	if (lastAmmoIndex >= MAX_AMMO_TYPES - 1)
	{
		AMMO_ERROR("Too many ammo types. Max is %d\n", MAX_AMMO_TYPES-1);
		return -1;
	}

	if (maxAmmo <= 0)
	{
		AMMO_ERROR("Invalid max ammo (%d) for '%s'\n", maxAmmo, name);
		return -1;
	}

	AmmoType& type = ammoTypes[lastAmmoIndex++];
	type.id = lastAmmoIndex;
	type.SetName(name);
	type.maxAmmo = maxAmmo;
	type.exhaustible = exhaustible;

	return type.id;
}

void AmmoRegistry::RegisterOnClient(const char *name, int maxAmmo, int index, bool exhaustible)
{
	if (!name)
		return;
	if (index <= 0 || index >= MAX_AMMO_TYPES)
	{
		AMMO_ERROR("Invalid ammo index %d\n", index);
		return;
	}
	AmmoType& type = ammoTypes[index - 1];
	type.SetName(name);
	type.maxAmmo = maxAmmo;
	type.id = index;
	type.exhaustible = exhaustible;

	ReportRegisteredType(type);
}

const AmmoType* AmmoRegistry::GetByName(const char *name) const
{
	return GetByIndex(IndexOf(name));
}

const AmmoType* AmmoRegistry::GetByIndex(int id) const
{
	if (id > 0 && id <= MAX_AMMO_TYPES)
	{
		const AmmoType& ammoType = ammoTypes[id-1];
		if (ammoType.IsValid())
			return &ammoType;
	}
	return NULL;
}

int AmmoRegistry::IndexOf(const char *name) const
{
	if (!name)
		return -1;
	for (int i = 0; i<ARRAYSIZE(ammoTypes); ++i)
	{
		if (!ammoTypes[i].IsValid())
			continue;
		if (stricmp(name, ammoTypes[i].name) == 0)
		{
			return i+1;
		}
	}
	return -1;
}

int AmmoRegistry::GetMaxAmmo(const char *name) const
{
	const AmmoType* ammoType = GetByName(name);
	if (ammoType)
		return ammoType->maxAmmo;
	return -1;
}

int AmmoRegistry::GetMaxAmmo(int index) const
{
	const AmmoType* ammoType = GetByIndex(index);
	if (ammoType)
		return ammoType->maxAmmo;
	return -1;
}

void AmmoRegistry::SetMaxAmmo(const char *name, int maxAmmo)
{
	int id = IndexOf(name);
	if (id > 0 && id <= MAX_AMMO_TYPES)
	{
		AmmoType& ammoType = ammoTypes[id-1];
		if (ammoType.IsValid())
		{
			ammoType.maxAmmo = maxAmmo;
		}
	}
}

void AmmoRegistry::ReportRegisteredTypes()
{
	for (int i = 0; i<lastAmmoIndex; ++i)
	{
		ReportRegisteredType(ammoTypes[i]);
	}
}

void AmmoRegistry::ReportRegisteredType(const AmmoType& ammoType)
{
	AMMO_LOG("Registered ammo type: '%s' (max: %d, index: %d)\n", ammoType.name, ammoType.maxAmmo, ammoType.id);
}

AmmoRegistry g_AmmoRegistry;
