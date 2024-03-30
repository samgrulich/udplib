#include <stdio.h>
#include <thread>

#include "message.h"
#include "receiver.h"
#include "sender.h"

// #define SENDER
#define RECEIVER

#define TARGET_IP   "10.0.0.11"

#ifdef SENDER
#define TARGET_PORT 5001
#define LOCAL_PORT 5002
#endif // SENDER

#ifdef RECEIVER
#define TARGET_PORT 5002 
#define LOCAL_PORT 5001 
#endif // RECEIVER

using namespace std::chrono_literals;
#define DELAY 100ms


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
    Sender sender(filePath, TARGET_IP, LOCAL_PORT, TARGET_PORT);
    sender.send(HeaderType::Name, filePath);
    std::this_thread::sleep_for(DELAY);
    sender.send(HeaderType::Size, sender.size());
    std::this_thread::sleep_for(DELAY);
    sender.send(HeaderType::Start);
    do {
        sender.sendChunk();
        std::this_thread::sleep_for(DELAY);
    } while (!sender.eof());
    sender.send(HeaderType::Stop);
#endif // SENDER

#ifdef RECEIVER
    Receiver receiver(TARGET_IP, LOCAL_PORT, TARGET_PORT);
    receiver.listen();
#endif // RECEIVER
    return 0;
}
