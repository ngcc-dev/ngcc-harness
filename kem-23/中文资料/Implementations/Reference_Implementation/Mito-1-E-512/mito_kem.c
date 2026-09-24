/**
 * @file mito_kem.c
 * @brief Implementation of mito_kem.h
 */

#include "mito_kem.h"
#include "mito_pke.h"
#include "vector.h"

/**
 * @brief Generates a keypair for the KEM (Key Encapsulation Mechanism) scheme.
 *
 * This function generates a public/private keypair used for key encapsulation and decapsulation.
 * The encapsulation key (`ek`) is used to encapsulate a shared secret, while the decapsulation key (`dk`)
 * is used to recover it.
 *
 * @param[out] ek_kem Pointer to the output buffer where the encapsulation key will be stored.
 * @param[out] dk_kem Pointer to the output buffer where the decapsulation key will be stored.
 *
 * @return 0 on success.
 *
 * @pre The PRNG **must be seeded** with ::prng_init() before calling this function.
 * @warning This function calls ::prng_get_bytes() to sample `seed_kem`. If the PRNG has not been
 *          properly seeded beforehand, the generated keys will be insecure/predictable.
 * @note An example of correct seeding is provided in `main_mito.c` (see `init_randomness()`), which
 *       seeds the PRNG using `syscall(SYS_getrandom, ...)` (32 bytes) by default..
 * @see prng_init, prng_get_bytes, main_mito.c
 */
int crypto_kem_keypair(unsigned char *pk, unsigned char *sk) {
    uint8_t seed_kem[SEED_BYTES] = { 0 };
    uint8_t sigma[PARAM_K_BYTES] = { 0 };
    uint8_t seed_pke[SEED_BYTES] = { 0 };
    DRNG_ctx ctx_kem;
    uint8_t dk_pke[SEED_BYTES] = { 0 };
    uint8_t ek_pke[PUBLIC_KEY_BYTES] = { 0 };

    // Sample seed_kem
    prng_get_bytes(seed_kem, SEED_BYTES);

    // Compute seed_pke and randomness sigma
    xof_init(&ctx_kem, seed_kem, SEED_BYTES);
    xof_get_bytes(&ctx_kem, seed_pke, SEED_BYTES);
    xof_get_bytes(&ctx_kem, sigma, PARAM_K_BYTES);

    // Compute MITO-PKE keypair
    mito_pke_keygen(ek_pke, dk_pke, seed_pke);

    // Compute MITO-KEM keypair
    memcpy(pk, ek_pke, PUBLIC_KEY_BYTES);
    memcpy(sk, pk, PUBLIC_KEY_BYTES);
    memcpy(sk + PUBLIC_KEY_BYTES, dk_pke, SEED_BYTES);
    memcpy(sk + PUBLIC_KEY_BYTES + SEED_BYTES, sigma, PARAM_K_BYTES);
    memcpy(sk + PUBLIC_KEY_BYTES + SEED_BYTES + PARAM_K_BYTES, seed_kem, SEED_BYTES);

    return 0;
}

/**
 * @brief Performs key encapsulation using the KEM scheme.
 *
 * This function uses the encapsulation key (`ek`) to generate a ciphertext (`c_kem`) and a shared secret (`K`)..
 *
 * @param[out] c_kem   Pointer to the output buffer where the KEM ciphertext will be stored.
 * @param[out] K       Pointer to the output buffer where the shared secret will be stored.
 * @param[in]  ek_kem      Pointer to the encapsulation key.
 *
 * @return Returns 0 on success.
 *
 * @pre The PRNG **must be seeded** with ::prng_init() before calling this function.
 * @warning This function calls ::prng_get_bytes() to sample `seed_kem`. If the PRNG has not been
 *          properly seeded beforehand, the generated keys will be insecure/predictable.
 * @note An example of correct seeding is provided in `main_mito.c` (see `init_randomness()`), which
 *       seeds the PRNG using `syscall(SYS_getrandom, ...)` (32 bytes) by default..
 * @see prng_init, prng_get_bytes, main_mito.c
 */
int crypto_kem_enc(unsigned char* ct, unsigned char* ss, const unsigned char* pk) {
    ciphertext_kem_t c_kem_t = { 0 };
    uint8_t m[PARAM_K_BYTES] = { 0 };
    uint8_t theta[SEED_BYTES] = { 0 };
    uint8_t hash_ek_kem[SEED_BYTES] = { 0 };
    uint8_t K_theta[SHARED_SECRET_BYTES + SEED_BYTES] = { 0 };

    // Sample message m and salt
    prng_get_bytes(m, PARAM_K_BYTES);
    prng_get_bytes(c_kem_t.salt, SALT_BYTES);

    // Compute shared key K and ciphertext c_kem
    hash_h(hash_ek_kem, pk);
    hash_g(K_theta, hash_ek_kem, m, c_kem_t.salt);
    memcpy(theta, K_theta + SEED_BYTES, SEED_BYTES);
    mito_pke_encrypt(&c_kem_t.c_pke, pk, (uint64_t*)m, theta);

    mito_c_kem_to_string(ct, &c_kem_t);
    memcpy(ss, K_theta, SHARED_SECRET_BYTES);

    return 0;
}

/**
 * @brief Performs key decapsulation using the KEM scheme.
 *
 * This function uses the decapsulation key (`dk`) to recover the shared secret (`K_prime`)
 * from the given KEM ciphertext (`c_kem`), which was generated during encapsulation.
 *
 * @param[out] K_prime   Pointer to the output buffer where the recovered shared secret will be stored.
 * @param[in]  c_kem     Pointer to the input KEM ciphertext.
 * @param[in]  dk_kem    Pointer to the decapsulation key.
 *
 * @return Returns 0 on success.
 */
int crypto_kem_dec(unsigned char* ss, const unsigned char* ct, const unsigned char* sk) {
    int i;
    uint8_t dk_pke[SEED_BYTES] = { 0 };
    uint8_t sigma[PARAM_K_BYTES] = { 0 };
    uint8_t m_prime[PARAM_K_BYTES] = { 0 };
    uint8_t hash_ek_kem[SEED_BYTES] = { 0 };
    uint8_t ek_pke[PUBLIC_KEY_BYTES] = { 0 };
    uint8_t K_theta_prime[SHARED_SECRET_BYTES + SEED_BYTES] = { 0 };
    uint8_t K_bar[SHARED_SECRET_BYTES] = { 0 };
    uint8_t theta_prime[SEED_BYTES] = { 0 };
    ciphertext_kem_t c_kem_t = { 0 };
    ciphertext_kem_t c_kem_prime_t = { 0 };
    uint8_t result;

    // Parse decapsulation key dk_kem
    memcpy(ek_pke, sk, PUBLIC_KEY_BYTES);
    memcpy(dk_pke, sk + PUBLIC_KEY_BYTES, SEED_BYTES);
    memcpy(sigma, sk + PUBLIC_KEY_BYTES + SEED_BYTES, PARAM_K_BYTES);

    // Parse ciphertext c_kem
    mito_c_kem_from_string(&c_kem_t.c_pke, c_kem_t.salt, ct);

    // Compute message m_prime
    result = mito_pke_decrypt((uint64_t*)m_prime, dk_pke, &c_kem_t.c_pke);

    // Compute shared key K_prime and ciphertext c_kem_prime
    hash_h(hash_ek_kem, ek_pke);
    hash_g(K_theta_prime, hash_ek_kem, m_prime, c_kem_t.salt);
    memcpy(ss, K_theta_prime, SHARED_SECRET_BYTES);
    memcpy(theta_prime, K_theta_prime + SHARED_SECRET_BYTES, SEED_BYTES);

    mito_pke_encrypt(&c_kem_prime_t.c_pke, ek_pke, (uint64_t*)m_prime, theta_prime);
    memcpy(c_kem_prime_t.salt, c_kem_t.salt, SALT_BYTES);

    // Compute rejection key K_bar
    hash_j(K_bar, hash_ek_kem, sigma, &c_kem_t);
    for (i = 0; i < PARAM_L; i++)
        result |= vect_compare((uint8_t*)c_kem_t.c_pke.u[i], (uint8_t*)c_kem_prime_t.c_pke.u[i], VEC_N_SIZE_BYTES);
    result |= vect_compare((uint8_t*)c_kem_t.c_pke.v, (uint8_t*)c_kem_prime_t.c_pke.v, VEC_N1N2_SIZE_BYTES);
    result |= vect_compare(c_kem_t.salt, c_kem_prime_t.salt, SALT_BYTES);
    result -= 1;
    for (size_t i = 0; i < SHARED_SECRET_BYTES; ++i) {
        ss[i] = (ss[i] & result) ^ (K_bar[i] & ~result);
    }

    return 0;
}
