#include <stdlib.h>
#include <string.h>
#include "../../common/sha3/fips202.h"
#include "../../common/random/random.h"
#include "frost_u32_core.h"
#include "frost_macrify.h"
#include <stdio.h>
#include <time.h>
#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
#endif

#define DITHER_DOMAIN_PK 0xA1
#define DITHER_DOMAIN_U  0xB1
#define DITHER_DOMAIN_V  0xC1
#ifndef FROST_U32_A_WORD_BYTES
#define FROST_U32_A_WORD_BYTES 3
#endif
#if (FROST_U32_A_WORD_BYTES != 3) && (FROST_U32_A_WORD_BYTES != 4)
#error "FROST_U32_A_WORD_BYTES must be 3 or 4"
#endif
#define AROW_WORDS_ONE_ROW ((size_t)PARAMS_N)
#define AROW_XOF_ONE_ROW ((size_t)PARAMS_N * FROST_U32_A_WORD_BYTES)

#ifndef PARAMS_NBAR_R
#define PARAMS_NBAR_R PARAMS_NBAR
#endif
#ifndef PARAMS_NBAR_S
#define PARAMS_NBAR_S PARAMS_NBAR
#endif
#define PK_PACKED_BYTES ((PARAMS_PK_LOGP * PARAMS_N * PARAMS_NBAR_R) / 8)
#define CT_C1_PACKED_BYTES ((PARAMS_U_LOGP * PARAMS_N * PARAMS_NBAR_S) / 8)
#define CT_C2_PACKED_BYTES ((PARAMS_V_LOGP * PARAMS_NBAR_R * PARAMS_NBAR_S) / 8)
#define PARAMS_SECRET_COEFF_BITS (((2u * PARAMS_ETA_S + 1u) <= 2u) ? 1u : \
                                  (((2u * PARAMS_ETA_S + 1u) <= 4u) ? 2u : \
                                   (((2u * PARAMS_ETA_S + 1u) <= 8u) ? 3u : \
                                    (((2u * PARAMS_ETA_S + 1u) <= 16u) ? 4u : 5u))))
#define SK_PKE_PACKED_BYTES (((PARAMS_SECRET_COEFF_BITS * PARAMS_N * PARAMS_NBAR_R) + 7u) / 8u)

#define SK_OFFSET_S 0
#define SK_OFFSET_PK (SK_OFFSET_S + SK_PKE_PACKED_BYTES)
#define SK_OFFSET_PKH (SK_OFFSET_PK + CRYPTO_PUBLICKEYBYTES)
#define SK_OFFSET_Z (SK_OFFSET_PKH + BYTES_PKHASH)

static inline uint32_t qmask_u32(void) { return (1u << PARAMS_LOGQ) - 1u; }
static inline uint32_t pmask_u32(unsigned logp) { return (1u << logp) - 1u; }
static int bench_verbose_enabled(void)
{
    const char *a = getenv("BENCH_VERBOSE");
    const char *b = getenv("DEBUG_BENCH");
    return ((a != NULL && strcmp(a, "1") == 0) || (b != NULL && strcmp(b, "1") == 0));
}
static void bench_log(const char *fn, const char *msg)
{
    if (!bench_verbose_enabled()) return;
    fprintf(stderr, "[bench-u32] level=%d fn=%s %s\n", PARAMS_N, fn, msg);
    fflush(stderr);
}
static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}
static unsigned long long now_cycles(void)
{
#if defined(__x86_64__) || defined(__i386__)
    return __rdtsc();
#else
    return (unsigned long long)(now_s() * 1e9);
#endif
}
static int u32_profile_enabled(void)
{
    const char *p = getenv("PROFILE_U32");
    const char *q = getenv("PROFILE_U32_AVX2");
    return ((p != NULL && strcmp(p, "1") == 0) || (q != NULL && strcmp(q, "1") == 0));
}
static size_t u32_row_batch_rows(void)
{
    const size_t default_rows = (PARAMS_N == 2176) ? 4u : 16u;
    const char *p = getenv("FROST_U32_ROW_BATCH");
    if (p == NULL || *p == '\0') return default_rows;
    long v = strtol(p, NULL, 10);
    if (v == 1 || v == 4 || v == 8 || v == 16) return (size_t)v;
    return default_rows;
}
static void u32_profile_report(const char *fn, const char *stage, unsigned long long cyc, unsigned long long total)
{
    if (!u32_profile_enabled()) return;
    double pct = (total == 0) ? 0.0 : (100.0 * (double)cyc / (double)total);
    fprintf(stderr, "[profile-u32] level=%d fn=%s stage=%s cycles=%llu (%.2f%%)\n",
            PARAMS_N, fn, stage, cyc, pct);
}
static void u32_profile_report_counts(const char *fn, const frost_u32_fast_stats_t *st)
{
    if (!u32_profile_enabled()) return;
    fprintf(stderr,
            "[profile-u32-counts] level=%d fn=%s expand_rows=%llu shake_init=%llu shake_squeeze=%llu bytes_squeezed=%llu rows_per_batch=%llu coeff_parse_bytes=%llu shake4x=%llu mac_ops=%llu matrix_products=%llu\n",
            PARAMS_N, fn,
            (unsigned long long)st->expand_row_calls,
            (unsigned long long)st->shake_init_calls,
            (unsigned long long)st->shake_squeeze_calls,
            (unsigned long long)st->bytes_squeezed_for_a,
            (unsigned long long)st->a_rows_per_shake_batch,
            (unsigned long long)st->coeff_parse_mode,
            (unsigned long long)st->shake4x_used,
            (unsigned long long)st->mac_ops,
            (unsigned long long)st->matrix_products);
}

static int8_t ct_verify_u32(const uint32_t *a, const uint32_t *b, size_t len)
{
    uint32_t r = 0;
    for (size_t i = 0; i < len; i++) r |= a[i] ^ b[i];
    return (r == 0) ? 0 : -1;
}

static int expand_dither_u32(uint32_t *d, size_t n, const uint8_t *seed, size_t seedlen, uint8_t domain, unsigned logp)
{
    unsigned shift = PARAMS_LOGQ - logp;
    uint32_t mask = (1u << shift) - 1u;
    uint8_t in[1 + BYTES_SEED_A + BYTES_SALT] = {0};
    uint8_t *buf = (uint8_t *)malloc(n * 4);
    if (!buf) return 1;
    in[0] = domain;
    memcpy(&in[1], seed, seedlen);
    shake(buf, n * 4, in, 1 + seedlen);
    for (size_t i = 0; i < n; i++) {
        uint32_t v = (uint32_t)buf[4*i] | ((uint32_t)buf[4*i+1] << 8) | ((uint32_t)buf[4*i+2] << 16) | ((uint32_t)buf[4*i+3] << 24);
        d[i] = v & mask;
    }
    clear_bytes(buf, n * 4);
    free(buf);
    return 0;
}

static uint32_t quantize_u32(uint32_t x, uint32_t d, unsigned logp)
{
    unsigned shift = PARAMS_LOGQ - logp;
    uint32_t z = (x & qmask_u32()) + (d & ((1u << shift) - 1u));
    z = (z + (1u << (shift - 1))) >> shift;
    return z & pmask_u32(logp);
}

static int quantize_dithered_u32(uint32_t *out, const uint32_t *in, size_t n, const uint8_t *seed, size_t seedlen, uint8_t domain, unsigned logp)
{
    uint32_t *d = (uint32_t *)malloc(n * sizeof(uint32_t));
    if (!d) return 1;
    if (expand_dither_u32(d, n, seed, seedlen, domain, logp) != 0) { free(d); return 1; }
    for (size_t i = 0; i < n; i++) out[i] = quantize_u32(in[i], d[i], logp);
    clear_bytes((uint8_t *)d, n * sizeof(uint32_t));
    free(d);
    return 0;
}

static int reconstruct_dithered_u32(uint32_t *normal, const uint32_t *split, size_t n, const uint8_t *seed, size_t seedlen, uint8_t domain, unsigned logp)
{
    uint32_t *d = (uint32_t *)malloc(n * sizeof(uint32_t));
    if (!d) return 1;
    if (expand_dither_u32(d, n, seed, seedlen, domain, logp) != 0) { free(d); return 1; }
    for (size_t i = 0; i < n; i++) {
        uint32_t r = (split[i] & pmask_u32(logp)) << (PARAMS_LOGQ - logp);
        normal[i] = (r - d[i]) & qmask_u32();
    }
    clear_bytes((uint8_t *)d, n * sizeof(uint32_t));
    free(d);
    return 0;
}

static void sample_from_seed(int32_t *S, const uint8_t *seedSE, uint8_t domain, unsigned eta)
{
    uint8_t in[1 + BYTES_SEED_SE];
    uint16_t *rnd = (uint16_t *)malloc((size_t)PARAMS_N * PARAMS_NBAR * sizeof(uint16_t));
    in[0] = domain;
    memcpy(&in[1], seedSE, BYTES_SEED_SE);
    shake((uint8_t *)rnd, (size_t)PARAMS_N * PARAMS_NBAR * sizeof(uint16_t), in, sizeof(in));
    for (size_t i = 0; i < (size_t)PARAMS_N * PARAMS_NBAR; i++) rnd[i] = LE_TO_UINT16(rnd[i]);
    frost_sample_n_u32(S, rnd, (size_t)PARAMS_N * PARAMS_NBAR, eta);
    clear_bytes((uint8_t *)rnd, (size_t)PARAMS_N * PARAMS_NBAR * sizeof(uint16_t));
    free(rnd);
}

int crypto_kem_keypair(unsigned char* pk, unsigned char* sk)
{
    double t0 = now_s();
    unsigned long long c0 = now_cycles(), c1;
    unsigned long long cyc_sample = 0, cyc_mul = 0, cyc_quant_pack = 0, cyc_hash = 0;
    bench_log("keygen", "start keygen");
    uint8_t *pk_seedA = &pk[0];
    uint8_t *pk_b = &pk[BYTES_SEED_A];
    uint8_t randomness[CRYPTO_BYTES + BYTES_SEED_SE + BYTES_SEED_A];
    uint8_t *randomness_z = &randomness[0];
    uint8_t *randomness_seedSE = &randomness[CRYPTO_BYTES];
    uint8_t *randomness_seedA = &randomness[CRYPTO_BYTES + BYTES_SEED_SE];

    uint32_t *B_raw = calloc((size_t)PARAMS_N * PARAMS_NBAR, sizeof(uint32_t));
    uint32_t *B_split = calloc((size_t)PARAMS_N * PARAMS_NBAR, sizeof(uint32_t));
    uint32_t *E_zero = calloc((size_t)PARAMS_N * PARAMS_NBAR, sizeof(uint32_t));
    int32_t *S = calloc((size_t)PARAMS_N * PARAMS_NBAR, sizeof(int32_t));
    size_t row_batch = u32_row_batch_rows();
    uint32_t *Arow = calloc(AROW_WORDS_ONE_ROW * row_batch, sizeof(uint32_t));
    uint8_t *Arow_bytes = calloc(AROW_XOF_ONE_ROW * row_batch, 1);
    frost_u32_workspace_t ws = { Arow, Arow_bytes, row_batch };
    if (!B_raw || !B_split || !E_zero || !S || !Arow || !Arow_bytes) return 1;

    if (randombytes(randomness, sizeof(randomness)) != 0) return 1;
    shake(pk_seedA, BYTES_SEED_A, randomness_seedA, BYTES_SEED_A);
    c1 = now_cycles();
    sample_from_seed(S, randomness_seedSE, 0x5F, PARAMS_ETA_S);
    cyc_sample += now_cycles() - c1;

    bench_log("keygen", "start keygen multiplication");
    c1 = now_cycles();
    frost_u32_fast_stats_reset();
    if (!frost_mul_add_as_plus_e_u32(B_raw, S, E_zero, pk_seedA, &ws)) return 1;
    cyc_mul += now_cycles() - c1;
    if (bench_verbose_enabled()) { fprintf(stderr, "[bench-u32] level=%d fn=keygen end keygen multiplication %.3fs\n", PARAMS_N, now_s() - t0); }
    c1 = now_cycles();
    if (quantize_dithered_u32(B_split, B_raw, (size_t)PARAMS_N * PARAMS_NBAR, pk_seedA, BYTES_SEED_A, DITHER_DOMAIN_PK, PARAMS_PK_LOGP) != 0) return 1;
    frost_pack_u32(pk_b, PK_PACKED_BYTES, B_split, (size_t)PARAMS_N * PARAMS_NBAR, PARAMS_PK_LOGP);
    cyc_quant_pack += now_cycles() - c1;

    memset(sk, 0, CRYPTO_SECRETKEYBYTES);
    frost_pack_u32(&sk[SK_OFFSET_S], SK_PKE_PACKED_BYTES, (const uint32_t *)S, (size_t)PARAMS_N * PARAMS_NBAR_R, PARAMS_SECRET_COEFF_BITS);
    memcpy(&sk[SK_OFFSET_PK], pk, CRYPTO_PUBLICKEYBYTES);
    memcpy(&sk[SK_OFFSET_Z], randomness_z, CRYPTO_BYTES);
    c1 = now_cycles();
    shake(&sk[SK_OFFSET_PKH], BYTES_PKHASH, pk, CRYPTO_PUBLICKEYBYTES);
    cyc_hash += now_cycles() - c1;

    clear_bytes((uint8_t *)B_raw, (size_t)PARAMS_N * PARAMS_NBAR * sizeof(uint32_t));
    clear_bytes((uint8_t *)B_split, (size_t)PARAMS_N * PARAMS_NBAR * sizeof(uint32_t));
    clear_bytes((uint8_t *)S, (size_t)PARAMS_N * PARAMS_NBAR * sizeof(int32_t));
    free(B_raw); free(B_split); free(E_zero); free(S); free(Arow); free(Arow_bytes);
    {
        unsigned long long total = now_cycles() - c0;
        frost_u32_fast_stats_t st = frost_u32_fast_stats_get();
        u32_profile_report("keygen", "cbd_sample_S", cyc_sample, total);
        u32_profile_report("keygen", "mul_A_times_S", cyc_mul, total);
        u32_profile_report("keygen", "pack_quantize_pk", cyc_quant_pack, total);
        u32_profile_report("keygen", "hash_kdf", cyc_hash, total);
        u32_profile_report_counts("keygen", &st);
    }
    if (bench_verbose_enabled()) { fprintf(stderr, "[bench-u32] level=%d fn=keygen end keygen total %.3fs\n", PARAMS_N, now_s() - t0); }
    return 0;
}

int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk)
{
    double t0 = now_s();
    unsigned long long c0 = now_cycles(), c1;
    unsigned long long cyc_g2_sample = 0, cyc_mul_u = 0, cyc_pk_rebuild = 0, cyc_mul_v = 0, cyc_misc = 0;
    bench_log("encaps", "start encaps");
    const uint8_t *pk_seedA = &pk[0];
    const uint8_t *pk_b = &pk[BYTES_SEED_A];
    uint8_t *ct_c1 = &ct[0];
    uint8_t *ct_c2 = &ct[CT_C1_PACKED_BYTES];
    uint8_t *salt = &ct[CRYPTO_CIPHERTEXTBYTES - BYTES_SALT];

    uint32_t *B_split = calloc((size_t)PARAMS_N * PARAMS_NBAR, sizeof(uint32_t));
    uint32_t *B_norm = calloc((size_t)PARAMS_N * PARAMS_NBAR, sizeof(uint32_t));
    uint32_t *Bp_raw = calloc((size_t)PARAMS_NBAR * PARAMS_N, sizeof(uint32_t));
    uint32_t *Bp_split = calloc((size_t)PARAMS_NBAR * PARAMS_N, sizeof(uint32_t));
    uint32_t *V_raw = calloc((size_t)PARAMS_NBAR * PARAMS_NBAR, sizeof(uint32_t));
    uint32_t *C_split = calloc((size_t)PARAMS_NBAR * PARAMS_NBAR, sizeof(uint32_t));
    uint32_t *C_enc = calloc((size_t)PARAMS_NBAR * PARAMS_NBAR, sizeof(uint32_t));
    uint32_t *E_zero = calloc((size_t)PARAMS_NBAR * PARAMS_N, sizeof(uint32_t));
    uint32_t *E_zero_nbar = calloc((size_t)PARAMS_NBAR * PARAMS_NBAR, sizeof(uint32_t));
    int32_t *Sp = calloc((size_t)PARAMS_NBAR * PARAMS_N, sizeof(int32_t));
    size_t row_batch = u32_row_batch_rows();
    uint32_t *Arow = calloc(AROW_WORDS_ONE_ROW * row_batch, sizeof(uint32_t));
    uint8_t *Arow_bytes = calloc(AROW_XOF_ONE_ROW * row_batch, 1);
    frost_u32_workspace_t ws = { Arow, Arow_bytes, row_batch };
    uint8_t *mu = calloc(BYTES_MU, 1);
    uint8_t rnd_mu_salt[BYTES_MU + BYTES_SALT];
    uint8_t G2in[BYTES_PKHASH + BYTES_MU + BYTES_SALT];
    uint8_t G2out[BYTES_SEED_SE + CRYPTO_BYTES];
    uint8_t Fin[CRYPTO_CIPHERTEXTBYTES + CRYPTO_BYTES];

    if (!B_split||!B_norm||!Bp_raw||!Bp_split||!V_raw||!C_split||!C_enc||!E_zero||!E_zero_nbar||!Sp||!Arow||!Arow_bytes||!mu) return 1;

    shake(&G2in[0], BYTES_PKHASH, pk, CRYPTO_PUBLICKEYBYTES);
    if (randombytes(rnd_mu_salt, BYTES_MU + BYTES_SALT) != 0) return 1;
    memcpy(mu, rnd_mu_salt, BYTES_MU);
    memcpy(&G2in[BYTES_PKHASH], mu, BYTES_MU);
    memcpy(&G2in[BYTES_PKHASH + BYTES_MU], rnd_mu_salt + BYTES_MU, BYTES_SALT);
    memcpy(salt, rnd_mu_salt + BYTES_MU, BYTES_SALT);

    c1 = now_cycles();
    shake(G2out, sizeof(G2out), G2in, sizeof(G2in));
    sample_from_seed(Sp, G2out, 0x96, PARAMS_ETA_R);
    cyc_g2_sample += now_cycles() - c1;

    bench_log("encaps", "start encaps multiplication-1");
    c1 = now_cycles();
    frost_u32_fast_stats_reset();
    if (!frost_mul_add_sa_plus_e_u32(Bp_raw, Sp, E_zero, pk_seedA, &ws)) return 1;
    if (quantize_dithered_u32(Bp_split, Bp_raw, (size_t)PARAMS_NBAR * PARAMS_N, salt, BYTES_SALT, DITHER_DOMAIN_U, PARAMS_U_LOGP) != 0) return 1;
    frost_pack_u32(ct_c1, CT_C1_PACKED_BYTES, Bp_split, (size_t)PARAMS_NBAR * PARAMS_N, PARAMS_U_LOGP);
    cyc_mul_u += now_cycles() - c1;

    c1 = now_cycles();
    frost_unpack_u32(B_split, (size_t)PARAMS_N * PARAMS_NBAR, pk_b, PK_PACKED_BYTES, PARAMS_PK_LOGP);
    if (reconstruct_dithered_u32(B_norm, B_split, (size_t)PARAMS_N * PARAMS_NBAR, pk_seedA, BYTES_SEED_A, DITHER_DOMAIN_PK, PARAMS_PK_LOGP) != 0) return 1;
    cyc_pk_rebuild += now_cycles() - c1;
    bench_log("encaps", "start encaps multiplication-2");
    c1 = now_cycles();
    frost_mul_add_sb_plus_e_u32(V_raw, B_norm, Sp, E_zero_nbar);

    frost_key_encode_u32(C_enc, mu);
    for (size_t i = 0; i < (size_t)PARAMS_NBAR * PARAMS_NBAR; i++) C_enc[i] = (C_enc[i] + V_raw[i]) & qmask_u32();
    if (quantize_dithered_u32(C_split, C_enc, (size_t)PARAMS_NBAR * PARAMS_NBAR, salt, BYTES_SALT, DITHER_DOMAIN_V, PARAMS_V_LOGP) != 0) return 1;
    frost_pack_u32(ct_c2, CT_C2_PACKED_BYTES, C_split, (size_t)PARAMS_NBAR * PARAMS_NBAR, PARAMS_V_LOGP);
    cyc_mul_v += now_cycles() - c1;

    c1 = now_cycles();
    memcpy(Fin, ct, CRYPTO_CIPHERTEXTBYTES);
    memcpy(&Fin[CRYPTO_CIPHERTEXTBYTES], &G2out[BYTES_SEED_SE], CRYPTO_BYTES);
    shake(ss, CRYPTO_BYTES, Fin, sizeof(Fin));
    cyc_misc += now_cycles() - c1;

    {
        unsigned long long total = now_cycles() - c0;
        frost_u32_fast_stats_t st = frost_u32_fast_stats_get();
        u32_profile_report("encaps", "hash_kdf_and_cbd_Sp", cyc_g2_sample, total);
        u32_profile_report("encaps", "mul_u_path", cyc_mul_u, total);
        u32_profile_report("encaps", "unpack_rebuild_pk", cyc_pk_rebuild, total);
        u32_profile_report("encaps", "mul_v_path", cyc_mul_v, total);
        u32_profile_report("encaps", "hash_kdf", cyc_misc, total);
        u32_profile_report_counts("encaps", &st);
    }
    free(Arow); free(Arow_bytes);
    if (bench_verbose_enabled()) { fprintf(stderr, "[bench-u32] level=%d fn=encaps end encaps total %.3fs\n", PARAMS_N, now_s() - t0); }
    return 0;
}

int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk)
{
    double t0 = now_s();
    unsigned long long c0 = now_cycles(), c1;
    unsigned long long cyc_rebuild_ct = 0, cyc_mul_bs = 0, cyc_reenc_u = 0, cyc_reenc_v = 0, cyc_other = 0;
    bench_log("decaps", "start decaps");
    const uint8_t *pk = &sk[SK_OFFSET_PK];
    const uint8_t *pk_seedA = &pk[0];
    const uint8_t *pk_b = &pk[BYTES_SEED_A];
    const uint8_t *sk_z = &sk[SK_OFFSET_Z];
    const uint8_t *sk_pkh = &sk[SK_OFFSET_PKH];

    const uint8_t *ct_c1 = &ct[0];
    const uint8_t *ct_c2 = &ct[CT_C1_PACKED_BYTES];
    const uint8_t *salt = &ct[CRYPTO_CIPHERTEXTBYTES - BYTES_SALT];

    uint32_t *B_split = calloc((size_t)PARAMS_N * PARAMS_NBAR, sizeof(uint32_t));
    uint32_t *B_norm = calloc((size_t)PARAMS_N * PARAMS_NBAR, sizeof(uint32_t));
    uint32_t *Bp_split = calloc((size_t)PARAMS_NBAR * PARAMS_N, sizeof(uint32_t));
    uint32_t *Bp_norm = calloc((size_t)PARAMS_NBAR * PARAMS_N, sizeof(uint32_t));
    uint32_t *BBp_raw = calloc((size_t)PARAMS_NBAR * PARAMS_N, sizeof(uint32_t));
    uint32_t *BBp_split = calloc((size_t)PARAMS_NBAR * PARAMS_N, sizeof(uint32_t));
    uint32_t *W = calloc((size_t)PARAMS_NBAR * PARAMS_NBAR, sizeof(uint32_t));
    uint32_t *C_split = calloc((size_t)PARAMS_NBAR * PARAMS_NBAR, sizeof(uint32_t));
    uint32_t *C_norm = calloc((size_t)PARAMS_NBAR * PARAMS_NBAR, sizeof(uint32_t));
    uint32_t *CC = calloc((size_t)PARAMS_NBAR * PARAMS_NBAR, sizeof(uint32_t));
    uint32_t *CC_split = calloc((size_t)PARAMS_NBAR * PARAMS_NBAR, sizeof(uint32_t));
    uint32_t *E_zero = calloc((size_t)PARAMS_NBAR * PARAMS_N, sizeof(uint32_t));
    uint32_t *E_zero_nbar = calloc((size_t)PARAMS_NBAR * PARAMS_NBAR, sizeof(uint32_t));
    int32_t *S = calloc((size_t)PARAMS_NBAR * PARAMS_N, sizeof(int32_t));
    int32_t *Sp = calloc((size_t)PARAMS_NBAR * PARAMS_N, sizeof(int32_t));
    size_t row_batch = u32_row_batch_rows();
    uint32_t *Arow = calloc(AROW_WORDS_ONE_ROW * row_batch, sizeof(uint32_t));
    uint8_t *Arow_bytes = calloc(AROW_XOF_ONE_ROW * row_batch, 1);
    frost_u32_workspace_t ws = { Arow, Arow_bytes, row_batch };
    uint8_t *muprime = calloc(BYTES_MU, 1);
    uint8_t *G2in = calloc(BYTES_PKHASH + BYTES_MU + BYTES_SALT, 1);
    uint8_t *G2out = calloc(BYTES_SEED_SE + CRYPTO_BYTES, 1);
    uint8_t *Fin = calloc(CRYPTO_CIPHERTEXTBYTES + CRYPTO_BYTES, 1);
    if (!B_split||!B_norm||!Bp_split||!Bp_norm||!BBp_raw||!BBp_split||!W||!C_split||!C_norm||!CC||!CC_split||!E_zero||!E_zero_nbar||!S||!Sp||!Arow||!Arow_bytes||!muprime||!G2in||!G2out||!Fin) return 1;

    frost_unpack_u32((uint32_t *)S, (size_t)PARAMS_NBAR_R * PARAMS_N, &sk[SK_OFFSET_S], SK_PKE_PACKED_BYTES, PARAMS_SECRET_COEFF_BITS);
    for (size_t i = 0; i < (size_t)PARAMS_NBAR_R * PARAMS_N; i++) {
        if ((((uint32_t *)S)[i] & (1u << (PARAMS_SECRET_COEFF_BITS - 1u))) != 0u) {
            S[i] = (int32_t)(((uint32_t *)S)[i] | ~((1u << PARAMS_SECRET_COEFF_BITS) - 1u));
        }
    }

    c1 = now_cycles();
    frost_unpack_u32(Bp_split, (size_t)PARAMS_NBAR * PARAMS_N, ct_c1, CT_C1_PACKED_BYTES, PARAMS_U_LOGP);
    frost_unpack_u32(C_split, (size_t)PARAMS_NBAR * PARAMS_NBAR, ct_c2, CT_C2_PACKED_BYTES, PARAMS_V_LOGP);
    if (reconstruct_dithered_u32(Bp_norm, Bp_split, (size_t)PARAMS_NBAR * PARAMS_N, salt, BYTES_SALT, DITHER_DOMAIN_U, PARAMS_U_LOGP) != 0) return 1;
    if (reconstruct_dithered_u32(C_norm, C_split, (size_t)PARAMS_NBAR * PARAMS_NBAR, salt, BYTES_SALT, DITHER_DOMAIN_V, PARAMS_V_LOGP) != 0) return 1;
    cyc_rebuild_ct += now_cycles() - c1;

    bench_log("decaps", "start decaps multiplication-1");
    c1 = now_cycles();
    frost_mul_bs_u32(W, Bp_norm, S);
    for (size_t i = 0; i < (size_t)PARAMS_NBAR * PARAMS_NBAR; i++) W[i] = (C_norm[i] - W[i]) & qmask_u32();
    frost_key_decode_u32(muprime, W);
    cyc_mul_bs += now_cycles() - c1;

    c1 = now_cycles();
    memcpy(G2in, sk_pkh, BYTES_PKHASH);
    memcpy(&G2in[BYTES_PKHASH], muprime, BYTES_MU);
    memcpy(&G2in[BYTES_PKHASH + BYTES_MU], salt, BYTES_SALT);
    shake(G2out, BYTES_SEED_SE + CRYPTO_BYTES, G2in, BYTES_PKHASH + BYTES_MU + BYTES_SALT);

    sample_from_seed(Sp, G2out, 0x96, PARAMS_ETA_R);
    cyc_other += now_cycles() - c1;
    bench_log("decaps", "start decaps reencryption-1");
    c1 = now_cycles();
    frost_u32_fast_stats_reset();
    if (!frost_mul_add_sa_plus_e_u32(BBp_raw, Sp, E_zero, pk_seedA, &ws)) return 1;
    if (quantize_dithered_u32(BBp_split, BBp_raw, (size_t)PARAMS_NBAR * PARAMS_N, salt, BYTES_SALT, DITHER_DOMAIN_U, PARAMS_U_LOGP) != 0) return 1;
    cyc_reenc_u += now_cycles() - c1;

    c1 = now_cycles();
    frost_unpack_u32(B_split, (size_t)PARAMS_N * PARAMS_NBAR, pk_b, PK_PACKED_BYTES, PARAMS_PK_LOGP);
    if (reconstruct_dithered_u32(B_norm, B_split, (size_t)PARAMS_N * PARAMS_NBAR, pk_seedA, BYTES_SEED_A, DITHER_DOMAIN_PK, PARAMS_PK_LOGP) != 0) return 1;
    bench_log("decaps", "start decaps reencryption-2");
    frost_mul_add_sb_plus_e_u32(CC, B_norm, Sp, E_zero_nbar);
    frost_key_encode_u32(W, muprime);
    for (size_t i = 0; i < (size_t)PARAMS_NBAR * PARAMS_NBAR; i++) CC[i] = (CC[i] + W[i]) & qmask_u32();
    if (quantize_dithered_u32(CC_split, CC, (size_t)PARAMS_NBAR * PARAMS_NBAR, salt, BYTES_SALT, DITHER_DOMAIN_V, PARAMS_V_LOGP) != 0) return 1;
    cyc_reenc_v += now_cycles() - c1;

    c1 = now_cycles();
    int8_t selector = ct_verify_u32(Bp_split, BBp_split, (size_t)PARAMS_NBAR * PARAMS_N) | ct_verify_u32(C_split, CC_split, (size_t)PARAMS_NBAR * PARAMS_NBAR);
    const uint8_t *k = (selector == 0) ? &G2out[BYTES_SEED_SE] : sk_z;

    memcpy(Fin, ct, CRYPTO_CIPHERTEXTBYTES);
    memcpy(&Fin[CRYPTO_CIPHERTEXTBYTES], k, CRYPTO_BYTES);
    shake(ss, CRYPTO_BYTES, Fin, CRYPTO_CIPHERTEXTBYTES + CRYPTO_BYTES);
    cyc_other += now_cycles() - c1;
    {
        unsigned long long total = now_cycles() - c0;
        frost_u32_fast_stats_t st = frost_u32_fast_stats_get();
        u32_profile_report("decaps", "unpack_rebuild_ct", cyc_rebuild_ct, total);
        u32_profile_report("decaps", "mul_bp_times_s", cyc_mul_bs, total);
        u32_profile_report("decaps", "reenc_u_path", cyc_reenc_u, total);
        u32_profile_report("decaps", "reenc_v_path", cyc_reenc_v, total);
        u32_profile_report("decaps", "hash_kdf_verify", cyc_other, total);
        u32_profile_report_counts("decaps", &st);
    }
    free(Arow); free(Arow_bytes);
    if (bench_verbose_enabled()) { fprintf(stderr, "[bench-u32] level=%d fn=decaps end decaps total %.3fs\n", PARAMS_N, now_s() - t0); }
    return 0;
}
