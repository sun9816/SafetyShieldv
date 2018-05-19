#pragma once
#include <thread>
#include <memory>
#include <string>
#include <list>
#include <atomic>
#include <mutex>

#include <WinSock2.h>

class Camouflage {
public:
	Camouflage();
	virtual ~Camouflage();

public:
	bool Start(const std::string & szUrl);
	bool Stop();
protected:
	void ThreadConnect(sockaddr_in connectAddr);
	bool GetSSNodeInfo(const std::string & szUrl);

private:
	bool			m_bStop;

	std::list<std::string>						m_listCamouflageNode;
	std::list<std::shared_ptr<std::thread>>		m_listThreadListenPtr;
};

