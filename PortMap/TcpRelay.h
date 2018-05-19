#pragma once
#include "../Common/Encryptor.h"

#include <thread>
#include <memory>
#include <string>
#include <list>
#include <atomic>
#include <mutex>

typedef unsigned int		SOCKET;

struct SSNodeInfo {
	SSNodeInfo() {
		m_nPort = 0;
	}
	SSNodeInfo(const std::string & szAddr, const std::string & szPort, const std::string & szPassword) {
		m_szAddress = szAddr;
		m_szPassword = szPassword;

		try {
			m_nPort = std::stoi(szPort);
		}
		catch(...){
			m_nPort = 0;
		}
	}
	std::string		m_szAddress;
	std::string		m_szPassword;
	short			m_nPort;
};

class CTcpRelay {
public:
	CTcpRelay();
	virtual ~CTcpRelay();

public:
	bool Start(const std::string & szSSUrl);
	bool AddMap(int nPort, const std::string &szServerAddr, int nServerPort);
	bool Stop();

protected:
	void ThreadListen(int nPort, const std::string &szServerAddr, int nServerPort);
	void ThreadRelay(SOCKET s, const std::string &szServerAddr, int nServerPort);
	SOCKET ConnectServer(const std::string & szSSAddr, int nSSPort, const std::string &szServerAddr, int nServerPort, CEncryptor &encryptor);
	bool GetSSNodeInfo(const std::string & szUrl);
	bool GetFristNode(SSNodeInfo & node);
private:
	bool			m_bStop;

	std::list<std::shared_ptr<std::thread>>	m_listThreadListenPtr;
	std::list<SSNodeInfo>					m_listSSNode;
	std::mutex								m_Lock;
	std::atomic<int>						m_nWorkerCount;
};

