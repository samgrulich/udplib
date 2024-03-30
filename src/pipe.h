#pragma once 
#include <string>

#include "socket.h"

#define TARGET_IP   "127.0.0.1"

#ifdef SENDER
#define TARGET_PORT 5001
#define LOCAL_PORT 5002
#endif // SENDER

#ifdef RECEIVER
#define TARGET_PORT 5002 
#define LOCAL_PORT 5001 
#endif // RECEIVER

#define BUFFERS_LEN 1024

class Pipe {
private:
    int socket_;
    int fromlen_;
    struct sockaddr_in local_;
    struct sockaddr_in dest_;
    struct sockaddr_in from_;
public:
    Pipe();
    size_t send(const std::string& message);
    size_t sendBytes(const char* bytes, int len);
    size_t recv(char* buffer);
};
