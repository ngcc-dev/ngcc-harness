/**
 * @file symmetric.c
 * @brief Cryptographic primitives: a drng pseudo-random number generator (PRNG),
 * plus hash functions built on sm3hash and pseudohash.
 */

#include <stdlib.h>
#include <string.h>
#include "symmetric.h"

 // DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;

/**
 *
 * Derived from drng function in drng.c
 *
 * @param[in] entropy_input Pointer to input entropy bytes
 * @param[in] personalization_string Pointer to the personalization string
 * @param[in] enlen Length of entropy string in bytes
 * @param[in] perlen Length of the personalization string in bytes
 */
void prng_init(uint8_t *entropy_input, uint8_t *personalization_string, uint32_t enlen, uint32_t perlen) {
    uint8_t* input = malloc(enlen + perlen);

    memcpy(input, entropy_input, enlen);
    memcpy(input + enlen, personalization_string, perlen);
    init_random_number(&drng_algorithm, input, enlen + perlen);
    free(input);
}

/**
 *
 * Derived from drng function in drng.c
 *
 * @param[out] output Pointer to output
 * @param[in] outlen length of output in bytes
 */
void prng_get_bytes(uint8_t *output, uint32_t outlen) {
    get_random_number(&drng_algorithm, output, outlen * 8);
}

/**
 * @brief Initializes a drng context with a given seed.
 *
 * @param[out] xof_ctx   Pointer to the drng context to be initialized.
 * @param[in]  seed      Pointer to the input seed.
 * @param[in]  seed_size Size of the seed in bytes.
 */
void xof_init(DRNG_ctx *xof_ctx, const uint8_t *seed, uint32_t seed_size) {
    init_random_number(xof_ctx, seed, seed_size);
}

/**
 * @brief Extracts pseudorandom bytes from a frng context.
 *
 * @param[in,out] xof_ctx     Pointer to the initialized drng context.
 * @param[out]    output      Pointer to the buffer where the output bytes will be written.
 * @param[in]     output_size Number of bytes to extract.
 *
 * @details This function squeezes the specified number of pseudorandom bytes from
 * the drng context and stores them in the provided output buffer.
 * The context must have been initialized beforehand using `xof_init()`.
 */
void xof_get_bytes(DRNG_ctx *xof_ctx, uint8_t *output, uint32_t output_size) {
    get_random_number(xof_ctx, output, output_size * 8);
}

/**
 * @brief Computes the hash function I (pseudohash-1024) with domain separation.
 *
 * @param[out] output Pointer to the buffer where the 128-byte hash output will be stored.
 * @param[in]  seed   Pointer to the input seed to be hashed.
 *
 * @details This function implements the random oracle `I` as specified,
 * using the pseudohash-1024 hash function. It produces a 128-byte output from the given seed.
 */
void hash_i(uint8_t *output, const uint8_t *seed) {
    pseudohash(SEED_BYTES * 16, seed, SEED_BYTES * 8, output);
}

/**
 * @brief Compute the hash function H (pseudohash-512) with domain separation.
 *
 * @param[out] output      Buffer (64 bytes) to receive the hash output.
 * @param[in]  ek_kem      Encapsulation key of the KEM.
 */
void hash_h(uint8_t *output, const uint8_t ek_kem[PUBLIC_KEY_BYTES]) {
    pseudohash(SEED_BYTES * 8, ek_kem, PUBLIC_KEY_BYTES * 8, output);
}

/**
 * @brief Compute the hash function G (pseudohash-1024) with domain separation.
 *
 * @param[out] output        Buffer (128 bytes) to receive the hash output.
 * @param[in]  hash_ek_kem   Hash of the KEM encapsulation key.
 * @param[in]  m             Message bytes.
 * @param[in]  salt          Salt value.
 */
void hash_g(uint8_t *output, const uint8_t hash_ek_kem[SEED_BYTES], const uint8_t m[PARAM_K_BYTES], const uint8_t salt[SALT_BYTES]) {
    uint8_t in[SEED_BYTES + PARAM_K_BYTES + SALT_BYTES];
    
    memcpy(in, hash_ek_kem, SEED_BYTES);
    memcpy(in + SEED_BYTES, m, PARAM_K_BYTES);
    memcpy(in + SEED_BYTES + PARAM_K_BYTES, salt, SALT_BYTES);
    pseudohash((SEED_BYTES + SHARED_SECRET_BYTES) * 8, in, sizeof(in) * 8, output);
}

/**
 * @brief Compute the hash function J (pseudohash-512) with domain separation.
 *
 * @param[out] output       Buffer (64 bytes) to receive the hash output.
 * @param[in]  hash_ek_kem  Hash of the KEM encapsulation key.
 * @param[in]  sigma        The string sigma.
 * @param[in]  c_kem        Pointer to ciphertext struct (includes c_pke.u, c_pke.v, and salt).
 */
void hash_j(uint8_t* output, const uint8_t hash_ek_kem[SEED_BYTES], const uint8_t sigma[PARAM_K_BYTES], const ciphertext_kem_t* c_kem) {
    int i, len = 0;
    uint8_t in[SEED_BYTES + PARAM_K_BYTES + sizeof(ciphertext_kem_t)];

    memcpy(in + len, hash_ek_kem, SEED_BYTES); len += SEED_BYTES;
    memcpy(in + len, sigma, PARAM_K_BYTES); len += PARAM_K_BYTES;
    for (i = 0; i < PARAM_L; i++) {
        memcpy(in + len, (uint8_t*)c_kem->c_pke.u[i], VEC_N_SIZE_BYTES);
        len += VEC_N_SIZE_BYTES;
    }
    memcpy(in + len, (uint8_t*)c_kem->c_pke.v, VEC_N1N2_SIZE_BYTES); len += VEC_N1N2_SIZE_BYTES;
    memcpy(in + len, c_kem->salt, SALT_BYTES); len += SALT_BYTES;
    pseudohash(SHARED_SECRET_BYTES * 8, in, sizeof(in) * 8, output);
}
