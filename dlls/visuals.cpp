#include "extdll.h"
#include "enginecallback.h"
#include "util.h"
#include "visuals.h"
#include "customentity.h"

#include "json_utils.h"

using namespace rapidjson;

const char* visualsSchema = R"(
{
  "type": "object",
  "additionalProperties": {
    "$ref": "definitions.json#/visual"
  }
}
)";

void Visual::DoPrecache()
{
	if (HasModel())
	{
		modelIndex = PRECACHE_MODEL(model);
	}
}

void Visual::CompleteFrom(const Visual &visual)
{
	if (ShouldCompleteFrom(visual, MODEL_DEFINED))
	{
		SetModel(visual.model);
	}
	if (ShouldCompleteFrom(visual, RENDERMODE_DEFINED))
	{
		SetRenderMode(visual.rendermode);
	}
	if (ShouldCompleteFrom(visual, COLOR_DEFINED))
	{
		SetColor(visual.rendercolor);
	}
	if (ShouldCompleteFrom(visual, ALPHA_DEFINED))
	{
		SetAlpha(visual.renderamt);
	}
	if (ShouldCompleteFrom(visual, RENDERFX_DEFINED))
	{
		SetRenderFx(visual.renderfx);
	}
	if (ShouldCompleteFrom(visual, SCALE_DEFINED))
	{
		SetScale(visual.scale);
	}
	if (ShouldCompleteFrom(visual, FRAMERATE_DEFINED))
	{
		SetFramerate(visual.framerate);
	}
	if (ShouldCompleteFrom(visual, BEAMWIDTH_DEFINED))
	{
		SetBeamWidth(visual.beamWidth);
	}
	if (ShouldCompleteFrom(visual, BEAMNOISE_DEFINED))
	{
		SetBeamNoise(visual.beamNoise);
	}
	if (ShouldCompleteFrom(visual, BEAMSCROLLRATE_DEFINED))
	{
		SetBeamScrollRate(visual.beamScrollRate);
	}
	if (ShouldCompleteFrom(visual, LIFE_DEFINED))
	{
		SetLife(visual.life);
	}
	if (ShouldCompleteFrom(visual, RADIUS_DEFINED))
	{
		SetRadius(visual.radius);
	}
	if (ShouldCompleteFrom(visual, BEAMFLAGS_DEFINED))
	{
		SetBeamFlags(visual.beamFlags);
	}
}

static bool ParseRenderMode(const char* str, int& rendermode)
{
	constexpr std::pair<const char*, int> modes[] = {
		{"normal", kRenderNormal},
		{"color", kRenderTransColor},
		{"texture", kRenderTransTexture},
		{"glow", kRenderGlow},
		{"solid", kRenderTransAlpha},
		{"additive", kRenderTransAdd},
	};

	for (auto& p : modes)
	{
		if (stricmp(str, p.first) == 0)
		{
			rendermode = p.second;
			return true;
		}
	}
	return false;
}

static bool ParseRenderFx(const char* str, int& renderfx)
{
	constexpr std::pair<const char*, int> modes[] = {
		{"normal", kRenderFxNone},
		{"constant glow", kRenderFxNoDissipation},
		{"distort", kRenderFxDistort},
		{"hologram", kRenderFxHologram},
		{"glow shell", kRenderFxGlowShell},
	};

	for (auto& p : modes)
	{
		if (stricmp(str, p.first) == 0)
		{
			renderfx = p.second;
			return true;
		}
	}
	return false;
}

bool VisualSystem::ReadFromFile(const char *fileName)
{
	int fileSize;
	char *pMemFile = (char*)g_engfuncs.pfnLoadFileForMe( fileName, &fileSize );
	if (!pMemFile)
		return false;

	ALERT(at_console, "Parsing %s\n", fileName);

	Document document;
	bool success = ReadJsonDocumentWithSchema(document, pMemFile, fileSize, visualsSchema, fileName);
	g_engfuncs.pfnFreeFile(pMemFile);

	if (!success)
		return false;

	for (auto scriptIt = document.MemberBegin(); scriptIt != document.MemberEnd(); ++scriptIt)
	{
		const char* name = scriptIt->name.GetString();

		Value& value = scriptIt->value;
		if (value.IsObject())
			AddVisualFromJsonValue(name, value);
		else
			ALERT(at_warning, "Visual '%s' is not an object!\n");
	}

	return true;
}

void VisualSystem::AddVisualFromJsonValue(const char *name, Value &value)
{
	Visual visual;

	{
		auto it = value.FindMember("model");
		if (it != value.MemberEnd())
		{
			std::string str = it->value.GetString();
			auto strIt = _modelStringSet.find(str);
			if (strIt == _modelStringSet.end())
			{
				auto p = _modelStringSet.insert(str);
				strIt = p.first;
			}
			visual.SetModel(strIt->c_str());
		}
	}

	{
		auto it = value.FindMember("sprite");
		if (it != value.MemberEnd())
		{
			if (visual.HasDefined(Visual::MODEL_DEFINED))
			{
				ALERT(at_warning, "Visual \"s\" has both 'model' and 'sprite' properties defined!\n", name);
			}
			else
			{
				std::string str = it->value.GetString();
				auto strIt = _modelStringSet.find(str);
				if (strIt == _modelStringSet.end())
				{
					auto p = _modelStringSet.insert(str);
					strIt = p.first;
				}
				visual.SetModel(strIt->c_str());
			}
		}
	}

	{
		auto it = value.FindMember("rendermode");
		if (it != value.MemberEnd())
		{
			Value& rendermodeValue = it->value;
			if (rendermodeValue.IsString())
			{
				int rendermode;
				if (ParseRenderMode(rendermodeValue.GetString(), rendermode))
				{
					visual.SetRenderMode(rendermode);
				}
			}
			else if (rendermodeValue.IsInt())
			{
				visual.SetRenderMode(rendermodeValue.GetInt());
			}
		}
	}

	Color color;
	if (UpdatePropertyFromJson(color, value, "color"))
	{
		visual.SetColor(color);
	}

	int renderamt;
	if (UpdatePropertyFromJson(renderamt, value, "alpha"))
	{
		visual.SetAlpha(renderamt);
	}

	{
		auto it = value.FindMember("renderfx");
		if (it != value.MemberEnd())
		{
			Value& renderfxValue = it->value;
			if (renderfxValue.IsString())
			{
				int renderfx;
				if (ParseRenderFx(renderfxValue.GetString(), renderfx))
				{
					visual.SetRenderFx(renderfx);
				}
			}
			else if (renderfxValue.IsInt())
			{
				visual.SetRenderFx(renderfxValue.GetInt());
			}
		}
	}

	FloatRange scale;
	if (UpdatePropertyFromJson(scale, value, "scale"))
	{
		visual.SetScale(scale);
	}

	float framerate;
	if (UpdatePropertyFromJson(framerate, value, "framerate"))
	{
		visual.SetFramerate(framerate);
	}

	int beamWidth, beamNoise, beamScrollRate;
	if (UpdatePropertyFromJson(beamWidth, value, "width"))
	{
		visual.SetBeamWidth(beamWidth);
	}
	if (UpdatePropertyFromJson(beamNoise, value, "noise"))
	{
		visual.SetBeamNoise(beamNoise);
	}
	if (UpdatePropertyFromJson(beamScrollRate, value, "scrollrate"))
	{
		visual.SetBeamScrollRate(beamScrollRate);
	}

	FloatRange life;
	if (UpdatePropertyFromJson(life, value, "life"))
	{
		visual.SetLife(life);
	}

	IntRange radius;
	if (UpdatePropertyFromJson(radius, value, "radius"))
	{
		visual.SetRadius(radius);
	}

	{
		auto it = value.FindMember("beamflags");
		if (it != value.MemberEnd())
		{
			int beamFlags = 0;
			Value::Array arr = it->value.GetArray();
			for (size_t i=0; i<arr.Size(); ++i)
			{
				const char* str = arr[i].GetString();
				if (stricmp(str, "sine") == 0)
					beamFlags |= BEAM_FSINE;
				else if (stricmp(str, "solid") == 0)
					beamFlags |= BEAM_FSOLID;
				else if (stricmp(str, "shadein") == 0)
					beamFlags |= BEAM_FSHADEIN;
				else if (stricmp(str, "shadeout") == 0)
					beamFlags |= BEAM_FSHADEOUT;
			}
			visual.SetBeamFlags(beamFlags);
		}
	}

	_visuals[name] = visual;
}

void VisualSystem::EnsureVisualExists(const std::string& name)
{
	auto it = _visuals.find(name);
	if (it == _visuals.end())
		_visuals[name] = Visual();
}

const Visual* VisualSystem::GetVisual(const char *name)
{
	if (!name || *name == '\0')
		return nullptr;
	_temp = name; // reuse the same std::string for search to avoid reallocation
	auto it = _visuals.find(_temp);
	if (it != _visuals.end())
		return &it->second;
	return nullptr;
}

const Visual* VisualSystem::ProvideDefaultVisual(const char *name, const Visual &visual, bool doPrecache)
{
	_temp = name;
	auto it = _visuals.find(_temp);
	if (it != _visuals.end())
	{
		Visual& existing = it->second;
		existing.CompleteFrom(visual);

		if (doPrecache)
			existing.DoPrecache();

		return &existing;
	}
	else
	{
		auto inserted = _visuals.insert(std::make_pair(_temp, visual));
		if (inserted.second)
		{
			Visual* insertedVisual = &inserted.first->second;
			if (doPrecache)
				insertedVisual->DoPrecache();
			return insertedVisual;
		}
		// Should never get here: if it already existed it should have used the first if branch.
		return nullptr;
	}
}

static void PrintRange(const char* name, FloatRange range)
{
	if (range.max <= range.min)
	{
		ALERT(at_console, "%s: %g. ", name, range.min);
	}
	else
	{
		ALERT(at_console, "%s: %g-%g. ", name, range.min, range.max);
	}
}

void VisualSystem::DumpVisualImpl(const char *name, const Visual &visual)
{
	ALERT(at_console, "%s:\n", name);

	ALERT(at_console, "Model/Sprite: \"%s\"\n", visual.model ? visual.model : "");

	ALERT(at_console, "Rendermode: %s. Color: (%d, %d, %d). Alpha: %d. Renderfx: %s. ",
		  RenderModeToString(visual.rendermode),
		  visual.rendercolor.r, visual.rendercolor.g, visual.rendercolor.b,
		  visual.renderamt,
		  RenderFxToString(visual.renderfx));

	PrintRange("Scale", visual.scale);
	PrintRange("Framerate", visual.framerate);

	if (visual.HasDefined(Visual::BEAMWIDTH_DEFINED))
	{
		ALERT(at_console, "Beam width: %d. Beam noise: %d. Beam scoll rate: %d. ", visual.beamWidth, visual.beamNoise, visual.beamScrollRate);
	}

	if (visual.HasDefined(Visual::LIFE_DEFINED))
	{
		PrintRange("Life", visual.life);
	}

	if (visual.HasDefined(Visual::RADIUS_DEFINED))
	{
		if (visual.radius.max <= visual.radius.min)
		{
			ALERT(at_console, "Radius: %d. ", visual.radius.min);
		}
		else
		{
			ALERT(at_console, "Radius: %d-%d. ", visual.radius.min, visual.radius.max);
		}
	}

	if (visual.HasDefined(Visual::BEAMFLAGS_DEFINED))
	{
		const int beamFlags = visual.beamFlags;
		ALERT(at_console, "Beam flags: ");
		if (FBitSet(beamFlags, BEAM_FSINE))
			ALERT(at_console, "Sine; ");
		if (FBitSet(beamFlags, BEAM_FSOLID))
			ALERT(at_console, "Solid; ");
		if (FBitSet(beamFlags, BEAM_FSHADEIN))
			ALERT(at_console, "Shadein; ");
		if (FBitSet(beamFlags, BEAM_FSHADEOUT))
			ALERT(at_console, "Shadeout; ");
	}

	ALERT(at_console, "\n\n");
}

void VisualSystem::DumpVisuals()
{
	for (const auto& p : _visuals)
	{
		DumpVisualImpl(p.first.c_str(),  p.second);
	}
}

void VisualSystem::DumpVisual(const char *name)
{
	_temp = name;
	if (_temp[_temp.size()-1] == '.' || _temp[_temp.size()-1] == '#')
	{
		bool foundSomething = false;
		for (const auto& p : _visuals)
		{
			if (strnicmp(p.first.c_str(), _temp.c_str(), _temp.size()) == 0)
			{
				foundSomething = true;
				DumpVisualImpl(p.first.c_str(),  p.second);
			}
		}
		if (foundSomething)
			return;
	}
	else
	{
		auto it = _visuals.find(_temp);
		if (it != _visuals.end())
		{
			DumpVisualImpl(name, it->second);
			return;
		}
	}
	ALERT(at_console, "Couldn't find a visual for %s\n", name);
}

VisualSystem g_VisualSystem;

void DumpVisuals()
{
	int argc = CMD_ARGC();
	if (argc > 1)
	{
		for (int i=1; i<argc; ++i)
			g_VisualSystem.DumpVisual(CMD_ARGV(i));
	}
	else
		g_VisualSystem.DumpVisuals();
}
