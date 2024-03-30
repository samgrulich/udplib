#include "sender.h"
#include "common.h"
#include <cstring>
#include <filesystem>

Sender::Sender(const char* const name, const char* remote_address, const int local_port, const int remote_port) 
    :name_(name), position_(0), pipe_(remote_address, local_port, remote_port)
{
    fstream_ = std::ifstream(name, std::ios::binary);
    size_ = std::filesystem::file_size({name});
}

void Sender::send(HeaderType type) {
    if (type != HeaderType::Start && type != HeaderType::Stop)
        return;
    pipe_.send(getLabel(type));
}

void Sender::send(HeaderType type, int value) { 
    if (type != HeaderType::Size)
        return;
    std::stringstream msg;
    msg << getLabel(type) << "=" << value;
    pipe_.send(msg.str());
}

void Sender::send(HeaderType type, std::string value) {
    if (type != HeaderType::Name)
        return;

    std::stringstream msg;
    msg << getLabel(type) << "=" << value;
    pipe_.send(msg.str());
}

void Sender::sendChunk() {
    char buff[BUFFERS_LEN];

    // read file
    fstream_.read(buff, BUFFERS_LEN-1);
    
    // create message bytes to send
    const int len = strlen(buff);
    const int bytesLen = 4+4+len+1;
    char bytes[bytesLen];
    uint32_t pos = htons(position_);
    const char* label = getLabel(HeaderType::Data);

    memcpy(bytes, label, 4);
    memcpy(bytes+4, &pos, 4);
    memcpy(bytes+8, buff, len);

    pipe_.sendBytes(bytes, bytesLen);
    position_ = len;
}

size_t Sender::size() {
    return size_;
}

bool Sender::eof() {
    return fstream_.eof();
}
