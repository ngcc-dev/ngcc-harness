/**
 * @file kem.c
 * @brief HARE KEM API implementation
 */

#include <stdint.h>
#include <string.h>
#include "api.h"
#include "crypto_memset.h"
#include "hqc.h"
#include "parameters.h"
#include "parsing.h"
#include "symmetric.h"
#include "vector.h"

#ifdef VERBOSE
#include <stdio.h>
#endif

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
 * @note The caller must seed the API_PKC DRNG before invoking the public API; KAT and self-evaluation harnesses do so deterministically.
 * @see prng_init, prng_get_bytes
 */
int crypto_kem_keypair(uint8_t *ek_kem, uint8_t *dk_kem) {
#ifdef VERBOSE
    printf("\n\n\n### KEYGEN ###");
#endif
    uint8_t seed_kem[SEED_BYTES] = {0};
    uint8_t sigma[PARAM_SECURITY_BYTES] = {0};
    uint8_t seed_pke[SEED_BYTES] = {0};
    shake256_xof_ctx ctx_kem;

    uint8_t ek_pke[PUBLIC_KEY_BYTES] = {0};
    uint8_t dk_pke[SEED_BYTES] = {0};

    // Sample seed_kem
    prng_get_bytes(seed_kem, SEED_BYTES);

    // Compute seed_pke and randomness sigma
    xof_init(&ctx_kem, seed_kem, SEED_BYTES);
    xof_get_bytes(&ctx_kem, seed_pke, SEED_BYTES);
    xof_get_bytes(&ctx_kem, sigma, PARAM_SECURITY_BYTES);

    // Compute HARE-PKE keypair
    hqc_pke_keygen(ek_pke, dk_pke, seed_pke);

    // Compute HARE-KEM keypair
    memcpy(ek_kem, ek_pke, PUBLIC_KEY_BYTES);
    memcpy(dk_kem, ek_kem, PUBLIC_KEY_BYTES);
    memcpy(dk_kem + PUBLIC_KEY_BYTES, dk_pke, SEED_BYTES);
    memcpy(dk_kem + PUBLIC_KEY_BYTES + SEED_BYTES, sigma, PARAM_SECURITY_BYTES);
    memcpy(dk_kem + PUBLIC_KEY_BYTES + SEED_BYTES + PARAM_SECURITY_BYTES, seed_kem, SEED_BYTES);

#ifdef VERBOSE
    printf("\n\nseed_kem: ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", seed_kem[i]);
    printf("\n\nseed_pke: ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", seed_pke[i]);
    printf("\n\nsigma: ");
    for (int i = 0; i < SHARED_SECRET_BYTES; ++i) printf("%02x", sigma[i]);
#endif

    // Zeroize sensitive data
    memset_zero(seed_kem, sizeof seed_kem);
    memset_zero(sigma, sizeof sigma);
    memset_zero(seed_pke, sizeof seed_pke);
    memset_zero(dk_pke, sizeof dk_pke);

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
 * @note The caller must seed the API_PKC DRNG before invoking the public API; KAT and self-evaluation harnesses do so deterministically.
 * @see prng_init, prng_get_bytes
 */
int crypto_kem_enc(uint8_t *c_kem, uint8_t *K, const uint8_t *ek_kem) {
#ifdef VERBOSE
    printf("\n\n\n\n### ENCAPS ###");
#endif

    uint8_t m[PARAM_SECURITY_BYTES] = {0};
    uint8_t K_theta[SHARED_SECRET_BYTES + SEED_BYTES] = {0};
    uint8_t theta[SEED_BYTES] = {0};
    uint8_t hash_ek_kem[SEED_BYTES] = {0};
    ciphertext_kem_t c_kem_t = {0};

    // Sample message m and salt
    // m is SHARED_SECRET_BYTES at KEM level, but only the first PARAM_K bytes are
    // transported by the concatenated-code path (RS message dimension).
    prng_get_bytes(m, PARAM_SECURITY_BYTES);
    prng_get_bytes(c_kem_t.salt, SALT_BYTES);

    // Compute shared key K and ciphertext c_kem
    hash_h(hash_ek_kem, ek_kem);
    hash_g(K_theta, hash_ek_kem, m, c_kem_t.salt);
    memcpy(theta, K_theta + SHARED_SECRET_BYTES, SEED_BYTES);
    hqc_pke_encrypt(&c_kem_t.c_pke, ek_kem, m, theta);

    hqc_c_kem_to_string(c_kem, &c_kem_t);
    memcpy(K, K_theta, SHARED_SECRET_BYTES);

#ifdef VERBOSE
    printf("\n\nek_kem: ");
    for (int i = 0; i < PUBLIC_KEY_BYTES; ++i) printf("%02x", ek_kem[i]);
    printf("\n\nm: ");
    for (size_t i = 0; i < SHARED_SECRET_BYTES; ++i) printf("%02x", m[i]);
    printf("\n\nsalt: ");
    for (int i = 0; i < SALT_BYTES; ++i) printf("%02x", c_kem_t.salt[i]);
    printf("\n\nH(ek_kem): ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", hash_ek_kem[i]);
    printf("\n\ntheta: ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", theta[i]);
    printf("\n\nc_kem: ");
    for (int i = 0; i < CIPHERTEXT_BYTES; ++i) printf("%02x", c_kem[i]);
    printf("\n\nK: ");
    for (int i = 0; i < SHARED_SECRET_BYTES; ++i) printf("%02x", K[i]);
#endif

    // Zeroize sensitive data
    memset_zero(m, sizeof m);
    memset_zero(K_theta, sizeof K_theta);
    memset_zero(theta, sizeof theta);

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
int crypto_kem_dec_status(uint8_t *K_prime, const uint8_t *c_kem, const uint8_t *dk_kem) {
#ifdef VERBOSE
    printf("\n\n\n\n### DECAPS ###");
#endif

    uint8_t ek_pke[PUBLIC_KEY_BYTES] = {0};
    uint8_t dk_pke[SEED_BYTES] = {0};
    uint8_t sigma[PARAM_SECURITY_BYTES] = {0};
    uint8_t m_prime[PARAM_SECURITY_BYTES] = {0};
    uint8_t hash_ek_kem[SEED_BYTES] = {0};
    uint8_t K_theta_prime[SHARED_SECRET_BYTES + SEED_BYTES] = {0};
    uint8_t K_bar[SHARED_SECRET_BYTES] = {0};
    uint8_t c_kem_prime[CIPHERTEXT_BYTES] = {0};
    uint8_t theta_prime[SEED_BYTES] = {0};
    ciphertext_kem_t c_kem_t = {0};
    ciphertext_kem_t c_kem_prime_t = {0};
    uint8_t result;
    uint8_t compare_fail;

    // Parse decapsulation key dk_kem
    memcpy(ek_pke, dk_kem, PUBLIC_KEY_BYTES);
    memcpy(dk_pke, dk_kem + PUBLIC_KEY_BYTES, SEED_BYTES);
    memcpy(sigma, dk_kem + PUBLIC_KEY_BYTES + SEED_BYTES, PARAM_SECURITY_BYTES);

    // Parse ciphertext c_kem
    hqc_c_kem_from_string(&c_kem_t.c_pke, c_kem_t.salt, c_kem);

    // Compute message m_prime
    result = hqc_pke_decrypt(m_prime, dk_pke, &c_kem_t.c_pke);

    // Compute shared key K_prime and ciphertext c_kem_prime
    hash_h(hash_ek_kem, ek_pke);
    hash_g(K_theta_prime, hash_ek_kem, m_prime, c_kem_t.salt);
    memcpy(K_prime, K_theta_prime, SHARED_SECRET_BYTES);
    memcpy(theta_prime, K_theta_prime + SHARED_SECRET_BYTES, SEED_BYTES);

    hqc_pke_encrypt(&c_kem_prime_t.c_pke, ek_pke, m_prime, theta_prime);
    memcpy(c_kem_prime_t.salt, c_kem_t.salt, SALT_BYTES);
    hqc_c_kem_to_string(c_kem_prime, &c_kem_prime_t);

    // Compute rejection key K_bar
    hash_j(K_bar, hash_ek_kem, sigma, c_kem);
    compare_fail = vect_compare(c_kem, c_kem_prime, CIPHERTEXT_BYTES);
    result |= compare_fail;
    result -= 1;
    for (size_t i = 0; i < SHARED_SECRET_BYTES; ++i) {
        K_prime[i] = (K_prime[i] & result) ^ (K_bar[i] & ~result);
    }

#ifdef VERBOSE
    printf("\n\nek_pke: ");
    for (int i = 0; i < PUBLIC_KEY_BYTES; ++i) printf("%02x", ek_pke[i]);
    printf("\n\ndk_pke: ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", dk_pke[i]);
    printf("\n\nc_kem: ");
    for (int i = 0; i < CIPHERTEXT_BYTES; ++i) printf("%02x", c_kem[i]);
    printf("\n\nm_prime: ");
    for (size_t i = 0; i < SHARED_SECRET_BYTES; ++i) printf("%02x", m_prime[i]);
    printf("\n\nH(ek_kem): ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", hash_ek_kem[i]);
    printf("\n\ntheta_prime: ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", theta_prime[i]);
    printf("\n\n\n# Checking Ciphertext - Begin #");
    printf("\n\nc_kem_prime_t.c_pke.u: ");
    vect_print(c_kem_prime_t.c_pke.u, VEC_N_SIZE_BYTES);
    printf("\n\nc_kem_prime_t.c_pke.m_tilde: ");
    vect_print(c_kem_prime_t.c_pke.m_tilde, VEC_NM_SIZE_BYTES);
    printf("\n\nc_kem_prime_t.c_pke.v2: ");
    vect_print(c_kem_prime_t.c_pke.v2, VEC_NV2_SIZE_BYTES);
    printf("\n\nsalt: ");
    for (int i = 0; i < SALT_BYTES; ++i) printf("%02x", c_kem_prime_t.salt[i]);
    printf("\n\n# Checking Ciphertext - End #\n");
    printf("\n\nK_prime: ");
    for (int i = 0; i < SHARED_SECRET_BYTES; ++i) printf("%02x", K_prime[i]);
#endif

    // Zeroize sensitive data
    memset_zero(dk_pke, sizeof dk_pke);
    memset_zero(sigma, sizeof sigma);
    memset_zero(m_prime, sizeof m_prime);
    memset_zero(K_theta_prime, sizeof K_theta_prime);
    memset_zero(K_bar, sizeof K_bar);
    memset_zero(c_kem_prime, sizeof c_kem_prime);
    memset_zero(theta_prime, sizeof theta_prime);

    return compare_fail ? -1 : 0;
}

/**
 * Low-level KEM decapsulation entry point with legacy always-success return semantics.
 *
 * HARE uses FO implicit rejection: a malformed ciphertext is mapped to the
 * rejection key K_bar rather than causing an early abort. This wrapper therefore
 * always returns 0 after writing the selected shared secret.
 *
 * The API_PKC-facing kem_dec() adapter does not call this wrapper; it calls
 * crypto_kem_dec_status() so that API_PKC callers receive -1 when ciphertext
 * verification fails, as required by the KEM interface.
 */
int crypto_kem_dec(uint8_t *K_prime, const uint8_t *c_kem, const uint8_t *dk_kem) {
    (void)crypto_kem_dec_status(K_prime, c_kem, dk_kem);
    return 0;
}
