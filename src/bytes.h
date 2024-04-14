#pragma once

#include <cstring>
struct Bytes {
    unsigned char* bytes;
    int len;

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
