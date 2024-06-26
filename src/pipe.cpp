#include "pipe.h"
#include "common.h"
#include <cstring>
#include <iostream>


Pipe::Pipe(const char* remote_address, const int local_port, const int remote_port) {
    from_.sin_port = htons(remote_port);
    fromlen_ = sizeof(struct sockaddr_in);
    local_.sin_family = AF_INET;
    local_.sin_port = htons(local_port);
    local_.sin_addr.s_addr = INADDR_ANY;

    socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (bind(socket_, (struct sockaddr*)&local_, sizeof(local_)) != 0) {
        perror("Binding error!");
        getchar(); //wait for press Enter
        throw "Couldn't bind socket to given port";
    }

    dest_.sin_family = AF_INET;
    dest_.sin_port = htons(remote_port);
    inet_pton(AF_INET, remote_address, &dest_.sin_addr);
}

Pipe::~Pipe() {
    closesocket(socket_);
}

size_t Pipe::send(const std::string& message) {
    // return sendto(socket_, message.c_str(), message.length()+1, 0, (struct sockaddr*)&dest_, sizeof(dest_)); 
    return sendBytes(message.c_str(), message.length()+1);
}

size_t Pipe::sendBytes(const char *bytes, int len) {
    std::cout << "bytes len: " << len << std::endl;
    return sendto(socket_, bytes, len, 0, (struct sockaddr*)&dest_, sizeof(dest_))-1; 
}

size_t Pipe::recv(char* buffer) {
    char buff[BUFFERS_LEN];

    size_t size = recvfrom(socket_, buff, BUFFERS_LEN, 0, (struct sockaddr*)&from_, (socklen_t*)&fromlen_);

    if (size == -1) {
        throw "Socket error";
    }

    memcpy(buffer, buff, size);
    return size-1;
}
