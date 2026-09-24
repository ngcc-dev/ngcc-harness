#ifndef INDCPA_H
#define INDCPA_H

#include <stdint.h>
#include "params.h"
#include "poly.h"

#define indcpa_keypair KEM_NAMESPACE(indcpa_keypair)
void indcpa_keypair(uint8_t pk[KEM_CPAPKE_PUBLICKEYBYTES],
                    uint8_t sk[KEM_CPAPKE_SECRETKEYBYTES]);

#define indcpa_enc KEM_NAMESPACE(indcpa_enc)
void indcpa_enc(uint8_t c[KEM_CPAPKE_CIPHERTEXTBYTES],
                const uint8_t m[KEM_MSGBYTES],
                const uint8_t pk[KEM_CPAPKE_PUBLICKEYBYTES],
                const uint8_t coins[SEEDBYTES]);

#define indcpa_dec KEM_NAMESPACE(indcpa_dec)
void indcpa_dec(uint8_t m[KEM_MSGBYTES],
                const uint8_t c[KEM_CPAPKE_CIPHERTEXTBYTES],
                const uint8_t sk[KEM_CPAPKE_SECRETKEYBYTES]);

#endif
