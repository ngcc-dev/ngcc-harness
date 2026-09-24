/**
 * @file parsing.c
 * @brief Functions to parse secret key, public key and ciphertext of the MITO scheme
 */

#include "vector.h"
#include "parsing.h"

/**
 * @brief Deserializes a decryption key into its internal vectorized form.
 *
 * @param[out] y        Pointer to the output buffer where the internal vectorized key will be stored.
 * @param[in]  dk_pke   Pointer to the serialized decryption key.
 */
void mito_dk_pke_from_string(uint64_t y[PARAM_L][VEC_N_SIZE_64], const uint8_t* dk_pke) {
    DRNG_ctx dk_xof_ctx = { 0 };
    xof_init(&dk_xof_ctx, dk_pke, SEED_BYTES);

    vect_sample_fixed_weight(&dk_xof_ctx, y, &PARAM_OMEGA[4 - PARAM_L], PARAM_L);
}

/**
 * @brief Deserializes an encryption key into its internal representation.
 *
 * @param[out] h        Pointer to the output buffer for `h` the first internal component of the key.
 * @param[out] s        Pointer to the output buffer for `s` the second internal component of the key.
 * @param[in]  ek_pke   Pointer to the serialized encryption key.
 */
void mito_ek_pke_from_string(uint64_t h[PARAM_L][VEC_N_SIZE_64], uint64_t s[PARAM_L][VEC_N_SIZE_64], const uint8_t* ek_pke) {
    int i;
    DRNG_ctx ek_xof_ctx = { 0 };

    xof_init(&ek_xof_ctx, ek_pke, SEED_BYTES);
    for (i = 0; i < PARAM_L; i++)
    {
        vect_set_random(&ek_xof_ctx, h[i]);
        memcpy(s[i], ek_pke + SEED_BYTES + i * VEC_N_SIZE_BYTES, VEC_N_SIZE_BYTES);
    }
}

/**
 * @brief Serializes a KEM ciphertext structure into a byte array.
 *
 * @param[out] ct       Pointer to the output buffer where the serialized ciphertext will be stored.
 * @param[in]  c_kem    Pointer to the KEM ciphertext structure to be serialized.
 */
void mito_c_kem_to_string(uint8_t *ct, const ciphertext_kem_t *c_kem) {
    int i;

    for (i = 0; i < PARAM_L; i++)
        memcpy(ct + i * VEC_N_SIZE_BYTES, c_kem->c_pke.u[i], VEC_N_SIZE_BYTES);
    memcpy(ct + PARAM_L * VEC_N_SIZE_BYTES, c_kem->c_pke.v, VEC_N1N2_SIZE_BYTES);
    memcpy(ct + PARAM_L * VEC_N_SIZE_BYTES + VEC_N1N2_SIZE_BYTES, c_kem->salt, SALT_BYTES);
}

/**
 * @brief Deserializes a KEM ciphertext byte array into its structured components.
 *
 * @param[out] c_pke    Pointer to the output buffer where the deserialized PKE ciphertext will be stored.
 * @param[out] salt     Pointer to the output buffer where the extracted salt will be stored.
 * @param[in]  ct       Pointer to the serialized KEM ciphertext.
 */
void mito_c_kem_from_string(ciphertext_pke_t *c_pke, uint8_t *salt, const uint8_t *ct) {
    int i;

    for (i = 0; i < PARAM_L; i++)
        memcpy(c_pke->u[i], ct + i * VEC_N_SIZE_BYTES, VEC_N_SIZE_BYTES);
    memcpy(c_pke->v, ct + PARAM_L * VEC_N_SIZE_BYTES, VEC_N1N2_SIZE_BYTES);
    memcpy(salt, ct + PARAM_L * VEC_N_SIZE_BYTES + VEC_N1N2_SIZE_BYTES, SALT_BYTES);
}
