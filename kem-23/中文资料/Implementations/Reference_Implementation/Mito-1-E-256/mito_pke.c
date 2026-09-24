/**
 * @file mito_pke.c
 * @brief High-level MITO-PKE API implementation
 */

#include "code.h"
#include "gf2x.h"
#include "vector.h"
#include "mito_pke.h"

 /**
  * @brief Generates a key pair for the MITO public-key encryption (PKE) scheme.
  *
  * This function creates a public encryption key (`ek_pke`) and a private decryption key (`dk_pke`)
  * for use in the MITO PKE scheme. The key generation process is seeded with the given `seed` input.
  *
  * @param[out] ek_pke  Pointer to the buffer that will receive the encryption key.
  * @param[out] dk_pke  Pointer to the buffer that will receive the decryption key.
  * @param[in]  seed    Pointer to the seed used to deterministically generate the key pair.
  *
  */
void mito_pke_keygen(uint8_t* ek_pke, uint8_t* dk_pke, uint8_t* seed) {
    int i;
    uint8_t keypair_seed[2 * SEED_BYTES] = { 0 };
    uint8_t* seed_dk = keypair_seed;
    uint8_t* seed_ek = keypair_seed + SEED_BYTES;
    DRNG_ctx dk_xof_ctx = { 0 };
    DRNG_ctx ek_xof_ctx = { 0 };

    uint64_t x[PARAM_L][VEC_N_SIZE_64] = { 0 };
    uint64_t h[PARAM_L][VEC_N_SIZE_64] = { 0 };
    uint64_t y[PARAM_L][VEC_N_SIZE_64] = { 0 };
    uint64_t s[PARAM_L][VEC_N_SIZE_64] = { 0 };

    // Derive keypair seeds
    hash_i(keypair_seed, seed);

    // Compute decryption key
    xof_init(&dk_xof_ctx, seed_dk, SEED_BYTES);

    vect_sample_fixed_weight(&dk_xof_ctx, y, &PARAM_OMEGA[4 - PARAM_L], PARAM_L);
    vect_sample_fixed_weight(&dk_xof_ctx, x, &PARAM_OMEGA[0], PARAM_L);

    // Compute encryption key
    xof_init(&ek_xof_ctx, seed_ek, SEED_BYTES);
    for (i = 0; i < PARAM_L; i++)
        vect_set_random(&ek_xof_ctx, h[i]);

    // Compute s = x + y.h
    vect_mul(s, h, y);

    // Parse encryption key to string
    memcpy(ek_pke, seed_ek, SEED_BYTES);
    for (i = 0; i < PARAM_L; i++)
    {
        vect_add(s[i], s[i], x[i], VEC_N_SIZE_64);
        memcpy(ek_pke + SEED_BYTES + i * VEC_N_SIZE_BYTES, s[i], VEC_N_SIZE_BYTES);
    }

    // Parse decryption key to string
    memcpy(dk_pke, seed_dk, SEED_BYTES);
}

/**
 * @brief Encrypts a message using the MITO public-key encryption (PKE) scheme.
 *
 * This function performs encryption in the MITO PKE scheme. It uses the given encryption key (`ek_pke`)
 * and encryption randomness (`theta`) to encrypt the message `m`, producing a ciphertext `c_pke`.
 *
 * @param[out] c_pke     Pointer to the output ciphertext structure (PKE ciphertext).
 * @param[in]  ek_pke    Pointer to the encryption key.
 * @param[in]  m         Pointer to the message to be encrypted.
 * @param[in]  theta     Pointer to the encryption randomness used during encryption.
 *
 */
void mito_pke_encrypt(ciphertext_pke_t* c_pke, const uint8_t* ek_pke, const uint64_t* m, const uint8_t* theta) {
    int i, OMEGA_E = PARAM_OMEGA_E;
    DRNG_ctx theta_xof_ctx = { 0 };
    uint64_t h[PARAM_L][VEC_N_SIZE_64] = { 0 };
    uint64_t s[PARAM_L][VEC_N_SIZE_64] = { 0 };
    uint64_t r[PARAM_L][VEC_N_SIZE_64] = { 0 };
    uint64_t t[PARAM_L][VEC_N_SIZE_64] = { 0 };
    uint64_t e[VEC_N_SIZE_64] = { 0 };
    uint64_t tmp[2][VEC_N_SIZE_64] = { 0 };

    // Initialize Xof using theta
    xof_init(&theta_xof_ctx, theta, SEED_BYTES);

    // Retrieve h and s from public key
    mito_ek_pke_from_string(h, s, ek_pke);

    // Generate r, t and e
    vect_sample_fixed_weight(&theta_xof_ctx, r, &PARAM_OMEGA_R[0], PARAM_L);
    vect_sample_fixed_weight(&theta_xof_ctx, t, &PARAM_OMEGA_R[4 - PARAM_L], PARAM_L);
    vect_sample_fixed_weight(&theta_xof_ctx, &e, (int*)&OMEGA_E, 1);

    // Compute u = r + t.h
    vect_mul(c_pke->u, h, t);

    // Compute v = C.encode(m)
    code_encode(c_pke->v, m);

    // Compute v = C.encode(m) + Truncate(s.t + e)
    for (i = 0; i < PARAM_L; i++)
    {
        vect_mul_1(tmp[0], t[i], s[i]);
        vect_add(tmp[1], tmp[1], tmp[0], VEC_N_SIZE_64);
        vect_add(c_pke->u[i], c_pke->u[i], r[i], VEC_N_SIZE_64);
    }
    vect_add(tmp[1], tmp[1], e, VEC_N_SIZE_64);
    vect_truncate(tmp[1]);
    vect_add(c_pke->v, c_pke->v, tmp[1], VEC_N1N2_SIZE_64);
}

/**
 * @brief Decrypts a ciphertext using the MITO public-key encryption (PKE) scheme.
 *
 * This function performs decryption in the MITO PKE scheme. It uses the given decryption key (`dk_pke`)
 * to decrypt the ciphertext `c_pke`, recovering the original message `m`.
 *
 * @param[out] m         Pointer to the output buffer where the decrypted message will be stored.
 * @param[in]  dk_pke    Pointer to the decryption key.
 * @param[in]  c_pke     Pointer to the input ciphertext structure (PKE ciphertext).
 *
 * @return Returns 0 on success.
 *
 */
uint8_t mito_pke_decrypt(uint64_t* m, const uint8_t* dk_pke, const ciphertext_pke_t* c_pke) {
    int i;
    uint64_t y[PARAM_L][VEC_N_SIZE_64] = { 0 };
    uint64_t tmp[2][VEC_N_SIZE_64] = { 0 };

    // Parse decryption key dk_pke
    mito_dk_pke_from_string(y, dk_pke);

    // Compute u.y
    for (i = 0; i < PARAM_L; i++)
    {
        vect_mul_1(tmp[0], c_pke->u[i], y[i]);
        vect_add(tmp[1], tmp[1], tmp[0], VEC_N_SIZE_64);
    }
    // Truncate(u.y)
    vect_truncate(tmp[1]);
    // Compute v - Truncate(u.y)
    vect_add(tmp[1], c_pke->v, tmp[1], VEC_N1N2_SIZE_64);

    // Compute plaintext m
    code_decode(m, tmp[1]);

    return 0;
}
