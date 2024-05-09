#include "newpipe.h"

#include <cstdio>
#include <cstring>
#include <ios>
#include <iostream>
#include <list>
#include <map>

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
    // std::cout << "recvBytes: len: " << len << std::endl;
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
    unsigned char bytes[3] = {0, 1, header};
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

void NewPipe::sendPositiveAck(int incomingWindowSize, int packetId) {
    unsigned char* buffer = new unsigned char[incomingWindowSize+3];
    buffer[0] = 0;
    buffer[1] = 1;
    buffer[2] = Ack;
    for (int i = 0; i < incomingWindowSize; i++) {
        buffer[i + 3] = 1;
    }
    sendBytes(buffer, incomingWindowSize+3, packetId);
}

long NewPipe::sendBatch(int start, int stop) {
    unsigned char res[BUFFERS_LEN];
    long reqLen = 0;
    int32_t resId = -1;
    int startPacket = start, stopPacket = stop;

    // window creation
    int remainingPacketCount = stop - start;
    std::map<int, bool> remPackets;
    std::list<Bytes> toSend;
    int windowSize = stop - start;
    for (int i = 0; i < windowSize; i++) {
        int packetId = start + i;
        Bytes packet = toSend_[packetId];
        unsigned char buffer[BUFFERS_LEN]; // todo: possible error due to big bytes
        memcpy(buffer+2, packet.bytes, packet.len);
        buffer[0] = i;
        buffer[1] = windowSize;
        remPackets[packetId] = true;
        toSend.push_back(Bytes(buffer, packet.len+2));
    }

    do {
        long resLen;
        do {
            int i = 0;
            for (Bytes& packet : toSend) {
                int packetId = start + i;
                if (remPackets[packetId]) { // packet has not been received yet, send it
                    long reqLen = sendBytes(packet, packetId);
                }
                i++; 
            }
            resLen = recvBytes(res, resId); 
            if (resLen < 0) {
                std::cerr << "send: received invalid Ack packet: " << resId << std::endl;
            }
        } while(resLen < 0); // no or wrong response received 
        if (res[2] == Ack) { // at least some packets received
            if (resId == start) { // is ack for current window?
                for (int i = 0; i < windowSize; i++) {
                    int packetId = start + i;
                    if (remPackets[packetId] == false) 
                        continue;
                    // todo: possible to add a layer of protection by checking if the packetId is in the map 
                    bool received = res[3+i] == 1 ? true : false;
                    remPackets[packetId] = !received; // if res == 1 the packet has been received
                    if (received)
                        remainingPacketCount--;
                }
                std::cout << "send: received ack, remainingPackets: " << remainingPacketCount << std::endl;
            } else {
                if (resId > start) {
                    std::cout << "send: received future ack - returning; " << remainingPacketCount << std::endl;
                    return reqLen;
                } else { // resId < start
                    std::cerr << "send: repeated ack: " << resId << std::endl;
                    // sendHeader(MissingAck, resId);
                    resLen = recvBytes(res, resId);
                }
            }
        } else if (res[2] == MissingAck) {
            std::cerr << "send: Sending missing ack: " << resId << std::endl;
            // todo: add check if it is "future" packet request resend of the previous
            // resend ack
            sendHeader(Ack, resId);
        } else if (res[2] == MissingPacket) {
            // resend packet
            std::cerr << "send: Resending missing packet: " << resId << std::endl;
            Bytes toSend = toSend_[resId];
            unsigned char buffer[BUFFERS_LEN]; 
            long resLen;
            do {
                sendBytes(toSend, resId); 
                resLen = recvBytes(res, resId); 
                // todo: check if this is the right ack
            } while(resLen < 0 || res[2] != Ack); // waiting for ack back
        } else if (res[2] != Ack && res[2] != Error) {
            // problem with transfer (the other one is stuck sending)
            std::cerr << "send: response is not ack, neither error: " << (int)(res[2]) << std::endl;
            if (resId < 0) 
                continue;
            if (resId <= incoming_) {
                sendPositiveAck(res[1], resId-res[0]); // send ack
                resLen = recvBytes(res, resId);
            }
            // sendHeader(MissingAck, resId);
            // resLen = recvBytes(res, resId);
        } 
    } while(remainingPacketCount > 0); // exit only if all packets have been received
    packet_ += stop - start;
    
    // req sent and ack received
    return reqLen;
}

long NewPipe::recv(unsigned char* buffer) {
    int32_t packetId;
    long reqLen; 
    do {
        reqLen = recvBytes(buffer, packetId);
        if (reqLen < 0) {
            std::cerr << "recv: invalid packet: " << reqLen << std::endl;
            continue;
        } else if (packetId < ack_+1) {
            std::cerr << "recv: repeated packet: " << packetId << std::endl;
            sendHeader(Ack, packetId);
        } else if (packetId > ack_+1) {
            toRecv_[packetId] = Bytes(buffer, reqLen);
            std::cerr << "recv: missing packet: " << packetId << std::endl;
            sendHeader(MissingPacket, ack_+1);
        } // else  packetId == ack+1
    } while (packetId != ack_+1 || reqLen == INVALID_CRC || reqLen == INVALID_PACKET || reqLen == TIMEOUT);

    ack_++;
    sendHeader(Ack, ack_);

    toRecv_[packetId] = Bytes(buffer, reqLen);
    if (packetId == incoming_+1) {
        incoming_++;
    }
    return reqLen;
}

long NewPipe::recvBatch(bool isFirst) {
    int32_t packetId;
    long reqLen; 

    // packet info catcher
    unsigned char infoBuffer[BUFFERS_LEN];
    int start, windowSize, i = WINDOW_SIZE;
    do {
        bool toReceive = true;
        do {
            reqLen = recvBytes(infoBuffer, packetId);
            toReceive = reqLen < 0 || infoBuffer[2] == Ack;
            if (!isFirst) i--;
        } while (toReceive && i != 0); // packets either didn't arrive or are invalid
        if (toReceive) { // both receiver and sender are receiving
            std::cerr << "recvBatch: double receive detected: " << packetId << std::endl;
            if (packet_ >= WINDOW_SIZE-1) {
                for (int i = packet_-WINDOW_SIZE+1; i < packet_+1; i++) {
                    long reqLen = sendBytes(toSend_[i], packetId);
                }
                recvBytes(infoBuffer, start); // just dump the result
            }
            i = WINDOW_SIZE;
            continue;
        }
        start = packetId - infoBuffer[0];
        windowSize = infoBuffer[1];
        if (start < incoming_) { // send ack for repeated packet
            std::cerr << "recvBatch: repeated packet: " << packetId << std::endl;
            i = WINDOW_SIZE;
            sendPositiveAck(windowSize, start);
        }
    } while (start < incoming_); // in case of missing ack from previous batch 
                // (meaning incoming packets and window have been already processed)
    toRecv_[packetId] = Bytes(infoBuffer+2, reqLen-2); // store the packet (without window information)
    int remainingPacketCount = windowSize - 1;

    bool* receivedPackets = new bool[windowSize];

    for (int i = 0; i < windowSize; i++) {
        receivedPackets[i] = false;
    }

    receivedPackets[infoBuffer[0]] = true;

    int remPacketCount = remainingPacketCount;
    do {
        for (int i = 0; i < remPacketCount; i++) { // listen for remaining packets
            unsigned char reqBuffer[BUFFERS_LEN];
            reqLen = recvBytes(reqBuffer, packetId);
            if (receivedPackets[reqBuffer[0]]) {
                continue;
            }
            if (reqLen < 0) {
                if (reqLen == TIMEOUT) {
                    std::cerr << "recvBatch: timeout: " << packetId << std::endl;
                    break;
                }
                std::cerr << "recvBatch: invalid packet, skipping: " << packetId << std::endl;
                continue;
            }
            // check if data incoming has wrong header
            toRecv_[packetId] = Bytes(reqBuffer+2, reqLen-2); // store the packet (without window information)
            receivedPackets[reqBuffer[0]] = true; 
            remainingPacketCount--;
        }
        // send ack
        unsigned char* buffer = new unsigned char[windowSize + 3];

        buffer[0] = 0;
        buffer[1] = 1;
        buffer[2] = Ack;
        for (int i = 0; i < windowSize; i++) {
            buffer[i + 3] = receivedPackets[i] ? 1 : 0;
        }

        sendBytes(buffer, windowSize+3, start);
    } while (remainingPacketCount > 0);

    incoming_ += windowSize;

    delete[] receivedPackets;
    return reqLen;
}

long NewPipe::submitHeader(const unsigned char header, bool forceSend) {
    return submit(&header, 1, forceSend);
}

long NewPipe::submit(HeaderType header, unsigned char* bytes, int len, bool forceSend) {
    return submit(header, (const unsigned char*)bytes, len, forceSend);
}


long NewPipe::submit(HeaderType header, const unsigned char* bytes, int len, bool forceSend) {
    unsigned char* newBytes = new unsigned char[len+1];
    newBytes[0] = header;
    memcpy(newBytes+1, bytes, len);
    long res = submit(newBytes, len+1, forceSend);
    delete[] newBytes;
    return res;
}

long NewPipe::submit(const unsigned char* bytes, int len, bool forceSend) {
    toSend_[submited_] = Bytes(bytes, len);
    std::cout << "submit: submited: " << submited_ << std::endl;
    if (forceSend) {
        sendBatch(packet_+1, submited_+1);
        std::cout << "submit: sending batch: " << packet_+1 << ", " << submited_+1 << std::endl;
    } else if ((submited_ % WINDOW_SIZE)  == WINDOW_SIZE-1 && submited_ != 0) {
        sendBatch(submited_ - WINDOW_SIZE+1, submited_+1);
        std::cout << "submit: sending batch: " << submited_-WINDOW_SIZE+1 << ", " << submited_+1 << std::endl;
    }
    submited_++;
    return len;
}

long NewPipe::next(unsigned char* buffer, bool forceRecv) {
    loaded_++;
    std::cout << "next: loading: " << loaded_ << std::endl;
    if (loaded_ % WINDOW_SIZE == 0)
        recvBatch(incoming_ == -1);
    while (toRecv_.find(loaded_) == toRecv_.end()) {
        if (forceRecv) {
            recvBatch(incoming_ == -1 && packet_ == -1);
        } else {
            std::cerr << "next: packet not found: " << loaded_ << ", listening for another batch" << std::endl;
            recvBatch(incoming_ == -1 && packet_ == -1);
        }
    }
    long len = toRecv_[loaded_].len;
    memcpy(buffer, toRecv_[loaded_].bytes, len);
    return len;
}

