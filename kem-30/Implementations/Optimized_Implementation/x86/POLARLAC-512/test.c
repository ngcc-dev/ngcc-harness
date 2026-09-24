/*
Copyright (c) 2026 Ying Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
File Description: Implements unit tests for the optimized POLARLAC-512 implementation.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drng.h"
#include "KEM_AlgorithmInstance.h"
#include "ntt.h"
#include "params.h"
#include "polar.h"
#include "poly.h"
#include "pke.h"
#include "sample.h"

DRNG_ctx drng_algorithm;
extern struct polar_control polar;

static int g_failures = 0;

#ifndef TestNum
#define TestNum 100000
#endif

static void report_test(const char *name, int ok)
{
    printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    fflush(stdout);
    if (!ok) {
        g_failures++;
    }
}

static void fill_seed(uint8_t *seed, uint8_t base)
{
    for (int i = 0; i < KEM_SEED_LEN_BYTES; i++) {
        seed[i] = (uint8_t)(base + i);
    }
}

static void fill_message(uint8_t *msg, uint8_t base)
{
    for (int i = 0; i < PKE_MESSAGE_BYTES; i++) {
        msg[i] = (uint8_t)(base + i);
    }
}

static void fill_canonical_poly(int16_t *poly)
{
    for (int i = 0; i < RL_KEM_N; i++) {
        poly[i] = rl_kem_mod_q((int32_t)(i * 37) - 200);
    }
}

static void fill_ternary_pattern(int16_t *poly)
{
    for (int i = 0; i < RL_KEM_N; i++) {
        poly[i] = (int16_t)((i % 3) - 1);
    }
}

static void fill_ternary_pattern_offset(int16_t *poly, int offset)
{
    for (int i = 0; i < RL_KEM_N; i++) {
        int v = (i + offset) % 8;
        poly[i] = (v == 0 || v == 3 || v == 6) ? 1 : ((v == 2 || v == 5) ? -1 : 0);
    }
}

static uint8_t ref_compress_c2_coeff(int16_t c, unsigned int d)
{
    uint32_t mask = (1u << d) - 1u;
    uint32_t x = (uint32_t)rl_kem_mod_q(c);
    uint32_t t = ((x << d) + (RL_KEM_Q >> 1)) / RL_KEM_Q;
    return (uint8_t)(t & mask);
}

static int16_t ref_decompress_c2_coeff(uint8_t c, unsigned int d)
{
    uint32_t half = 1u << (d - 1);
    return (int16_t)((((uint32_t)c * (uint32_t)RL_KEM_Q) + half) >> d);
}

static void naive_negacyclic_mul(int16_t *out, const int16_t *a, const int16_t *b)
{
    int64_t acc[RL_KEM_N];

    memset(acc, 0, sizeof(acc));
    for (int i = 0; i < RL_KEM_N; i++) {
        for (int j = 0; j < RL_KEM_N; j++) {
            int idx = i + j;
            int64_t prod = (int64_t)a[i] * b[j];
            if (idx >= RL_KEM_N) {
                acc[idx - RL_KEM_N] -= prod;
            } else {
                acc[idx] += prod;
            }
        }
    }

    for (int i = 0; i < RL_KEM_N; i++) {
        out[i] = rl_kem_mod_q((int32_t)(acc[i] % RL_KEM_Q));
    }
}

static void poly_zero_coeffs(int16_t *poly)
{
    memset(poly, 0, RL_KEM_N * sizeof(int16_t));
}

static void poly_add_assign_coeffs(int16_t *acc, const int16_t *rhs)
{
    for (int i = 0; i < RL_KEM_N; i++) {
        acc[i] = rl_kem_mod_q((int32_t)acc[i] + rhs[i]);
    }
}

static void poly_ntt_from_coeffs(int16_t *dst, const int16_t *src)
{
    memcpy(dst, src, RL_KEM_N * sizeof(int16_t));
    mq_poly_ntt(dst);
}

static void poly_intt_to_coeffs(int16_t *poly)
{
    mq_poly_intt(poly);
}

static void ntt_pointwise_accumulate(int16_t *acc, const int16_t *a_ntt, const int16_t *b_ntt)
{
    int16_t prod[RL_KEM_N];

    mq_poly_pointwise_mul(prod, (int16_t *)a_ntt, (int16_t *)b_ntt);
    poly_add_assign_coeffs(acc, prod);
}

static int compare_poly_mod_q(const char *name, const int16_t *got, const int16_t *want)
{
    for (int i = 0; i < RL_KEM_N; i++) {
        int16_t got_mod = rl_kem_mod_q(got[i]);
        int16_t want_mod = rl_kem_mod_q(want[i]);
        if (got_mod != want_mod) {
            fprintf(stderr, "%s mismatch at %d: got %d want %d\n",
                name, i, got_mod, want_mod);
            return 0;
        }
    }
    return 1;
}

static int test_uniform_sampler_range(void)
{
    uint8_t seed[PK_SEED_LEN_BYTES];
    polarlac_polymat poly;

    for (int i = 0; i < PK_SEED_LEN_BYTES; i++) {
        seed[i] = (uint8_t)(0x10U + i);
    }
    poly_generate_uniformQ(&poly, seed, 0);
    for (int i = 0; i < RL_KEM_K; i++) {
        for(int j = 0; j < RL_KEM_K; j++){
            for(int t = 0; t < RL_KEM_N; t++){
                if (poly.row[i].vec[j].coeffs[t] < 0 || poly.row[i].vec[j].coeffs[t] >= RL_KEM_Q) {
                    fprintf(stderr, "uniform sampler out of range at %d,%d,%d: %d\n", i, j, t, poly.row[i].vec[j].coeffs[t]);
                    return 0;
                }
            }
        }
        

        
    }
    return 1;
}

static int test_ternary_sampler_range(void)
{
    uint8_t seed[KEM_SEED_LEN_BYTES];
    int16_t poly[RL_KEM_N];

    for (int i = 0; i < KEM_SEED_LEN_BYTES; i++) {
        seed[i] = (uint8_t)(0x80U + i);
    }

    poly_generate_tenary(poly, seed, 0);
    for (int i = 0; i < RL_KEM_N; i++) {
        if (poly[i] < -1 || poly[i] > 1) {
            fprintf(stderr, "ternary sampler out of range at %d: %d\n", i, poly[i]);
            return 0;
        }
    }
    return 1;
}


static int test_poly_compress_zero_selector_roundtrip(void)
{
    int16_t src[RL_KEM_N];
    int16_t dec[RL_KEM_N];
    uint8_t code[PK_POLY_BYTES];

    for (int i = 0; i < RL_KEM_N; i++) {
        src[i] = (int16_t)(i % RL_KEM_Q);
    }

    memset(code, 0xA5, sizeof(code));
    memset(dec, 0, sizeof(dec));
    if (poly_compress(code, src) != 0) {
        fprintf(stderr, "poly_compress zero-selector unexpectedly rejected roundtrip input\n");
        return 0;
    }
    poly_decompress(dec, code);

    for (int i = 0; i < RL_KEM_N; i++) {
        if (dec[i] != src[i]) {
            fprintf(stderr, "poly_compress zero-selector mismatch at %d: got %d want %d\n",
                i, dec[i], src[i]);
            return 0;
        }
    }

    return 1;
}

static int test_poly_compress_zero_selector_capacity(void)
{
    int16_t src[RL_KEM_N];
    int16_t dec[RL_KEM_N];
    uint8_t code[PK_POLY_BYTES];

    for (int i = 0; i < RL_KEM_N; i++) {
        src[i] = 1;
    }
    for (int i = 0; i < PK_ZERO_SELECTOR_BITS; i++) {
        src[i] = (int16_t)((i & 1) << 8);
    }

    memset(code, 0, sizeof(code));
    memset(dec, 0, sizeof(dec));
    if (poly_compress(code, src) != 0) {
        fprintf(stderr, "poly_compress zero-selector rejected exact-capacity input\n");
        return 0;
    }
    poly_decompress(dec, code);
    for (int i = 0; i < RL_KEM_N; i++) {
        if (dec[i] != src[i]) {
            fprintf(stderr, "zero-selector exact-capacity mismatch at %d: got %d want %d\n",
                i, dec[i], src[i]);
            return 0;
        }
    }

    src[PK_ZERO_SELECTOR_BITS] = 0;
    if (poly_compress(code, src) == 0) {
        fprintf(stderr, "poly_compress zero-selector accepted overflow input\n");
        return 0;
    }

    return 1;
}

static int test_polyvec_compress_zero_selector_roundtrip(void)
{
    polarlac_polyvec src;
    polarlac_polyvec dec;
    uint8_t code[PK_LEN_BYTES];

    for (int k = 0; k < RL_KEM_K; k++) {
        for (int i = 0; i < RL_KEM_N; i++) {
            src.vec[k].coeffs[i] = (int16_t)((i * 17 + k * 131) % RL_KEM_Q);
        }
    }

    memset(code, 0x5A, sizeof(code));
    memset(&dec, 0, sizeof(dec));
    if (polyvec_compress(code, &src) != 0) {
        fprintf(stderr, "polyvec_compress zero-selector unexpectedly rejected roundtrip input\n");
        return 0;
    }
    polyvec_decompress(&dec, code);
    for (int k = 0; k < RL_KEM_K; k++) {
        for (int i = 0; i < RL_KEM_N; i++) {
            if (dec.vec[k].coeffs[i] != src.vec[k].coeffs[i]) {
                fprintf(stderr, "polyvec_compress zero-selector mismatch at vec %d coeff %d: got %d want %d\n",
                    k, i, dec.vec[k].coeffs[i], src.vec[k].coeffs[i]);
                return 0;
            }
        }
    }

    return 1;
}

static int test_poly_compress_c2_reference(void)
{
    int16_t src[RL_KEM_N];
    int16_t dec[RL_KEM_N];
    uint8_t code[RL_KEM_Lv];

    fill_canonical_poly(src);
    poly_compress_c2(code, src, D_C2_BITS);

    for (int i = 0; i < RL_KEM_Lv; i++) {
        uint8_t want = ref_compress_c2_coeff(src[i], D_C2_BITS);
        if (code[i] != want) {
            fprintf(stderr, "compress_c2 mismatch at %d: got %u want %u\n",
                i, (unsigned)code[i], (unsigned)want);
            return 0;
        }
    }

    poly_decompress_c2(dec, code, D_C2_BITS);
    for (int i = 0; i < RL_KEM_Lv; i++) {
        int16_t want = ref_decompress_c2_coeff(code[i], D_C2_BITS);
        if (dec[i] != want) {
            fprintf(stderr, "decompress_c2 mismatch at %d: got %d want %d\n",
                i, dec[i], want);
            return 0;
        }
    }
    for (int i = RL_KEM_Lv; i < RL_KEM_N; i++) {
        if (dec[i] != 0) {
            fprintf(stderr, "decompress_c2 tail mismatch at %d: got %d want 0\n",
                i, dec[i]);
            return 0;
        }
    }

    return 1;
}

static int test_pack_c2_dbit_roundtrip(void)
{
    uint8_t src[RL_KEM_Lv];
    uint8_t packed[C2_LEN_BYTES];
    uint8_t unpacked[RL_KEM_Lv];
    uint8_t mask = (uint8_t)((1u << D_C2_BITS) - 1u);

    for (int i = 0; i < RL_KEM_Lv; i++) {
        src[i] = (uint8_t)(i & mask);
    }

    memset(packed, 0, sizeof(packed));
    memset(unpacked, 0, sizeof(unpacked));
    pack_c2_dbit(packed, src);
    unpack_c2_dbit(unpacked, packed);

    for (int i = 0; i < RL_KEM_Lv; i++) {
        if (unpacked[i] != src[i]) {
            fprintf(stderr, "pack/unpack c2 mismatch at %d: got %u want %u\n",
                i, (unsigned)unpacked[i], (unsigned)src[i]);
            return 0;
        }
    }
    return 1;
}


static int test_kem_roundtrip_many(void)
{
    uint8_t drng_seed[64];
    const unsigned long long pk_len_const = kem_get_pk_len_bytes();
    const unsigned long long sk_len_const = kem_get_sk_len_bytes();
    const unsigned long long ct_len_const = kem_get_ct_len_bytes();
    const unsigned long long ss_len_const = kem_get_ss_len_bytes();
    unsigned char *pk = NULL;
    unsigned char *sk = NULL;
    unsigned char *ct = NULL;
    unsigned char *ss = NULL;
    unsigned char *ss_dec = NULL;
    int ok = 0;

    for (int i = 0; i < (int)sizeof(drng_seed); i++) {
        drng_seed[i] = (uint8_t)(0x5AU + (uint8_t)(3 * i));
    }
    if (init_random_number(&drng_algorithm, drng_seed, sizeof(drng_seed)) != 0) {
        fprintf(stderr, "KEM many-test init_random_number failed\n");
        return 0;
    }

    pk = (unsigned char *)malloc((size_t)pk_len_const);
    sk = (unsigned char *)malloc((size_t)sk_len_const);
    ct = (unsigned char *)malloc((size_t)ct_len_const);
    ss = (unsigned char *)malloc((size_t)ss_len_const);
    ss_dec = (unsigned char *)malloc((size_t)ss_len_const);
    if (pk == NULL || sk == NULL || ct == NULL || ss == NULL || ss_dec == NULL) {
        fprintf(stderr, "KEM many-test allocation failed\n");
        goto done;
    }

    printf("Running %d KEM end-to-end roundtrip tests...\n", TestNum);
    fflush(stdout);

    for (int iter = 0; iter < TestNum; iter++) {
        unsigned long long pk_len = pk_len_const;
        unsigned long long sk_len = sk_len_const;
        unsigned long long ct_len = ct_len_const;
        unsigned long long ss_len_enc = ss_len_const;
        unsigned long long ss_len_dec = ss_len_const;

        if (kem_keygen(pk, &pk_len, sk, &sk_len) != 0) {
            fprintf(stderr, "KEM many-test keygen failed at iter %d\n", iter);
            goto done;
        }
        if (pk_len != pk_len_const || sk_len != sk_len_const) {
            fprintf(stderr, "KEM many-test key length mismatch at iter %d\n", iter);
            goto done;
        }
        if (kem_enc(pk, pk_len, ss, &ss_len_enc, ct, &ct_len) != 0) {
            fprintf(stderr, "KEM many-test encapsulation failed at iter %d\n", iter);
            goto done;
        }
        if (ct_len != ct_len_const || ss_len_enc != ss_len_const) {
            fprintf(stderr, "KEM many-test encapsulation length mismatch at iter %d\n", iter);
            goto done;
        }
        if (kem_dec(sk, sk_len, ct, ct_len, ss_dec, &ss_len_dec) != 0) {
            fprintf(stderr, "KEM many-test decapsulation failed at iter %d\n", iter);
            goto done;
        }
        if (ss_len_dec != ss_len_const) {
            fprintf(stderr, "KEM many-test decapsulation length mismatch at iter %d\n", iter);
            goto done;
        }
        if (memcmp(ss, ss_dec, (size_t)ss_len_const) != 0) {
            fprintf(stderr, "KEM many-test shared secret mismatch at iter %d\n", iter);
            goto done;
        }
    }

    ok = 1;

done:
    free(ss_dec);
    free(ss);
    free(ct);
    free(sk);
    free(pk);
    return ok;
}

static int test_kem_ss_len_matches_message_len(void)
{
    return kem_get_ss_len_bytes() == MESSAGE_LEN_BYTES;
}

static int test_ntt_multiplication_chain(void)
{
    int16_t a[RL_KEM_N];
    int16_t b[RL_KEM_N];
    int16_t a_ntt[RL_KEM_N];
    int16_t b_ntt[RL_KEM_N];
    int16_t got[RL_KEM_N];
    int16_t want[RL_KEM_N];
    uint8_t seed[KEM_SEED_LEN_BYTES];

    fill_seed(seed, 0x51U);
    for (int trial = 0; trial < 3; trial++) {
        if (trial == 0) {
            fill_canonical_poly(a);
            fill_ternary_pattern(b);
        } else if (trial == 1) {
            fill_canonical_poly(a);
            fill_ternary_pattern_offset(b, 5);
        } else {
            for (int i = 0; i < RL_KEM_N; i++) {
                a[i] = rl_kem_mod_q((int32_t)(i * 113 + 29));
            }
            poly_generate_tenary(b, seed, 7);
        }

        poly_ntt_from_coeffs(a_ntt, a);
        poly_ntt_from_coeffs(b_ntt, b);
        mq_poly_pointwise_mul(got, a_ntt, b_ntt);
        poly_intt_to_coeffs(got);
        naive_negacyclic_mul(want, a, b);

        if (!compare_poly_mod_q("NTT multiplication chain", got, want)) {
            fprintf(stderr, "NTT multiplication chain failed on trial %d\n", trial);
            return 0;
        }
    }
    return 1;
}

static int test_ntt_kem_noiseless_chain(void)
{
    uint8_t seed_a[PK_SEED_LEN_BYTES];
    uint8_t seed_s[KEM_SEED_LEN_BYTES];
    uint8_t seed_r[KEM_SEED_LEN_BYTES];
    polarlac_polymat a_ntt;
    polarlac_polymat at_ntt;
    polarlac_polyvec s;
    polarlac_polyvec r;
    polarlac_polyvec s_ntt;
    polarlac_polyvec r_ntt;
    polarlac_polyvec b_ntt;
    polarlac_polyvec u_ntt;
    polarlac_polyvec u_wire;
    int16_t v_ntt[RL_KEM_N];
    int16_t v_coeff[RL_KEM_N];
    int16_t su_ntt[RL_KEM_N];
    int16_t su_coeff[RL_KEM_N];
    uint8_t c1[PK_LEN_BYTES];

    for (int i = 0; i < PK_SEED_LEN_BYTES; i++) {
        seed_a[i] = (uint8_t)(0x21U + 7U * i);
    }
    fill_seed(seed_s, 0x63U);
    fill_seed(seed_r, 0xA7U);

    poly_generate_uniformQ(&a_ntt, seed_a, 0);
    poly_generate_uniformQ(&at_ntt, seed_a, 1);

    for (int k = 0; k < RL_KEM_K; k++) {
        poly_generate_tenary(s.vec[k].coeffs, seed_s, (uint8_t)k);
        poly_generate_tenary(r.vec[k].coeffs, seed_r, (uint8_t)(k + RL_KEM_K));
        poly_ntt_from_coeffs(s_ntt.vec[k].coeffs, s.vec[k].coeffs);
        poly_ntt_from_coeffs(r_ntt.vec[k].coeffs, r.vec[k].coeffs);
    }

    for (int i = 0; i < RL_KEM_K; i++) {
        poly_zero_coeffs(b_ntt.vec[i].coeffs);
        poly_zero_coeffs(u_ntt.vec[i].coeffs);
        for (int j = 0; j < RL_KEM_K; j++) {
            ntt_pointwise_accumulate(b_ntt.vec[i].coeffs,
                a_ntt.row[i].vec[j].coeffs, s_ntt.vec[j].coeffs);
            ntt_pointwise_accumulate(u_ntt.vec[i].coeffs,
                at_ntt.row[i].vec[j].coeffs, r_ntt.vec[j].coeffs);
        }
    }
    if (polyvec_compress(c1, &u_ntt) != 0) {
        fprintf(stderr, "NTT KEM noiseless chain failed to pack c1\n");
        return 0;
    }
    polyvec_decompress(&u_wire, c1);

    poly_zero_coeffs(v_ntt);
    poly_zero_coeffs(su_ntt);
    for (int i = 0; i < RL_KEM_K; i++) {
        ntt_pointwise_accumulate(v_ntt, b_ntt.vec[i].coeffs, r_ntt.vec[i].coeffs);
        ntt_pointwise_accumulate(su_ntt, u_wire.vec[i].coeffs, s_ntt.vec[i].coeffs);
    }

    memcpy(v_coeff, v_ntt, sizeof(v_coeff));
    memcpy(su_coeff, su_ntt, sizeof(su_coeff));
    poly_intt_to_coeffs(v_coeff);
    poly_intt_to_coeffs(su_coeff);

    return compare_poly_mod_q("NTT KEM noiseless chain", su_coeff, v_coeff);
}


static int test_poly_mul_against_reference(void)
{
    int16_t a[RL_KEM_N];
    int16_t s[RL_KEM_N];
    int16_t got[RL_KEM_N];
    int16_t want[RL_KEM_N];

    fill_canonical_poly(a);
    fill_ternary_pattern(s);
    poly_mul(got, a, s);
    naive_negacyclic_mul(want, a, s);

    for (int i = 0; i < RL_KEM_N; i++) {
        int16_t got_mod = rl_kem_mod_q(got[i]);
        if (got_mod != want[i]) {
            fprintf(stderr, "poly_mul mismatch at %d: got %d want %d\n",
                i, got_mod, want[i]);
            return 0;
        }
    }
    return 1;
}

static int test_poly_mul_add_against_reference(void)
{
    int16_t a[RL_KEM_N];
    int16_t s[RL_KEM_N];
    int16_t e[RL_KEM_N];
    int16_t got[RL_KEM_N];
    int16_t prod[RL_KEM_N];

    fill_canonical_poly(a);
    fill_ternary_pattern(s);
    fill_ternary_pattern(e);
    poly_mul_add(got, a, s, e);
    naive_negacyclic_mul(prod, a, s);

    for (int i = 0; i < RL_KEM_N; i++) {
        int16_t want = rl_kem_mod_q((int32_t)prod[i] + e[i]);
        if (got[i] != want) {
            fprintf(stderr, "poly_mul_add mismatch at %d: got %d want %d\n",
                i, got[i], want);
            return 0;
        }
    }
    return 1;
}

static int test_poly_mul_add_with_ntt_against_reference(void)
{
    int16_t a[RL_KEM_N];
    int16_t a_ntt[RL_KEM_N];
    int16_t s[RL_KEM_N];
    int16_t e[RL_KEM_N];
    int16_t got[RL_KEM_N];
    int16_t prod[RL_KEM_N];

    fill_canonical_poly(a);
    fill_ternary_pattern(s);
    fill_ternary_pattern(e);
    memcpy(a_ntt, a, sizeof(a_ntt));
    mq_poly_ntt(a_ntt);

    poly_mul_add_with_ntt(got, a_ntt, s, e);
    naive_negacyclic_mul(prod, a, s);

    for (int i = 0; i < RL_KEM_N; i++) {
        int16_t want = rl_kem_mod_q((int32_t)prod[i] + e[i]);
        if (got[i] != want) {
            fprintf(stderr, "poly_mul_add_with_ntt mismatch at %d: got %d want %d\n",
                i, got[i], want);
            return 0;
        }
    }
    return 1;
}

static int test_polar_encode_decode_ideal(void)
{
    uint8_t msg[PKE_MESSAGE_BYTES];
    uint8_t code_m[RL_KEM_Lv];
    uint8_t dec[PKE_MESSAGE_BYTES];
    int16_t hatm[RL_KEM_Lv];
    int expected_n = 0;
    int info_count = 0;

    while ((1u << expected_n) < CODE_LEN * 8) {
        expected_n++;
    }
    for (int i = 0; i < polar.N; i++) {
        info_count += info_nodes[i];
    }

    if (polar.N != CODE_LEN * 8 || polar.n != expected_n ||
            polar.K != (PKE_MESSAGE_BYTES * 8) ||
            polar.ecc_bytes != CODE_LEN ||
            info_count != (PKE_MESSAGE_BYTES * 8)) {
        fprintf(stderr,
            "polar parameter mismatch: N=%d/%d n=%d/%d K=%d/%d ecc_bytes=%d/%d info=%d/%d\n",
            polar.N, (CODE_LEN * 8),
            polar.n, expected_n,
            polar.K, (PKE_MESSAGE_BYTES * 8),
            polar.ecc_bytes, CODE_LEN,
            info_count, (PKE_MESSAGE_BYTES * 8));
        return 0;
    }

    fill_message(msg, 0x3CU);
    Encode_m(code_m, msg);
    for (int i = 0; i < RL_KEM_Lv; i++) {
        hatm[i] = code_m[i] ? RATIO : 0;
    }
    Decode_m(dec, hatm);

    if (memcmp(msg, dec, sizeof(msg)) != 0) {
        fprintf(stderr, "polar encode/decode mismatch\n");
        return 0;
    }
    return 1;
}

static int test_pke_roundtrip(void)
{
    uint8_t seed_kg[KEM_SEED_LEN_BYTES];
    uint8_t seed_enc[KEM_SEED_LEN_BYTES];
    uint8_t pk[PKE_PUBLIC_KEY_BYTES];
    uint8_t sk[PKE_SECRET_KEY_BYTES];
    uint8_t ct[PKE_CIPHERTEXT_BYTES];
    uint8_t msg[PKE_MESSAGE_BYTES];
    uint8_t dec[PKE_MESSAGE_BYTES];

    fill_seed(seed_kg, 0x01U);
    fill_seed(seed_enc, 0xA5U);
    fill_message(msg, 0x30U);

    if (PKE_KeyGen(pk, sk, seed_kg) != 0) {
        fprintf(stderr, "PKE_KeyGen failed\n");
        return 0;
    }

    PKE_Encrypt(ct, pk, msg, seed_enc);
    PKE_Decrypt(dec, ct, sk);
    if (memcmp(msg, dec, sizeof(msg)) != 0) {
        fprintf(stderr, "PKE roundtrip mismatch\n");
        return 0;
    }
    return 1;
}

static int test_kem_roundtrip_and_fallback(void)
{
    uint8_t drng_seed[64];
    unsigned long long pk_len = kem_get_pk_len_bytes();
    unsigned long long sk_len = kem_get_sk_len_bytes();
    unsigned long long ct_len = kem_get_ct_len_bytes();
    unsigned long long ss_len = kem_get_ss_len_bytes();
    unsigned char *pk;
    unsigned char *sk;
    unsigned char *ct;
    unsigned char *ss;
    unsigned char *ss_dec;
    unsigned char *ct_bad;
    unsigned char *ss_bad_1;
    unsigned char *ss_bad_2;
    int ok = 0;

    for (int i = 0; i < (int)sizeof(drng_seed); i++) {
        drng_seed[i] = (uint8_t)(0xC3U + i);
    }
    if (init_random_number(&drng_algorithm, drng_seed, sizeof(drng_seed)) != 0) {
        fprintf(stderr, "init_random_number failed\n");
        return 0;
    }

    pk = (unsigned char *)malloc((size_t)pk_len);
    sk = (unsigned char *)malloc((size_t)sk_len);
    ct = (unsigned char *)malloc((size_t)ct_len);
    ss = (unsigned char *)malloc((size_t)ss_len);
    ss_dec = (unsigned char *)malloc((size_t)ss_len);
    ct_bad = (unsigned char *)malloc((size_t)ct_len);
    ss_bad_1 = (unsigned char *)malloc((size_t)ss_len);
    ss_bad_2 = (unsigned char *)malloc((size_t)ss_len);
    if (pk == NULL || sk == NULL || ct == NULL || ss == NULL || ss_dec == NULL ||
            ct_bad == NULL || ss_bad_1 == NULL || ss_bad_2 == NULL) {
        fprintf(stderr, "KEM test allocation failed\n");
        goto done;
    }

    if (kem_keygen(pk, &pk_len, sk, &sk_len) != 0 ||
            kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len) != 0 ||
            kem_dec(sk, sk_len, ct, ct_len, ss_dec, &ss_len) != 0) {
        fprintf(stderr, "KEM roundtrip call failed\n");
        goto done;
    }
    if (memcmp(ss, ss_dec, (size_t)ss_len) != 0) {
        fprintf(stderr, "KEM shared secret mismatch\n");
        goto done;
    }

    memcpy(ct_bad, ct, (size_t)ct_len);
    ct_bad[0] ^= 0x01U;
    if (kem_dec(sk, sk_len, ct_bad, ct_len, ss_bad_1, &ss_len) != 0 ||
            kem_dec(sk, sk_len, ct_bad, ct_len, ss_bad_2, &ss_len) != 0) {
        fprintf(stderr, "KEM fallback call failed\n");
        goto done;
    }
    if (memcmp(ss_bad_1, ss_bad_2, (size_t)ss_len) != 0) {
        fprintf(stderr, "KEM fallback key is not deterministic\n");
        goto done;
    }
    if (memcmp(ss, ss_bad_1, (size_t)ss_len) == 0) {
        fprintf(stderr, "KEM fallback key unexpectedly matches valid secret\n");
        goto done;
    }

    ok = 1;

done:
    free(ss_bad_2);
    free(ss_bad_1);
    free(ct_bad);
    free(ss_dec);
    free(ss);
    free(ct);
    free(sk);
    free(pk);
    return ok;
}

int main(void)
{
    report_test("uniform sampler range", test_uniform_sampler_range());
    report_test("ternary sampler range", test_ternary_sampler_range());
    report_test("poly_compress zero-selector roundtrip", test_poly_compress_zero_selector_roundtrip());
    report_test("poly_compress zero-selector capacity", test_poly_compress_zero_selector_capacity());
    report_test("polyvec_compress zero-selector roundtrip", test_polyvec_compress_zero_selector_roundtrip());
    report_test("poly_compress_c2/poly_decompress_c2 reference", test_poly_compress_c2_reference());
    report_test("pack_c2_dbit/unpack_c2_dbit", test_pack_c2_dbit_roundtrip());
    report_test("kem ss length matches message length", test_kem_ss_len_matches_message_len());
    report_test("NTT multiplication chain", test_ntt_multiplication_chain());
    report_test("NTT KEM noiseless chain", test_ntt_kem_noiseless_chain());
    report_test("poly_mul against reference", test_poly_mul_against_reference());
    report_test("poly_mul_add against reference", test_poly_mul_add_against_reference());
    report_test("poly_mul_add_with_ntt against reference", test_poly_mul_add_with_ntt_against_reference());
    report_test("polar encode/decode ideal", test_polar_encode_decode_ideal());
    report_test("PKE roundtrip", test_pke_roundtrip());
    report_test("KEM roundtrip and fallback", test_kem_roundtrip_and_fallback());
    report_test("KEM repeated end-to-end roundtrip", test_kem_roundtrip_many());

    if (g_failures != 0) {
        printf("Unit tests finished with %d failure(s)\n", g_failures);
        return 1;
    }

    printf("All unit tests passed\n");
    return 0;
}
