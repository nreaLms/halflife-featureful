#pragma once
#ifndef ICASE_COMPARE_H
#define ICASE_COMPARE_H

#include <cstring>
#include <string>

struct CaseInsensitiveCompare
{
	bool operator()(const std::string& lhs, const std::string& rhs) const noexcept
	{
		return stricmp(lhs.c_str(), rhs.c_str()) < 0;
	}
};

#endif
