#pragma once
#ifndef PARSETEXT_H
#define PARSETEXT_H

bool IsValidIdentifierCharacter(char c);
void SkipSpaceCharacters(const char* const text, int& i, const int length);
void SkipSpaces(const char* const text, int& i, const int length);
void ConsumeNonSpaceCharacters(const char* const text, int& i, const int length);
void ConsumeLine(const char* const text, int& i, const int length);
bool ReadIdentifier(const char* const text, int &i, char* identBuf, unsigned int identBufSize);
bool ParseInteger(const char* const valueText, const int valueLength, int& result);
bool ParseColor(const char* const valueText, const int valueLength, int& result);
bool ParseBoolean(const char* const valueText, const int valueLength, bool& result);

#endif
