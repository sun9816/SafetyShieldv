#pragma once
#include <string>

class TextEncryptor {
public:
	TextEncryptor();
	virtual ~TextEncryptor();

	static std::string Encrypt(const std::string & data);
	static std::string Decrypt(const std::string & data);

};

