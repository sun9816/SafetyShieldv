#pragma once
#include <string>
#include <vector>

class CEncryptor {
public:
	CEncryptor(const std::string &szPassword);
	virtual ~CEncryptor();

public:
	std::string Encrypt(const std::string &data);
	std::string Decrypt(const std::string &data);
	void InitEncipher(std::string &header);
	void InitDecipher(const std::string &data, size_t &offset);
	bool IsInitEncipher();
	bool IsInitDecipher();

private:
	std::string Md5Hash(const std::string &in);
	std::string EvpBytesToKey(const std::string	&szPassword);
	std::string DataAes_256_cfb(const std::string &in, std::string &iv, const std::string &password);
private:
	std::string			m_szPassword;
	std::string			m_szEncipherIv, m_szDecipherIv;

};

