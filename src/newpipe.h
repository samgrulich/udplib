#pragma once

#include "message.h"
#include "socket.h"
#include "common.h"
#include <map>
#include <vector>

class NewPipe {
private:
    int socket_;
    int fromlen_;
    struct sockaddr_in local_;
    struct sockaddr_in dest_;
    struct sockaddr_in from_;
    bool isOpen = true;
protected:
    // Number of submited packets for sending
    int32_t submited_ = 0;
    // Number of last loaded packet from the buffer
    int32_t loaded_ = -1;
    // Number of last sent packet
    int32_t packet_ = -1; // todo reset on err
    // Number of last accepted packet
    int32_t incoming_ = -1;
    // Number of last acknowledgment packet
    int32_t ack_ = -1;
    std::map<int32_t, Bytes> toSend_;
    std::map<int32_t, Bytes> toRecv_;
private:
    long sendBytes(const unsigned char* bytes, int len, int32_t packetId);
    long recvBytes(unsigned char* buffer, int32_t& packetId);
    long send(const unsigned char* bytes, int len);
    long recv(unsigned char* buffer);
    void sendHeader(const unsigned char header, int32_t packetId);
public:
    NewPipe(const char* remote_address, const int local_port, const int remote_port);
    ~NewPipe();
    long submitHeader(const unsigned char header);
    long submit(HeaderType header, unsigned char* bytes, int len);
    long submit(HeaderType header, const unsigned char* bytes, int len);
    // submit packet for sending
    long submit(const unsigned char* bytes, int len);
    // load packet from the buffer if at the end return -1
    long next(unsigned char* buffer);
};
