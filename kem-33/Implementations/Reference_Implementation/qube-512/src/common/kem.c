/**
 * @file kem.c
 * @brief QUBE.KEM from Algorithm specifications.pdf, Figures 4-6.
 */

#include <stdint.h>
#include <string.h>

#include "api.h"
#include "crypto_memset.h"
#include "drng.h"
#include "kem_qube.h"
#include "parameters.h"
#include "parsing.h"
#include "qube.h"
#include "symmetric.h"
#include "vector.h"

extern DRNG_ctx drng_algorithm;

static int random_bytes(uint8_t *out, size_t out_len);
static int kem_encaps_deterministic(uint8_t *ct, uint8_t *ss, const uint8_t *pk,
                                    const uint8_t m[PARAM_SECURITY_BYTES], const uint8_t sigma[SALT_BYTES]);

static int random_bytes(uint8_t *out, size_t out_len) {
    return get_random_number(&drng_algorithm, out, (unsigned long long)out_len * 8);
}

static int kem_encaps_deterministic(uint8_t *ct, uint8_t *ss, const uint8_t *pk,
                                    const uint8_t m[PARAM_SECURITY_BYTES], const uint8_t sigma[SALT_BYTES]) {
    uint8_t h_pk[SEED_BYTES] = {0};
    uint8_t g_out[2 * SEED_BYTES] = {0};
    ciphertext_kem_t c_kem;
    int ret;

    memset(&c_kem, 0, sizeof c_kem);

    ret = qube_sym_h(h_pk, pk);
    if (ret != 0) {
        goto cleanup;
    }
    ret = qube_sym_g(g_out, m, sigma, h_pk);
    if (ret != 0) {
        goto cleanup;
    }
    ret = qube_pke_encrypt(&c_kem.c_pke, pk, m, g_out + SHARED_SECRET_BYTES);
    if (ret != 0) {
        goto cleanup;
    }

    memcpy(c_kem.salt, sigma, SALT_BYTES);
    qube_c_kem_to_string(ct, &c_kem);
    memcpy(ss, g_out, SHARED_SECRET_BYTES);

cleanup:
    memset_zero(h_pk, sizeof h_pk);
    memset_zero(g_out, sizeof g_out);
    memset_zero(&c_kem, sizeof c_kem);
    return ret;
}

int crypto_kem_keypair(uint8_t *pk, uint8_t *sk) {
    uint8_t rho[SEED_BYTES] = {0};
    uint8_t rho_k[SEED_BYTES] = {0};
    uint8_t sk_pke[SEED_BYTES] = {0};
    int ret;

    if (pk == NULL || sk == NULL) {
        return -1;
    }

    ret = random_bytes(rho, sizeof rho);
    if (ret != 0) {
        goto cleanup;
    }
    ret = random_bytes(rho_k, sizeof rho_k);
    if (ret != 0) {
        goto cleanup;
    }

    ret = qube_pke_keygen(pk, sk_pke, rho);
    if (ret != 0) {
        goto cleanup;
    }

    memcpy(sk, sk_pke, SEED_BYTES);
    memcpy(sk + SEED_BYTES, pk, PUBLIC_KEY_BYTES);
    memcpy(sk + SEED_BYTES + PUBLIC_KEY_BYTES, rho_k, SEED_BYTES);

cleanup:
    if (ret != 0) {
        memset(sk, 0, SECRET_KEY_BYTES);
        memset(pk, 0, PUBLIC_KEY_BYTES);
    }
    memset_zero(rho, sizeof rho);
    memset_zero(rho_k, sizeof rho_k);
    memset_zero(sk_pke, sizeof sk_pke);
    return ret;
}

int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk) {
    uint8_t m[PARAM_SECURITY_BYTES] = {0};
    uint8_t sigma[SALT_BYTES] = {0};
    int ret;

    if (ct == NULL || ss == NULL || pk == NULL) {
        return -1;
    }

    ret = random_bytes(m, sizeof m);
    if (ret != 0) {
        goto cleanup;
    }
    ret = random_bytes(sigma, sizeof sigma);
    if (ret != 0) {
        goto cleanup;
    }

    ret = kem_encaps_deterministic(ct, ss, pk, m, sigma);

cleanup:
    if (ret != 0) {
        memset(ct, 0, CIPHERTEXT_BYTES);
        memset(ss, 0, SHARED_SECRET_BYTES);
    }
    memset_zero(m, sizeof m);
    memset_zero(sigma, sizeof sigma);
    return ret;
}

int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk) {
    const uint8_t *sk_pke;
    const uint8_t *pk;
    const uint8_t *rho_k;
    ciphertext_kem_t c_kem;
    uint8_t m_prime[PARAM_SECURITY_BYTES] = {0};
    uint8_t ct_prime[CIPHERTEXT_BYTES] = {0};
    uint8_t ss_prime[SHARED_SECRET_BYTES] = {0};
    uint8_t ss_fallback[SHARED_SECRET_BYTES] = {0};
    uint8_t h_pk[SEED_BYTES] = {0};
    int ret;
    uint8_t mismatch;

    if (ss == NULL || ct == NULL || sk == NULL) {
        return -1;
    }

    sk_pke = sk;
    pk = sk + SEED_BYTES;
    rho_k = sk + SEED_BYTES + PUBLIC_KEY_BYTES;

    memset(&c_kem, 0, sizeof c_kem);
    qube_c_kem_from_string(&c_kem, ct);

    ret = qube_pke_decrypt(m_prime, sk_pke, &c_kem.c_pke);
    if (ret != 0) {
        goto cleanup;
    }

    ret = kem_encaps_deterministic(ct_prime, ss_prime, pk, m_prime, c_kem.salt);
    if (ret != 0) {
        goto cleanup;
    }

    ret = qube_sym_h(h_pk, pk);
    if (ret != 0) {
        goto cleanup;
    }
    ret = qube_sym_j(ss_fallback, rho_k, ct, h_pk);
    if (ret != 0) {
        goto cleanup;
    }

    mismatch = vect_compare(ct, ct_prime, CIPHERTEXT_BYTES);
    vect_select(ss, ss_prime, ss_fallback, SHARED_SECRET_BYTES, mismatch);

cleanup:
    if (ret != 0) {
        memset(ss, 0, SHARED_SECRET_BYTES);
    }
    memset_zero(&c_kem, sizeof c_kem);
    memset_zero(m_prime, sizeof m_prime);
    memset_zero(ct_prime, sizeof ct_prime);
    memset_zero(ss_prime, sizeof ss_prime);
    memset_zero(ss_fallback, sizeof ss_fallback);
    memset_zero(h_pk, sizeof h_pk);
    return ret;
}
