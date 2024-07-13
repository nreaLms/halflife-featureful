#pragma once
#ifndef FOLLOWERS_H
#define FOLLOWERS_H

#include <vector>
#include <string>

class FollowersDescription
{
public:
	FollowersDescription(): fastRecruitRange(500.0f) {}
	bool ReadFromFile(const char* fileName);
	float FastRecruitRange() const {
		return fastRecruitRange;
	}
	std::vector<std::string>::const_iterator RecruitsBegin() {
		return fastRecruitMonsters.begin();
	}
	std::vector<std::string>::const_iterator RecruitsEnd() {
		return fastRecruitMonsters.cend();
	}
private:
	std::vector<std::string> fastRecruitMonsters;
	float fastRecruitRange;
};

extern FollowersDescription g_FollowersDescription;

void ReadFollowersDescription();

#endif
