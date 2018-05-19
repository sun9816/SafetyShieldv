#pragma once
#include <string>
#include <vector>
class StringEx {
public:
	StringEx();
	virtual ~StringEx();

	static std::string MidStr(const std::string & szStr, const std::string & szLeft, const std::string & szRight);
	static std::string Relpace(const std::string & szStr, const std::string & szOld, const std::string & szNew);
	static std::vector<std::string> Split(const std::string & szStr, const std::string & szSep, size_t nMaxCount = -1);
};

