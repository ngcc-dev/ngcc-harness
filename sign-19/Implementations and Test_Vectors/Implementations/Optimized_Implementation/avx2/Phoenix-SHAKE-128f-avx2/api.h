/*
 * Phoenix-SHAKE AVX2 Signature API
 *
 * High-performance Phoenix-SHAKE-AVX2 optimized signature implementation using 4-way AVX2
 * vectorization with SHAK hash function and KeccakP1600 permutation.
 */

#ifndef PHOENIX_API_H
#define PHOENIX_API_H

#include <stdint.h>
#include <stddef.h>

#include "params.h"

/* Algorithm identifier */
#define CRYPTO_ALGNAME    "PH"

/* Key and signature sizes */
#define CRYPTO_SECRETKEYBYTES    SPX_SK_BYTES
#define CRYPTO_PUBLICKEYBYTES    SPX_PK_BYTES
#define CRYPTO_BYTES             SPX_BYTES
#define CRYPTO_SEEDBYTES         (3 * SPX_N)

/* ========== Key Generation Functions ========== */

/**
 * crypto_sign_secretkeybytes - Returns the length of a secret key in bytes.
 *
 * @return Secret key length in bytes
 */
unsigned long long crypto_sign_secretkeybytes(void);

/**
 * crypto_sign_publickeybytes - Returns the length of a public key in bytes.
 *
 * @return Public key length in bytes
 */
unsigned long long crypto_sign_publickeybytes(void);

/**
 * crypto_sign_seedbytes - Returns the length of the seed required for key pair generation
 *
 * @return Seed length in bytes
 */
unsigned long long crypto_sign_seedbytes(void);

/**
 * crypto_sign_seed_keypair - Generates a Phoenix-SHAKE key pair from a given seed.
 *
 * @param[out] pk   Public key output buffer (SPX_PK_BYTES)
 * @param[out] sk   Secret key output buffer (SPX_SK_BYTES)
 * @param[in]  seed Seed value (CRYPTO_SEEDBYTES)
 * @retval 0 Success
 * @retval non-zero Error code
 *
 * Secret key format:   [SK_SEED || SK_PRF || PUB_SEED || root]
 * Public key format:   [root || PUB_SEED]
 */
int crypto_sign_seed_keypair(unsigned char *pk, unsigned char *sk,
                             const unsigned char *seed);

/**
 * crypto_sign_keypair - Generates a Phoenix-SHAKE key pair using random generation.
 *
 * @param[out] pk Public key output buffer (SPX_PK_BYTES)
 * @param[out] sk Secret key output buffer (SPX_SK_BYTES)
 * @retval 0 Success
 * @retval non-zero Error code
 *
 * Secret key format:   [SK_SEED || SK_PRF || PUB_SEED || root]
 * Public key format:   [root || PUB_SEED]
 */
int crypto_sign_keypair(unsigned char *pk, unsigned char *sk);

/* ========== Signature Functions ========== */

/**
 * crypto_sign_bytes - Returns the length of a signature in bytes.
 *
 * @return Signature length in bytes
 */
unsigned long long crypto_sign_bytes(void);

/**
 * crypto_sign_signature - Computes a detached signature over a message.
 *
 * @param[out] sig     Signature output buffer
 * @param[in]  siglen  Available signature buffer size
 * @param[out] slen    Actual signature length produced
 * @param[in]  m       Message to sign
 * @param[in]  mlen    Message length in bytes
 * @param[in]  sk      Secret key for signing
 * @retval 0 Success
 * @retval non-zero Error code
 */
int crypto_sign_signature(uint8_t *sig, size_t *siglen, size_t *slen,
                          const uint8_t *m, size_t mlen, const uint8_t *sk);

/**
 * crypto_sign_verify - Verifies a detached signature against a message and public key.
 *
 * @param[in] sig     Signature to verify
 * @param[in] siglen  Signature length in bytes
 * @param[in] m       Message that was signed
 * @param[in] mlen    Message length in bytes
 * @param[in] pk      Public key for verification
 * @retval 0 Signature valid
 * @retval non-zero Signature invalid
 */
int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen, const uint8_t *pk);

/**
 * crypto_sign - Signs a message and outputs the signature concatenated with the message.
 *
 * @param[out] sm      Output buffer (signature || message)
 * @param[out] smlen   Total output length
 * @param[out] slen    Signature portion length
 * @param[out] tfslen  Tweak field set length (AVX2 optimization parameter)
 * @param[in]  m       Message to sign
 * @param[in]  mlen    Message length in bytes
 * @param[in]  sk      Secret key for signing
 * @retval 0 Success
 * @retval non-zero Error code
 */
int crypto_sign(unsigned char *sm, unsigned long long *smlen,
                unsigned long long *slen, unsigned long long *tfslen,
                const unsigned char *m, unsigned long long mlen,
                const unsigned char *sk);

/**
 * crypto_sign_open - Recovers the message from a signed message and verifies the signature.
 *
 * @param[out] m       Recovered message buffer
 * @param[out] mlen    Recovered message length
 * @param[out] slen    Signature length
 * @param[out] tfslen  Tweak field set length (AVX2 optimization parameter)
 * @param[in]  sm      Signed message (signature || message)
 * @param[in]  smlen   Signed message total length
 * @param[in]  pk      Public key for verification
 * @retval 0 Signature valid, message recovered
 * @retval non-zero Signature invalid
 */
int crypto_sign_open(unsigned char *m, unsigned long long *mlen,
                     unsigned long long *slen, unsigned long long *tfslen,
                     const unsigned char *sm, unsigned long long smlen,
                     const unsigned char *pk);

#endif