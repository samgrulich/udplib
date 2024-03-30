#pragma once
#pragma comment(lib, "ws2_32.lib")
#include <SDKDDKVer.h>
#include <winsock2.h>
#include "ws2tcpip.h"

void InitWinsock();