#include <stdio.h>
#include <string.h>
#include <thread>

#include "message.h"
#include "receiver.h"
#include "sender.h"
#include "socket.h"

#define TARGET_IP   "127.0.0.1"

// #define SENDER
// #define RECEIVER

using namespace std::chrono_literals;
#define DELAY 100ms

// void sendFile(int socketS, const char* filePath, const sockaddr_in& addrDest)
// {
//     FILE* file;
//     if (!(file = fopen(filePath, "rb"))) {
//         fprintf(stderr, "ERROR: Unable to open file: %s\n", filePath);
//         return;
//     }
//
//     char buffer_tx[BUFFERS_LEN];
//     size_t bytesRead;
//     while ((bytesRead = fread(buffer_tx, 1, BUFFERS_LEN, file)) > 0) {
//         sendto(socketS, buffer_tx, bytesRead, 0, (sockaddr*)&addrDest, sizeof(addrDest));
//         std::this_thread::sleep_for(DELAY);
//     }
//
//     sendto(socketS, "", 0, 0, (sockaddr*)&addrDest, sizeof(addrDest));
//
//     fclose(file);
// }
//
// void receiveFile(int socketS, const char* filePath) {
//     FILE* file;
//     if (!(file = fopen(filePath, "wb"))) {
//         fprintf(stderr, "ERROR: Unable to create file: %s\n", filePath);
//         return;
//     }
//
//     char buffer_rx[BUFFERS_LEN];
//     int bytesReceived;
//     printf("Output file ready, waiting for sender\n");
//     int timeoutCounter = 0;
//     do {
//         bytesReceived = recv(socketS, buffer_rx, sizeof(buffer_rx), 0);
//         if (bytesReceived > 0) {
//             //printf("Writing bytes...\n");
//             fwrite(buffer_rx, 1, bytesReceived, file);
//             timeoutCounter = 0;
//             //printf("Bytes written successfully\n");
//         }
//         else if (bytesReceived == 0) {
//             printf("End of file reached\n");
//             timeoutCounter++;
//             std::this_thread::sleep_for(DELAY);
//         }
//         else {
//             perror("ERROR: Recieving failed\n");
//             break;
//         }
//     } while (timeoutCounter < 10);
//
//     printf("Closing file.\n");
//
//     fclose(file);
// }



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

    // struct sockaddr_in local;
    // struct sockaddr_in from;
    //
    // int fromlen = sizeof(from);
    // local.sin_family = AF_INET;
    // local.sin_port = htons(LOCAL_PORT);
    // local.sin_addr.s_addr = INADDR_ANY;
    //
    // socketS = socket(AF_INET, SOCK_DGRAM, 0);
    // if (bind(socketS, (struct sockaddr*)&local, sizeof(local)) != 0) {
    //     perror("Binding error!");
    //     getchar(); //wait for press Enter
    //     return 1;
    // }

#ifdef SENDER
    printf("Qe passo1\n");
    Sender sender(filePath);
    sender.send(HeaderType::Name, filePath);
    sender.send(HeaderType::Size, sender.size());
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
