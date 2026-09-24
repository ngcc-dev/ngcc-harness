#ifndef SYMMETRIC_H
#define SYMMETRIC_H

#include <stddef.h>
#include <stdint.h>
#include "params.h"

#include "fips202.h"

typedef keccak_state xof_state;

#define kem_shake256_prf KEM_NAMESPACE(_kem_shake256_prf)
void kem_shake256_prf(uint8_t *out,
                      size_t outlen,
                      const uint8_t key[SEEDBYTES],
                      uint8_t nonce);

// HASH
#define hash_h(OUT, IN, INBYTES) sha3_256(OUT, IN, INBYTES)
#define hash_g(OUT, IN, INBYTES) sha3_512(OUT, IN, INBYTES)

// PRF
#define prf(OUT, OUTBYTES, KEY, NONCE) kem_shake256_prf(OUT, OUTBYTES, KEY, NONCE)

// KDF
#define kdf(OUT, IN, INBYTES) shake256(OUT, SEEDBYTES, IN, INBYTES)

#endif