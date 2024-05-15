#include "newpipe.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ios>
#include <iostream>
#include <list>
#include <locale>
#include <map>
#include <set>

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

    int packet_ = 0;
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
    unsigned char* buffer = new unsigned char[3];
    buffer[0] = 0;
    buffer[1] = 1;
    buffer[2] = packetId; // todo: switch for window
    buffer[3] = Ack;
    // todo: add all the packet numbers
    sendBytes(buffer, 3, packetId);
}

long NewPipe::sendBatch() {
    unsigned char res[BUFFERS_LEN];
    int32_t resId = -1;
    long msgLen = 0;
    int startPacket = windowStart_, stopPacket = windowEnd_;

    int windowSize = stopPacket - startPacket;
    window_++;
    std::unordered_map<int, Bytes> window;

    for (int i = 0; i < windowSize; i++) {
        int packetId = startPacket + i;
        Bytes packet = toSend_[packetId];
        unsigned char buffer[BUFFERS_LEN]; // todo: possible error due to big bytes
        memcpy(buffer+3, packet.bytes, packet.len);
        buffer[0] = i;
        buffer[1] = windowSize;
        buffer[2] = window_;
        window[packetId] = Bytes(buffer, packet.len+3);
    }

    do {
        int resLen;
        // send current window
        do {
            for (auto &[packetId, packet] : window) {
                sendBytes(packet, packetId);
            }
            resLen = recvBytes(res, resId); // listen for ack
        } while(resLen < 0); // repeat until response is received
        int resHeader = res[3];
        // all checks
        if (resHeader == Ack) {
            // unpack the received packets
            int idsLen = (resLen - 4)/4; // number of packetIds in the message
            int* ackData = (int*)(res+4);
            // remove received packets
            for (int i = 0; i < idsLen; i++) {
                window.erase(ackData[i]); // unpack the packet id and remove it from win
            }
            if (window.empty()) { // empty window = acknowledged window 
                break;
            } else {
                // repopulate the window
                for (int i = 0; i < idsLen; i++) {
                    int newPacketId = windowEnd_++;
                    if (toSend_.find(newPacketId) != toSend_.end()) { // are there any more packets to send
                        Bytes newPacket = toSend_[newPacketId];
                        unsigned char buffer[BUFFERS_LEN]; 
                        memcpy(buffer+3, newPacket.bytes, newPacket.len);
                        window[newPacketId] = Bytes(buffer, newPacket.len+3);
                    } else {
                        break;
                    }
                }
                window_++;
                int i = 0;
                for (auto &[packetId, packet] : window) {
                    packet.bytes[0] = i;
                    packet.bytes[1] = window.size();
                    packet.bytes[2] = window_;
                }
            }
        } else if (resHeader == ForceAck) {
            // todo: check if the ack is for this window?
            break;
        } else {
            std::cerr << "send: response is not ack: " << (int)(res[2]) << std::endl;
            unsigned char windowId = res[2];
            if (resId < 0) 
                continue;
            if (windowId <= window_) {
                int windowSize = res[1];
                unsigned char buffer[4] = {0, 1, windowId, ForceAck};
                for (int i = 0; i < windowSize-1; i++) {
                    resLen = recvBytes(res, resId);
                }
                // sendPositiveAck(res[1], resId-res[0]); // send ack
            }
        }
    } while(true); // repeat
    
    // req sent and ack received
    return windowEnd_ - windowStart_;
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
    long msgLen; 

    // packet info catcher
    int windowSize = WINDOW_SIZE, i = WINDOW_SIZE, windowId;

    // do {
    //     bool toReceive = true;
    //     do {
    //         reqLen = recvBytes(infoBuffer, packetId);
    //         toReceive = reqLen < 0 || infoBuffer[2] == Ack;
    //         if (!isFirst) i--;
    //     } while (toReceive && i != 0); // packets either didn't arrive or are invalid
    //     if (toReceive) { // both receiver and sender are receiving
    //         std::cerr << "recvBatch: double receive detected: " << packetId << std::endl;
    //         if (packet_ >= WINDOW_SIZE-1) {
    //             for (int i = packet_-WINDOW_SIZE+1; i < packet_+1; i++) {
    //                 long reqLen = sendBytes(toSend_[i], packetId);
    //             }
    //             recvBytes(infoBuffer, start); // just dump the result
    //         }
    //         i = WINDOW_SIZE;
    //         continue;
    //     }
    //     start = packetId - infoBuffer[0]; 
    //     windowSize = infoBuffer[1];
    //     if (start < incoming_) { // send ack for repeated packet
    //         std::cerr << "recvBatch: repeated packet: " << packetId << std::endl;
    //         i = WINDOW_SIZE;
    //         sendPositiveAck(windowSize, start);
    //     }
    // } while (start < incoming_); // in case of missing ack from previous batch 
    //             // (meaning incoming packets and window have been already processed)
    // toRecv_[packetId] = Bytes(infoBuffer+2, reqLen-2); // store the packet (without window information)
    
    std::map<int, bool> receivedPackets;
    std::unordered_map<int, std::set<int>> windows;
    // receivedPackets[packetId] = true;
    // int stopPacket = start + windowSize;

    do {
        std::list<int> receivedPacketIds;
        for (int i = 0; i < windowSize; i++) {
            unsigned char msgBuffer[BUFFERS_LEN];
            msgLen = recvBytes(msgBuffer, packetId);
            // check if the packet is a ok
            if (msgLen < 0) {
                if (msgLen == TIMEOUT) {
                    if (isFirst && i == 0) {
                        i = -1; 
                        continue;
                    }
                    std::cerr << "recvBatch: timeout: " << packetId << std::endl;
                    break;
                }
                std::cerr << "recvBatch: invalid packet, skipping: " << packetId << std::endl;
                continue;
            }
            // check if the packet is from the current window
            windowId = msgBuffer[2];
            windowSize = msgBuffer[1];
            receivedPackets[packetId] = true; 
            receivedPacketIds.push_back(packetId);
            if (windows.find(windowId) == windows.end()) {
                windows[windowId] = std::set<int>();
            }
            windows[windowId].insert(packetId);
            if (windowId != window_+1) {
                if (windowId < window_+1) {
                    std::cerr << "recvBatch: repeated packet: " << packetId << std::endl;
                    // sendHeader(Ack, packetId);
                } else {
                    if (toRecv_.find(packetId) == toRecv_.end()) {
                        toRecv_[packetId] = Bytes(msgBuffer+3, msgLen-3); // store the packet (without window information)
                    } else {
                        std::cerr << "recvBatch: invalid window: " << packetId << std::endl;
                    }
                }
                continue;
            }
            // check if the packet is not already received
            toRecv_[packetId] = Bytes(msgBuffer+3, msgLen-3); // store the packet (without window information)
            // if (!receivedPackets[packetId] && toRecv_.find(packetId) == toRecv_.end()) {
            //     toRecv_[packetId] = Bytes(msgBuffer+3, msgLen-3); // store the packet (without window information)
            // }
        }
        if (windowId < window_+1) {
            // generate and send ack
            unsigned char* buffer = new unsigned char[4 + 4 * windowSize];
            buffer[0] = 0;
            buffer[1] = 1;
            buffer[2] = windowId;
            buffer[3] = Ack;
            int* idsBuffer = (int*)(buffer+4), i = 0;
            std::set<int> receivedIds = windows[windowId];
            for (int packetId : receivedIds) {
                idsBuffer[i++] = packetId;
            }
            sendBytes(buffer, 4 + 4 * receivedPacketIds.size(), window_);
        } else {
            // generate and send ack
            unsigned char* buffer = new unsigned char[4 + 4 * receivedPacketIds.size()];
            buffer[0] = 0;
            buffer[1] = 1;
            buffer[2] = ++window_;
            buffer[3] = Ack;
            int* idsBuffer = (int*)(buffer+4);
            int i = 0;
            for (int& packetId : receivedPacketIds) {
                idsBuffer[i++] = packetId;
            }
            sendBytes(buffer, 4 + 4 * receivedPacketIds.size(), window_);
        }

        // break if whole window was received
        if (receivedPacketIds.size() == windowSize) {
            break;
        }
    } while(true);

    incoming_ += windowSize;


    // int remainingPacketCount = windowSize - 1;
    //
    //
    // receivedPackets[infoBuffer[0]] = true;
    //
    // int remPacketCount = remainingPacketCount;
    // do {
    //     for (int i = 0; i < remPacketCount; i++) { // listen for remaining packets
    //         unsigned char reqBuffer[BUFFERS_LEN];
    //         reqLen = recvBytes(reqBuffer, packetId);
    //         int packetWindowId = packetId - start;
    //         if (packetWindowId < 0 || packetWindowId >= windowSize) {
    //             std::cerr << "recvBatch: invalid packet, skipping: " << packetId << std::endl;
    //             continue;
    //         }
    //         if (receivedPackets[reqBuffer[0]]) {
    //             continue;
    //         }
    //         if (reqLen < 0) {
    //             if (reqLen == TIMEOUT) {
    //                 std::cerr << "recvBatch: timeout: " << packetId << std::endl;
    //                 break;
    //             }
    //             std::cerr << "recvBatch: invalid packet, skipping: " << packetId << std::endl;
    //             continue;
    //         }
    //         // check if data incoming has wrong header
    //         toRecv_[packetId] = Bytes(reqBuffer+2, reqLen-2); // store the packet (without window information)
    //         receivedPackets[reqBuffer[0]] = true; 
    //         remainingPacketCount--;
    //     }
    //     // send ack
    //     unsigned char* buffer = new unsigned char[windowSize + 3];
    //
    //     buffer[0] = 0;
    //     buffer[1] = 1;
    //     buffer[2] = Ack;
    //     for (int i = 0; i < windowSize; i++) {
    //         buffer[i + 3] = receivedPackets[i] ? 1 : 0;
    //     }
    //
    //     sendBytes(buffer, windowSize+3, start);
    // } while (remainingPacketCount > 0);
    //
    // incoming_ += windowSize;
    //
    // delete[] receivedPackets;
    return msgLen;
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
    // if (forceSend) {
    //     std::cout << "submit: sending batch: " << packet_+1 << ", " << submited_+1 << std::endl;
    //     sendBatch(packet_+1, submited_+1);
    // } else if ((submited_ % WINDOW_SIZE)  == WINDOW_SIZE-1 && submited_ != 0) {
    //     std::cout << "submit: sending batch: " << submited_-WINDOW_SIZE+1 << ", " << submited_+1 << std::endl;
    //     sendBatch(submited_ - WINDOW_SIZE+1, submited_+1);
    // }
    submited_++;
    return len;
}

long NewPipe::flush() {
    int messages = submited_ / WINDOW_SIZE + 1;
    windowStart_ = 0;
    windowEnd_ = WINDOW_SIZE;

    for (int i = 0; i < messages && windowEnd_ != windowStart_; i++) {
        sendBatch();
        windowStart_ = windowEnd_;
        windowEnd_ = windowStart_ + WINDOW_SIZE;
        if (windowEnd_ > submited_) {
            windowEnd_ = submited_;
        }
    }

    return 0;
}

long NewPipe::next(unsigned char* buffer, bool forceRecv) {
    loaded_++;
    std::cout << "next: loading: " << loaded_ << std::endl;
    if (loaded_ % WINDOW_SIZE == 0)
        recvBatch(incoming_ == -1);
    while (toRecv_.find(loaded_) == toRecv_.end()) {
        std::cerr << "next: packet not found: " << loaded_ << ", listening for another batch" << std::endl;
        if (forceRecv) {
            recvBatch();
        } else {
            std::cerr << "next: packet not found: " << loaded_ << ", listening for another batch" << std::endl;
            recvBatch(incoming_ == -1);
        }
    }
    long len = toRecv_[loaded_].len;
    memcpy(buffer, toRecv_[loaded_].bytes, len);
    return len;
}

