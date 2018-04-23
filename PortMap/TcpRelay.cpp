#include "TcpRelay.h"

#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")

#define RELEASE_SOCKET(s)		if(s == INVALID_SOCKET){closesocket(s);s=INVALID_SOCKET;}

CTcpRelay::CTcpRelay() {

	WSADATA wsaData = { 0 };
	WSAStartup(MAKEWORD(2, 2), &wsaData);

}

CTcpRelay::~CTcpRelay() {
	Stop();

	WSACleanup();
}

bool CTcpRelay::Start(int nPort, std::string &szServerAddr, int nServerPort) {
	if (!m_bStop) {
		return false;
	}
	m_nPort = nPort;
	m_szServerAddr = szServerAddr;
	m_nServerPort = nServerPort;
	m_bStop = false;
	m_nWorkerCount = 0;

	m_ThreadListenPtr = std::make_shared<std::thread>(&CTcpRelay::ThreadListen, this);

	return static_cast<bool>(m_ThreadListenPtr);
}

bool CTcpRelay::Stop() {
	if (m_bStop) {
		return false;
	}

	m_bStop = false;
	if (m_ThreadListenPtr) {
		m_ThreadListenPtr->join();
		m_ThreadListenPtr.reset();
	}

return true;
}

void CTcpRelay::ThreadListen() {
	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET clientSocket = INVALID_SOCKET;
	sockaddr_in listenAddr = { 0 }, acceptAddr = { 0 };
	FD_SET readSet = { 0 };
	timeval timeOut = { 1,0 };
	int nResult = 0,nAcceptAddrlen = 0;

	if ((listenSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		goto cleanup;
	}

	listenAddr.sin_family = AF_INET;
	listenAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	//inet_pton(AF_INET, "127.0.0.1", &listenAddr.sin_addr);
	listenAddr.sin_port = htons(m_nPort);
	if (bind(listenSocket, reinterpret_cast<const sockaddr *>(&listenAddr), sizeof(listenAddr)) == SOCKET_ERROR) {
		printf("CTcpRelay::ThreadListen bind() failed error is %d", WSAGetLastError());
		goto cleanup;
	}

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("CTcpRelay::ThreadListen listen() failed error is %d", WSAGetLastError());
		goto cleanup;
	}

	while (!m_bStop) {
		FD_ZERO(&readSet);
		FD_SET(listenSocket, &readSet);

		if ((nResult = select(0, &readSet, nullptr, nullptr, &timeOut)) == SOCKET_ERROR) {
			printf("CTcpRelay::ThreadListen select() failed error is %d", WSAGetLastError());
			break;
		}
		if (nResult == 0) {
			continue;
		}

		//�пͻ�������,����һ���߳�
		nAcceptAddrlen = sizeof(acceptAddr);
		if ((clientSocket = accept(listenSocket, reinterpret_cast<sockaddr *>(&acceptAddr), &nAcceptAddrlen)) == SOCKET_ERROR) {
			printf("CTcpRelay::ThreadListen accept() failed error is %d", WSAGetLastError());
			continue;
		}

		if (m_nWorkerCount > 20) {
			RELEASE_SOCKET(clientSocket);
			continue;
		}

		std::thread th(&CTcpRelay::ThreadRelay, this, clientSocket);
		th.detach();
	}

cleanup:
	m_bStop = true;

	while (m_nWorkerCount != 0) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	RELEASE_SOCKET(listenSocket);
}

void CTcpRelay::ThreadRelay(SOCKET s) {
	++m_nWorkerCount;
	//�пͻ�������, ����ss�ڵ�
	SOCKET ssSocket = INVALID_SOCKET, clientSocket = s;
	sockaddr_in ssAddr = { 0 };
	CEncryptor encryptor("qwertyuiop");
	FD_SET readSet = { 0 };
	timeval timeOut = { 1,0 };
	int nResult = 0;

	if ((ssSocket = ConnectServer(m_szServerAddr, m_nServerPort, encryptor)) == INVALID_SOCKET) {
		goto cleanup;
	}

	while (!m_bStop) {
		FD_ZERO(&readSet);
		FD_SET(ssSocket, &readSet);
		FD_SET(clientSocket, &readSet);

		if ((nResult = select(0, &readSet, nullptr, nullptr, &timeOut)) == SOCKET_ERROR) {
			printf("CTcpRelay::ThreadRelay select() failed error is %d", WSAGetLastError());
			break;
		}
		if (nResult == 0) {
			continue;
		}

		std::string data(8 * 1024, 0);
		//�ж��Ƿ�Զ�̶η���������
		if (FD_ISSET(ssSocket, &readSet)) {
			if ((nResult = recv(ssSocket, const_cast<char *>(data.data()), data.size(), 0)) <= 0) {
				break;
			}

			data.resize(nResult);
			if (!encryptor.IsInitDecipher()) {
				//��ʼ��iv
				size_t nOffset = 0;
				encryptor.InitDecipher(data, nOffset);
				data.erase(0, nOffset);
			}

			data = encryptor.Decrypt(data);
			if ((nResult = send(clientSocket, data.data(), data.size(), 0)) != data.size()) {
				break;
			}
		}
		//�ж��Ƿ񱾵ط���������
		if (FD_ISSET(clientSocket, &readSet)) {
			if ((nResult = recv(clientSocket, const_cast<char *>(data.data()), data.size(), 0)) <= 0) {
				break;
			}

			data.resize(nResult);
			data = encryptor.Encrypt(data);
			if ((nResult = send(ssSocket, data.data(), data.size(), 0)) != data.size()) {
				break;
			}
		}
	}

cleanup:
	RELEASE_SOCKET(clientSocket);
	RELEASE_SOCKET(ssSocket);
	--m_nWorkerCount;
}

SOCKET CTcpRelay::ConnectServer(std::string & nServerAddr, int nServerPort, CEncryptor &encryptor) {
	SOCKET ssSocket = INVALID_SOCKET;
	sockaddr_in ssAddr = { 0 };
	std::string data = { 0x01 };	//host is a 4-byte IPv4 address.
	std::string iv;
	int netAddr = 0;
	u_short netPort = 0;

	if ((ssSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		goto cleanup;
	}

	ssAddr.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &ssAddr.sin_addr);
	ssAddr.sin_port = htons(54321);
	if (connect(ssSocket, reinterpret_cast<const sockaddr *>(&ssAddr), sizeof(ssAddr)) == INVALID_SOCKET) {
		printf("CTcpRelay::ConnectServer connect() failed error is %d", WSAGetLastError());
		goto cleanup;
	}

	//���ӳɹ�,��������ʹss���ӷ�����
	const char *p = m_szServerAddr.data();
	inet_pton(AF_INET, m_szServerAddr.data(), &netAddr);
	netPort = htons(m_nServerPort);
	data.append(reinterpret_cast<const char *>(&netAddr), sizeof(netAddr));
	data.append(reinterpret_cast<const char *>(&netPort), sizeof(netAddr));
	encryptor.InitEncipher(iv);

	data = iv + encryptor.Encrypt(data);
	if (send(ssSocket, data.data(), data.size(), 0) != data.size()) {
		printf("CTcpRelay::ConnectServer send() failed error is %d", WSAGetLastError());
		goto cleanup;
	}

	return ssSocket;

cleanup:
	RELEASE_SOCKET(ssSocket);
	return INVALID_SOCKET;
}
