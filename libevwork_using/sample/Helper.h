#ifndef _HELPER_H_
#define _HELPER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <map>
#include <sstream>
#include <time.h>
#include <math.h>
#include <algorithm>

class CHashResult
{
public:

	void addKV(const std::string& K, const std::string& V)
	{
		m_mapKV[K] = V;
	}

	/* 返回模板类型 */
	template <typename R>
	R getValue(const std::string& K, const R& def)
	{
		R r = def;

		std::map<std::string, std::string>::iterator iter = m_mapKV.find(K);
		if (iter == m_mapKV.end())
			return r;

		std::stringstream ss;
		ss << iter->second;
		ss >> r;
		return r;
	}

	std::string getValue(const std::string& K, const std::string& def)
	{
		std::string r = def;

		std::map<std::string, std::string>::iterator iter = m_mapKV.find(K);
		if (iter == m_mapKV.end())
			return r;

		return iter->second;
	}

private:
	std::map<std::string, std::string> m_mapKV;
};

inline int m_random(int start, int end)
{
	return start + rand() % (end - start + 1);
}

inline long long int getTickCount()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (((long long int)ts.tv_sec)*1000 + ts.tv_nsec/1000000);
}

inline bool willChangeTable()
{
	int rd = rand() % 100;
	// 30%概率换房
	return rd >= 40 && rd <= 70;
}

#endif
