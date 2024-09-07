#include "extdll.h"
#include "util.h"
#include "string_utils.h"
#include "json_utils.h"

#include "followers.h"

using namespace rapidjson;

const char followersSchema[] = R"(
{
  "type": "object",
  "definitions": {
    "fast_recruit_monsters": {
      "type": "array",
      "items": {
        "type": "string",
        "pattern": "^monster_.*$"
      },
      "uniqueItems": true
    },
    "fast_recruit_range": {
      "type": "number",
      "exclusiveMinimum": 0
    }
  }
}
)";

bool FollowersDescription::ReadFromFile(const char *fileName)
{
	int fileSize;
	char *pMemFile = (char*)g_engfuncs.pfnLoadFileForMe( fileName, &fileSize );
	if (!pMemFile)
		return false;

	ALERT(at_console, "Parsing %s\n", fileName);

	Document document;
	bool success = ReadJsonDocumentWithSchema(document, pMemFile, fileSize, followersSchema, fileName);
	g_engfuncs.pfnFreeFile(pMemFile);

	if (!success)
		return false;

	auto monstersIt = document.FindMember("fast_recruit_monsters");
	if (monstersIt != document.MemberEnd())
	{
		Value& a = monstersIt->value;
		for (auto it = a.Begin(); it != a.End(); ++it)
		{
			const char* recruitName = it->GetString();
			ALERT(at_console, "Registered recruit: %s\n", recruitName);
			fastRecruitMonsters.push_back(std::string(recruitName));
		}
	}

	UpdatePropertyFromJson(fastRecruitRange, document, "fast_recruit_range");

	return true;
}

FollowersDescription g_FollowersDescription;

void ReadFollowersDescription()
{
	g_FollowersDescription.ReadFromFile("features/followers.json");
}

