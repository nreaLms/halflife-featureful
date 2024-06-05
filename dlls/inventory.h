#pragma once
#ifndef INVENTORY_H
#define INVENTORY_H

struct InventoryItemSpec
{
	InventoryItemSpec();
	char itemName[24];
	int maxCount;
};

void ReadInventorySpec();

const InventoryItemSpec* GetInventoryItemSpec(const char* itemName);

#endif
