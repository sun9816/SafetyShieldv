#include <iostream>
#include "TcpRelay.h"
using namespace std;

int main(void) {
	CTcpRelay relay;
	relay.Start(9000, string("103.232.147.22"), 9000);
	getchar();
	return 0;
}