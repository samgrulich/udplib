#include "receiver.h"
#include "message.h"
#include <cstring>

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

bool Reciever::matchHashes() {
    std::string hash = getHash(); 
    std::string incoming;
    incoming.reserve(32+1);
    incoming = buffer_+5;
    return hash == incoming;
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
    msg.resize(outLen);
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
    char *sizeBytes = new char [bufferLen_-start];
    memcpy(sizeBytes, buffer_+start, bufferLen_-start-1);
    size_ = atoi(sizeBytes);
    delete[] sizeBytes;
    return size_;
}

int Reciever::saveDataPayload() {
    if (!hasHeader(HeaderType::Data)) {
        return 0;
    }
    char buff[BUFFERS_LEN];
    int offset = 4+4; // to strip the header and position
    memcpy(buff, buffer_+offset, bufferLen_-offset);
    updateHash(buff, bufferLen_-offset-1);
    fstream_.write(buff, bufferLen_-offset-1);
    return bufferLen_-offset;
}

Pipe& Reciever::pipe() {
    return pipe_;
}
    
std::ofstream& Reciever::fstream() {
    return fstream_;
}
