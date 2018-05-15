#include <iostream>
#include "TcpRelay.h"
using namespace std;

int main(void) {
	CTcpRelay relay;
	relay.Start(9000, string("103.232.147.22"), 9000);
	relay.Start(8080, string("103.232.147.22"), 8080);
	getchar();
	return 0;
}