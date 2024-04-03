#pragma once
#include <openssl/md5.h>
#include <string>

class Hasher {
    MD5_CTX ctx_;
    unsigned char hash_[MD5_DIGEST_LENGTH];
    bool isDigested_ = false;

public:
    Hasher();
    void updateHash(char* bytes, int len);
    unsigned char* getHash();
};
