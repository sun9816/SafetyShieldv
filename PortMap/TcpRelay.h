#pragma once
#include "Encryptor.h"

#include <thread>
#include <memory>
#include <string>
#include <list>
#include <atomic>

typedef unsigned int		SOCKET;

class CTcpRelay {
public:
	CTcpRelay();
	virtual ~CTcpRelay();

public:
	bool Start(int nPort, std::string &szServerAddr, int nServerPort);
	bool Stop();

protected:
	void ThreadListen();
	void ThreadRelay(SOCKET s);

	SOCKET ConnectServer(std::string &szServerAddr, int nServerPort, CEncryptor &encryptor);

private:
	int				m_nPort;
	bool			m_bStop;
	std::string		m_szServerAddr;
	int				m_nServerPort;

	std::shared_ptr<std::thread>	m_ThreadListenPtr;
	std::atomic<int>				m_nWorkerCount;
};

