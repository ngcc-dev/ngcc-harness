/********************************************************************************************
* MAMBA-Frost-CC: parameters and API for MAMBA-Frost-CC-128.
*********************************************************************************************/

#ifndef _API_FROSTCC128_H_
#define _API_FROSTCC128_H_


#define CRYPTO_SECRETKEYBYTES  6736
#define CRYPTO_PUBLICKEYBYTES  5152
#define CRYPTO_BYTES              16
#define CRYPTO_CIPHERTEXTBYTES 5192

// Algorithm name
#define CRYPTO_ALGNAME "MAMBA-Frost-CC-128"


int crypto_kem_keypair_FrostCC128(unsigned char *pk, unsigned char *sk);
int crypto_kem_enc_FrostCC128(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int crypto_kem_dec_FrostCC128(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);


#endif
