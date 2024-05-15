#include <cstring>
#include <fstream>
#include <filesystem>
#include <stdio.h>
#include <iostream>

#include "common.h"
#include "hash.h"
#include "message.h"
#include "newpipe.h"

// #define SENDER
// #define RECEIVER

// #define TARGET_IP   "127.0.0.1"
// #define TARGET_IP   "147.32.218.251"
#define TARGET_IP   "10.0.0.7"

#ifdef SENDER
#define TARGET_PORT 4000
#define LOCAL_PORT 5001
#endif // SENDER

#ifdef RECEIVER
#define TARGET_PORT 4001 
#define LOCAL_PORT 5000 
#endif // RECEIVER
// #ifdef RECEIVER
// #define TARGET_PORT 5001 
// #define LOCAL_PORT 4000 
// #endif // RECEIVER


int main(int argc, char* argv[]) {
#ifdef SENDER
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    const char* filePath = argv[1];
#endif

#ifndef LINUX 
    InitWinsock();
#endif

    Hasher hasher = Hasher();
    NewPipe pipe(TARGET_IP, LOCAL_PORT, TARGET_PORT);

#ifdef SENDER
    std::ifstream file(filePath, std::ios::binary);
    uint64_t fileSize = std::filesystem::file_size(filePath);

    std::cout << "Sending file: " << filePath << std::endl;
    pipe.submit(HeaderType::Name, (unsigned char*)filePath, strlen(filePath)+1);
    pipe.submit(HeaderType::Size, (unsigned char*)&fileSize, sizeof(fileSize));
    // send file
    char data[BUFFERS_LEN];
    unsigned char msg[BUFFERS_LEN];
    do {
        pipe.submitHeader(HeaderType::Start);
        do {
            // crc, packetid, header
            int size = file.read(data, BUFFERS_LEN-12).gcount();
            pipe.submit(HeaderType::Data, (unsigned char*)data, size);
            hasher.updateHash(data, size);
        } while (!file.eof());
        pipe.submitHeader(HeaderType::Stop);
        std::string hash = hasher.getHash();
        pipe.submit(HeaderType::Hash, (unsigned char*)(hash.c_str()), hash.length()+1, true);
        pipe.flush();
        pipe.next(msg, true);
    } while (msg[0] != FileAck);
    unsigned char buffer[BUFFERS_LEN];
    int32_t resId;

    for (int i = 0; i < 5; i++) {
        if (pipe.recvBytes(buffer, resId) >= 0) {
            int start = resId - buffer[0];
            int windowSize = buffer[1];
            pipe.sendPositiveAck(windowSize, resId);
        }
    }

    std::cout << "File sent!" << std::endl;
#endif // SENDER

#ifdef RECEIVER
    std::cout << "Listening for incoming connection." << std::endl;
    // initial recv 
    unsigned char buffer[BUFFERS_LEN];
    size_t len = 0;
    len = pipe.next(buffer);
    std::string name = std::string((char*)buffer+1, len-1);
    std::ofstream file(name, std::ios::binary);
    len = pipe.next(buffer);
    uint64_t size = *(uint64_t*)(buffer+1);
    std::cout << "File: " << name << ", size: " << size << std::endl;
    bool hashesMatch = true;
    do {
        len = pipe.next(buffer); // start
        len = pipe.next(buffer);  // data
        while (buffer[0] != HeaderType::Stop) {
            hasher.updateHash((char*)(buffer+1), len-1);
            file.write((char*)(buffer+1), len-1);
            len = pipe.next(buffer);  // data or stop
        }
        len = pipe.next(buffer, true);  // hash
        hashesMatch = strcmp(hasher.getHash().c_str(), (char*)(buffer+1)) == 0;
        if(!hashesMatch) {
            pipe.submitHeader(HeaderType::FileError, true);
            std::cerr << "File recieve failed, restarting" << std::endl;
        }
    } while(!hashesMatch); // restart recv file
    pipe.submitHeader(HeaderType::FileAck, true);
    pipe.flush();
    std::cout << "File recieved!" << std::endl;
#endif // RECEIVER
    return 0;
}

