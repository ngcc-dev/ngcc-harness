#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "KEM_AlgorithmInstance.h"
#include "params.h"
#include "poly.h"
#include "symmetric.h"
#include "drng.h"

extern DRNG_ctx drng_algorithm;

static int ct_diff(const uint8_t *a, const uint8_t *b, size_t len)
{
    uint8_t acc = 0;
    for (size_t i = 0; i < len; i++)
        acc |= (uint8_t)(a[i] ^ b[i]);
    return acc != 0;
}

unsigned long long kem_get_pk_len_bytes(void) { return NTRE_PUBLICKEYBYTES; }
unsigned long long kem_get_sk_len_bytes(void) { return NTRE_SECRETKEYBYTES; }
unsigned long long kem_get_ss_len_bytes(void) { return NTRE_SSBYTES; }
unsigned long long kem_get_ct_len_bytes(void) { return NTRE_CIPHERTEXTBYTES; }

static void sample_cbd(poly *out, const uint8_t seed[NTRE_SYMBYTES])
{
    uint8_t buf[NTRE_SAMPLEBYTES];

    sample_psi1(buf, seed);
    poly_cbd1(out, buf);
}

static void sample_f(poly *fhat, const uint8_t seed[NTRE_SYMBYTES])
{
    poly tmp;

    sample_cbd(&tmp, seed);
    poly_double_add_one(fhat, &tmp);
    poly_ntt(fhat);
}

static void sample_g(poly *ghat, const uint8_t seed[NTRE_SYMBYTES])
{
    poly tmp;

    sample_cbd(&tmp, seed);
    poly_double(ghat, &tmp);
    poly_ntt(ghat);
}

static int sample_invertible_f(poly *fhat, poly *finv,
                               const uint8_t seed[NTRE_SYMBYTES])
{
    sample_f(fhat, seed);
    return poly_baseinv(finv, fhat);
}

static void cpapke_enc(uint8_t c[NTRE_POLYBYTES],
                       const uint8_t pk[NTRE_PUBLICKEYBYTES],
                       const uint8_t msg[NTRE_MSGBYTES],
                       const uint8_t rho[NTRE_RHOBYTES])
{
    poly hhat, rhat, ehat, cthat;

    poly_frombytes(&hhat, pk);
    poly_cbd1(&rhat, rho);
    poly_ntt(&rhat);
    poly_cbd1_prime(&ehat, msg, rho + NTRE_SAMPLEBYTES);
    poly_ntt(&ehat);
    poly_basemul_add(&cthat, &hhat, &rhat, &ehat);
    poly_tobytes(c, &cthat);
}

static void cpapke_dec(uint8_t msg[NTRE_MSGBYTES],
                       const uint8_t sk_f[NTRE_POLYBYTES],
                       const uint8_t c[NTRE_POLYBYTES])
{
    poly fhat, cthat, m;

    poly_frombytes(&fhat, sk_f);
    poly_frombytes(&cthat, c);
    poly_basemul(&m, &cthat, &fhat);
    poly_invntt(&m);
    poly_msg_mod2_to_bytes(msg, &m);
}

int kem_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes)
{
    uint8_t seed[NTRE_SYMBYTES];
    poly fhat, finv, ghat, hhat;

    if (pk == NULL || pk_len_bytes == NULL ||
        sk == NULL || sk_len_bytes == NULL)
        return -1;

    do {
        get_random_number(&drng_algorithm, seed,
                          (unsigned long long)NTRE_SYMBYTES * 8ULL);
    } while (sample_invertible_f(&fhat, &finv, seed));

    get_random_number(&drng_algorithm, seed,
                      (unsigned long long)NTRE_SYMBYTES * 8ULL);
    sample_g(&ghat, seed);
    poly_basemul(&hhat, &ghat, &finv);

    poly_tobytes(pk, &hhat);
    poly_tobytes(sk, &fhat);
    memcpy(sk + NTRE_POLYBYTES, pk, NTRE_PUBLICKEYBYTES);
    hash_G(sk + NTRE_POLYBYTES + NTRE_PUBLICKEYBYTES, pk);

    *pk_len_bytes = NTRE_PUBLICKEYBYTES;
    *sk_len_bytes = NTRE_SECRETKEYBYTES;
    return 0;
}

int kem_enc(unsigned char *pk, unsigned long long pk_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes,
            unsigned char *ct, unsigned long long *ct_len_bytes)
{
    uint8_t r[NTRE_RBYTES];
    uint8_t g_pk[NTRE_GBYTES];
    uint8_t out_h[HASH_H_OUTBYTES];
    uint8_t mask[NTRE_RBYTES];
    uint8_t msg[NTRE_MSGBYTES];
    const uint8_t *m1;
    const uint8_t *rho;
    const uint8_t *key;

    if (pk == NULL || ss == NULL || ss_len_bytes == NULL ||
        ct == NULL || ct_len_bytes == NULL)
        return -1;
    if (pk_len_bytes != NTRE_PUBLICKEYBYTES)
        return -1;

    get_random_number(&drng_algorithm, r,
                      (unsigned long long)NTRE_RBYTES * 8ULL);
    hash_G(g_pk, pk);
    hash_H(out_h, r, g_pk);

    m1 = out_h;
    rho = out_h + NTRE_M1BYTES;
    key = out_h + NTRE_M1BYTES + NTRE_RHOBYTES;

    memcpy(msg, m1, NTRE_M1BYTES);
    hash_F(mask, m1);
    for (size_t i = 0; i < NTRE_RBYTES; i++)
        msg[NTRE_M1BYTES + i] = (uint8_t)(r[i] ^ mask[i]);

    cpapke_enc(ct, pk, msg, rho);
    memcpy(ss, key, NTRE_SSBYTES);

    *ss_len_bytes = NTRE_SSBYTES;
    *ct_len_bytes = NTRE_CIPHERTEXTBYTES;
    return 0;
}

int kem_dec(unsigned char *sk, unsigned long long sk_len_bytes,
            unsigned char *ct, unsigned long long ct_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes)
{
    uint8_t msg[NTRE_MSGBYTES];
    uint8_t r[NTRE_RBYTES];
    uint8_t out_h[HASH_H_OUTBYTES];
    uint8_t mask[NTRE_RBYTES];
    uint8_t ct_check[NTRE_CIPHERTEXTBYTES];
    const uint8_t *sk_f;
    const uint8_t *pk;
    const uint8_t *g_pk;
    const uint8_t *m1;
    const uint8_t *m2;
    const uint8_t *m1_check;
    const uint8_t *rho_check;
    const uint8_t *key;
    int fail;
    uint8_t mask_byte;

    if (sk == NULL || ct == NULL || ss == NULL || ss_len_bytes == NULL)
        return -1;
    if (sk_len_bytes != NTRE_SECRETKEYBYTES ||
        ct_len_bytes != NTRE_CIPHERTEXTBYTES)
        return -1;

    sk_f = sk;
    pk = sk + NTRE_POLYBYTES;
    g_pk = sk + NTRE_POLYBYTES + NTRE_PUBLICKEYBYTES;

    cpapke_dec(msg, sk_f, ct);

    m1 = msg;
    m2 = msg + NTRE_M1BYTES;
    hash_F(mask, m1);
    for (size_t i = 0; i < NTRE_RBYTES; i++)
        r[i] = (uint8_t)(m2[i] ^ mask[i]);

    hash_H(out_h, r, g_pk);
    m1_check = out_h;
    rho_check = out_h + NTRE_M1BYTES;
    key = out_h + NTRE_M1BYTES + NTRE_RHOBYTES;

    fail = ct_diff(m1, m1_check, NTRE_M1BYTES);
    cpapke_enc(ct_check, pk, msg, rho_check);
    fail |= ct_diff(ct, ct_check, NTRE_CIPHERTEXTBYTES);

    mask_byte = (uint8_t)(fail - 1);
    for (size_t i = 0; i < NTRE_SSBYTES; i++)
        ss[i] = (uint8_t)(key[i] & mask_byte);

    *ss_len_bytes = NTRE_SSBYTES;
    return fail ? -1 : 0;
}
