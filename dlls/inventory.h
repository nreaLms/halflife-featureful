#pragma once
#ifndef INVENTORY_H
#define INVENTORY_H

#include <string>

struct InventoryItemSpec
{
	InventoryItemSpec();
	std::string itemName;
	int maxCount;
};

void ReadInventorySpec();

const InventoryItemSpec* GetInventoryItemSpec(const char* itemName);

#endif
