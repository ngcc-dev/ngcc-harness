#ifndef Reference_Implementation_KR_API_H
#define Reference_Implementation_KR_API_H

#define CRYPTO_SECRETKEYBYTES 22004
#define CRYPTO_PUBLICKEYBYTES 21812
#define CRYPTO_CIPHERTEXTBYTES 39294
#define CRYPTO_BYTES 64
#define CRYPTO_ALGNAME "HARE-512-kr"

int crypto_kem_keypair(unsigned char *pk, unsigned char *sk);
int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);

int crypto_kem_dec_status(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);
#endif
