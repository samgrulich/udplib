#pragma once
#ifdef LINUX 
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else 
#pragma comment(lib, "ws2_32.lib")
#include <SDKDDKVer.h>
#include <winsock2.h>
#include "ws2tcpip.h"

void InitWinsock(){
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}
#endif


