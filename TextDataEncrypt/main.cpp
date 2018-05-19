#include <iostream>
#include <fstream>
#include "../Common/TextEncryptor.h"
using namespace std;

int main(int argc, char *argv[]) {
	if (argc != 2) {
		cout << "��������ܵ��ļ���" << endl;
		return 0;
	}

	fstream fs;
	fs.open(argv[1], ios::in);
	if (fs.is_open()) {
		string inText;
		fs >> inText;

		cout << "[begin]";
		cout << TextEncryptor::Encrypt(inText);
		cout << "[end]";
		cout << endl;
	}
	else {
		cout << "�ļ���ȡʧ��, �����Ƿ���Ȩ��" << endl;
	}

	return 0;
}