#include <iostream>
#include "TcpRelay.h"
#include "Camouflage.h"

#include "../Common/TextEncryptor.h"
#include "../Common/StringEx.h"

using namespace std;

int main(int argc, char *argv[]) {
	if (argc != 2) {
		return 0;
	}

	string args = TextEncryptor::Decrypt(string(argv[1], strlen(argv[1])));
	auto strList = StringEx::Split(args, ";");
	if (strList.size() != 3) {
		return 0;
	}

	CTcpRelay relay;
	if (!relay.Start(strList[0])) {
		return 0;
	}

	auto addrList = StringEx::Split(strList[1], "|");
	for (auto & it : addrList) {
		auto addrInfoList = StringEx::Split(it, ":");
		if (addrInfoList.size() == 3) {
			try {
				relay.AddMap(stoi(addrInfoList[0]), addrInfoList[1], stoi(addrInfoList[2]));
			}
			catch(...) { }
		}
	}

	Camouflage camouflage;
	camouflage.Start(strList[2]);

	getchar();
	return 0;
}