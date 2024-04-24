#include "extdll.h"
#include "eiface.h"
#include "util.h"
#include "parsetext.h"

#include <regex>

#include "soundreplacement.h"

SoundReplacementList::SoundReplacementList(): _isValid(false) {}

SoundReplacementList::SoundReplacementList(ReplacementMap &&replacementMap):
	_replacementMap(replacementMap), _isValid(true) {}

bool SoundReplacementList::IsValid() const {
	return _isValid;
}

std::string SoundReplacementList::ReplacementFor(const std::string &soundName)
{
	auto it = _replacementMap.find(soundName);
	if (it != _replacementMap.end()) {
		return it->second;
	}
	return std::string();
}

bool SoundReplacementSystem::EnsureReplacementFile(const char *fileName)
{
	std::string name(fileName);
	auto it = _fileMap.find(name);
	if (it != _fileMap.end())
	{
		return it->second.IsValid();
	}

	int filePos = 0;
	int fileSize;
	byte *pMemFile = g_engfuncs.pfnLoadFileForMe( fileName, &fileSize );
	if (!pMemFile)
	{
		ALERT(at_console, "Couldn't open sound replacement file %s\n", fileName);
		return false;
	}

	SoundReplacementList::ReplacementMap replacementMap;
	const std::regex regex("\\s*\"(.+)\"\\s+\"(.+)\"\\s*");
	char buffer[512] = { 0 };
	bool success = true;
	while( memfgets( pMemFile, fileSize, filePos, buffer, sizeof(buffer)-1 ) ) {
		std::cmatch base_match;
		if (std::regex_match(buffer, base_match, regex)) {
			auto originalName = base_match[1].str();
			auto replacementName = base_match[2].str();
			replacementMap[originalName] = replacementName;
			ALERT(at_console, "File '%s' is replaced with '%s'\n", originalName.c_str(), replacementName.c_str());
		} else {
			ALERT(at_console, "Line '%s' in file '%s' has a bad format\n", buffer, fileName);
			success = false;
			break;
		}
	}

	if (success) {
		_fileMap[name] = SoundReplacementList(std::move(replacementMap));
	} else {
		_fileMap[name] = SoundReplacementList();
	}

	g_engfuncs.pfnFreeFile( pMemFile );
	return true;
}

std::string SoundReplacementSystem::FindReplacement(const char *fileName, const char *originalSoundName)
{
	auto fileIt = _fileMap.find(fileName);
	if (fileIt != _fileMap.end()) {
		SoundReplacementList& soundReplacementFile = fileIt->second;
		if (soundReplacementFile.IsValid()) {
			return soundReplacementFile.ReplacementFor(originalSoundName);
		}
	}
	return std::string();
}
