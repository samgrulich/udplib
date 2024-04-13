#include "pipe.h"
#include "common.h"
#include "message.h"

#include "CRC.h"

#include <cstdint>
#include <cstring>
#include <ios>
#include <iostream>

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
    char *newBytes = new char [len+8];
    memcpy(newBytes+4, &packetId_, 4);
    memcpy(newBytes+8, bytes, len);

    uint32_t crc = CRC::Calculate(newBytes+4, len+4, CRC::CRC_32());
    memcpy(newBytes, &crc, 4);
    // std::cout << "pipe: sendBytesCRC: crc: " << crc << std::endl;
    // std::cout << "pipe: sendBytesCRC: crclen: " << len+4 << std::endl;
    // std::cout << "pipe: sendBytesCRC: buffer: ";
    // for (int i = 4; i < len+4; i++) {
    //     printf("%x", bytes[i]);
    // }
    // std::cout << std::endl;
    return sendBytes(newBytes, len+8);
}

// private 
long Pipe::sendBytesCRC(const std::string& message) {
    return sendBytesCRC(message.c_str(), message.length()+1);
}

// private
long Pipe::recvBytes(char* buffer) {
    char buff[BUFFERS_LEN];

    long size = recvfrom(socket_, buff, BUFFERS_LEN, 0, (struct sockaddr*)&from_, (socklen_t*)&fromlen_);

    std::cout << "pipe: recvBytes: received size: " << size << std::endl;
    if (size != SOCKET_ERR)
        memcpy(buffer, buff, size);
    return size;
}

// private
long Pipe::recvBytesCRC(char* buffer) {
    long len = recvBytes(buffer);
    if (len == SOCKET_ERR)
        return SOCKET_ERR;

    // check crc
    uint32_t crc;
    memcpy(&crc, buffer, 4);
    // std::cout << "pipe: recvBytesCRC: CRC: " << crc << std::endl;
    // std::cout << "pipe: recvBytesCRC: CRClen: " << len-4 << std::endl;
    // std::cout << "pipe: recvBytesCRC: Calculated CRC: " << CRC::Calculate(buffer+4, len-4, CRC::CRC_32()) << std::endl;
    // std::cout << "pipe: recvBytesCRC: buffer:";
    // for (int i = 4; i < len; i++) {
    //     printf("%x", buffer[i]);
    // }
    // std::cout << std::endl;
    if (crc != CRC::Calculate(buffer+4, len-4, CRC::CRC_32()))
        return CRC_ERR;
    
    // check recvId
    int32_t packetId;
    memcpy(&packetId, buffer+4, 4);
    if (packetId < packetId_)
        return ID_ERR;

    if (packetId > packetId_)
        packetId_ = packetId;
   
    len = len-8;
    memcpy(buffer, buffer+8, len);
    return len;
}

long Pipe::send(const std::string& message) {
    return send(message.c_str(), message.length()+1);
}

long Pipe::send(const char* bytes, int len) {
    size_t sendLen; // bytes sent
    long responseLen; // bytes received
    char response[BUFFERS_LEN]; // response buffer
    packetId_++;
    std::cout << "pipe: send " << packetId_ << ", data: " << bytes << std::endl;
    do {
        // initial send
        sendLen = sendBytesCRC(bytes, len);
        responseLen = recvBytesCRC(response); 
        std::cout << "pipe: send: response: " << response << std::endl;
        while(responseLen < 0) { // resend
            std::cout << "pipe: send: error - resending errcode: " << responseLen << std::endl;
            sendLen = sendBytesCRC(bytes, len);
            responseLen = recvBytesCRC(response); 
        }
    } while(strcmp(response, (getLabel(HeaderType::Ack))) != 0);
    std::cout << "pipe: send: ack received, " << packetId_ << std::endl;
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
        std::cout << "pipe: recv: PacketId: " << packetId_ << std::endl;
        std::cout << "pipe: recv: Error(len): " << len << std::endl;
    } while(len < 0);

    // ack
    sendBytesCRC(getLabel(HeaderType::Ack));

    return len;
}

void Pipe::incrementPacketId() {
    packetId_++;
}
