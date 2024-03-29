#pragma once
#include "pipe.h"
#include <fstream>

class Receiver {
private:
    std::ofstream fstream_;
    size_t size_;
    Pipe pipe_;
private:
    void recvFile();
public: 
    Receiver(const char* const name);
    void listen();
};
