#pragma once
#include <hashes/md5.h>
#include <string>

class Hasher {
    MD5 ctx_;
    std::string hash_;
    bool isDigested_ = false;

public:
    Hasher();
    void updateHash(char* bytes, int len);
    std::string getHash();
};
