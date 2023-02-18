#include "parsetext.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

bool IsValidIdentifierCharacter(char c)
{
	return c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

void SkipSpaceCharacters(const char* const text, int& i, const int length)
{
	while (i<length && (text[i] == ' ' || text[i] == '\r' || text[i] == '\n'))
	{
		++i;
	}
}

void SkipSpaces(const char* const text, int& i, const int length)
{
	while (i<length && text[i] == ' ')
	{
		++i;
	}
}

void ConsumeNonSpaceCharacters(const char* const text, int& i, const int length)
{
	while(i<length && text[i] != ' ' && text[i] != '\n' && text[i] != '\r')
	{
		++i;
	}
}

void ConsumeLine(const char* const text, int& i, const int length)
{
	while(i<length && text[i] != '\n' && text[i] != '\r')
	{
		++i;
	}
}

bool ReadIdentifier(const char* const text, int& i, char* identBuf, unsigned int identBufSize)
{
	if (identBufSize < 2)
		return false;

	unsigned int identLength = 0;
	while(IsValidIdentifierCharacter(text[i]) && identLength < (identBufSize-1))
	{
		identBuf[identLength++] = text[i];
		++i;
	}
	identBuf[identLength] = '\0';
	return identLength > 0;
}

bool ParseInteger(const char* const valueText, const int valueLength, int& result)
{
	return sscanf(valueText, "%d", &result) == 1;
}

static int ClampColorComponent(int colorComponent)
{
	if (colorComponent > 255)
		return 255;
	else if (colorComponent < 0)
		return 0;
	else
		return colorComponent;
}

bool ParseColor(const char* const valueText, const int valueLength, int& result)
{
	if (valueLength >= 8 && strncmp(valueText, "0x", 2) == 0)
	{
		const char* colorStart = valueText + 2;
		return sscanf(colorStart, "%x", &result) == 1;
	}
	else
	{
		int r, g, b;
		if (sscanf(valueText, "%d %d %d", &r, &g, &b) == 3)
		{
			r = ClampColorComponent(r);
			g = ClampColorComponent(g);
			b = ClampColorComponent(b);
			result = (r << 16) + (g << 8) + b;
			return true;
		}
	}
	return false;
}

bool ParseBoolean(const char* const valueText, const int valueLength, bool& result)
{
	if (strncmp(valueText, "0", valueLength) == 0 || strnicmp(valueText, "false", valueLength) == 0 || strnicmp(valueText, "no", valueLength) == 0)
	{
		result = false;
		return true;
	}
	else if (strncmp(valueText, "1", valueLength) == 0 || strnicmp(valueText, "true", valueLength) == 0 || strnicmp(valueText, "yes", valueLength) == 0)
	{
		result = true;
		return true;
	}
	return false;
}
