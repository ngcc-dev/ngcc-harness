/**
 * @file api.h
 * @brief KEM API used by the TriQ-KEM IND-CCA2 scheme
 */

 #ifndef TRIQ_API_H
 #define TRIQ_API_H

 #define CRYPTO_ALGNAME "TriQ-KEM-256"

 #define CRYPTO_SECRETKEYBYTES  6424
 #define CRYPTO_PUBLICKEYBYTES  6328
 #define CRYPTO_BYTES           32
 #define CRYPTO_CIPHERTEXTBYTES 12472

 int crypto_kem_keypair(unsigned char *pk, unsigned char *sk);
 int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
 int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);

 #endif  // TRIQ_API_H
