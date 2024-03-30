#pragma once
#include "pipe.h"
#include <fstream>

class Receiver {
private:
    std::ofstream fstream_;
    size_t size_;
    Pipe pipe_;
private:
    // receive file
    void recvFile();
public: 
    // create new receiver
    Receiver(const char* remote_address, const int local_port, const int remote_port);
    // listen for incoming packets
    void listen();
};
