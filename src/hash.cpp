#include "hash.h"
#include <iostream>

Hasher::Hasher() {
}

void Hasher::updateHash(char* bytes, int len) {
    std::cout << "hasher: updateHash: buffer " << len;
    for (int i = 0; i < len; i++) {
        printf("%c", bytes[i]);
    }
    std::cout << std::endl;
    ctx_.add(bytes, len);
}

std::string Hasher::getHash() {
    if (!isDigested_) {
        hash_ = ctx_.getHash();
        isDigested_ = true;
    }
    return hash_;
}
