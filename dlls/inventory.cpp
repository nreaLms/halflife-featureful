#include <algorithm>
#include <vector>

#include "inventory.h"

#include "extdll.h"
#include "util.h"
#include "string_utils.h"
#include "json_utils.h"

using namespace rapidjson;

const char inventorySpecSchema[] = R"(
{
  "type": "object",
  "properties": {
    "items": {
      "additionalProperties": {
        "type": "object",
        "properties": {
          "max_count": {
            "type": "integer",
            "minimum": 0
          }
        }
      }
    }
  }
}
)";

InventoryItemSpec::InventoryItemSpec(): maxCount(0) {}

class InventorySpec
{
public:
	bool ReadFromFile(const char* fileName);
	const InventoryItemSpec* GetInventoryItemSpec(const char* itemName);
private:
	std::vector<InventoryItemSpec> inventory;
};

bool InventorySpec::ReadFromFile(const char *fileName)
{
	int fileSize;
	char *pMemFile = (char*)g_engfuncs.pfnLoadFileForMe( fileName, &fileSize );
	if (!pMemFile)
		return false;

	ALERT(at_console, "Parsing %s\n", fileName);

	Document document;
	bool success = ReadJsonDocumentWithSchema(document, pMemFile, fileSize, inventorySpecSchema, fileName);
	g_engfuncs.pfnFreeFile(pMemFile);

	if (!success)
		return false;

	auto itemsIt = document.FindMember("items");
	if (itemsIt != document.MemberEnd())
	{
		Value& items = itemsIt->value;
		for (auto itemIt = items.MemberBegin(); itemIt != items.MemberEnd(); ++itemIt)
		{
			InventoryItemSpec item;
			item.itemName = itemIt->name.GetString();
			Value& value = itemIt->value;
			UpdatePropertyFromJson(item.maxCount, value, "max_count");
			inventory.push_back(item);
		}
	}
	std::sort(inventory.begin(), inventory.end(), [](const InventoryItemSpec& a, const InventoryItemSpec& b) {
		return strcmp(a.itemName.c_str(), b.itemName.c_str()) < 0;
	});

	return true;
}

struct InventoryItemCompare
{
	bool operator ()(const InventoryItemSpec& lhs, const char* rhs)
	{
		return strcmp(lhs.itemName.c_str(), rhs) < 0;
	}
	bool operator ()(const char* lhs, const InventoryItemSpec& rhs)
	{
		return strcmp(lhs, rhs.itemName.c_str()) < 0;
	}
};

const InventoryItemSpec* InventorySpec::GetInventoryItemSpec(const char *itemName)
{
	auto result = std::equal_range(inventory.begin(), inventory.end(), itemName, InventoryItemCompare());
	if (result.first != result.second)
		return &(*result.first);
	return nullptr;
}

InventorySpec g_InventorySpec;

void ReadInventorySpec()
{
	g_InventorySpec.ReadFromFile("templates/inventory.json");
}

const InventoryItemSpec* GetInventoryItemSpec(const char* itemName)
{
	return g_InventorySpec.GetInventoryItemSpec(itemName);
}
