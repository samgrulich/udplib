#pragma once
#include "common.h"
#include "hash.h"
#include "message.h"
#include "pipe.h"
#include <fstream>

class Reciever : Hasher {
private:
    uint32_t size_;
    std::string name_;
    std::ofstream fstream_;
    Pipe pipe_;
    char buffer_[BUFFERS_LEN];
    size_t bufferLen_;
public: 
    // create new receiver
    Reciever(const char* remote_address, const int local_port, const int remote_port);
    /** Blocks until a packet comes in.
     * Then stores the incoming mesage
     */
    int recv();
    int recv(HeaderType type);
    // Check if incoming message has this type of header
    bool hasHeader(HeaderType type);
    // get hash from payload and compare it with the calculated 
    bool matchHashes();
    // strips header up to = and returns the string (works for name and hash)
    void getPayloadString(std::string* out);
    // Parse incoming payload as int (works only if header is size)
    int getPayloadInt();
    // Get incoming payload (works only for header data)
    int saveDataPayload();
    
    // get reference to the pipe
    Pipe& pipe();
    // get fstream
    std::ofstream& fstream();
};
