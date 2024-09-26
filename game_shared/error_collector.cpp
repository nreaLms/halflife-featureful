#include "error_collector.h"

void ErrorCollector::AddError(const char *str)
{
	if (str)
		_errors.push_back(str);
}

bool ErrorCollector::HasErrors() const
{
	return _errors.size() > 0;
}

std::string ErrorCollector::GetFullString() const
{
	std::string fullString;
	for (const std::string& str : _errors)
	{
		fullString += str;
		fullString += '\n';
	}
	return fullString;
}

void ErrorCollector::Clear()
{
	_errors.clear();
}

ErrorCollector g_errorCollector;
