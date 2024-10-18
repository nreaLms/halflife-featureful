#pragma once
#ifndef ENT_TEMPLATES_H
#define ENT_TEMPLATES_H

#include "visuals.h"
#include "soundscripts.h"

#include <map>
#include <string>

struct EntTemplate
{
public:
	const char* OwnVisualName() const;
	void SetOwnVisualName(const std::string& name) {
		_ownVisual = name;
	}

	const char* GibVisualName() const;
	void SetGibVisualName(const std::string& name) {
		_gibVisual = name;
	}

	const char* GetSoundScriptNameOverride(const char* name) const;
	void SetSoundScriptReplacement(const char* soundScript, const std::string& replacement);

	const char* GetVisualNameOverride(const char* name) const;
	void SetVisualReplacement(const char* visual, const std::string& replacement);

	bool IsClassifyDefined() const {
		return (_defined & CLASSIFY_DEFINED) != 0;
	}
	int Classify() const {
		return _classify;
	}
	void SetClassify(int classify) {
		_defined |= CLASSIFY_DEFINED;
		_classify = classify;
	}

	bool IsBloodDefined() const {
		return (_defined & BLOOD_DEFINED) != 0;
	}
	int BloodColor() const {
		return _bloodColor;
	}
	void SetBloodColor(int bloodColor) {
		_defined |= BLOOD_DEFINED;
		_bloodColor = bloodColor;
	}

	bool IsHealthDefined() const {
		return (_defined & HEALTH_DEFINED) != 0;
	}
	float Health() const {
		return _health;
	}
	void SetHealth(float health) {
		_defined |= HEALTH_DEFINED;
		_health = health;
	}

	bool IsFielfOfViewDefined() const {
		return (_defined & FIELDOFVIEW_DEFINED) != 0;
	}
	float FieldOfView() const {
		return _fieldOfView;
	}
	void SetFieldOfView(float fieldOfView) {
		_defined |= FIELDOFVIEW_DEFINED;
		_fieldOfView = fieldOfView;
	}

	bool IsSizeDefined() const {
		return (_defined & SIZE_DEFINED) != 0;
	}
	Vector MinSize() const {
		return _minSize;
	}
	Vector MaxSize() const {
		return _maxSize;
	}
	void SetSize(const Vector& minSize, const Vector& maxSize)
	{
		_defined |= SIZE_DEFINED;
		_minSize = minSize;
		_maxSize = maxSize;
	}

	bool IsSizeForGrappleDefined() const {
		return (_defined & SIZEFORGRAPPLE_DEFINED) != 0;
	}
	int SizeForGrapple() const {
		return _sizeForGrapple;
	}
	void SetSizeForGrapple(int sizeForGrapple)
	{
		_defined |= SIZEFORGRAPPLE_DEFINED;
		_sizeForGrapple = sizeForGrapple;
	}

	const char* SpeechPrefix() const;
	void SetSpeechPrefix(const std::string& speechPrefix) {
		_speechPrefix = speechPrefix;
	}
private:
	std::map<std::string, std::string> _soundScripts;
	std::map<std::string, std::string> _visuals;
	std::string _ownVisual;
	std::string _gibVisual;

	enum
	{
		CLASSIFY_DEFINED = (1 << 0),
		BLOOD_DEFINED = (1 << 1),
		HEALTH_DEFINED = (1 << 2),
		FIELDOFVIEW_DEFINED = (1 << 3),
		SIZE_DEFINED = (1 << 4),
		SIZEFORGRAPPLE_DEFINED = (1 << 5),
	};

	int _defined = 0;
	int _classify = 0;
	int _bloodColor = 0;
	float _health = 0.0f;
	float _fieldOfView = 0.0f;
	Vector _minSize = Vector(0,0,0);
	Vector _maxSize = Vector(0,0,0);
	short _sizeForGrapple = 0;

	std::string _speechPrefix;
};

void ReadEntTemplates();

const EntTemplate* GetEntTemplate(const char* name);

void EnsureVisualReplacementForTemplate(const char* templateName, const char* visualName);

#endif
