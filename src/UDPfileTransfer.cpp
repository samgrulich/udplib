#include <stdio.h>
#include <string.h>
#include <iostream>
#include <ostream>
#include <chrono>
#include <thread>

#ifdef LINUX
#define SOCKETS "linuxSockets.h"
#else 
#define SOCKETS "windowsSockets.h"
#endif
#include SOCKETS

#define TARGET_IP   "127.0.0.1"

#define BUFFERS_LEN 1024

//#define SENDER
#define RECEIVER

#ifdef SENDER
#define TARGET_PORT 5001
#define LOCAL_PORT 5002
#endif // SENDER

#ifdef RECEIVER
#define TARGET_PORT 5002 
#define LOCAL_PORT 5001 
#endif // RECEIVER

void sendFile(SOCKET socketS, const char* filePath, const sockaddr_in& addrDest, int delayMs)
{
    FILE* file;
    if (fopen_s(&file, filePath, "rb") != 0)
    {
        fprintf(stderr, "ERROR: Unable to open file: %s\n", filePath);
        return;
    }

    char buffer_tx[BUFFERS_LEN];
    size_t bytesRead;
    while ((bytesRead = fread(buffer_tx, 1, BUFFERS_LEN, file)) > 0)
    {
        sendto(socketS, buffer_tx, bytesRead, 0, (sockaddr*)&addrDest, sizeof(addrDest));
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    }

    sendto(socketS, "", 0, 0, (sockaddr*)&addrDest, sizeof(addrDest));

    fclose(file);
}

void receiveFile(SOCKET socketS, const char* filePath)
{
    FILE* file;
    if (fopen_s(&file, filePath, "wb") != 0)
    {
        fprintf(stderr, "ERROR: Unable to create file: %s\n", filePath);
        return;
    }

    char buffer_rx[BUFFERS_LEN];
    int bytesReceived;
    printf("Output file ready, waiting for sender\n");
    do
    {
        bytesReceived = recv(socketS, buffer_rx, sizeof(buffer_rx), 0);
        if (bytesReceived > 0)
        {
            //printf("Writing bytes...\n");
            fwrite(buffer_rx, 1, bytesReceived, file);
            //printf("Bytes written successfully\n");
        }
        else if (bytesReceived == 0)
        {
            printf("End of file reached\n");
            break;
        }
        else
        {
            perror("ERROR: Recieving failed\n");
            break;
        }
    } while (true);

    printf("Closing file.\n");

    fclose(file);
}



int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    const char* inputFilePath = argv[1];
    const char* outputFilePath = argv[2];

    int socketS;

#ifndef LINUX 
    InitWinsock();
#endif

    struct sockaddr_in local;
    struct sockaddr_in from;

    int fromlen = sizeof(from);
    local.sin_family = AF_INET;
    local.sin_port = htons(LOCAL_PORT);
    local.sin_addr.s_addr = INADDR_ANY;

    socketS = socket(AF_INET, SOCK_DGRAM, 0);
    if (bind(socketS, (struct sockaddr*)&local, sizeof(local)) != 0) {
        perror("Binding error!");
        getchar(); //wait for press Enter
        return 1;
    }

#ifdef SENDER
    sockaddr_in addrDest;
    addrDest.sin_family = AF_INET;
    addrDest.sin_port = htons(TARGET_PORT);
    InetPton(AF_INET, TARGET_IP, &addrDest.sin_addr.s_addr);

    printf("Sending file....\n");
    sendFile(socketS, inputFilePath, addrDest, 100);

    close(socketS);
#endif // SENDER

#ifdef RECEIVER
    receiveFile(socketS, outputFilePath);

    close(socketS);
#endif // RECEIVER

    return 0;
}
