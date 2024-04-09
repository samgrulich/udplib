#include "pipe.h"
#include "common.h"
#include "message.h"

#include "CRC.h"

#include <cstdint>
#include <cstring>
#include <thread>


Pipe::Pipe(const char* remote_address, const int local_port, const int remote_port) {
    from_.sin_port = htons(remote_port);
    fromlen_ = sizeof(struct sockaddr_in);
    local_.sin_family = AF_INET;
    local_.sin_port = htons(local_port);
    local_.sin_addr.s_addr = INADDR_ANY;

    socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (bind(socket_, (struct sockaddr*)&local_, sizeof(local_)) != 0) {
        perror("Binding error!");
        getchar(); //wait for press Enter
        throw "Couldn't bind socket to given port";
    }

    dest_.sin_family = AF_INET;
    dest_.sin_port = htons(remote_port);
    inet_pton(AF_INET, remote_address, &dest_.sin_addr);
}

Pipe::~Pipe() {
    closesocket(socket_);
}

void Pipe::set_timeout() {
    struct timeval timeout_ = {
        .tv_sec = 1,
    };
    setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, &timeout_, sizeof(timeout_));
}

// private
size_t Pipe::sendBytes(const char *bytes, int len) {
    return sendto(socket_, bytes, len, 0, (struct sockaddr*)&dest_, sizeof(dest_)); 
}

// private
size_t Pipe::recvBytes(char* buffer) {
    char buff[BUFFERS_LEN];

    size_t size = recvfrom(socket_, buff, BUFFERS_LEN, 0, (struct sockaddr*)&from_, (socklen_t*)&fromlen_);;

    if (size != -1)
        memcpy(buffer, buff, size);
    return size;
}

size_t Pipe::send(const std::string& message) {
    // return sendto(socket_, message.c_str(), message.length()+1, 0, (struct sockaddr*)&dest_, sizeof(dest_)); 
    return send(message.c_str(), message.length()+1);
}

size_t Pipe::send(const char* bytes, int len) {
    using namespace std::chrono_literals;

    uint32_t crc = CRC::Calculate(bytes, len, CRC::CRC_32());
    char *newBytes = new char [len+4];

    memcpy(newBytes, bytes, len);
    memcpy(newBytes+len, &crc, 4);

    // wait for response
    size_t sendLen; 
    char msg[BUFFERS_LEN];
    size_t msgLen;
    do {
        do {
            sendLen = sendBytes(newBytes, len+4);
            msgLen = recvBytes(msg)-1;
            if (msgLen == -1)
                std::this_thread::sleep_for(100ms);
        } while(msgLen == -1);
    } while(msgLen != strlen(getLabel(HeaderType::Ack)));
    set_timeout();

    return sendLen;
}

size_t Pipe::recv(char* buffer) {
    // unpack message
    size_t len = recvBytes(buffer)-4;
    uint32_t crc;
    memcpy(&crc, buffer+len, 4);

    // verify crc
    while(crc != CRC::Calculate(buffer, len, CRC::CRC_32())) {
        errors_++;
        std::string msg = getLabel(HeaderType::Error);
        sendBytes(msg.c_str(), msg.length()+1);
        // unpack new mesasge
        len = recvBytes(buffer)-4;
        memcpy(&crc, buffer+len, 4);
    } 

    // ack
    std::string msg = getLabel(HeaderType::Ack);
    sendBytes(msg.c_str(), msg.length()+1);

    return len;
}
