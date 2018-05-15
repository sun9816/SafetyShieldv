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
	bool Start(int nPort, const std::string &szServerAddr, int nServerPort);
	bool Stop();

protected:
	void ThreadListen(int nPort, const std::string &szServerAddr, int nServerPort);
	void ThreadRelay(SOCKET s, const std::string &szServerAddr, int nServerPort);

	SOCKET ConnectServer(const std::string & szSSAddr, int nSSPort, const std::string &szServerAddr, int nServerPort, CEncryptor &encryptor);

private:
	bool			m_bStop;

	std::list<std::shared_ptr<std::thread>>	m_listThreadListenPtr;
	std::atomic<int>						m_nWorkerCount;
};

