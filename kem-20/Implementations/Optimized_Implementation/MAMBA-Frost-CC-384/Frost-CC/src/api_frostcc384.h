/********************************************************************************************
* MAMBA-Frost-CC: parameters and API for MAMBA-Frost-CC-384.
*********************************************************************************************/

#ifndef _API_FROSTCC384_H_
#define _API_FROSTCC384_H_


#define CRYPTO_SECRETKEYBYTES  43492
#define CRYPTO_PUBLICKEYBYTES  37628
#define CRYPTO_BYTES              48
#define CRYPTO_CIPHERTEXTBYTES 25204

// Algorithm name
#define CRYPTO_ALGNAME "MAMBA-Frost-CC-384"


int crypto_kem_keypair_FrostCC384(unsigned char *pk, unsigned char *sk);
int crypto_kem_enc_FrostCC384(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int crypto_kem_dec_FrostCC384(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);


#endif
