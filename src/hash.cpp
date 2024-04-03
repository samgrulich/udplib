#include "hash.h"
#include <openssl/md5.h>

Hasher::Hasher() {
    MD5_Init(&ctx_);
}

void Hasher::updateHash(char* bytes, int len) {
    MD5_Update(&ctx_, bytes, len);
}

unsigned char* Hasher::getHash() {
    if (!isDigested_) {
        MD5_Final(hash_, &ctx_);
        isDigested_ = true;
    }
    return hash_;
}
