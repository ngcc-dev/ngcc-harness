#ifndef CRYPTO_HASH_SHA3256
#define CRYPTO_HASH_SHA3256

#include "auxfunc.h"

static inline void crypto_hash_sha3256(unsigned char *out, const unsigned char *in, unsigned long long inlen)
{
  sm3hash(256, in, inlen * 8, out);
}

static inline void crypto_hash_sha3512(unsigned char *out, const unsigned char *in, unsigned long long inlen)
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
