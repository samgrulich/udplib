#include "receiver.h"
#include "message.h"
#include <cstdio>
#include <cstring>
#include "common.h"

#include <iostream>

Receiver::Receiver(const char* remote_address, const int local_port, const int remote_port) 
    : size_(0), pipe_(remote_address, local_port, remote_port)
{
    fstream_ = std::ofstream();
}

// local funciton
bool isHeaderType(HeaderType type, char* msg) {
    return memcmp(msg, getLabel(type), 4) == 0;
}

void stripDataHeader(char* msg) {
    memcpy(msg, msg+4+4, BUFFERS_LEN-4-4);
}

void Receiver::recvFile() {
    bool isOpen = true;
    do { 
        char buffer[BUFFERS_LEN];
        int len = 0;
        if ((len = pipe_.recv(buffer)) < 0)
            continue;
        if (isHeaderType(HeaderType::Data, buffer)) {
            stripDataHeader(buffer);
            fstream_.write(buffer, len-4-4);
            // fstream_ << buffer;
        } else if (isHeaderType(HeaderType::Stop, buffer)) {
            isOpen = false;
        } else if (isHeaderType(HeaderType::Start, buffer)) {
            // pass
        }
    } while(isOpen);
}

void Receiver::listen() {
    size_t size = 0;
    char name[BUFFERS_LEN];

    // load name an size
    char buffer[BUFFERS_LEN];
    pipe_.recv(buffer);
    if (isHeaderType(HeaderType::Name, buffer)) {
        int start = getSplitPos(HeaderType::Name);
        memcpy(name, buffer+start, BUFFERS_LEN-start);
    } 
    pipe_.recv(buffer);
    if (isHeaderType(HeaderType::Size, buffer)) {
        int start = getSplitPos(HeaderType::Size);
        char sizeBytes[BUFFERS_LEN];
        memcpy(sizeBytes, buffer+start, BUFFERS_LEN-start);
        size = atoi(sizeBytes);
    }

    std::cout << "Filename: " << name << std::endl;
    fstream_ = std::ofstream(name, std::ios::binary);
    size_ = size;
    recvFile();
}
