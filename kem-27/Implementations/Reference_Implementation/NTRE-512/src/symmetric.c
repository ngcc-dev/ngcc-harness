#include <string.h>
#include "symmetric.h"
#include "auxfunc.h"

#define BITS(n) ((unsigned long long)(n) * 8)

void hash_G(uint8_t out[NTRE_GBYTES],
            const uint8_t pk[NTRE_PUBLICKEYBYTES])
{
    sm3hash(256, pk, BITS(NTRE_PUBLICKEYBYTES), out);
}

void hash_H(uint8_t out[HASH_H_OUTBYTES],
            const uint8_t r[NTRE_RBYTES],
            const uint8_t h_pk[NTRE_GBYTES])
{
    uint8_t data[1 + NTRE_RBYTES + NTRE_GBYTES];

    data[0] = 0x02;
    memcpy(data + 1,                r,    NTRE_RBYTES);
    memcpy(data + 1 + NTRE_RBYTES,  h_pk, NTRE_GBYTES);
    pseudoXOF(BITS(HASH_H_OUTBYTES), data, BITS(sizeof data), out);
}

void hash_F(uint8_t out[NTRE_RBYTES],
            const uint8_t m1[NTRE_M1BYTES])
{
    uint8_t data[1 + NTRE_M1BYTES];

    data[0] = 0x01;
    memcpy(data + 1, m1, NTRE_M1BYTES);
    pseudoXOF(BITS(NTRE_RBYTES), data, BITS(sizeof data), out);
}

void sample_psi1(uint8_t buf[NTRE_SAMPLEBYTES],
                 const uint8_t seed[NTRE_SYMBYTES])
{
    uint8_t data[1 + NTRE_SYMBYTES];

    data[0] = 0x00;
    memcpy(data + 1, seed, NTRE_SYMBYTES);
    pseudoXOF(BITS(NTRE_SAMPLEBYTES), data, BITS(sizeof data), buf);
}
