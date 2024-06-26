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
    // create new sender
    Sender(const char* const name, const char* remote_address, const int local_port, const int remote_port);
    // send (with header types: start, stop)
    void send(HeaderType type);
    // send (with header types: size)
    void send(HeaderType type, int value);
    // send (with header types: name)
    void send(HeaderType type, std::string value);
    // reads file chunk, sends it and increments position
    void sendChunk();
    // get size of the file
    size_t size();
    // has the program reached end of file
    bool eof();
};
