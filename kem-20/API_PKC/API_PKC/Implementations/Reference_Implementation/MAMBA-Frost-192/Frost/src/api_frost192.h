/********************************************************************************************
* MAMBA-Frost: parameters and API for MAMBA-Frost-192.
*********************************************************************************************/

#ifndef _API_Frost192_H_
#define _API_Frost192_H_


#define CRYPTO_SECRETKEYBYTES  11528
#define CRYPTO_PUBLICKEYBYTES  9712
#define CRYPTO_BYTES              24
#define CRYPTO_CIPHERTEXTBYTES 9760

// Algorithm name
#define CRYPTO_ALGNAME "MAMBA-Frost-192"


int crypto_kem_keypair_Frost192(unsigned char *pk, unsigned char *sk);
int crypto_kem_enc_Frost192(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int crypto_kem_dec_Frost192(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);


#endif
