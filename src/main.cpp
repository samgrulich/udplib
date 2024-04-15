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
#define RECEIVER

// #define TARGET_IP   "127.0.0.1"
#define TARGET_IP   "147.32.217.249"
//#define TARGET_IP   "10.0.0.7"

#ifdef SENDER
#define TARGET_PORT 4000
#define LOCAL_PORT 5001
#endif // SENDER

#ifdef RECEIVER
#define TARGET_PORT 4001 
#define LOCAL_PORT 5000 
#endif // RECEIVER


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
    std::cout << "Name" << std::endl;
    pipe.submit(HeaderType::Name, (unsigned char*)filePath, strlen(filePath)+1);
    std::cout << "Size" << std::endl;
    pipe.submit(HeaderType::Size, (unsigned char*)&fileSize, sizeof(fileSize));
    // send file
    char data[BUFFERS_LEN];
    unsigned char msg[BUFFERS_LEN];
    std::cout << "Data" << std::endl;
    do {
        pipe.submitHeader(HeaderType::Start);
        do {
            // crc, packetid, header
            int size = file.read(data, BUFFERS_LEN-9).gcount();
            pipe.submit(HeaderType::Data, (unsigned char*)data, size);
            hasher.updateHash(data, size);
        } while (!file.eof());
        std::cout << "Stop" << std::endl;
        pipe.submitHeader(HeaderType::Stop);
        std::cout << "Hash" << std::endl;
        std::string hash = hasher.getHash();
        pipe.submit(HeaderType::Hash, (unsigned char*)(hash.c_str()), hash.length()+1);
        std::cout << "Waiting for fileack" << std::endl;
        pipe.next(msg);
    } while (msg[0] != FileAck);
    pipe.submitHeader(HeaderType::End);
    std::cout << "File sent!" << std::endl;
#endif // SENDER

#ifdef RECEIVER
    std::cout << "Listening for incoming connection." << std::endl;
    // initial recv 
    std::cout << "Waiting for name" << std::endl;
    unsigned char buffer[BUFFERS_LEN];
    size_t len = 0;
    len = pipe.next(buffer);
    std::cout << (char*)(buffer+1) << std::endl;
    std::cout << (int)len << std::endl;
    std::string name = std::string((char*)buffer+1, len-1);
    std::ofstream file(name, std::ios::binary);
    std::cout << "Waiting for size" << std::endl;
    len = pipe.next(buffer);
    uint64_t size = *(uint64_t*)(buffer+1);
    std::cout << "File: " << name << ", size: " << size << std::endl;
    bool hashesMatch = true;
    do {
        std::cout << "Waiting for start" << std::endl;
        len = pipe.next(buffer); // start
        std::cout << "Receiving file" << std::endl;
        len = pipe.next(buffer);  // data
        while (buffer[0] != HeaderType::Stop) {
            hasher.updateHash((char*)(buffer+1), len-1);
            file.write((char*)(buffer+1), len-1);
            std::cout << "Waiting for next data" << std::endl;
            len = pipe.next(buffer);  // data
            printf("Data[0]: %d", buffer[0]);
            std::cout << ", Stop " << HeaderType::Stop << ", eq: " << HeaderType::Stop << std::endl;
        }
        std::cout << "Waiting for hash" << std::endl;
        len = pipe.next(buffer);  // hash
        hashesMatch = strcmp(hasher.getHash().c_str(), (char*)(buffer+1)) == 0;
        if(!hashesMatch) {
            pipe.submitHeader(HeaderType::FileError);
            std::cout << "File recieve failed, restarting" << std::endl;
        }
        std::cout << "After hash match" << std::endl;
    } while(!hashesMatch); // restart recv file
    std::cout << "Sending fileack" << std::endl;
    pipe.submitHeader(HeaderType::FileAck);
    pipe.next(buffer); // end
    std::cout << "File recieved!" << std::endl;
#endif // RECEIVER
    return 0;
}

