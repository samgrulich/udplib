#include "pipe.h"
#include "common.h"
#include "message.h"

#include "CRC.h"

#include <cstdint>
#include <cstring>
#include <ios>
#include <iostream>
#include <thread>

#define SOCKET_ERR -1
#define ID_ERR -2
#define CRC_ERR -3


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
    setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_, sizeof(timeout_));
}

// private
long Pipe::sendBytes(const char *bytes, int len) {
    return sendto(socket_, bytes, len, 0, (struct sockaddr*)&dest_, sizeof(dest_)); 
}

// private
long Pipe::sendBytesCRC(const char *bytes, int len) {
    sendId_++;
    uint32_t crc = CRC::Calculate(bytes, len, CRC::CRC_32());
    char *newBytes = new char [len+8];
    memcpy(newBytes, &sendId_, 4);
    memcpy(newBytes+4, &crc, 4);
    memcpy(newBytes+8, bytes, len);
    return sendBytes(newBytes, len+8);
}

// private 
long Pipe::sendBytesCRC(const std::string& message) {
    return sendBytesCRC(message.c_str(), message.length()+1);
}

// private
long Pipe::recvBytes(char* buffer) {
    char buff[BUFFERS_LEN];

    long size = recvfrom(socket_, buff, BUFFERS_LEN, 0, (struct sockaddr*)&from_, (socklen_t*)&fromlen_);;

    std::cout << size << std::endl;
    if (size != SOCKET_ERR)
        memcpy(buffer, buff, size);
    return size;
}

// private
long Pipe::recvBytesCRC(char* buffer) {
    long size = recvBytes(buffer);
    if (size == SOCKET_ERR)
        return SOCKET_ERR;
    // check recvId
    uint32_t recvId;
    memcpy(&recvId, buffer, 4);
    if (recvId == recvId_)
        return ID_ERR;
    // possible packet loss
    recvId_ = recvId;

    // check crc
    uint32_t crc;
    memcpy(&crc, buffer+4, 4);
    size = size-8;
    memcpy(buffer, buffer+8, size);
    return crc == CRC::Calculate(buffer, size, CRC::CRC_32()) ? size : CRC_ERR;
}

long Pipe::send(const std::string& message) {
    return send(message.c_str(), message.length()+1);
}

long Pipe::send(const char* bytes, int len) {
    using namespace std::chrono_literals;

    size_t sendLen; // bytes sent
    size_t responseLen; // bytes received
    char response[BUFFERS_LEN]; // response buffer
    do {
        // initial send
        sendLen = sendBytesCRC(bytes, len);
        responseLen = recvBytesCRC(response); 
        while(responseLen < 0) { // resend
            sendLen = sendBytesCRC(bytes, len);
            responseLen = recvBytesCRC(response); 
        }
        std::cout << "Response: " << response << std::endl;
        std::cout << responseLen << std::endl;
    } while(strcmp(response, (getLabel(HeaderType::Ack))) != 0);
    set_timeout(); // set timout after first successful send (also sets for each following :))

    return sendLen;
}

long Pipe::recv(char* buffer) {
    long len;
    // unpack message
    do {
        len = recvBytesCRC(buffer);
        if (len == CRC_ERR) // not matching crc
            sendBytesCRC(getLabel(HeaderType::Error)); // send error
        std::cout << "Len: " << len << std::endl;
        std::cout << "Received: " << buffer << std::endl;
    } while(len < 0);

    // ack
    sendBytesCRC(getLabel(HeaderType::Ack));

    return len;
}
