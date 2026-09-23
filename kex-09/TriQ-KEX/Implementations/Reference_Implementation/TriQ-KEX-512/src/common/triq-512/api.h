/**
 * @file api.h
 * @brief KEM API used by the TriQ-KEM IND-CCA2 scheme
 */

 #ifndef TRIQ_API_H
 #define TRIQ_API_H

 #define CRYPTO_ALGNAME "TriQ-KEM-512"

 #define CRYPTO_SECRETKEYBYTES  19960
 #define CRYPTO_PUBLICKEYBYTES  19768
 #define CRYPTO_BYTES           64
 #define CRYPTO_CIPHERTEXTBYTES 39320

 int crypto_kem_keypair(unsigned char *pk, unsigned char *sk);
 int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
 int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);

 #endif  // TRIQ_API_H
