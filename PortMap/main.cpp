#include <iostream>
#include "TcpRelay.h"
using namespace std;

int main(void) {
	CTcpRelay relay;
	relay.Start(2018, string("192.168.1.224"), 9999);
	getchar();
	return 0;
}