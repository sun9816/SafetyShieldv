#include "TcpRelay.h"

#include "../Common/StringEx.h"
#include "../Common/TextEncryptor.h"

#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")

#define RELEASE_SOCKET(s)		if(s != INVALID_SOCKET){closesocket(s);s=INVALID_SOCKET;}

CTcpRelay::CTcpRelay() {
	m_bStop = true;
	m_nWorkerCount = 0;

	WSADATA wsaData = { 0 };
	WSAStartup(MAKEWORD(2, 2), &wsaData);

}

CTcpRelay::~CTcpRelay() {
	Stop();

	WSACleanup();
}

bool CTcpRelay::Start(const std::string & szSSUrl) {
	if (!m_bStop) {
		return false;
	}

	m_bStop = !GetSSNodeInfo(szSSUrl);
	return !m_bStop;
}

bool CTcpRelay::AddMap(int nPort, const std::string &szServerAddr, int nServerPort) {
	if (m_bStop) {
		return false;
	}

	m_listThreadListenPtr.push_back(std::make_shared<std::thread>(&CTcpRelay::ThreadListen, this, nPort, szServerAddr, nServerPort));
	
	return true;
}

bool CTcpRelay::Stop() {
	if (m_bStop) {
		return false;
	}

	m_bStop = true;
	for (auto & it : m_listThreadListenPtr) {
		it->join();
	}
	
	m_listThreadListenPtr.clear();
	m_nWorkerCount = 0;
	return true;
}

void CTcpRelay::ThreadListen(int nPort, const std::string &szServerAddr, int nServerPort) {
	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET clientSocket = INVALID_SOCKET;
	sockaddr_in listenAddr = { 0 }, acceptAddr = { 0 };
	FD_SET readSet = { 0 };
	timeval timeOut = { 1,0 };
	int nResult = 0,nAcceptAddrlen = 0;

	printf("CTcpRelay::ThreadListen Start LocalPort is %d\n", nPort);

	if ((listenSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		goto cleanup;
	}

	listenAddr.sin_family = AF_INET;
	//listenAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	inet_pton(AF_INET, "127.0.0.1", &listenAddr.sin_addr);
	listenAddr.sin_port = htons(nPort);
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

		//有客户端连接,创建一个线程
		nAcceptAddrlen = sizeof(acceptAddr);
		if ((clientSocket = accept(listenSocket, reinterpret_cast<sockaddr *>(&acceptAddr), &nAcceptAddrlen)) == SOCKET_ERROR) {
			printf("CTcpRelay::ThreadListen accept() failed error is %d", WSAGetLastError());
			continue;
		}

		if (m_nWorkerCount > 20) {
			printf("CTcpRelay::ThreadListen client count > 20\n");
			RELEASE_SOCKET(clientSocket);
			continue;
		}

		std::thread th(&CTcpRelay::ThreadRelay, this, clientSocket, szServerAddr, nServerPort);
		th.detach();
	}

cleanup:
	m_bStop = true;

	while (m_nWorkerCount != 0) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	RELEASE_SOCKET(listenSocket);
}

void CTcpRelay::ThreadRelay(SOCKET s, const std::string &szServerAddr, int nServerPort) {
	++m_nWorkerCount;
	//有客户端连接, 连接ss节点
	SOCKET ssSocket = INVALID_SOCKET, clientSocket = s;
	sockaddr_in ssAddr = { 0 };
	CEncryptor encryptor;
	FD_SET readSet = { 0 }, writeSet = { 0 };
	timeval timeOut = { 1,0 };
	std::string ssSendBuffer, clientSendBuffer;
	int nResult = 0;
	u_long iMode = 1;
	SSNodeInfo ssInfo;

	if (!GetFristNode(ssInfo)) {
		printf("CTcpRelay::ThreadRelay GetFristNode() failed error is list empty\n");
		goto cleanup;
	}

	encryptor.SetPassword(ssInfo.m_szPassword);
	if ((ssSocket = ConnectServer(ssInfo.m_szAddress, ssInfo.m_nPort, szServerAddr, nServerPort, encryptor)) == INVALID_SOCKET) {
		printf("CTcpRelay::ThreadRelay connect() failed error is %d\n", WSAGetLastError());
		goto cleanup;
	}

	//设置为异步
	ioctlsocket(clientSocket, FIONBIO, &iMode);
	ioctlsocket(ssSocket, FIONBIO, &iMode);
	//关闭延迟确认
	setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char *>(&iMode), sizeof(iMode));
	setsockopt(ssSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char *>(&iMode), sizeof(iMode));


	while (!m_bStop) {
		FD_ZERO(&readSet);
		FD_ZERO(&writeSet);
		FD_SET(ssSocket, &readSet);
		FD_SET(clientSocket, &readSet);
		if (!ssSendBuffer.empty()) {
			FD_SET(ssSocket, &writeSet);
		}
		if (!clientSendBuffer.empty()) {
			FD_SET(clientSocket, &writeSet);
		}

		if ((nResult = select(0, &readSet, &writeSet, nullptr, &timeOut)) == SOCKET_ERROR) {
			printf("CTcpRelay::ThreadRelay select() failed error is %d\n", WSAGetLastError());
			break;
		}
		if (nResult == 0) {
			continue;
		}

		std::string data(8 * 1024, 0);
		//判断是否远程段发送来数据
		if (FD_ISSET(ssSocket, &readSet)) {
			if ((nResult = recv(ssSocket, const_cast<char *>(data.data()), data.size(), 0)) <= 0) {
				break;
			}

			data.resize(nResult);
			if (!encryptor.IsInitDecipher()) {
				//初始化iv
				size_t nOffset = 0;
				encryptor.InitDecipher(data, nOffset);
				data.erase(0, nOffset);
			}

			clientSendBuffer.append(encryptor.Decrypt(data));
		}
		//判断是否本地发送来数据
		if (FD_ISSET(clientSocket, &readSet)) {
			if ((nResult = recv(clientSocket, const_cast<char *>(data.data()), data.size(), 0)) <= 0) {
				break;
			}
			data.resize(nResult);
			ssSendBuffer.append(encryptor.Encrypt(data));
		}
		//判断有没有可写数据
		if (FD_ISSET(ssSocket, &writeSet)) {
			if ((nResult = send(ssSocket, ssSendBuffer.data(), ssSendBuffer.size(), 0)) <= 0) {
				break;
			}
			ssSendBuffer.erase(0, nResult);
		}
		if (FD_ISSET(clientSocket, &writeSet)) {
			if ((nResult = send(clientSocket, clientSendBuffer.data(), clientSendBuffer.size(), 0)) <= 0) {
				break;
			}
			clientSendBuffer.erase(0, nResult);
		}

	}

cleanup:
	RELEASE_SOCKET(clientSocket);
	RELEASE_SOCKET(ssSocket);
	--m_nWorkerCount;
}

SOCKET CTcpRelay::ConnectServer(const std::string & szSSAddr, int nSSPort, const std::string & szServerAddr, int nServerPort, CEncryptor &encryptor) {
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
	inet_pton(AF_INET, szSSAddr.c_str(), &ssAddr.sin_addr);
	ssAddr.sin_port = htons(nSSPort);
	if (connect(ssSocket, reinterpret_cast<const sockaddr *>(&ssAddr), sizeof(ssAddr)) == SOCKET_ERROR) {
		printf("CTcpRelay::ConnectServer connect() failed error is %d\n", WSAGetLastError());
		goto cleanup;
	}
	
	//连接成功,发送命令使ss连接服务器
	
	inet_pton(AF_INET, szServerAddr.data(), &netAddr);
	netPort = htons(nServerPort);
	data.append(reinterpret_cast<const char *>(&netAddr), sizeof(netAddr));
	data.append(reinterpret_cast<const char *>(&netPort), sizeof(netPort));
	encryptor.InitEncipher(iv);

	data = iv + encryptor.Encrypt(data);
	if (send(ssSocket, data.data(), data.size(), 0) != data.size()) {
		printf("CTcpRelay::ConnectServer send() failed error is %d\n", WSAGetLastError());
		goto cleanup;
	}
	
	return ssSocket;

cleanup:
	RELEASE_SOCKET(ssSocket);
	return INVALID_SOCKET;
}

#import <WinHttpCom.dll> no_namespace
bool CTcpRelay::GetSSNodeInfo(const std::string & szUrl) {
	CoInitialize(nullptr);
	std::string retHtml;
	m_listSSNode.clear();

	try {
		IWinHttpRequestPtr httpPtr = nullptr;
		BSTR bstrBody;

		httpPtr.CreateInstance(__uuidof(WinHttpRequest));
		httpPtr->Open("GET", szUrl.c_str());
		httpPtr->Send();
		httpPtr->get_ResponseText(&bstrBody);
		retHtml = (_bstr_t)(bstrBody);
	}
	catch (...) { }

	
	retHtml = StringEx::MidStr(retHtml, "[begin]", "[end]");
	retHtml = StringEx::Relpace(retHtml, "<wbr>", "");
	if (!retHtml.empty()
		&& (retHtml = TextEncryptor::Decrypt(retHtml), !retHtml.empty())) {
		auto strList = StringEx::Split(retHtml, ";");
		for (auto & it : strList) {
			auto nodeList = StringEx::Split(it, "&");
			if (nodeList.size() == 3) {
				m_listSSNode.push_back(SSNodeInfo(nodeList[0], nodeList[1], nodeList[2]));
			}
		}
	}

 	CoUninitialize();
	return !m_listSSNode.empty();
}

bool CTcpRelay::GetFristNode(SSNodeInfo & node) {
	std::unique_lock<std::mutex> lock(m_Lock);
	if (m_listSSNode.empty()) {
		return false;
	}

	node = *m_listSSNode.begin();
	m_listSSNode.erase(m_listSSNode.begin());
	m_listSSNode.push_back(node);
	return true;
}
