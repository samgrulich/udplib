#pragma once 

#define BUFFERS_LEN 1024

#include <cstring>

struct Bytes {
    unsigned char* bytes;
    int len;

    Bytes() {
        bytes = new unsigned char[BUFFERS_LEN];
        len = 0;
    }
    Bytes(unsigned char* bytes, int len) {
        this->bytes = new unsigned char[len];
        this->len = len;
        memcpy(this->bytes, bytes, len);
    }
    Bytes(const unsigned char* bytes, int len) {
        this->bytes = new unsigned char[len];
        this->len = len;
        memcpy(this->bytes, bytes, len);
    }
    ~Bytes() {
        delete[] bytes;
    }
};
