#include "extdll.h"
#include "eiface.h"
#include "util.h"
#include "parsetext.h"

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

static void LogExpectedQuoteError(const char* fileName, int line, int pos)
{
	ALERT(at_console, "Parsing %s. Expected \" at line %d, position %d\n", fileName, line, pos);
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
	//const std::regex regex("\\s*\"(.+)\"\\s+\"(.+)\"\\s*");
	char buffer[512] = { 0 };
	bool success = true;
	int lineNum = 0;
	while( memfgets( pMemFile, fileSize, filePos, buffer, sizeof(buffer)-1 ) ) {
		lineNum++;
		int i = 0;
		SkipSpaceCharacters(buffer, i, sizeof(buffer));
		if (buffer[i] == '\0') // it's an empty line, skip
			continue;
		if (buffer[i] == '/') // it's a comment, skip
			continue;
		if (buffer[i] != '"')
		{
			success = false;
			LogExpectedQuoteError(fileName, lineNum, i);
			break;
		}
		++i;
		const int origNameStart = i;
		if (ConsumeLineUntil(buffer, i, sizeof(buffer), '"'))
		{
			const int origNameEnd = i;
			i++;
			SkipSpaceCharacters(buffer, i, sizeof(buffer));

			if (buffer[i] != '"')
			{
				LogExpectedQuoteError(fileName, lineNum, i);
				success = false;
				break;
			}
			else
			{
				++i;
				const int replNameStart = i;
				if (ConsumeLineUntil(buffer, i, sizeof(buffer), '"'))
				{
					const int replNameEnd = i;

					const std::string originalName(buffer + origNameStart, buffer + origNameEnd);
					const std::string replacementName(buffer + replNameStart, buffer + replNameEnd);
					replacementMap[originalName] = replacementName;
					ALERT(at_console, "File '%s' is replaced with '%s'\n", originalName.c_str(), replacementName.c_str());
				}
				else
				{
					LogExpectedQuoteError(fileName, lineNum, i);
					success = false;
					break;
				}
			}
		}
		else
		{
			LogExpectedQuoteError(fileName, lineNum, i);
			success = false;
			break;
		}
		/*std::cmatch base_match;
		if (std::regex_match(line, base_match, regex)) {
			auto originalName = base_match[1].str();
			auto replacementName = base_match[2].str();
			replacementMap[originalName] = replacementName;
			ALERT(at_console, "File '%s' is replaced with '%s'\n", originalName.c_str(), replacementName.c_str());
		} else {
			ALERT(at_console, "Line '%s' in file '%s' has a bad format\n", buffer, fileName);
			success = false;
			break;
		}*/
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
