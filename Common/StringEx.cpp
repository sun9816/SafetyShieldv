#include "StringEx.h"

StringEx::StringEx() {
}

StringEx::~StringEx() {
}

std::string StringEx::MidStr(const std::string & szStr, const std::string & szLeft, const std::string & szRight) {
	size_t nBegPos = szStr.find(szLeft);
	if (nBegPos == std::string::npos) {
		return "";
	}
	nBegPos += szLeft.size();
	size_t nEndPos = szStr.find(szRight, nBegPos);
	if (nEndPos == std::string::npos) {
		return "";
	}
	return szStr.substr(nBegPos, nEndPos - nBegPos);
}

std::string StringEx::Relpace(const std::string & szStr, const std::string & szOld, const std::string & szNew) {
	std::string szRet = szStr;
	size_t nPos = 0;

	while ((nPos = szRet.find(szOld, nPos)) != std::string::npos) {
		szRet.replace(nPos, szOld.size(), szNew);
		nPos += szNew.size();
	}
	return szRet;
}

std::vector<std::string> StringEx::Split(const std::string & szStr, const std::string & szSep, size_t nMaxCount) {
	size_t pos = 0, offset = 0;
	std::vector<std::string> list;

	while ((pos = szStr.find(szSep, offset)) != std::string::npos && nMaxCount > 1) {
		list.push_back(std::move(std::string(szStr.data() + offset, pos - offset)));
		offset = pos + szSep.size();
		--nMaxCount;
	}
	if (offset <= szStr.size() && nMaxCount > 0) {
		list.push_back(std::move(std::string(szStr.data() + offset, szStr.size() - offset)));
	}

	return list;
}
