/**
 * @file PKE_AlogrithmInstance.c
 * @brief High-level HEP-QC-PKE API implementation
 */

#include <auxfunc.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "PKE_HEP_QC.h"
#include "code.h"
#include "crypto_memset.h"
#include "gf2x.h"
#include "parameters.h"
#include "symmetric.h"
#include "vector.h"

/**
 * @brief Generates a key pair for the HEP-QC public-key encryption (PKE) scheme.
 *
 * This function creates a public encryption key (`ek_pke`) and a private decryption key (`dk_pke`)
 * for use in the HEP-QC PKE scheme. The key generation process is seeded with the given `seed` input.
 *
 * @param[out] ek_pke  Pointer to the buffer that will receive the encryption key.
 * @param[out] dk_pke  Pointer to the buffer that will receive the decryption key.
 * @param[in]  seed    Pointer to the seed used to deterministically generate the key pair.
 *
 */
void hep_qc_pke_keygen(uint8_t *ek_pke, uint8_t *dk_pke, uint8_t *seed) {
    uint8_t keypair_seed[2 * SEED_BYTES] = {0};
    uint8_t *seed_dk = keypair_seed;
    uint8_t *seed_ek = keypair_seed + SEED_BYTES;
    shake256_xof_ctx dk_xof_ctx = {0};
    shake256_xof_ctx ek_xof_ctx = {0};

    uint64_t x[VEC_N_SIZE_64] = {0};
    uint64_t y[VEC_N_SIZE_64] = {0};
    uint64_t h[VEC_N_SIZE_64] = {0};
    uint64_t s[VEC_N_SIZE_64] = {0};
    uint8_t t[MSG_BIT][MSG_BIT] = {0};
    uint32_t p[VEC_N_SIZE_64_BIT] = {0};
    
    // Derive keypair seeds
    pseudohash(SEED_BYTES * 2 * 8, seed, SEED_BYTES * 8, keypair_seed);

    // Compute decryption key
    xof_init(&dk_xof_ctx, seed_dk, SEED_BYTES);
    vect_sample_fixed_weight1(&dk_xof_ctx, y, PARAM_OMEGA);
    vect_sample_fixed_weight1(&dk_xof_ctx, x, PARAM_OMEGA);
    matrix_set_random(&dk_xof_ctx, t, MSG_BIT);
    permutation_set_random(&dk_xof_ctx, p);

    // Compute encryption key
    xof_init(&ek_xof_ctx, seed_ek, SEED_BYTES);
    vect_set_random(&ek_xof_ctx, h);
    vect_mul(s, y, h);
    vect_add(s, x, s, VEC_N_SIZE_64);

    uint64_t (*intermediate_matrix)[VEC_N_SIZE_64] = calloc(MSG_BIT, sizeof(*intermediate_matrix));
    uint64_t (*G_mask)[VEC_N_SIZE_64] = calloc(MSG_BIT, sizeof(*G_mask));
    code_generator_matrix();
    code_generator_matrix_randomized_permuted(&ek_xof_ctx, intermediate_matrix, p);
    matrix_mul_binary_u8_u64(G_mask, t, intermediate_matrix);

    // Parse encryption key to string
    memcpy(ek_pke, seed_ek, SEED_BYTES);
    memcpy(ek_pke + SEED_BYTES, s, VEC_N_SIZE_BYTES);
    for(size_t i = 0; i < MSG_BIT; i++)
        memcpy(ek_pke + SEED_BYTES + VEC_N_SIZE_BYTES + i * (VEC_N_SIZE_64 * 8), G_mask[i], (VEC_N_SIZE_64 * 8));

    // Parse decryption key to string
    memcpy(dk_pke, seed_dk, SEED_BYTES);

#ifdef VERBOSE
    printf("\n\nseed_dk: ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", seed_dk[i]);
    printf("\n\nseed_ek: ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", seed_ek[i]);
    printf("\n\ny: ");
    vect_print(y, VEC_N_SIZE_BYTES);
    printf("\n\nx: ");
    vect_print(x, VEC_N_SIZE_BYTES);
    printf("\n\nh: ");
    vect_print(h, VEC_N_SIZE_BYTES);
    printf("\n\ns: ");
    vect_print(s, VEC_N_SIZE_BYTES);
#endif

    // Zeroize sensitive data
    memset_zero(keypair_seed, sizeof keypair_seed);
    memset_zero(x, sizeof x);
    memset_zero(y, sizeof y);
    memset_zero(&dk_xof_ctx, sizeof dk_xof_ctx);

    free(intermediate_matrix);
    free(G_mask);
}

/**
 * @brief Encrypts a message using the HEP-QC public-key encryption (PKE) scheme.
 *
 * This function performs encryption in the HEP-QC PKE scheme. It uses the given encryption key (`ek_pke`)
 * and encryption randomness (`theta`) to encrypt the message `m`, producing a ciphertext `c_pke`.
 *
 * @param[out] c_pke     Pointer to the output ciphertext structure (PKE ciphertext).
 * @param[in]  ek_pke    Pointer to the encryption key.
 * @param[in]  m         Pointer to the message to be encrypted.
 * @param[in]  theta     Pointer to the encryption randomness used during encryption.
 *
 */
void hep_qc_pke_encrypt(ciphertext_pke_t *c_pke, const uint8_t *ek_pke, const uint64_t *m, const uint8_t *theta) {
    shake256_xof_ctx theta_xof_ctx = {0};
    uint64_t h[VEC_N_SIZE_64] = {0};
    uint64_t s[VEC_N_SIZE_64] = {0};
    uint64_t r1[VEC_N_SIZE_64] = {0};
    uint64_t r2[VEC_N_SIZE_64] = {0};
    uint64_t e[VEC_N_SIZE_64] = {0};
    uint64_t tmp[VEC_N_SIZE_64] = {0};

    uint64_t (*G_mask)[VEC_N_SIZE_64] = calloc(MSG_BIT, sizeof(*G_mask));

    // Initialize Xof using theta
    xof_init(&theta_xof_ctx, theta, SEED_BYTES);

    // Retrieve h and s from public key
    hep_qc_ek_pke_from_string_all(h, s, G_mask, ek_pke);

    // Generate re, e and r1
    vect_sample_fixed_weight2(&theta_xof_ctx, r2, PARAM_OMEGA_R);
    vect_sample_fixed_weight2(&theta_xof_ctx, e, PARAM_OMEGA_E);
    vect_sample_fixed_weight2(&theta_xof_ctx, r1, PARAM_OMEGA_R);

    // Compute u = r1 + r2.h
    vect_mul(c_pke->u, r2, h);
    vect_add(c_pke->u, r1, c_pke->u, VEC_N_SIZE_64);

    // Compute v = C.encode(m)
    code_encode_matrix(c_pke->v, m, G_mask);

    // Compute v = C.encode(m) + s.r2 + e
    vect_mul(tmp, r2, s);
    vect_add(tmp, e, tmp, VEC_N_SIZE_64);
    vect_add(c_pke->v, c_pke->v, tmp, VEC_N1N2_SIZE_64);

#ifdef VERBOSE
    printf("\n\nh: ");
    vect_print(h, VEC_N_SIZE_BYTES);
    printf("\n\ns: ");
    vect_print(s, VEC_N_SIZE_BYTES);
    printf("\n\nr1: ");
    vect_print(r1, VEC_N_SIZE_BYTES);
    printf("\n\nr2: ");
    vect_print(r2, VEC_N_SIZE_BYTES);
    printf("\n\ne: ");
    vect_print(e, VEC_N_SIZE_BYTES);
    printf("\n\nTruncate(s.r2 + e): ");
    vect_print(tmp, VEC_N1N2_SIZE_BYTES);
    printf("\n\nc_pke->u: ");
    vect_print(c_pke->u, VEC_N_SIZE_BYTES);
    printf("\n\nc_pke->v: ");
    vect_print(c_pke->v, VEC_N1N2_SIZE_BYTES);
#endif

    // Zeroize sensitive data
    memset_zero(r1, sizeof r1);
    memset_zero(r2, sizeof r2);
    memset_zero(e, sizeof e);
    memset_zero(tmp, sizeof tmp);
    memset_zero(&theta_xof_ctx, sizeof theta_xof_ctx);
}

/**
 * @brief Decrypts a ciphertext using the HEP-QC public-key encryption (PKE) scheme.
 *
 * This function performs decryption in the HEP-QC PKE scheme. It uses the given decryption key (`dk_pke`)
 * to decrypt the ciphertext `c_pke`, recovering the original message `m`.
 *
 * @param[out] m         Pointer to the output buffer where the decrypted message will be stored.
 * @param[in]  dk_pke    Pointer to the decryption key.
 * @param[in]  c_pke     Pointer to the input ciphertext structure (PKE ciphertext).
 *
 * @return Returns 0 on success.
 *
 */
uint8_t hep_qc_pke_decrypt(uint64_t *m, const uint8_t *dk_pke, const ciphertext_pke_t *c_pke) {
    // Parse decryption key dk_pke
    uint64_t y[VEC_N_SIZE_64] = {0};
    uint8_t t[MSG_BIT][MSG_BIT] = {0};
    uint32_t p[VEC_N_SIZE_64_BIT] = {0};
    hep_qc_dk_pke_from_string_all(y, t, p, dk_pke);
    
    uint64_t tmp1[VEC_N_SIZE_64] = {0};
    uint64_t tmp2[VEC_N_SIZE_64] = {0};
    // Compute u.y
    vect_mul(tmp1, y, c_pke->u);
    // Compute v - u.y
    vect_add(tmp2, c_pke->v, tmp1, VEC_N1N2_SIZE_64);

    vect_permute_columns_inplace(tmp2, p);

#ifdef VERBOSE
    printf("\n\nc_pke.u: ");
    vect_print(c_pke->u, VEC_N_SIZE_BYTES);
    printf("\n\nc_pke.v: ");
    vect_print(c_pke->v, VEC_N1N2_SIZE_BYTES);
    printf("\n\ny: ");
    vect_print(y, VEC_N_SIZE_BYTES);
    printf("\n\nTruncate(u.y): ");
    vect_print(tmp1, VEC_N1N2_SIZE_BYTES);
    printf("\n\nv - Truncate(u.y): ");
    vect_print(tmp2, VEC_N1N2_SIZE_BYTES);
#endif

    // Compute intermediate_text mt
    uint64_t mt[CEIL_DIVIDE(PARAM_K, 8)] = {0};
    code_decode(mt, tmp2);

    uint8_t t_inverse[MSG_BIT][MSG_BIT] = {0};
    invertible_matrix(t_inverse, t, MSG_BIT);
    msg_matrix_mul(m, mt, t_inverse);

    // Zeroize sensitive data
    memset_zero(y, sizeof y);
    memset_zero(tmp1, sizeof tmp1);
    memset_zero(tmp2, sizeof tmp2);

    return 0;
}
