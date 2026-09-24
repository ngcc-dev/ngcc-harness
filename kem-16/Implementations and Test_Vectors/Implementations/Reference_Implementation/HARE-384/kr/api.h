#ifndef Reference_Implementation_KR_API_H
#define Reference_Implementation_KR_API_H

#define CRYPTO_SECRETKEYBYTES 13301
#define CRYPTO_PUBLICKEYBYTES 13157
#define CRYPTO_CIPHERTEXTBYTES 23693
#define CRYPTO_BYTES 48
#define CRYPTO_ALGNAME "HARE-384-kr"

int crypto_kem_keypair(unsigned char *pk, unsigned char *sk);
int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);

int crypto_kem_dec_status(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);
#endif
