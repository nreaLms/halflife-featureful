#include "parsetext.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

bool IsValidIdentifierCharacter(char c)
{
	return c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

void SkipSpaceCharacters(const char* text, int& i, const int length)
{
	while (i<length && (text[i] == ' ' || text[i] == '\r' || text[i] == '\n'))
	{
		++i;
	}
}

void SkipSpaces(const char *text, int& i, const int length)
{
	while (i<length && text[i] == ' ')
	{
		++i;
	}
}

void ConsumeNonSpaceCharacters(const char *text, int& i, const int length)
{
	while(i<length && text[i] != ' ' && text[i] != '\n' && text[i] != '\r' && text[i] != '\0')
	{
		++i;
	}
}

void ConsumeLine(const char *text, int& i, const int length)
{
	while(i<length && text[i] != '\n' && text[i] != '\r' && text[i] != '\0')
	{
		++i;
	}
}

bool ReadIdentifier(const char *text, int& i, char* identBuf, unsigned int identBufSize)
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

bool ParseInteger(const char *valueText, int& result)
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

bool ParseColor(const char *valueText, int& result)
{
	if (strncmp(valueText, "0x", 2) == 0)
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

bool ParseBoolean(const char* valueText, bool& result)
{
	if (strcmp(valueText, "0") == 0 || stricmp(valueText, "false") == 0 || stricmp(valueText, "no") == 0)
	{
		result = false;
		return true;
	}
	else if (strcmp(valueText, "1") == 0 || stricmp(valueText, "true") == 0 || stricmp(valueText, "yes") == 0)
	{
		result = true;
		return true;
	}
	return false;
}
