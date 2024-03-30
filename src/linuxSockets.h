#pragma once
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define InetPton(x, y, z) inet_pton(x, y, z)
#define closesocket(socket) close(socket)
