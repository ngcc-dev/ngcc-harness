#ifndef Reference_Implementation_KR_API_H
#define Reference_Implementation_KR_API_H

#define CRYPTO_SECRETKEYBYTES 2677
#define CRYPTO_PUBLICKEYBYTES 2629
#define CRYPTO_CIPHERTEXTBYTES 4688
#define CRYPTO_BYTES 16
#define CRYPTO_ALGNAME "HARE-128-kr"

int crypto_kem_keypair(unsigned char *pk, unsigned char *sk);
int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);

int crypto_kem_dec_status(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);
#endif
