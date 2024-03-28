#pragma once
#pragma comment(lib, "ws2_32.lib")
#include <SDKDDKVer.h>
#include <winsock2.h>
#include "ws2tcpip.h"

#define close(x) closesocket(x)

void InitWinsock() {
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}