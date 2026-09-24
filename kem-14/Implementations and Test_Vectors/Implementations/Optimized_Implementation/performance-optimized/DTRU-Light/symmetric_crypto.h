#ifndef SYMMETRIC_CRYPTO
#define SYMMETRIC_CRYPTO

#include <stddef.h>
#include <stdint.h>

#include "auxfunc.h"

int crypto_stream(unsigned char *out,
                  unsigned long long len,
                  const unsigned char nonce[16],
                  const unsigned char key[32]);

void sha256(uint8_t out[32], const uint8_t *in, size_t inlen);
void sha512(uint8_t out[64], const uint8_t *in, size_t inlen);

#define crypto_hash_sha_256 sha256
#define crypto_hash_sha_512 sha512

static inline void crypto_hash_sha3_256(unsigned char *out, const unsigned char *in, unsigned long long inlen)
{
  sm3hash(256, in, inlen * 8, out);
}

static inline void crypto_hash_sha3_512(unsigned char *out, const unsigned char *in, unsigned long long inlen)
{
  pseudohash(512, in, inlen * 8, out);
}

static inline void crypto_hash_shake256(unsigned char *out, unsigned long long outlen, const unsigned char *in, unsigned long long inlen)
{
  pseudoXOF(outlen * 8, in, inlen * 8, out);
}

static inline void crypto_hash_shake128(unsigned char *out, unsigned long long outlen, const unsigned char *in, unsigned long long inlen)
{
  pseudoXOF(outlen * 8, in, inlen * 8, out);
}

#endif
