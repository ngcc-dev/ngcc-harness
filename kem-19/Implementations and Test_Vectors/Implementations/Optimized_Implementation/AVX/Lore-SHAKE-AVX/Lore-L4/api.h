#ifndef API_H
#define API_H

#include "params.h"

// Define algorithm name based on the security level parameter K
#if   (LORE_LEVEL == 1)
#define CRYPTO_ALGNAME "LORE-128"
#elif (LORE_LEVEL == 2)
#define CRYPTO_ALGNAME "LORE-256"
#elif (LORE_LEVEL == 3)
#define CRYPTO_ALGNAME "LORE-384"
#elif (LORE_LEVEL == 4)
#define CRYPTO_ALGNAME "LORE-512"
#endif

// === KEM API ===
#define CRYPTO_SECRETKEYBYTES  LORE_KEM_SECRETKEYBYTES
#define CRYPTO_PUBLICKEYBYTES  LORE_KEM_PUBLICKEYBYTES
#define CRYPTO_CIPHERTEXTBYTES LORE_CIPHERTEXTBYTES
#define CRYPTO_BYTES           LORE_BYTES
#define CRYPTO_KEYPAIRCOINBYTES (2 * LORE_SYMBYTES) // For derand keypair
#define CRYPTO_ENCCOINBYTES    LORE_MSG_BYTES      // For derand encaps

int crypto_kem_keypair_derand(unsigned char *pk, unsigned char *sk, const unsigned char *coins);
int crypto_kem_keypair(unsigned char *pk, unsigned char *sk);

int crypto_kem_enc_derand(unsigned char *ct, unsigned char *ss, const unsigned char *pk, const unsigned char *coins);
int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk);

int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);


// === PKE API (for benchmarking comparison and legacy tests) ===
#define CRYPTO_PKE_PUBLICKEYBYTES LORE_PUBLICKEYBYTES
#define CRYPTO_PKE_SECRETKEYBYTES LORE_SECRETKEYBYTES
#define CRYPTO_PKE_CIPHERTEXTBYTES LORE_INDCPA_BYTES

int crypto_pke_keypair_derand(unsigned char *pk, unsigned char *sk, const unsigned char *coins);
int crypto_pke_keypair(unsigned char *pk, unsigned char *sk);

int crypto_pke_enc_derand(unsigned char *ct, const unsigned char *m, const unsigned char *pk, const unsigned char *coins);
int crypto_pke_encrypt(unsigned char *ct, const unsigned char *m, unsigned long long mlen, const unsigned char *pk);

int crypto_pke_decrypt(unsigned char *m, unsigned long long *mlen, const unsigned char *ct, const unsigned char *sk);

#endif