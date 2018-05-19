#include "TextEncryptor.h"
#include "Encryptor.h"

TextEncryptor::TextEncryptor() {
}

TextEncryptor::~TextEncryptor() {
}

std::string TextEncryptor::Encrypt(const std::string & data) {
	CEncryptor en("piaoxue");
	std::string header, outBuffer;

	en.InitEncipher(header);
	outBuffer = en.Encrypt(data);
	
	return en.Base64Encrypt(header + outBuffer);
}

std::string TextEncryptor::Decrypt(const std::string & data) {
	CEncryptor en("piaoxue");
	std::string outBuffer = data;
	size_t offset = 0;

	outBuffer = en.Base64Decrypt(outBuffer);
	en.InitDecipher(outBuffer, offset);
	return en.Decrypt(outBuffer.substr(offset));
}