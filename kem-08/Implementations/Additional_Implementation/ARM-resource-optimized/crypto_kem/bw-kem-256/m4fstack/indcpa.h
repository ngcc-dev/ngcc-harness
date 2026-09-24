#ifndef INDCPA_H
#define INDCPA_H

#include <stdint.h>
#include "params.h"
#include "polyvec.h"

#define indcpa_keypair_derand KYBER_NAMESPACE(indcpa_keypair_derand)
void indcpa_keypair_derand(uint8_t pk[KYBER_INDCPA_PUBLICKEYBYTES],
                           uint8_t sk[KYBER_INDCPA_SECRETKEYBYTES],
                           const uint8_t coins[KYBER_SYMBYTES]);

#define indcpa_enc KYBER_NAMESPACE(indcpa_enc)
void indcpa_enc(uint8_t c[KYBER_INDCPA_BYTES],
                const uint8_t m[KYBER_INDCPA_MSGBYTES],
                const uint8_t pk[KYBER_INDCPA_PUBLICKEYBYTES],
                const uint8_t coins[KYBER_SYMBYTES]);

#define indcpa_dec KYBER_NAMESPACE(indcpa_dec)
void indcpa_dec(uint8_t m[KYBER_INDCPA_MSGBYTES],
                const uint8_t c[KYBER_INDCPA_BYTES],
                const uint8_t sk[KYBER_INDCPA_SECRETKEYBYTES]);

/* Re-encrypt and constant-time compare against an existing ciphertext c,
 * without materializing the re-encrypted ciphertext on the stack (FO check).
 * Returns 0 if c matches the re-encryption, 1 otherwise. */
#define indcpa_enc_cmp KYBER_NAMESPACE(indcpa_enc_cmp)
unsigned char indcpa_enc_cmp(const uint8_t c[KYBER_INDCPA_BYTES],
                             const uint8_t m[KYBER_INDCPA_MSGBYTES],
                             const uint8_t pk[KYBER_INDCPA_PUBLICKEYBYTES],
                             const uint8_t coins[KYBER_SYMBYTES]);

#endif
