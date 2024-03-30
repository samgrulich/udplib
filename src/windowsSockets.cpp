#include "windowsSockets.h"

void InitWinsock() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		perror("Socket errror");
	}
}