#ifndef NTRE_SYMMETRIC_H
#define NTRE_SYMMETRIC_H

#include <stdint.h>
#include "params.h"

#define HASH_H_OUTBYTES (NTRE_M1BYTES + NTRE_RHOBYTES + NTRE_SSBYTES)

void hash_G(uint8_t out[NTRE_GBYTES],
            const uint8_t pk[NTRE_PUBLICKEYBYTES]);

void hash_H(uint8_t out[HASH_H_OUTBYTES],
            const uint8_t r[NTRE_RBYTES],
            const uint8_t h_pk[NTRE_GBYTES]);

void hash_F(uint8_t out[NTRE_RBYTES],
            const uint8_t m1[NTRE_M1BYTES]);

void sample_psi1(uint8_t buf[NTRE_SAMPLEBYTES],
                 const uint8_t seed[NTRE_SYMBYTES]);

#endif /* NTRE_SYMMETRIC_H */
