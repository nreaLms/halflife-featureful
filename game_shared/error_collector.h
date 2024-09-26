#pragma once
#ifndef ERROR_COLLECTOR_H
#define ERROR_COLLECTOR_H

#include <vector>
#include <string>

class ErrorCollector
{
public:
	void AddError(const char* str);
	bool HasErrors() const;
	std::string GetFullString() const;
	void Clear();
private:
	std::vector<std::string> _errors;
};

extern ErrorCollector g_errorCollector;

#endif
