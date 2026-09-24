#ifndef INDCPA_H
#define INDCPA_H

#include <stdint.h>
#include "params.h"
#include "polyvec.h"

#define gen_matrix WEAVER_NAMESPACE(_gen_matrix)
void gen_matrix(polyvec *a, const uint8_t seed[WEAVER_SYMBYTES], int transposed);
#define indcpa_keypair_derand WEAVER_NAMESPACE(_indcpa_keypair_derand)
void indcpa_keypair_derand(uint8_t pk[WEAVER_INDCPA_PUBLICKEYBYTES],
                           uint8_t sk[WEAVER_INDCPA_SECRETKEYBYTES],
                           const uint8_t coins[WEAVER_SYMBYTES]);

#define indcpa_enc WEAVER_NAMESPACE(_indcpa_enc)
void indcpa_enc(uint8_t c[WEAVER_INDCPA_BYTES],
                const uint8_t m[WEAVER_INDCPA_MSGBYTES],
                const uint8_t pk[WEAVER_INDCPA_PUBLICKEYBYTES],
                const uint8_t coins[WEAVER_SYMBYTES]);

#define indcpa_dec WEAVER_NAMESPACE(_indcpa_dec)
void indcpa_dec(uint8_t m[WEAVER_INDCPA_MSGBYTES],
                const uint8_t c[WEAVER_INDCPA_BYTES],
                const uint8_t sk[WEAVER_INDCPA_SECRETKEYBYTES]);

// For performance Test (temporarily expose/remove 'static'):
void unpack_pk(polyvec *pk,
               uint8_t seed[WEAVER_SYMBYTES],
               const uint8_t packedpk[WEAVER_INDCPA_PUBLICKEYBYTES]);
                      
void pack_ciphertext(uint8_t r[WEAVER_INDCPA_BYTES],
                     polyvec *b,
                     poly *v);              

#endif
