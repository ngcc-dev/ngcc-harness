#include <openssl/sha.h>
#include <iostream>
#include <iomanip>
#include <cstring>


void sha256_openssl(unsigned char *data, size_t len) {
    unsigned char *hash = new unsigned char[SHA256_DIGEST_LENGTH];
    // SHA256计算
    SHA256_CTX sha256_ctx;
    SHA256_Init(&sha256_ctx);
    SHA256_Update(&sha256_ctx, data, len);
    SHA256_Final(hash, &sha256_ctx);
}

void sha512_openssl(unsigned char *data, size_t len, unsigned char *hash) {
    //unsigned char *hash = new unsigned char[SHA512_DIGEST_LENGTH];
    SHA512_CTX sha512;
    SHA512_Init(&sha512);
    SHA512_Update(&sha512, data, len);
    SHA512_Final(hash, &sha512);
}