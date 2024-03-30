#include <stdio.h>
#include <string.h>
#include <thread>

#include "message.h"
#include "receiver.h"
#include "sender.h"
#include "socket.h"

// #define SENDER
// #define RECEIVER

using namespace std::chrono_literals;
#define DELAY 100ms


int main(int argc, char* argv[]) {
    if (argc != 2) {
#ifdef SENDER
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
#else 
        fprintf(stderr, "Usage: %s <output_file>\n", argv[0]);
#endif
        return 1;
    }

    const char* filePath = argv[1];
    int socketS;

#ifndef LINUX 
    InitWinsock();
#endif

#ifdef SENDER
    printf("Qe passo1\n");
    Sender sender(filePath);
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
    printf("Qe passo2\n");
    Receiver receiver(filePath);
    receiver.listen();
#endif // RECEIVER

    close(socketS);
    return 0;
}
