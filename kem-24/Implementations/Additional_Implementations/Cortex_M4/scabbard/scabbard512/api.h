#ifndef API_H
#define API_H

#include "params.h"

#define CRYPTO_ALGNAME "scabbard512"
#define CRYPTO_PUBLICKEYBYTES SCABBARD_PUBLICKEYBYTES
#define CRYPTO_SECRETKEYBYTES SCABBARD_SECRETKEYBYTES
#define CRYPTO_CIPHERTEXTBYTES SCABBARD_CIPHERTEXTBYTES
#define CRYPTO_BYTES SCABBARD_SSBYTES

int crypto_kem_keypair(unsigned char *pk, unsigned char *sk);
int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);

#endif
