#include "newpipe.h"

#include <cstdio>
#include <cstring>

#include "bytes.h"
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
    do {
        res = sendto(socket_, newBytes, len+8, 0, (struct sockaddr*)&dest_, sizeof(dest_));
    } while (res == TIMEOUT);
    delete[] newBytes;
    return res;
}

long NewPipe::recvBytes(unsigned char* buffer, int32_t& packetId) {
    char buff[BUFFERS_LEN];
    long len;
    do {
        len = recvfrom(socket_, buff, BUFFERS_LEN, 0, (struct sockaddr*)&from_, (socklen_t*)&fromlen_);
    } while(len == TIMEOUT);
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
        reqLen = sendBytes(bytes, len, packet_);
        long resLen = recvBytes(res, resId);
        if (resLen == INVALID_CRC || resLen == INVALID_PACKET) {
            continue;
        }
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
            toRecv_[resId] = Bytes(res, resLen);
            if (incoming_+1 == resId) {
                incoming_++;
            }
        } 
    } while (res[0] != Ack || resId < ack_+1);
    ack_++;

    // req sent and ack received
    return reqLen;
}

long NewPipe::recv(unsigned char* buffer) {
    int32_t packetId;
    long reqLen; 
    ack_++;
    do {
        reqLen = recvBytes(buffer, packetId);
    } while (reqLen == INVALID_CRC || reqLen == INVALID_PACKET);

    sendHeader(Ack, ack_);

    if (packetId == incoming_+1) {
        incoming_++;
    }
    return reqLen;
}

long NewPipe::submit(const unsigned char* bytes, int len) {
    toSend_[submited_++] = Bytes(bytes, len);
    return len;
}

long NewPipe::next(unsigned char* buffer) {
    if (toRecv_.find(loaded_+1) == toRecv_.end()) {
        return -1;
    }
    loaded_++;
    long len = toRecv_[loaded_].len;
    memcpy(buffer, toRecv_[loaded_].bytes, len);
    toRecv_.erase(loaded_);
    return len;
}

