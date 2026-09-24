/********************************************************************************************
* MAMBA-Frost: parameters and API for MAMBA-Frost-384.
*********************************************************************************************/

#ifndef _API_Frost384_H_
#define _API_Frost384_H_


#define CRYPTO_SECRETKEYBYTES  29032
#define CRYPTO_PUBLICKEYBYTES  25096
#define CRYPTO_BYTES              48
#define CRYPTO_CIPHERTEXTBYTES 37736

// Algorithm name
#define CRYPTO_ALGNAME "MAMBA-Frost-384"


int crypto_kem_keypair_Frost384(unsigned char *pk, unsigned char *sk);
int crypto_kem_enc_Frost384(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int crypto_kem_dec_Frost384(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);


#endif
