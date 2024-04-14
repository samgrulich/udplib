#pragma once 

#define BUFFERS_LEN 1024

#include <cstring>

struct Bytes {
    unsigned char bytes[BUFFERS_LEN];
    int len = 0;

    Bytes() { }
    Bytes(unsigned char* bytes, int len) {
        this->len = len;
        memcpy(this->bytes, bytes, len);
    }
    Bytes(const unsigned char* bytes, int len) {
        this->len = len;
        memcpy(this->bytes, bytes, len);
    }
    ~Bytes() { }
};
