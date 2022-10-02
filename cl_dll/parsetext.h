#pragma once
#ifndef PARSETEXT_H
#define PARSETEXT_H

void SkipSpaceCharacters(const char* const text, int& i, const int length);
void SkipSpaces(const char* const text, int& i, const int length);
void ConsumeNonSpaceCharacters(const char* const text, int& i, const int length);
void ConsumeLine(const char* const text, int& i, const int length);
bool ReadInteger(const char* const valueText, const int valueLength, int& result);
bool ReadColor(const char* const valueText, const int valueLength, int& result);
bool ReadBoolean(const char* const valueText, const int valueLength, bool& result);

#endif
