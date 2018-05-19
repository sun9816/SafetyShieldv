#include "Camouflage.h"

#include "../Common/StringEx.h"
#include "../Common/TextEncryptor.h"

#include <WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")

#define RELEASE_SOCKET(s)		if(s != INVALID_SOCKET){closesocket(s);s=INVALID_SOCKET;}


Camouflage::Camouflage() {
	m_bStop = true;

	WSADATA wsaData = { 0 };
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

Camouflage::~Camouflage() {
	Stop();

	WSACleanup();
}

bool Camouflage::Start(const std::string & szUrl) {
	if (!m_bStop) {
		return false;
	}


	m_bStop = !GetSSNodeInfo(szUrl);

	if (!m_bStop) {
		for (auto & it : m_listCamouflageNode) {
			addrinfo addrInfo = { 0 }, *pAddrInfo = nullptr;
			addrInfo.ai_family = AF_INET;
			addrInfo.ai_socktype = SOCK_STREAM;

			if (getaddrinfo(it.c_str(), "https", &addrInfo, &pAddrInfo) == -1
				|| pAddrInfo == nullptr) {
				continue;
			}

			while (pAddrInfo != nullptr) {
				m_listThreadListenPtr.push_back(std::make_shared<std::thread>(&Camouflage::ThreadConnect
					, this
					, *reinterpret_cast<sockaddr_in *>(pAddrInfo->ai_addr)));

				pAddrInfo = pAddrInfo->ai_next;
			}
		}
	}

	return !m_bStop;
}

bool Camouflage::Stop() {
	if (m_bStop) {
		return false; 
	}

	m_bStop = true;

	for (auto & it : m_listThreadListenPtr) {
		it->join();
	}
	m_listCamouflageNode.clear();


	return true;
}

void Camouflage::ThreadConnect(sockaddr_in connectAddr) {
	SOCKET s = INVALID_SOCKET;

	while (!m_bStop) {
		SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
		if (s == INVALID_SOCKET) {
			break;
		}

		if (connect(s, reinterpret_cast<const sockaddr *>(&connectAddr), sizeof(connectAddr)) == SOCKET_ERROR) {
			break;
		}

		//…Ë÷√Œ™“Ï≤Ω
		u_long iMode = 1;
		ioctlsocket(s, FIONBIO, &iMode);

		while (!m_bStop) {
			char buffer[100] = { 0 };
			bool bIsBreak = false;
			int nRet = recv(s, buffer, sizeof(buffer), 0);

			switch (nRet) {
			case -1:
				if (WSAGetLastError() == WSAEWOULDBLOCK) {
					break;
				}
			default:
				bIsBreak = true;
				break;
			}

			if (bIsBreak) {
				break;
			}

			Sleep(1000);
		}

		Sleep(1000);
		RELEASE_SOCKET(s);
	}

	RELEASE_SOCKET(s);
}

#import <WinHttpCom.dll> no_namespace
bool Camouflage::GetSSNodeInfo(const std::string & szUrl) {
	CoInitialize(nullptr);
	std::string retHtml;
	m_listCamouflageNode.clear();

	try {
		IWinHttpRequestPtr httpPtr = nullptr;
		BSTR bstrBody;

		httpPtr.CreateInstance(__uuidof(WinHttpRequest));
		httpPtr->Open("GET", szUrl.c_str());
		httpPtr->Send();
		httpPtr->get_ResponseText(&bstrBody);
		retHtml = (_bstr_t)(bstrBody);
	}
	catch (...) {}


	retHtml = StringEx::MidStr(retHtml, "[begin]", "[end]");
	retHtml = StringEx::Relpace(retHtml, "<wbr>", "");
	if (!retHtml.empty()
		&& (retHtml = TextEncryptor::Decrypt(retHtml), !retHtml.empty())) {
		auto strList = StringEx::Split(retHtml, ";");
		for (auto & it : strList) {
			m_listCamouflageNode.push_back(it);
		}
	}

	CoUninitialize();
	return !m_listCamouflageNode.empty();
}