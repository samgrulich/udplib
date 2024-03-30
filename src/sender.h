#pragma once 
#include <cstdint>
#include <fstream>
#include <string>

#include "message.h"
#include "pipe.h"

class Sender {
private:
    std::string name_;
    uint32_t size_;
    uint32_t position_;
    std::ifstream fstream_;
    Pipe pipe_;
public:
    Sender(const char* const name);
    void send(HeaderType type);
    void send(HeaderType type, int value);
    void send(HeaderType type, std::string value);
    void sendChunk();
    size_t size();
    bool eof();
};
