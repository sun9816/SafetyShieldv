#include "Encryptor.h"

#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/md5.h>
#pragma comment(lib, "libeay32MTd.lib")

#define IV_LENGTH			16
#define KEY_LENGTH			32

CEncryptor::CEncryptor(const std::string &szPassword) {
	m_szPassword = szPassword;
}

CEncryptor::~CEncryptor() {
}

std::string CEncryptor::Encrypt(const std::string &data) {
	return DataAes_256_cfb(data, m_szEncipherIv, m_szPassword, m_nEncipherNum, AES_ENCRYPT);
}

std::string CEncryptor::Decrypt(const std::string &data) {
	return DataAes_256_cfb(data, m_szDecipherIv, m_szPassword, m_nDecipherNum, AES_DECRYPT);
}

void CEncryptor::InitEncipher(std::string &header) {
	m_nEncipherNum = 0;
	m_szEncipherIv.resize(IV_LENGTH, 0);
	RAND_bytes(
		  const_cast<unsigned char *>(reinterpret_cast<const unsigned char*>(m_szEncipherIv.data()))
		, m_szEncipherIv.size());

	header = m_szEncipherIv;
}

void CEncryptor::InitDecipher(const std::string &data, size_t &offset) {
	m_nDecipherNum = 0;
	m_szDecipherIv = data.substr(0, IV_LENGTH);
	offset = IV_LENGTH;
}

bool CEncryptor::IsInitEncipher() {
	return !m_szEncipherIv.empty();
}

bool CEncryptor::IsInitDecipher() {
	return !m_szDecipherIv.empty();
}

std::string CEncryptor::Md5Hash(const std::string &in) {
	std::string md(IV_LENGTH, 0);

	MD5(  reinterpret_cast<const unsigned char*>(in.data())
		, in.size()
		, const_cast<unsigned char *>(reinterpret_cast<const unsigned char*>(md.data())));

	return md;
}

std::string CEncryptor::EvpBytesToKey(const std::string	&szPassword) {
	std::vector<std::string> m;
	std::string data;
	int i = 0;

	while (m.size() < KEY_LENGTH + IV_LENGTH) {
		if (i == 0) {
			data = szPassword;
		}
		else {
			data = m[i - 1] + szPassword;
		}
		m.push_back(Md5Hash(data));
		++i;
	}

	std::string ms;
	for (auto &it : m) {
		ms += it;
	}

	return ms.substr(0, KEY_LENGTH);
}

std::string CEncryptor::DataAes_256_cfb(const std::string &in, std::string &iv, const std::string &password, int &nNum, int nEnc) {
	std::string strKey = EvpBytesToKey(password);
	//std::string strIv = iv;
	AES_KEY key = { 0 };

	if (AES_set_encrypt_key(reinterpret_cast<const unsigned char *>(strKey.data()), KEY_LENGTH*8, &key) < 0) {
		return "";
	}

	std::string out(in.size(), 0);
	AES_cfb128_encrypt(
		reinterpret_cast<const unsigned char *>(in.data())
		, const_cast<unsigned char *>(reinterpret_cast<const unsigned char *>(out.data()))
		, in.size()
		, &key
		, const_cast<unsigned char *>(reinterpret_cast<const unsigned char *>(iv.data()))
		, &nNum
		, nEnc);

	return out;
}
