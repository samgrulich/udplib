#include <cstring>
#include <fstream>
#include <stdio.h>
#include <iostream>

#include "common.h"
#include "message.h"
#include "receiver.h"
#include "sender.h"

// #define SENDER
// #define RECEIVER

// #define TARGET_IP   "147.32.217.58"
#define TARGET_IP   "127.0.0.1"

#ifdef SENDER
#define TARGET_PORT 5001
#define LOCAL_PORT 5002
#endif // SENDER

#ifdef RECEIVER
#define TARGET_PORT 5002 
#define LOCAL_PORT 5001 
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

#ifdef SENDER
    std::cout << "Sending file: " << filePath << std::endl;
    Sender sender(filePath, TARGET_IP, LOCAL_PORT, TARGET_PORT);
    sender.send(HeaderType::Name, filePath);
    sender.send(HeaderType::Size, sender.size());
    // send file
    char msg[BUFFERS_LEN];
    Pipe& pipe = sender.pipe();
    do {
        sender.send(HeaderType::Start);
        do {
            sender.sendChunk();
        } while (!sender.eof());
        sender.send(HeaderType::Stop);
        sender.sendHash();
        pipe.recv(msg);
    } while (strcmp(msg, getLabel(HeaderType::FileAck)) != 0);
    std::cout << "File sent!" << std::endl;
#endif // SENDER

#ifdef RECEIVER
    std::cout << "Listening for incoming connection." << std::endl;
    Reciever receiver(TARGET_IP, LOCAL_PORT, TARGET_PORT);
    // initial recv 
    receiver.recv();
    if (!receiver.hasHeader(HeaderType::Name)) {
        std::cout << "Wrong incoming packet" << std::endl;
        return -1;
    }
    std::string name = "";
    receiver.getPayloadString(&name);
    receiver.recv();
    int size = receiver.getPayloadInt();
    std::cout << "File: " << name << ", size: " << size << std::endl;
    do {
        receiver.recv(); // start
        receiver.recv(); // first data 
        do { 
            receiver.saveDataPayload();
            receiver.recv();
        } while (!receiver.hasHeader(HeaderType::Stop)); // recv file
        receiver.recv(); // hash
        if(!receiver.matchHashes()) {
            receiver.pipe().send(getLabel(HeaderType::FileError));
            std::cout << "File recieve failed, restarting" << std::endl;
        }
    } while(!receiver.matchHashes()); // restart recv file
    receiver.pipe().send(getLabel(HeaderType::FileAck));
    std::cout << "File recieved!" << std::endl;
#endif // RECEIVER
    return 0;
}

