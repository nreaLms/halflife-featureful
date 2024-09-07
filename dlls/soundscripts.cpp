#include "extdll.h"
#include "enginecallback.h"
#include "soundscripts.h"
#include "util.h"

#include <map>
#include <set>
#include <string>

#include "json_utils.h"

using namespace rapidjson;

const char* soundScriptsSchema = R"(
{
  "definitions": {
    "range": {
      "type": ["string", "object", "number"],
      "minimum": 0.0,
      "maximum": 1.0,
      "pattern": "[0-9]+(\\.[0-9]+)?(,[0-9]+(\\.[0-9]+)?)?",
      "properties": {
        "min": {
          "type": "number"
        },
        "max": {
          "type": "number"
        }
      }
    },
    "range_int": {
      "type": ["string", "object", "integer"],
      "pattern": "[0-9]+(,[0-9]+)?",
      "properties": {
        "min": {
          "type": "integer"
        },
        "max": {
          "type": "integer"
        }
      }
    },
    "soundscript": {
      "type": "object",
      "properties": {
        "waves": {
          "type": "array",
          "items": {
            "type": "string"
          },
          "maxItems": 10
        },
        "channel": {
          "type": "string",
          "pattern": "^auto|weapon|voice|item|body|static$"
        },
        "volume": {
          "$ref": "#/definitions/range",
        },
        "attenuation": {
          "type": ["number", "string"],
          "minimum": 0,
          "pattern": "^norm|idle|static|none$"
        },
        "pitch": {
          "$ref": "#/definitions/range_int"
        }
      }
    }
  },
  "type": "object",
  "additionalProperties": {
    "$ref": "#/definitions/soundscript"
  }
}
)";

SoundScript::SoundScript(): waves(), waveCount(0), channel(CHAN_AUTO), volume(VOL_NORM), attenuation(ATTN_NORM), pitch(PITCH_NORM) {}

SoundScript::SoundScript(int soundChannel, std::initializer_list<const char*> sounds, FloatRange soundVolume, float soundAttenuation, IntRange soundPitch)
	: channel(soundChannel), volume(soundVolume), attenuation(soundAttenuation), pitch(soundPitch)
{
	SetSoundList(sounds);
}

SoundScript::SoundScript(int soundChannel, std::initializer_list<const char *> sounds, IntRange soundPitch)
	: channel(soundChannel), volume(VOL_NORM), attenuation(ATTN_NORM), pitch(soundPitch)
{
	SetSoundList(sounds);
}

void SoundScript::SetSoundList(std::initializer_list<const char *> sounds)
{
	waveCount = Q_min(sounds.size(), MAX_RANDOM_SOUNDS);
	size_t i = 0;
	for (auto it = sounds.begin(); it != sounds.end(); ++it)
	{
		waves[i] = *it;
		++i;
		if (i >= waveCount)
			break;
	}
}

const char* SoundScript::Wave() const
{
	if (waveCount > 1)
	{
		return waves[RANDOM_LONG(0, waveCount - 1)];
	}
	else if (waveCount == 1)
	{
		return waves[0];
	}
	return nullptr;
}

void SoundScriptParamOverride::OverrideVolumeAbsolute(FloatRange newVolume)
{
	volumeOverride = OVERRIDE_ABSOLUTE;
	volume = newVolume;
}

void SoundScriptParamOverride::OverrideVolumeRelative(FloatRange newVolume)
{
	volumeOverride = OVERRIDE_RELATIVE;
	volume = newVolume;
}

void SoundScriptParamOverride::OverrideAttenuationAbsolute(float newAttenuation)
{
	attenuationOverride = OVERRIDE_ABSOLUTE;
	attenuation = newAttenuation;
}

void SoundScriptParamOverride::OverrideAttenuationRelative(float newAttenuation)
{
	attenuationOverride = OVERRIDE_RELATIVE;
	attenuation = newAttenuation;
}

void SoundScriptParamOverride::OverridePitchAbsolute(IntRange newPitch)
{
	pitchOverride = OVERRIDE_ABSOLUTE;
	pitch = newPitch;
}

void SoundScriptParamOverride::OverridePitchRelative(IntRange newPitch)
{
	pitchOverride = OVERRIDE_RELATIVE;
	pitch = newPitch;
}

void SoundScriptParamOverride::OverridePitchShifted(int pitchShift)
{
	pitchOverride = OVERRIDE_SHIFT;
	pitch = pitchShift;
}

void SoundScriptParamOverride::ApplyOverride(FloatRange &origVolume, float &origAttenuation, IntRange &origPitch) const
{
	if (volumeOverride == OVERRIDE_ABSOLUTE)
	{
		origVolume = volume;
	}
	else if (volumeOverride == OVERRIDE_RELATIVE)
	{
		origVolume = FloatRange(origVolume.min * volume.min, origVolume.max * volume.max);
	}

	if (attenuationOverride == OVERRIDE_ABSOLUTE)
	{
		origAttenuation = attenuation;
	}
	else if (attenuationOverride == OVERRIDE_RELATIVE)
	{
		origAttenuation *= attenuation;
	}

	if (pitchOverride == OVERRIDE_ABSOLUTE)
	{
		origPitch = pitch;
	}
	else if (pitchOverride == OVERRIDE_RELATIVE)
	{
		origPitch = IntRange(pitch.min * origPitch.min / 100, pitch.max * origPitch.max / 100);
	}
	else if (pitchOverride == OVERRIDE_SHIFT)
	{
		origPitch = IntRange(origPitch.min + pitch.min, origPitch.max + pitch.max);
	}
}

struct SoundScriptMeta
{
	bool defaultSet = false;
	bool wavesSet = false;
	bool channelSet = false;
	bool volumeSet = false;
	bool attenuationSet = false;
	bool pitchSet = false;
};

struct CaseInsensitiveCompare
{
	bool operator()(const std::string& lhs, const std::string& rhs) const noexcept
	{
		return stricmp(lhs.c_str(), rhs.c_str()) < 0;
	}
};

class SoundScriptSystem
{
public:
	bool ReadFromFile(const char* fileName);
	const SoundScript* GetSoundScript(const char* name);
	const SoundScript* ProvideDefaultSoundScript(const char* name, const SoundScript& soundScript);
	const SoundScript* ProvideDefaultSoundScript(const char* derivative, const char* base, const SoundScript& soundScript, const SoundScriptParamOverride paramOverride = SoundScriptParamOverride());
	void DumpSoundScripts();
	void DumpSoundScript(const char* name);
private:
	void DumpSoundScriptImpl(const char* name, const SoundScript& soundScript, const SoundScriptMeta& meta);
	void EnsureExistingScriptDefined(SoundScript& existing, SoundScriptMeta& meta, const SoundScript& soundScript);

	static constexpr const char* notDefinedYet = "waiting for default";

	std::map<std::string, std::pair<SoundScript, SoundScriptMeta>, CaseInsensitiveCompare> _soundScripts;
	std::set<std::string> _waveStringSet;
	std::string _temp;
};

static bool ParseChannel(const char* str, int& channel)
{
	constexpr std::pair<const char*, int> channels[] = {
		{"auto", CHAN_AUTO},
		{"weapon", CHAN_WEAPON},
		{"voice", CHAN_VOICE},
		{"item", CHAN_ITEM},
		{"body", CHAN_BODY},
		{"static", CHAN_STATIC},
	};

	for (auto& p : channels)
	{
		if (stricmp(str, p.first) == 0)
		{
			channel = p.second;
			return true;
		}
	}
	return false;
}

static const char* ChannelToString(int channel)
{
	switch (channel) {
	case CHAN_AUTO:
		return "auto";
	case CHAN_WEAPON:
		return "weapon";
	case CHAN_VOICE:
		return "voice";
	case CHAN_ITEM:
		return "item";
	case CHAN_BODY:
		return "body";
	case CHAN_STATIC:
		return "static";
	default:
		return "unknown";
	}
}

static bool ParseAttenuation(const char* str, float& attenuation)
{
	constexpr std::pair<const char*, int> attenuations[] = {
		{"norm", ATTN_NORM},
		{"idle", ATTN_IDLE},
		{"static", ATTN_STATIC},
		{"none", ATTN_NONE},
	};

	for (auto& p : attenuations)
	{
		if (stricmp(str, p.first) == 0)
		{
			attenuation = p.second;
			return true;
		}
	}
	return false;
}

bool SoundScriptSystem::ReadFromFile(const char *fileName)
{
	int fileSize;
	char *pMemFile = (char*)g_engfuncs.pfnLoadFileForMe( fileName, &fileSize );
	if (!pMemFile)
		return false;

	ALERT(at_console, "Parsing %s\n", fileName);

	Document document;
	bool success = ReadJsonDocumentWithSchema(document, pMemFile, fileSize, soundScriptsSchema);
	g_engfuncs.pfnFreeFile(pMemFile);

	if (!success)
		return false;

	for (auto scriptIt = document.MemberBegin(); scriptIt != document.MemberEnd(); ++scriptIt)
	{
		const char* name = scriptIt->name.GetString();

		SoundScript soundScript;
		SoundScriptMeta soundScriptMeta;

		Value& value = scriptIt->value;
		{
			auto it = value.FindMember("waves");
			if (it != value.MemberEnd())
			{
				Value::Array arr = it->value.GetArray();
				soundScript.waveCount = arr.Size();
				for (size_t i=0; i<soundScript.waveCount; ++i)
				{
					std::string str = arr[i].GetString();
					auto strIt = _waveStringSet.find(str);
					if (strIt == _waveStringSet.end())
					{
						auto p = _waveStringSet.insert(str);
						strIt = p.first;
					}
					soundScript.waves[i] = strIt->c_str();
				}
				soundScriptMeta.wavesSet = true;
			}
		}
		{
			auto it = value.FindMember("channel");
			if (it != value.MemberEnd())
			{
				soundScriptMeta.channelSet = ParseChannel(it->value.GetString(), soundScript.channel);
			}
		}
		soundScriptMeta.volumeSet = UpdatePropertyFromJson(soundScript.volume, value, "volume");
		{
			auto it = value.FindMember("attenuation");
			if (it != value.MemberEnd())
			{
				Value& attnValue = it->value;
				if (attnValue.IsString())
				{
					soundScriptMeta.attenuationSet = ParseAttenuation(attnValue.GetString(), soundScript.attenuation);
				}
				else if (attnValue.IsNumber())
				{
					soundScript.attenuation = attnValue.GetFloat();
					soundScriptMeta.attenuationSet = true;
				}
			}
		}
		soundScriptMeta.pitchSet = UpdatePropertyFromJson(soundScript.pitch, value, "pitch");

		_soundScripts[name] = std::make_pair(soundScript, soundScriptMeta);
	}

	return true;
}

const SoundScript* SoundScriptSystem::GetSoundScript(const char *name)
{
	_temp = name; // reuse the same std::string for search to avoid reallocation
	auto it = _soundScripts.find(_temp);
	if (it != _soundScripts.end())
		return &it->second.first;
	return nullptr;
}

static void MarkSoundScriptAllDefined(SoundScriptMeta& meta)
{
	meta.defaultSet = true;
	meta.wavesSet = true;
	meta.channelSet = true;
	meta.volumeSet = true;
	meta.attenuationSet = true;
	meta.pitchSet = true;
}

void SoundScriptSystem::EnsureExistingScriptDefined(SoundScript& existing, SoundScriptMeta& meta, const SoundScript& soundScript)
{
	if (!meta.defaultSet)
	{
		if (!meta.wavesSet)
		{
			existing.waves = soundScript.waves;
			existing.waveCount = soundScript.waveCount;
		}

		if (!meta.channelSet)
			existing.channel = soundScript.channel;

		if (!meta.volumeSet)
			existing.volume = soundScript.volume;

		if (!meta.attenuationSet)
			existing.attenuation = soundScript.attenuation;

		if (!meta.pitchSet)
			existing.pitch = soundScript.pitch;

		MarkSoundScriptAllDefined(meta);
	}
}

const SoundScript* SoundScriptSystem::ProvideDefaultSoundScript(const char *name, const SoundScript &soundScript)
{
	_temp = name;
	auto it = _soundScripts.find(_temp);
	if (it != _soundScripts.end())
	{
		SoundScript& existing = it->second.first;
		SoundScriptMeta& meta = it->second.second;
		EnsureExistingScriptDefined(existing, meta, soundScript);
		return &existing;
	}
	else
	{
		SoundScriptMeta meta;
		MarkSoundScriptAllDefined(meta);
		auto inserted = _soundScripts.insert(std::make_pair(_temp, std::make_pair(soundScript, meta)));
		if (inserted.second)
		{
			return &inserted.first->second.first;
		}
		return nullptr;
	}
}

const SoundScript* SoundScriptSystem::ProvideDefaultSoundScript(const char *derivative, const char *base, const SoundScript &soundScript, const SoundScriptParamOverride paramOverride)
{
	_temp = derivative;
	auto it = _soundScripts.find(_temp);
	if (it != _soundScripts.end())
	{
		SoundScript& existing = it->second.first;
		SoundScriptMeta& meta = it->second.second;

		if (!meta.defaultSet)
		{
			const SoundScript* baseScript = ProvideDefaultSoundScript(base, soundScript);
			if (baseScript)
			{
				if (paramOverride.HasOverrides())
				{
					SoundScript overrideSoundScript = *baseScript;
					paramOverride.ApplyOverride(overrideSoundScript.volume, overrideSoundScript.attenuation, overrideSoundScript.pitch);
					EnsureExistingScriptDefined(existing, meta, overrideSoundScript);
				}
				else
				{
					EnsureExistingScriptDefined(existing, meta, *baseScript);
				}
			}
		}
		return &existing;
	}
	else
	{
		//ALERT(at_console, "Derivative %s hasn't been registerd yet. Searching for %s as a base\n", derivative, base);
		const SoundScript* baseScript = ProvideDefaultSoundScript(base, soundScript);
		if (baseScript)
		{
			//ALERT(at_console, "Found a script for base %s\n", base);
			if (paramOverride.HasOverrides())
			{
				SoundScript overrideSoundScript = *baseScript;
				paramOverride.ApplyOverride(overrideSoundScript.volume, overrideSoundScript.attenuation, overrideSoundScript.pitch);
				return ProvideDefaultSoundScript(derivative, overrideSoundScript);
			}
			else
			{
				return ProvideDefaultSoundScript(derivative, *baseScript);
			}
		}
	}
	return nullptr;
}

void SoundScriptSystem::DumpSoundScriptImpl(const char *name, const SoundScript &soundScript, const SoundScriptMeta &meta)
{
	ALERT(at_console, "%s:\n", name);

	ALERT(at_console, "Waves: ");
	if (meta.wavesSet)
	{
		for (size_t i=0; i<soundScript.waveCount; ++i)
		{
			ALERT(at_console, "\"%s\"; ", soundScript.waves[i]);
		}
		ALERT(at_console, "\n");
	}
	else
	{
		ALERT(at_console, "%s\n", notDefinedYet);
	}

	ALERT(at_console, "Channel: %s. ", meta.channelSet ? ChannelToString(soundScript.channel) : notDefinedYet);

	ALERT(at_console, "Volume: ");
	if (meta.volumeSet)
	{
		if (soundScript.volume.max <= soundScript.volume.min)
		{
			ALERT(at_console, "%g. ", soundScript.volume.min);
		}
		else
		{
			ALERT(at_console, "%g-%g. ", soundScript.volume.min, soundScript.volume.max);
		}
	}
	else
	{
		ALERT(at_console, "%s. ", notDefinedYet);
	}

	ALERT(at_console, "Attenuation: ");
	if (meta.attenuationSet)
	{
		ALERT(at_console, "%g. ", soundScript.attenuation);
	}
	else
	{
		ALERT(at_console, "%s. ", notDefinedYet);
	}

	ALERT(at_console, "Pitch: ");
	if (meta.pitchSet)
	{
		if (soundScript.pitch.max <= soundScript.pitch.min)
		{
			ALERT(at_console, "%d\n\n", soundScript.pitch.min);
		}
		else
		{
			ALERT(at_console, "%d-%d\n\n", soundScript.pitch.min, soundScript.pitch.max);
		}
	}
	else
	{
		ALERT(at_console, "%s\n\n", notDefinedYet);
	}
}

void SoundScriptSystem::DumpSoundScripts()
{
	for (const auto& p : _soundScripts)
	{
		DumpSoundScriptImpl(p.first.c_str(),  p.second.first, p.second.second);
	}
}

void SoundScriptSystem::DumpSoundScript(const char *name)
{
	_temp = name;
	if (_temp[_temp.size()-1] == '.')
	{
		bool foundSomething = false;
		for (const auto& p : _soundScripts)
		{
			if (strnicmp(p.first.c_str(), _temp.c_str(), _temp.size()) == 0)
			{
				foundSomething = true;
				DumpSoundScriptImpl(p.first.c_str(),  p.second.first, p.second.second);
			}
		}
		if (foundSomething)
			return;
	}
	else
	{
		auto it = _soundScripts.find(_temp);
		if (it != _soundScripts.end())
		{
			DumpSoundScriptImpl(name, it->second.first, it->second.second);
			return;
		}
	}
	ALERT(at_console, "Couldn't find a sound script for %s\n", name);
}

SoundScriptSystem g_SoundScriptSystem;

void ReadSoundScripts()
{
	g_SoundScriptSystem.ReadFromFile("sound/soundscripts.json");
}

const SoundScript* ProvideDefaultSoundScript(const char* name, const SoundScript& soundScript)
{
	return g_SoundScriptSystem.ProvideDefaultSoundScript(name, soundScript);
}

const SoundScript* ProvideDefaultSoundScript(const char *derivative, const char *base, const SoundScript &soundScript, const SoundScriptParamOverride paramOverride)
{
	return g_SoundScriptSystem.ProvideDefaultSoundScript(derivative, base, soundScript, paramOverride);
}

const SoundScript* GetSoundScript(const char* name)
{
	return g_SoundScriptSystem.GetSoundScript(name);
}

void DumpSoundScripts()
{
	int argc = CMD_ARGC();
	if (argc > 1)
	{
		for (int i=1; i<argc; ++i)
			g_SoundScriptSystem.DumpSoundScript(CMD_ARGV(i));
	}
	else
		g_SoundScriptSystem.DumpSoundScripts();
}
