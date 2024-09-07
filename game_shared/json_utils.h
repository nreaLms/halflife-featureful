#pragma once
#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <string>

#include "rapidjson/document.h"
#include "template_property_types.h"

bool ReadJsonDocumentWithSchema(rapidjson::Document& document, const char* pMemFile, int fileSize, const char* schemaText, const char* fileName);

bool UpdatePropertyFromJson(std::string& str, rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(int& i, rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(float& f, rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(bool& b, rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(Color& color, rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(FloatRange& floatRange, rapidjson::Value& jsonValue, const char* key);
bool UpdatePropertyFromJson(IntRange& intRange, rapidjson::Value& jsonValue, const char* key);

#endif
