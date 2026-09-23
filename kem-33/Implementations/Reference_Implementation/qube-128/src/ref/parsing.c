/**
 * @file parsing.c
 * @brief Serialization helpers for QUBE keys and ciphertexts.
 */

#include "parsing.h"

#include <string.h>

#include "crypto_memset.h"
#include "symmetric.h"
#include "vector.h"

int qube_dk_pke_from_string(uint64_t *y, const uint8_t *dk_pke) {
    qube_xof_stream_t xof;
    uint32_t support[PARAM_OMEGA_MAX] = {0};
    int ret;

    ret = qube_xof_stream_init(&xof, dk_pke);
    if (ret != 0) {
        memset(y, 0, VEC_N_SIZE_64 * sizeof(uint64_t));
        return ret;
    }

    ret = vect_sample_fixed_weight(&xof, y, support, PARAM_OMEGA_Y1);
    qube_xof_stream_release(&xof);
    if (ret != 0) {
        memset(y, 0, VEC_N_SIZE_64 * sizeof(uint64_t));
    }
    memset_zero(support, sizeof support);
    return ret;
}

int qube_ek_pke_from_string(uint64_t *h1, uint64_t *h2, uint64_t *s1, uint64_t *s2, const uint8_t *ek_pke) {
    qube_xof_stream_t xof;
    int ret;

    ret = qube_xof_stream_init(&xof, ek_pke);
    if (ret != 0) {
        memset(h1, 0, VEC_N_SIZE_64 * sizeof(uint64_t));
        memset(h2, 0, VEC_N_SIZE_64 * sizeof(uint64_t));
        memset(s1, 0, VEC_N_SIZE_64 * sizeof(uint64_t));
        memset(s2, 0, VEC_N_SIZE_64 * sizeof(uint64_t));
        return ret;
    }

    ret = vect_set_random(&xof, h1);
    if (ret == 0) {
        ret = vect_set_random(&xof, h2);
    }
    qube_xof_stream_release(&xof);
    if (ret != 0) {
        memset(h1, 0, VEC_N_SIZE_64 * sizeof(uint64_t));
        memset(h2, 0, VEC_N_SIZE_64 * sizeof(uint64_t));
        memset(s1, 0, VEC_N_SIZE_64 * sizeof(uint64_t));
        memset(s2, 0, VEC_N_SIZE_64 * sizeof(uint64_t));
        return ret;
    }

    vect_from_bytes(s1, ek_pke + SEED_BYTES, PARAM_N);
    vect_from_bytes(s2, ek_pke + SEED_BYTES + VEC_N_SIZE_BYTES, PARAM_N);
    return 0;
}

void qube_c_pke_to_string(uint8_t *ct, const ciphertext_pke_t *c_pke) {
    vect_to_bytes(ct, c_pke->u, PARAM_N);
    vect_to_bytes(ct + VEC_N_SIZE_BYTES, c_pke->v, PARAM_N1N2);
}

void qube_c_pke_from_string(ciphertext_pke_t *c_pke, const uint8_t *ct) {
    vect_from_bytes(c_pke->u, ct, PARAM_N);
    vect_from_bytes(c_pke->v, ct + VEC_N_SIZE_BYTES, PARAM_N1N2);
}

void qube_c_kem_to_string(uint8_t *ct, const ciphertext_kem_t *c_kem) {
    qube_c_pke_to_string(ct, &c_kem->c_pke);
    memcpy(ct + VEC_N_SIZE_BYTES + VEC_N1N2_SIZE_BYTES, c_kem->salt, SALT_BYTES);
}

void qube_c_kem_from_string(ciphertext_kem_t *c_kem, const uint8_t *ct) {
    qube_c_pke_from_string(&c_kem->c_pke, ct);
    memcpy(c_kem->salt, ct + VEC_N_SIZE_BYTES + VEC_N1N2_SIZE_BYTES, SALT_BYTES);
}
