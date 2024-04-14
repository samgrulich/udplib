#include "hash.h"

Hasher::Hasher() {
}

void Hasher::updateHash(char* bytes, int len) {
    ctx_.add(bytes, len);
}

std::string Hasher::getHash() {
    if (!isDigested_) {
        hash_ = ctx_.getHash();
        isDigested_ = true;
    }
    return hash_;
}
