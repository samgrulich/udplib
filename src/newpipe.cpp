#include "newpipe.h"

#include <cstdio>
#include <cstring>
#include <iostream>

#include "CRC.h"
#include "common.h"
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
        .tv_sec = 0,
        .tv_usec = 250000
    };
    int res = setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_, sizeof(timeout_));
#ifndef LINUX
    if (res == SOCKET_ERROR) {
        printf("Setsockopt failed\n");
    }
#endif
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
    res = sendto(socket_, (const char*)newBytes, len+8, 0, (struct sockaddr*)&dest_, sizeof(dest_));
    delete[] newBytes;
    return res;
}

long NewPipe::sendBytes(Bytes bytes, int32_t packetId) {
    return sendBytes(bytes.bytes, bytes.len, packetId);
}

long NewPipe::recvBytes(unsigned char* buffer, int32_t& packetId) {
#ifndef LINUX
    struct timeval timeout_ = {
        .tv_sec = 1,
    };

    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(socket_, &readSet);
    int result = select(0, &readSet, nullptr, nullptr, &timeout_);
    if (result == SOCKET_ERROR) {
        std::cerr << "select failed:" << WSAGetLastError() << std::endl;
    }
    else if (result == 0) {
        return TIMEOUT;
    }
#endif
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
            std::cout << "send: missing ack: " << resId << std::endl;
            // resend ack
            // todo: add check if it is "future" packet request resend of the previous
            sendHeader(Ack, resId);
        } else if (res[0] == MissingPacket) {
            // resend packet
            std::cout << "send: missing packet: " << resId << std::endl;
            Bytes toSend = toSend_[resId];
            sendBytes(toSend.bytes, toSend.len, resId);
        } else if (res[0] != Ack && res[0] != Error) {
            std::cout << "send: invalid response: " << res[0] << std::endl;
            // problem here
            if (resId < 0) 
                continue;
            sendHeader(MissingAck, resId);
            resLen = recvBytes(res, resId);
        } 
    } while (res[0] != Ack || resId < packet_);

    // req sent and ack received
    return reqLen;
}

long NewPipe::sendBatch(int start, int stop) {
    unsigned char res[BUFFERS_LEN];
    long reqLen;
    int32_t resId = -1;
    int startPacket = start, stopPacket = stop;

    packet_ += WINDOW_SIZE;
    do {
        long resLen;
        do {
            for (int i = startPacket; i < stopPacket; i++) {
                Bytes msg = toSend_[i];
                reqLen = sendBytes(msg, start);
            } 
            resLen = recvBytes(res, resId);
        } while (resLen < 0); // loop for resending dropped windows (or acks)
        if (res[0] == MissingAck) {
            std::cout << "send: missing ack: " << resId << std::endl;
            // todo: add check if it is "future" packet request resend of the previous
            // resend ack
            sendHeader(Ack, resId);
        } else if (res[0] == MissingPacket) {
            // resend packet
            std::cout << "send: missing packet: " << resId << std::endl;
            Bytes toSend = toSend_[resId];
            sendBytes(toSend, resId);
        } else if (res[0] != Ack && res[0] != Error) {
            std::cout << "send: invalid response: " << res[0] << std::endl;
            // problem here
            if (resId < 0) 
                continue;
            sendHeader(MissingAck, resId);
            resLen = recvBytes(res, resId);
        } 
        if (startPacket < resId < stopPacket) {
            startPacket = resId;
        }
        // if resId > stopPacket somethings wrong
    } while (resId != stopPacket); // exit only if came in ack with the stopPacket

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

long NewPipe::recvBatch() {
    int32_t packetId;
    long reqLen; 
    int missingPackets = WINDOW_SIZE;

    do {
        for (int i = 0; i < missingPackets; i++) {
            unsigned char buffer[BUFFERS_LEN];
            reqLen = recvBytes(buffer, packetId);
            if (reqLen == TIMEOUT) {
                // send ack of last received packet
                break;
            } else if (reqLen == INVALID_CRC) {
                // drop packet
                break;
            } else if (packetId != ack_+5) { // todo: correct this formula to check if the packet is within current window
                toRecv_[packetId] = Bytes(buffer, reqLen);
                missingPackets--;
                ack_++;
                // imcoming++ ?
            }
        }
        sendHeader(Ack, ack_);
    } while (missingPackets != 0);
    std::cout << "recv: len: " << reqLen << ", isTimeout: " << (reqLen == TIMEOUT) << std::endl;
    std::cout << "recv: ack: " << ack_ << std::endl;
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
    toSend_[submited_] = Bytes(bytes, len);
    if (submited_ % WINDOW_SIZE && submited_ != 0)
        sendBatch(submited_ - WINDOW_SIZE, submited_);
    submited_++;
    return len;
}

long NewPipe::next(unsigned char* buffer) {
    if (loaded_ % WINDOW_SIZE == 0)
        recvBatch();
    if (toRecv_.find(loaded_+1) == toRecv_.end()) {
        return -1;
    }
    loaded_++;
    std::cout << "next: loaded: " << loaded_ << std::endl;
    long len = toRecv_[loaded_].len;
    memcpy(buffer, toRecv_[loaded_].bytes, len);
    return len;
}

