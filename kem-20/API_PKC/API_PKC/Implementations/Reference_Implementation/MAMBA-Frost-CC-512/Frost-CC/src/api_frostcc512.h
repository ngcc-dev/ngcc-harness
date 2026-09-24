/********************************************************************************************
* MAMBA-Frost-CC: parameters and API for MAMBA-Frost-CC-512.
*********************************************************************************************/

#ifndef _API_FROSTCC512_H_
#define _API_FROSTCC512_H_


#define CRYPTO_SECRETKEYBYTES  83328
#define CRYPTO_PUBLICKEYBYTES  72832
#define CRYPTO_BYTES              64
#define CRYPTO_CIPHERTEXTBYTES 36544

// Algorithm name
#define CRYPTO_ALGNAME "MAMBA-Frost-CC-512"


int crypto_kem_keypair_FrostCC512(unsigned char *pk, unsigned char *sk);
int crypto_kem_enc_FrostCC512(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int crypto_kem_dec_FrostCC512(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);


#endif
