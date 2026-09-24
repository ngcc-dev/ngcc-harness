#ifndef API_H
#define API_H

/* MAMBA-Viper implementation and implementation support layer where applicable. */

#include "viper_params.h"

#define CRYPTO_ALGNAME VIPER_ALGNAME
#define CRYPTO_SECRETKEYBYTES VIPER_SECRETKEYBYTES
#define CRYPTO_PUBLICKEYBYTES VIPER_PUBLICKEYBYTES
#define CRYPTO_BYTES VIPER_SHARED_SECRET_BYTES
#define CRYPTO_CIPHERTEXTBYTES VIPER_CIPHERTEXTBYTES

int crypto_kem_keypair(unsigned char *pk, unsigned char *sk);
int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);

#endif
