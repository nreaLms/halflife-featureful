#pragma once
#ifndef MAPCONFIG_H
#define MAPCONFIG_H

#include "extdll.h"

enum SuitLogon
{
	SuitNoLogon = 0,
	SuitShortLogon,
	SuitLongLogon
};

#define MAPCONFIG_ENTRY_LENGTH 32
#define MAPCONFIG_MAX_OVERRIDE_CVARS 32
#define MAPCONFIG_MAX_PICKUP_ENTS 64

struct PickupEnt
{
	PickupEnt();
	string_t entName;
	short count;
};

struct AmmoQuantity
{
	char name[MAPCONFIG_ENTRY_LENGTH];
	short count;
};

struct OverrideCvar
{
	char name[MAPCONFIG_ENTRY_LENGTH];
	char value[MAPCONFIG_ENTRY_LENGTH];
};

struct MapConfig
{
	MapConfig();
	PickupEnt pickupEnts[64];
	int pickupEntCount;

	AmmoQuantity ammo[MAX_AMMO_SLOTS];
	int ammoCount;

	OverrideCvar overrideCvars[MAPCONFIG_MAX_OVERRIDE_CVARS];
	int cvarCount;

	int starthealth;
	int startarmor;

	bool nomedkit; // for co-op

	bool nosuit;
	short suitLogon;
	bool longjump;

	bool valid;
};

const char* FixedAmmoName(const char* ammoName);

bool ReadMapConfigFromText(MapConfig& mapConfig, byte* pMemFile, int fileSize);
bool ReadMapConfigFromFile(MapConfig& mapConfig, const char* fileName);
bool ReadMapConfigByMapName(MapConfig& mapConfig, const char* mapName);

#endif
