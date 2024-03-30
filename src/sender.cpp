#include "sender.h"
#include "message.h"
#include "pipe.h"
#include <filesystem>
#include <sstream>

Sender::Sender(const char* const name) 
    :name_(name), position_(0)
{
    fstream_ = std::ifstream(name, std::ios::binary);
    size_ = std::filesystem::file_size({name});
}

void Sender::send(HeaderType type) {
    if (type != HeaderType::Start || type != HeaderType::Stop)
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
    std::stringstream msg;

    fstream_.read(buff, BUFFERS_LEN-1);
    uint32_t pos = htons(position_);
    char* position = (char*)&pos;

    msg << getLabel(HeaderType::Data) << position[0] << position[1] << position[2] << position[3];
    msg << buff;
    pipe_.send(msg.str());
}

size_t Sender::size() {
    return size_;
}

bool Sender::eof() {
    return fstream_.eof();
}
