/********************************************************************************************
* MAMBA-Frost: parameters and API for MAMBA-Frost-512.
*********************************************************************************************/

#ifndef _API_Frost512_H_
#define _API_Frost512_H_


#define CRYPTO_SECRETKEYBYTES  41728
#define CRYPTO_PUBLICKEYBYTES  36432
#define CRYPTO_BYTES              64
#define CRYPTO_CIPHERTEXTBYTES 72944

// Algorithm name
#define CRYPTO_ALGNAME "MAMBA-Frost-512"


int crypto_kem_keypair_Frost512(unsigned char *pk, unsigned char *sk);
int crypto_kem_enc_Frost512(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int crypto_kem_dec_Frost512(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);


#endif
