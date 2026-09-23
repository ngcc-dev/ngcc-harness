/**
 * @file qube.c
 * @brief QUBE-PKE from Algorithm specifications.pdf, Figures 1-3.
 */

#include "qube.h"

#include <string.h>

#include "code.h"
#include "crypto_memset.h"
#include "gf2x.h"
#include "parsing.h"
#include "symmetric.h"
#include "vector.h"

static int derive_public_h(uint64_t *h1, uint64_t *h2, const uint8_t rho0[SEED_BYTES]);

static int derive_public_h(uint64_t *h1, uint64_t *h2, const uint8_t rho0[SEED_BYTES]) {
    qube_xof_stream_t xof;
    int ret = qube_xof_stream_init(&xof, rho0);
    if (ret != 0) {
        return ret;
    }
    ret = vect_set_random(&xof, h1);
    if (ret == 0) {
        ret = vect_set_random(&xof, h2);
    }
    qube_xof_stream_release(&xof);
    return ret;
}

int qube_pke_keygen(uint8_t *pk_pke, uint8_t *sk_pke, const uint8_t rho[SEED_BYTES]) {
    uint8_t seeds[2 * SEED_BYTES] = {0};
    const uint8_t *rho0 = seeds;
    const uint8_t *rho1 = seeds + SEED_BYTES;
    qube_xof_stream_t short_xof;
    uint64_t h1[VEC_N_SIZE_64] = {0};
    uint64_t h2[VEC_N_SIZE_64] = {0};
    uint64_t y[VEC_N_SIZE_64] = {0};
    uint64_t x1[VEC_N_SIZE_64] = {0};
    uint64_t x2[VEC_N_SIZE_64] = {0};
    uint64_t tmp[VEC_N_SIZE_64] = {0};
    uint64_t s1[VEC_N_SIZE_64] = {0};
    uint64_t s2[VEC_N_SIZE_64] = {0};
    uint32_t y_support[PARAM_OMEGA_MAX] = {0};
    uint32_t x_support[PARAM_OMEGA_MAX] = {0};
    int ret;

    ret = qube_sym_prg(seeds, rho);
    if (ret != 0) {
        goto cleanup;
    }

    ret = derive_public_h(h1, h2, rho0);
    if (ret != 0) {
        goto cleanup;
    }

    ret = qube_xof_stream_init(&short_xof, rho1);
    if (ret != 0) {
        goto cleanup;
    }
    ret = vect_sample_fixed_weight(&short_xof, y, y_support, PARAM_OMEGA_Y1);
    if (ret == 0) {
        ret = vect_sample_fixed_weight(&short_xof, x1, x_support, PARAM_OMEGA_X1);
    }
    if (ret == 0) {
        ret = vect_sample_fixed_weight(&short_xof, x2, x_support, PARAM_OMEGA_X2);
    }
    qube_xof_stream_release(&short_xof);
    if (ret != 0) {
        goto cleanup;
    }

    ring_mul_by_support(tmp, h1, y_support, PARAM_OMEGA_Y1);
    vect_add(s1, tmp, x1, VEC_N_SIZE_64);
    ring_mul_by_support(tmp, h2, y_support, PARAM_OMEGA_Y1);
    vect_add(s2, tmp, x2, VEC_N_SIZE_64);

    memcpy(pk_pke, rho0, SEED_BYTES);
    vect_to_bytes(pk_pke + SEED_BYTES, s1, PARAM_N);
    vect_to_bytes(pk_pke + SEED_BYTES + VEC_N_SIZE_BYTES, s2, PARAM_N);
    memcpy(sk_pke, rho1, SEED_BYTES);

cleanup:
    memset_zero(seeds, sizeof seeds);
    memset_zero(h1, sizeof h1);
    memset_zero(h2, sizeof h2);
    memset_zero(y, sizeof y);
    memset_zero(x1, sizeof x1);
    memset_zero(x2, sizeof x2);
    memset_zero(tmp, sizeof tmp);
    memset_zero(s1, sizeof s1);
    memset_zero(s2, sizeof s2);
    memset_zero(y_support, sizeof y_support);
    memset_zero(x_support, sizeof x_support);
    return ret;
}

int qube_pke_encrypt(ciphertext_pke_t *c_pke, const uint8_t *pk_pke,
                     const uint8_t m[PARAM_SECURITY_BYTES], const uint8_t rho_c[SEED_BYTES]) {
    qube_xof_stream_t short_xof;
    uint64_t h1[VEC_N_SIZE_64] = {0};
    uint64_t h2[VEC_N_SIZE_64] = {0};
    uint64_t s1[VEC_N_SIZE_64] = {0};
    uint64_t s2[VEC_N_SIZE_64] = {0};
    uint64_t f1[VEC_N_SIZE_64] = {0};
    uint64_t f2[VEC_N_SIZE_64] = {0};
    uint64_t e[VEC_N_SIZE_64] = {0};
    uint64_t g[VEC_N_SIZE_64] = {0};
    uint64_t tmp1[VEC_N_SIZE_64] = {0};
    uint64_t tmp2[VEC_N_SIZE_64] = {0};
    uint32_t f1_support[PARAM_OMEGA_MAX] = {0};
    uint32_t f2_support[PARAM_OMEGA_MAX] = {0};
    uint32_t scratch_support[PARAM_OMEGA_MAX] = {0};
    int ret;

    ret = qube_ek_pke_from_string(h1, h2, s1, s2, pk_pke);
    if (ret != 0) {
        goto cleanup;
    }

    ret = qube_xof_stream_init(&short_xof, rho_c);
    if (ret != 0) {
        goto cleanup;
    }
    ret = vect_sample_fixed_weight(&short_xof, f1, f1_support, PARAM_OMEGA_R21);
    if (ret == 0) {
        ret = vect_sample_fixed_weight(&short_xof, f2, f2_support, PARAM_OMEGA_R22);
    }
    if (ret == 0) {
        ret = vect_sample_fixed_weight(&short_xof, e, scratch_support, PARAM_OMEGA_E);
    }
    if (ret == 0) {
        ret = vect_sample_fixed_weight(&short_xof, g, scratch_support, PARAM_OMEGA_R11);
    }
    qube_xof_stream_release(&short_xof);
    if (ret != 0) {
        goto cleanup;
    }

    ring_mul_by_support(tmp1, h1, f1_support, PARAM_OMEGA_R21);
    ring_mul_by_support(tmp2, h2, f2_support, PARAM_OMEGA_R22);
    vect_add(c_pke->u, tmp1, tmp2, VEC_N_SIZE_64);
    vect_add(c_pke->u, c_pke->u, g, VEC_N_SIZE_64);

    code_encode(c_pke->v, m);
    ring_mul_by_support(tmp1, s1, f1_support, PARAM_OMEGA_R21);
    ring_mul_by_support(tmp2, s2, f2_support, PARAM_OMEGA_R22);
    vect_add(tmp1, tmp1, tmp2, VEC_N_SIZE_64);
    vect_add(tmp1, tmp1, e, VEC_N_SIZE_64);
    vect_truncate(tmp1);
    vect_add(c_pke->v, c_pke->v, tmp1, VEC_N1N2_SIZE_64);

cleanup:
    memset_zero(h1, sizeof h1);
    memset_zero(h2, sizeof h2);
    memset_zero(s1, sizeof s1);
    memset_zero(s2, sizeof s2);
    memset_zero(f1, sizeof f1);
    memset_zero(f2, sizeof f2);
    memset_zero(e, sizeof e);
    memset_zero(g, sizeof g);
    memset_zero(tmp1, sizeof tmp1);
    memset_zero(tmp2, sizeof tmp2);
    memset_zero(f1_support, sizeof f1_support);
    memset_zero(f2_support, sizeof f2_support);
    memset_zero(scratch_support, sizeof scratch_support);
    return ret;
}

int qube_pke_decrypt(uint8_t m[PARAM_SECURITY_BYTES], const uint8_t sk_pke[SEED_BYTES],
                     const ciphertext_pke_t *c_pke) {
    qube_xof_stream_t short_xof;
    uint64_t y[VEC_N_SIZE_64] = {0};
    uint64_t tmp[VEC_N_SIZE_64] = {0};
    uint64_t em[VEC_N1N2_SIZE_64] = {0};
    uint32_t y_support[PARAM_OMEGA_MAX] = {0};
    int ret;

    ret = qube_xof_stream_init(&short_xof, sk_pke);
    if (ret != 0) {
        return ret;
    }
    ret = vect_sample_fixed_weight(&short_xof, y, y_support, PARAM_OMEGA_Y1);
    qube_xof_stream_release(&short_xof);
    if (ret != 0) {
        goto cleanup;
    }

    ring_mul_by_support(tmp, c_pke->u, y_support, PARAM_OMEGA_Y1);
    vect_truncate(tmp);
    vect_add(em, c_pke->v, tmp, VEC_N1N2_SIZE_64);
    code_decode(m, em);

cleanup:
    memset_zero(y, sizeof y);
    memset_zero(tmp, sizeof tmp);
    memset_zero(em, sizeof em);
    memset_zero(y_support, sizeof y_support);
    return ret;
}
