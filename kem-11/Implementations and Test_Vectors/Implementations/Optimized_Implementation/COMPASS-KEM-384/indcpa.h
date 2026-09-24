#ifndef INDCPA_H
#define INDCPA_H

#include <stdint.h>
#include "params.h"
#include "polyvec.h"

#define gen_matrix COMPASS_KEM_NAMESPACE(gen_matrix)
void gen_matrix(polyvec *a, const uint8_t seed[COMPASS_KEM_SYMBYTES], int transposed);

#define indcpa_keypair_derand COMPASS_KEM_NAMESPACE(indcpa_keypair_derand)
void indcpa_keypair_derand(uint8_t pk[COMPASS_KEM_INDCPA_PUBLICKEYBYTES],
                           uint8_t sk[COMPASS_KEM_INDCPA_SECRETKEYBYTES],
                           const uint8_t coins[COMPASS_KEM_SYMBYTES]);

#define indcpa_enc COMPASS_KEM_NAMESPACE(indcpa_enc)
void indcpa_enc(uint8_t c[COMPASS_KEM_INDCPA_BYTES],
                const uint8_t m[COMPASS_KEM_INDCPA_MSGBYTES],
                const uint8_t pk[COMPASS_KEM_INDCPA_PUBLICKEYBYTES],
                const uint8_t coins[COMPASS_KEM_SYMBYTES]);

#define indcpa_dec COMPASS_KEM_NAMESPACE(indcpa_dec)
void indcpa_dec(uint8_t m[COMPASS_KEM_INDCPA_MSGBYTES],
                const uint8_t c[COMPASS_KEM_INDCPA_BYTES],
                const uint8_t sk[COMPASS_KEM_INDCPA_SECRETKEYBYTES]);

#endif
