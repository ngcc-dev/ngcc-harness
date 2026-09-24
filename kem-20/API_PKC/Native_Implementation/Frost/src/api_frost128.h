/********************************************************************************************
* MAMBA-Frost: parameters and API for MAMBA-Frost-128.
*********************************************************************************************/

#ifndef _API_Frost128_H_
#define _API_Frost128_H_


#define CRYPTO_SECRETKEYBYTES  6736
#define CRYPTO_PUBLICKEYBYTES  5152
#define CRYPTO_BYTES              16
#define CRYPTO_CIPHERTEXTBYTES 5192

// Algorithm name
#define CRYPTO_ALGNAME "MAMBA-Frost-128"


int crypto_kem_keypair_Frost128(unsigned char *pk, unsigned char *sk);
int crypto_kem_enc_Frost128(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int crypto_kem_dec_Frost128(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);


#endif
