#include "receiver.h"
#include "message.h"
#include <cstdio>
#include <cstring>
#include "common.h"

#include <iostream>

Reciever::Reciever(const char* remote_address, const int local_port, const int remote_port) 
    : size_(0), pipe_(remote_address, local_port, remote_port), bufferLen_(0)
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

int Reciever::recv() {
    bufferLen_ = pipe_.recv(buffer_);
    return bufferLen_;
}

bool Reciever::hasHeader(HeaderType type) {
    std::string label = getLabel(type);
    return memcmp(buffer_, &label[0], label.length()) == 0;
}

void Reciever::getPayloadString(std::string* out) {
    if (!hasHeader(HeaderType::Hash) && !hasHeader(HeaderType::Name)) {
        *out = "";
        return;
    }
    std::string msg = buffer_;
    int start = msg.find_first_of("=");
    int outLen = msg.length()-start;
    memcpy(&msg[0], &msg[start+1], msg.length());
    *out = msg;
    
    if (hasHeader(HeaderType::Name)) {
        name_ = *out;
        fstream_ = std::ofstream(name_, std::ios::binary);
    }
}

int Reciever::getPayloadInt() {
    if (!hasHeader(HeaderType::Size)) 
        return 0;
    int start = getSplitPos(HeaderType::Size);
    char sizeBytes[bufferLen_-start];
    memcpy(sizeBytes, buffer_+start, bufferLen_-start-1);
    size_ = atoi(sizeBytes);
    return size_;
}

int Reciever::saveDataPayload() {
    if (!hasHeader(HeaderType::Data)) {
        return 0;
    }
    char buff[BUFFERS_LEN];
    int offset = 4+4; // to strip the header and position
    memcpy(buff, buffer_+offset, bufferLen_-offset);
    fstream_.write(buff, bufferLen_-offset-1);
    return bufferLen_-offset;
}

// bool Reciever::recvFile() {
//     bool isOpen = true;
//     do { 
//         char buffer[BUFFERS_LEN];
//         int len = 0;
//         if ((len = pipe_.recv(buffer)) < 0)
//             continue;
//         if (isHeaderType(HeaderType::Data, buffer)) {
//             stripDataHeader(buffer);
//             // update md5 context
//             fstream_.write(buffer, len-4-4);
//             // fstream_ << buffer;
//         } else if (isHeaderType(HeaderType::Stop, buffer)) {
//             isOpen = false;
//         } else if (isHeaderType(HeaderType::Start, buffer)) {
//             // pass
//         }
//     } while(isOpen);
//     return true;
// }
//
// void Reciever::listen() {
//     size_t size = 0;
//     char name[BUFFERS_LEN];
//
//     // load name an size
//     char buffer[BUFFERS_LEN];
//     pipe_.recv(buffer);
//     if (isHeaderType(HeaderType::Name, buffer)) {
//         int start = getSplitPos(HeaderType::Name);
//         memcpy(name, buffer+start, BUFFERS_LEN-start);
//     } 
//     pipe_.recv(buffer);
//     if (isHeaderType(HeaderType::Size, buffer)) {
//         int start = getSplitPos(HeaderType::Size);
//         char sizeBytes[BUFFERS_LEN];
//         memcpy(sizeBytes, buffer+start, BUFFERS_LEN-start);
//         size = atoi(sizeBytes);
//     }
//
//     std::cout << "Filename: " << name << std::endl;
//     fstream_ = std::ofstream(name, std::ios::binary);
//     size_ = size;
// }

Pipe& Reciever::pipe() {
    return pipe_;
}
    
std::ofstream& Reciever::fstream() {
    return fstream_;
}
