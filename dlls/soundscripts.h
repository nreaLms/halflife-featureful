#pragma once
#ifndef SOUNDSCRIPTS_H
#define SOUNDSCRIPTS_H

#include <cstddef>
#include <initializer_list>
#include <array>
#include "const.h"
#include "rapidjson/document.h"
#include "template_property_types.h"

#include <map>
#include <set>
#include <string>
#include "icase_compare.h"

constexpr size_t MAX_RANDOM_SOUNDS = 10;

struct SoundScript
{
	SoundScript();
	SoundScript(int soundChannel, std::initializer_list<const char*> sounds, FloatRange soundVolume, float soundAttenuation, IntRange soundPitch = PITCH_NORM);
	SoundScript(int soundChannel, std::initializer_list<const char*> sounds, IntRange soundPitch = PITCH_NORM);
	std::array<const char*, MAX_RANDOM_SOUNDS> waves;
	size_t waveCount;
	int channel;
	FloatRange volume;
	float attenuation;
	IntRange pitch;

	const char* Wave() const;

private:
	void SetSoundList(std::initializer_list<const char*> sounds);
};

struct NamedSoundScript : public SoundScript
{
	NamedSoundScript(int soundChannel, std::initializer_list<const char*> sounds, FloatRange soundVolume, float soundAttenuation, IntRange soundPitch, const char* scriptName):
		SoundScript(soundChannel, sounds, soundVolume, soundAttenuation, soundPitch), name(scriptName) {}
	NamedSoundScript(int soundChannel, std::initializer_list<const char*> sounds, FloatRange soundVolume, float soundAttenuation, const char* scriptName):
		SoundScript(soundChannel, sounds, soundVolume, soundAttenuation), name(scriptName) {}
	NamedSoundScript(int soundChannel, std::initializer_list<const char*> sounds, IntRange soundPitch, const char* scriptName):
		SoundScript(soundChannel, sounds, soundPitch), name(scriptName) {}
	NamedSoundScript(int soundChannel, std::initializer_list<const char*> sounds, const char* scriptName):
		SoundScript(soundChannel, sounds), name(scriptName) {}
	const char* name;
	operator const char* () const {
		return name;
	}
};

struct SoundScriptParamOverride
{
	void OverrideVolumeAbsolute(FloatRange newVolume);
	void OverrideVolumeRelative(FloatRange newVolume);
	void OverrideAttenuationAbsolute(float newAttenuation);
	void OverrideAttenuationRelative(float newAttenuation);
	void OverridePitchAbsolute(IntRange newPitch);
	void OverridePitchRelative(IntRange newPitch);
	void OverridePitchShifted(int pitchShift);
	void OverrideChannel(int newChannel);
	void ApplyOverride(int& origChannel, FloatRange& origVolume, float& origAttenuation, IntRange& origPitch) const;
	bool HasOverrides() const {
		return channelOverride || volumeOverride || attenuationOverride || pitchOverride;
	}

private:
	enum
	{
		NO_OVERRIDE = 0,
		OVERRIDE_ABSOLUTE = 1,
		OVERRIDE_RELATIVE = 2,
		OVERRIDE_SHIFT = 3,
	};

	unsigned char channelOverride = NO_OVERRIDE;
	unsigned char volumeOverride = NO_OVERRIDE;
	unsigned char attenuationOverride = NO_OVERRIDE;
	unsigned char pitchOverride = NO_OVERRIDE;
	int channel = CHAN_AUTO;
	FloatRange volume = VOL_NORM;
	float attenuation = ATTN_NORM;
	IntRange pitch = PITCH_NORM;
};

struct SoundScriptMeta
{
	bool defaultSet = false;
	bool wavesSet = false;
	bool channelSet = false;
	bool volumeSet = false;
	bool attenuationSet = false;
	bool pitchSet = false;
};

class SoundScriptSystem
{
public:
	bool ReadFromFile(const char* fileName);
	void AddSoundScriptFromJsonValue(const char* name, rapidjson::Value& value);
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

extern SoundScriptSystem g_SoundScriptSystem;

void DumpSoundScripts();

#endif
