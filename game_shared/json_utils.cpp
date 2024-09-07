#include "json_utils.h"

#if CLIENT_DLL
#include "cl_dll.h"
#define JSON_LOG gEngfuncs.Con_DPrintf
#define JSON_ERROR gEngfuncs.Con_DPrintf
#else
#include "util.h"
#define JSON_LOG(...) ALERT(at_aiconsole, ##__VA_ARGS__ )
#define JSON_ERROR(...) ALERT(at_error, ##__VA_ARGS__ )
#endif

#include "color_utils.h"
#include "parsetext.h"

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/schema.h"
#include "rapidjson/error/en.h"

using namespace rapidjson;

static void CalculateLineAndColumnFromOffset(const char* pMemFile, size_t offset, size_t& line, size_t& column)
{
	const char* cur = pMemFile;
	line = 1;
	column = 0;
	size_t i = 0;
	bool nextLine = false;
	while (cur && *cur && i <= offset)
	{
		if (nextLine)
		{
			nextLine = false;
			line++;
			column = 0;
		}
		if (*cur == '\n')
		{
			nextLine = true;
		}
		++column;
		++cur;
		++i;
	}
}

static void ReportParseErrors(const char* fileName, ParseResult& parseResult, const char *pMemFile)
{
	size_t errorLine, errorColumn;
	CalculateLineAndColumnFromOffset(pMemFile, parseResult.Offset(), errorLine, errorColumn);
	JSON_ERROR("%s: JSON parse error: %s (Line %lu, column %lu)\n", fileName, GetParseError_En(parseResult.Code()), errorLine, errorColumn);
}

bool ReadJsonDocumentWithSchema(Document &document, const char *pMemFile, int fileSize, const char *schemaText, const char* fileName)
{
	if (!fileName)
		fileName = "";

	Document schemaDocument;
	schemaDocument.Parse<kParseTrailingCommasFlag | kParseCommentsFlag>(schemaText);
	ParseResult parseResult = schemaDocument;
	if (!parseResult) {
		ReportParseErrors(fileName, parseResult, pMemFile);
		return false;
	}
	SchemaDocument schema(schemaDocument);

	document.Parse<kParseTrailingCommasFlag | kParseCommentsFlag>(pMemFile, fileSize);
	parseResult = document;
	if (!parseResult) {
		ReportParseErrors(fileName, parseResult, pMemFile);
		return false;
	}

	SchemaValidator validator(schema);
	if (!document.Accept(validator))
	{
		Pointer schemaPointer = validator.GetInvalidSchemaPointer();
		StringBuffer schemaPathBuffer;
		schemaPointer.Stringify(schemaPathBuffer);

		Pointer docPointer = validator.GetInvalidDocumentPointer();
		StringBuffer docPathBuffer;
		docPointer.Stringify(docPathBuffer);

		StringBuffer badValueBuffer;
		Value *badVal = GetValueByPointer(document, docPointer);
		if (badVal)
		{
			Writer<StringBuffer> writer(badValueBuffer);
			badVal->Accept(writer);
		}

		StringBuffer schemaPartBuffer;
		Pointer schemaKeywordPointer = schemaPointer.Append(validator.GetInvalidSchemaKeyword());
		Value* schemaPartValue = GetValueByPointer(schemaDocument, schemaKeywordPointer);
		if (schemaPartValue)
		{
			Writer<StringBuffer> writer(schemaPartBuffer);
			schemaPartValue->Accept(writer);
		}

		JSON_ERROR("%s: value %s of property '%s' doesn't match the constraint '%s' in '%s': %s\n",
			fileName,
			badValueBuffer.GetString(),
			docPathBuffer.GetString(),
			validator.GetInvalidSchemaKeyword(),
			schemaPathBuffer.GetString(),
			schemaPartBuffer.GetString());

		return false;
	}
	return true;
}

bool UpdatePropertyFromJson(std::string& str, Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		str = it->value.GetString();
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(int& i, Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		i = it->value.GetInt();
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(float& f, Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		f = it->value.GetFloat();
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(bool& b, Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		b = it->value.GetBool();
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(Color& color, Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		if (it->value.IsString())
		{
			const char* colorStr = it->value.GetString();
			int packedColor;
			if (ParseColor(colorStr, packedColor)) {
				UnpackRGB(color.r, color.g, color.b, packedColor);
				return true;
			}
		}
		else if (it->value.IsArray())
		{
			Value::Array arr = it->value.GetArray();
			color.r = arr[0].GetInt();
			color.g = arr[1].GetInt();
			color.b = arr[2].GetInt();
		}
	}
	return false;
}

bool UpdatePropertyFromJson(FloatRange& floatRange, Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		Value& value = it->value;
		if (value.IsNumber())
		{
			floatRange.min = value.GetFloat();
			floatRange.max = floatRange.min;
		}
		else if (value.IsObject())
		{
			auto minIt = value.FindMember("min");
			auto maxIt = value.FindMember("max");
			if (minIt != value.MemberEnd())
			{
				if (minIt->value.IsFloat())
					floatRange.min = minIt->value.GetFloat();
			}
			if (maxIt != value.MemberEnd())
			{
				if (maxIt->value.IsFloat())
					floatRange.max = maxIt->value.GetFloat();
			}
		}
		else if (value.IsString())
		{
			const char* str = value.GetString();
			const char* found = strchr(str, ',');
			floatRange.min = atof(str);
			if (found) {
				found++;
				floatRange.max = atof(found);
			} else {
				floatRange.max = floatRange.min;
			}
		}
		else if (value.IsArray())
		{
			Value::Array arr = value.GetArray();
			floatRange.min = arr[0].GetFloat();
			floatRange.max = arr[1].GetFloat();
		}

		if (floatRange.min > floatRange.max) {
			floatRange.min = floatRange.max;
		}
		return true;
	}
	return false;
}

bool UpdatePropertyFromJson(IntRange& intRange, Value& jsonValue, const char* key)
{
	auto it = jsonValue.FindMember(key);
	if (it != jsonValue.MemberEnd())
	{
		Value& value = it->value;
		if (value.IsInt())
		{
			intRange.min = value.GetInt();
			intRange.max = intRange.min;
		}
		else if (value.IsObject())
		{
			auto minIt = value.FindMember("min");
			auto maxIt = value.FindMember("max");
			if (minIt != value.MemberEnd())
			{
				if (minIt->value.IsInt())
					intRange.min = minIt->value.GetInt();
			}
			if (maxIt != value.MemberEnd())
			{
				if (maxIt->value.IsInt())
					intRange.max = maxIt->value.GetInt();
			}
		}
		else if (value.IsString())
		{
			const char* str = value.GetString();
			const char* found = strchr(str, ',');
			intRange.min = atoi(str);
			if (found) {
				found++;
				intRange.max = atoi(found);
			} else {
				intRange.max = intRange.min;
			}
		}
		else if (value.IsArray())
		{
			Value::Array arr = value.GetArray();
			intRange.min = arr[0].GetInt();
			intRange.max = arr[1].GetInt();
		}

		if (intRange.min > intRange.max) {
			intRange.min = intRange.max;
		}
		return true;
	}
	return false;
}
