#include "newpipe.h"

#include <cstdio>
#include <cstring>
#include <iostream>

#include "CRC.h"
#include "message.h"

#define SOCKET_ERR -1
#define TIMEOUT -1
#define INVALID_PACKET -2
#define INVALID_CRC -3


NewPipe::NewPipe(const char* remote_address, const int local_port, const int remote_port) 
    : toSend_(), toRecv_()
{
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
    
    // activate the timeout
    struct timeval timeout_ = {
        .tv_sec = 1,
    };
    setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_, sizeof(timeout_));
}

NewPipe::~NewPipe() {
    closesocket(socket_);
}

long NewPipe::sendBytes(const unsigned char* bytes, int len, int32_t packetId) {
    unsigned char *newBytes = new unsigned char [len+8];
    memcpy(newBytes+4, &packetId, 4);
    memcpy(newBytes+8, bytes, len); 

    uint32_t crc = CRC::Calculate(newBytes+4, len+4, CRC::CRC_32());
    memcpy(newBytes, &crc, 4);
    long res;
    res = sendto(socket_, newBytes, len+8, 0, (struct sockaddr*)&dest_, sizeof(dest_));
    delete[] newBytes;
    return res;
}

long NewPipe::recvBytes(unsigned char* buffer, int32_t& packetId) {
    char buff[BUFFERS_LEN];
    long len;
    len = recvfrom(socket_, buff, BUFFERS_LEN, 0, (struct sockaddr*)&from_, (socklen_t*)&fromlen_);
    std::cout << "recvBytes: len: " << len << std::endl;
    if (len == TIMEOUT) {
        return TIMEOUT;
    }
    if (len < 8) {
        return INVALID_PACKET;
    }
    uint32_t crc = CRC::Calculate(buff+4, len-4, CRC::CRC_32());
    uint32_t crc2;
    memcpy(&crc2, buff, 4);
    if (crc != crc2) {
        return INVALID_CRC;
    }
    memcpy(&packetId, buff+4, 4);
    memcpy(buffer, buff+8, len-8);
    return len-8;
}

void NewPipe::sendHeader(const unsigned char header, int32_t packetId) {
    unsigned char bytes[1] = {header};
    sendBytes(bytes, 1, packetId);
}

long NewPipe::send(const unsigned char* bytes, int len) {
    unsigned char res[BUFFERS_LEN];
    long reqLen;
    int32_t resId = -1;

    packet_++;
    do {
        long resLen;
        do {
            reqLen = sendBytes(bytes, len, packet_);
            resLen = recvBytes(res, resId);
        } while(resLen < 0);
        if (res[0] == MissingAck) {
            // resend ack
            // todo: add check if it is "future" packet request resend of the previous
            sendHeader(Ack, resId);
        } else if (res[0] == MissingPacket) {
            // resend packet
            Bytes toSend = toSend_[resId];
            sendBytes(toSend.bytes, toSend.len, resId);
        } else if (res[0] != Ack && res[0] != Error) {
            if (resId < 0) 
                continue;
            sendHeader(MissingAck, resId);
        } 
    } while (res[0] != Ack || resId < packet_);

    // req sent and ack received
    return reqLen;
}

long NewPipe::recv(unsigned char* buffer) {
    int32_t packetId;
    long reqLen; 
    do {
        reqLen = recvBytes(buffer, packetId);
        if (reqLen < 0) {
            std::cout << "recv: invalid packet: " << reqLen << std::endl;
            continue;
        } else if (packetId < ack_+1) {
            std::cout << "recv: repeated packet: " << packetId << std::endl;
            sendHeader(Ack, packetId);
        } else if (packetId > ack_+1) {
            toRecv_[packetId] = Bytes(buffer, reqLen);
            std::cout << "recv: missing packet: " << packetId << std::endl;
            sendHeader(MissingPacket, ack_+1);
        } // else  packetId == ack+1
    } while (packetId != ack_+1 || reqLen == INVALID_CRC || reqLen == INVALID_PACKET || reqLen == TIMEOUT);
    std::cout << reqLen << ", " << (reqLen == TIMEOUT) << std::endl;

    ack_++;
    std::cout << "recv: ack: " << ack_ << std::endl;
    sendHeader(Ack, ack_);

    toRecv_[packetId] = Bytes(buffer, reqLen);
    if (packetId == incoming_+1) {
        incoming_++;
    }
    std::cout << "recv: incoming: " << incoming_ << std::endl;
    return reqLen;
}

long NewPipe::submitHeader(const unsigned char header) {
    return submit(&header, 1);
}

long NewPipe::submit(HeaderType header, unsigned char* bytes, int len) {
    return submit(header, (const unsigned char*)bytes, len);
}


long NewPipe::submit(HeaderType header, const unsigned char* bytes, int len) {
    unsigned char* newBytes = new unsigned char[len+1];
    newBytes[0] = header;
    memcpy(newBytes+1, bytes, len);
    long res = submit(newBytes, len+1);
    delete[] newBytes;
    return res;
}

long NewPipe::submit(const unsigned char* bytes, int len) {
    toSend_[submited_++] = Bytes(bytes, len);
    long res = send(bytes, len);
    return res;
    return len;
}

long NewPipe::next(unsigned char* buffer) {
    long res = recv(buffer);
    if (toRecv_.find(loaded_+1) == toRecv_.end()) {
        return -1;
    }
    loaded_++;
    std::cout << "next: loaded: " << loaded_ << std::endl;
    long len = toRecv_[loaded_].len;
    memcpy(buffer, toRecv_[loaded_].bytes, len);
    // toRecv_.erase(loaded_);
    return res;
    return len;
}

