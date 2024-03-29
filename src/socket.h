#pragma once 

#ifdef LINUX
#define SOCKETS "linuxSockets.h"
#else 
#define SOCKETS "windowsSockets.h"
#endif
#include SOCKETS
