#include <stdio.h>
#include <string.h>

#define LINUX // comment out for windows functionality
#include "universal.h"

#define TARGET_IP   "147.32.217.21"

#define BUFFERS_LEN 1024

#define SENDER
//#define RECEIVER

#ifdef SENDER
#define TARGET_PORT 5001
#define LOCAL_PORT 5002
#endif // SENDER

#ifdef RECEIVER
#define TARGET_PORT 5002 
#define LOCAL_PORT 5001 
#endif // RECEIVER

int main() {
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

    char buffer_rx[BUFFERS_LEN];
    char buffer_tx[BUFFERS_LEN];

#ifdef SENDER
    sockaddr_in addrDest;
    addrDest.sin_family = AF_INET;
    addrDest.sin_port = htons(TARGET_PORT);
    inet_pton(AF_INET, TARGET_IP, &addrDest.sin_addr);

    strncpy(buffer_tx, "Hello world payload!\n", BUFFERS_LEN); //put some data to buffer
    printf("Sending packet.\n");
    sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (struct sockaddr*)&addrDest, sizeof(addrDest));

    close(socketS);
#endif // SENDER

#ifdef RECEIVER
    strncpy(buffer_rx, "No data received.\n", BUFFERS_LEN);
    printf("Waiting for datagram ...\n");
    if (recvfrom(socketS, buffer_rx, sizeof(buffer_rx), 0, (struct sockaddr*)&from, &fromlen) == -1) {
        perror("Socket error!");
        return 1;
    }
    else
        printf("Datagram: %s", buffer_rx);

    close(socketS);
#endif // RECEIVER

    return 0;
}
