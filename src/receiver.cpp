#include "receiver.h"
#include "message.h"
#include <fstream>

Receiver::Receiver(const char* const name) 
    : size_(0)
{
    fstream_ = std::ofstream();
}

// local funciton
bool isHeaderType(HeaderType type, std::string& msg) {
    return msg.rfind(getLabel(type), 0) == 0;
}

void Receiver::recvFile() {
    bool isOpen = true;
    do { 
        std::string buffer;
        if (pipe_.recv(&buffer) <= 0)
            continue;
        if (isHeaderType(HeaderType::Data, buffer)) {
            fstream_ << buffer;
        } else if (isHeaderType(HeaderType::Stop, buffer)) {
            isOpen = false;
        } else if (isHeaderType(HeaderType::Start, buffer)) {
            // pass
        }
    } while(isOpen);
}

void Receiver::listen() {
    size_t size = 0;
    std::string name;

    // load name an size
    do {
        std::string buffer;
        if (pipe_.recv(&buffer) <= 0)
            continue;

        printf("Something's here\n");
        if (isHeaderType(HeaderType::Name, buffer)) {
            int start = getSplitPos(HeaderType::Name);
            name = buffer.substr(start);
            printf("Name, %s\n", name.c_str());
        } else if (isHeaderType(HeaderType::Size, buffer)) {
            int start = getSplitPos(HeaderType::Size);
            size = atoi(buffer.substr(start).c_str());
            printf("Size, %ld\n", size);
        }
    } while (size == 0 && name.empty());

    fstream_ = std::ofstream(name, std::ios::binary);
    size_ = size;
    recvFile();
}
