#include "pipe.h"


Pipe::Pipe() {
    local_.sin_family = AF_INET;
    printf("Port: %d\n", LOCAL_PORT);
    local_.sin_port = htons(LOCAL_PORT);
    local_.sin_addr.s_addr = INADDR_ANY;

    socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    printf("New pip\n");
    if (bind(socket_, (struct sockaddr*)&local_, sizeof(local_)) != 0) {
        perror("Binding error!");
        getchar(); //wait for press Enter
        throw "Couldn't bind socket to given port";
    }
}

size_t Pipe::send(const std::string& message) {
    printf("sending %d %s %ld \n", socket_, message.c_str(), message.length());
    return sendto(socket_, message.c_str(), message.length()*sizeof(char), 0, (struct sockaddr*)&local_, sizeof(local_)); 
}

size_t Pipe::recv(std::string* buffer) {
    socklen_t fromlen = sizeof(from_);
    char buff[BUFFERS_LEN];

    size_t size = recvfrom(socket_, buff, BUFFERS_LEN, 0, (struct sockaddr*)&from_, &fromlen);

    *buffer = buff;
    return size;
}
