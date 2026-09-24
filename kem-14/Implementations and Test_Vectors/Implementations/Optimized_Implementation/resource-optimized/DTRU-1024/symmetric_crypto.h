#ifndef SYMMETRIC_CRYPTO
#define SYMMETRIC_CRYPTO

#include <stddef.h>
#include <stdint.h>

#include "fips202.h"

int crypto_stream(unsigned char *out,
                  unsigned long long len,
                  const unsigned char nonce[16],
                  const unsigned char key[32]);

void sha256(uint8_t out[32], const uint8_t *in, size_t inlen);
void sha512(uint8_t out[64], const uint8_t *in, size_t inlen);

#define crypto_hash_sha_256 sha256
#define crypto_hash_sha_512 sha512

#define crypto_hash_sha3_256 sha3_256
#define crypto_hash_sha3_512 sha3_512
#define crypto_hash_shake256 shake256
#define crypto_hash_shake128 shake128

#endif
