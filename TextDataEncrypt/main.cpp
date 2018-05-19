#include <iostream>
#include <fstream>
#include "../Common/TextEncryptor.h"
using namespace std;

int main(int argc, char *argv[]) {
	if (argc != 2) {
		cout << "请输入加密的文件名" << endl;
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
		cout << "文件读取失败, 请检查是否有权限" << endl;
	}

	return 0;
}