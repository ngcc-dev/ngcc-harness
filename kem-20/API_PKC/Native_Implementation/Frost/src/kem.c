/********************************************************************************************
* MAMBA-Frost: unstructured LWQ-Z key encapsulation mechanism.
*
* Abstract: key encapsulation mechanism (KEM).
*
*********************************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "../../common/sha3/fips202.h"
#include "../../common/random/random.h"
#if defined(FROST_PROFILE_BREAKDOWN) && !defined(PROFILE_ALL_LEVELS)
#define PROFILE_ALL_LEVELS
#endif
#ifdef PROFILE_ALL_LEVELS
#include <stdio.h>
#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
static inline unsigned long long prof_now_cycles(void) { return __rdtsc(); }
#else
#include <time.h>
static inline unsigned long long prof_now_cycles(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long)ts.tv_sec * 1000000000ull + (unsigned long long)ts.tv_nsec;
}
#endif
static int prof_all_enabled(void)
{
    const char *p = getenv("FROST_PROFILE_BREAKDOWN");
    const char *legacy = getenv("PROFILE_ALL_LEVELS");
    return (p != NULL && strcmp(p, "1") == 0) || (legacy != NULL && strcmp(legacy, "1") == 0);
}
#define PROF_DECL() unsigned long long __attribute__((unused)) __p_total=0,__p_t0=0
#define PROF_BEGIN() do { if (prof_all_enabled()) __p_total = prof_now_cycles(); } while (0)
#define PROF_MARK(acc) do { if (prof_all_enabled()) { __p_t0 = prof_now_cycles(); } } while (0)
#define PROF_ADD(acc) do { if (prof_all_enabled()) { acc += prof_now_cycles() - __p_t0; } } while (0)
#define PROF_END(total) do { if (prof_all_enabled()) { total = prof_now_cycles() - __p_total; } } while (0)
static unsigned long long frost_prof_a_expand_cycles = 0;
static unsigned long long frost_prof_a_mul_cycles = 0;
static void frost_prof_mat_reset(void)
{
    frost_prof_a_expand_cycles = 0;
    frost_prof_a_mul_cycles = 0;
}
static void frost_prof_mat_add_expand(unsigned long long cycles)
{
    if (prof_all_enabled()) {
        frost_prof_a_expand_cycles += cycles;
    }
}
static void frost_prof_mat_add_mul(unsigned long long cycles)
{
    if (prof_all_enabled()) {
        frost_prof_a_mul_cycles += cycles;
    }
}
#else
#define PROF_DECL()
#define PROF_BEGIN()
#define PROF_MARK(acc) do { (void)(acc); } while (0)
#define PROF_ADD(acc) do { (void)(acc); } while (0)
#define PROF_END(total) do { (void)(total); } while (0)
#endif

#ifdef DO_VALGRIND_CHECK
#include <valgrind/memcheck.h>
#endif

#define DITHER_DOMAIN_PK 0xA1
#define DITHER_DOMAIN_U  0xB1
#define DITHER_DOMAIN_V  0xC1

#ifndef PARAMS_NBAR_R
#define PARAMS_NBAR_R PARAMS_NBAR
#endif
#ifndef PARAMS_NBAR_S
#define PARAMS_NBAR_S PARAMS_NBAR
#endif
#define PK_PACKED_BYTES ((PARAMS_PK_LOGP * PARAMS_N * PARAMS_NBAR_R) / 8)
#define CT_C1_PACKED_BYTES ((PARAMS_U_LOGP * PARAMS_N * PARAMS_NBAR_S) / 8)
#define CT_C2_PACKED_BYTES ((PARAMS_V_LOGP * PARAMS_NBAR_R * PARAMS_NBAR_S) / 8)
#define SK_OFFSET_S 0
#define SK_OFFSET_PK (SK_OFFSET_S + SK_PKE_PACKED_BYTES)
#define SK_OFFSET_PKH (SK_OFFSET_PK + CRYPTO_PUBLICKEYBYTES)
#define SK_OFFSET_Z (SK_OFFSET_PKH + BYTES_PKHASH)

#define PARAMS_P_PK (1u << PARAMS_PK_LOGP)
#define PARAMS_P_U  (1u << PARAMS_U_LOGP)
#define PARAMS_P_V  (1u << PARAMS_V_LOGP)
#define PARAMS_DELTA_PK (PARAMS_Q / PARAMS_P_PK)
#define PARAMS_DELTA_U  (PARAMS_Q / PARAMS_P_U)
#define PARAMS_DELTA_V  (PARAMS_Q / PARAMS_P_V)
#define PARAMS_SECRET_COEFF_BITS (((2u * PARAMS_ETA_S + 1u) <= 2u) ? 1u : \
                                  (((2u * PARAMS_ETA_S + 1u) <= 4u) ? 2u : \
                                   (((2u * PARAMS_ETA_S + 1u) <= 8u) ? 3u : \
                                    (((2u * PARAMS_ETA_S + 1u) <= 16u) ? 4u : 5u))))
#define SK_PKE_PACKED_BYTES (((PARAMS_SECRET_COEFF_BITS * PARAMS_N * PARAMS_NBAR_R) + 7u) / 8u)
#define SK_FO_AUX_BYTES (BYTES_PKHASH + CRYPTO_BYTES)

_Static_assert(PARAMS_Q == 65536u || PARAMS_Q == 32768u, "Frost profiles require q = 2^15 or 2^16");
_Static_assert(PARAMS_P_PK == (1u << PARAMS_PK_LOGP), "p_pk must equal 2^t_pk");
_Static_assert(PARAMS_P_U == (1u << PARAMS_U_LOGP), "p_u must equal 2^t_u");
_Static_assert(PARAMS_P_V == (1u << PARAMS_V_LOGP), "p_v must equal 2^t_v");
_Static_assert(PARAMS_DELTA_PK == (PARAMS_Q / PARAMS_P_PK), "Delta_pk must equal q / p_pk");
_Static_assert(PARAMS_DELTA_U == (PARAMS_Q / PARAMS_P_U), "Delta_u must equal q / p_u");
_Static_assert(PARAMS_DELTA_V == (PARAMS_Q / PARAMS_P_V), "Delta_v must equal q / p_v");
_Static_assert(BYTES_SEED_A + PK_PACKED_BYTES == CRYPTO_PUBLICKEYBYTES, "public-key byte length mismatch");
_Static_assert(CT_C1_PACKED_BYTES + CT_C2_PACKED_BYTES + BYTES_SALT == CRYPTO_CIPHERTEXTBYTES, "ciphertext byte length mismatch");
_Static_assert(BYTES_PKHASH == 32u, "public-key hash byte length mismatch");
_Static_assert(SK_OFFSET_Z + CRYPTO_BYTES == CRYPTO_SECRETKEYBYTES, "secret-key byte length mismatch");
_Static_assert(CRYPTO_BYTES == (PARAMS_EXTRACTED_BITS * PARAMS_NBAR_R * PARAMS_NBAR_S) / 8u, "shared-secret byte length mismatch");

static inline uint16_t frost_q_mask_local(void)
{
    return (uint16_t)((1u << PARAMS_LOGQ) - 1u);
}

static inline uint16_t frost_p_mask_local(unsigned int logp)
{
    return (uint16_t)((1u << logp) - 1u);
}

static inline uint16_t frost_reconstruct_local(uint16_t x, unsigned int logp)
{
    return (uint16_t)((x & frost_p_mask_local(logp)) << (PARAMS_LOGQ - logp));
}

static inline uint16_t frost_quantize_local(uint16_t x, uint16_t d, unsigned int logp)
{
    const unsigned int shift = PARAMS_LOGQ - logp;
    const uint32_t qmask = frost_q_mask_local();
    uint32_t z = ((uint32_t)x & qmask) + ((uint32_t)d & ((1u << shift) - 1u));
    z = (z + (1u << (shift - 1))) >> shift;
    return (uint16_t)(z & frost_p_mask_local(logp));
}

static int frost_expand_dither_local(uint16_t *d, size_t n, const uint8_t *seed, size_t seedlen, uint8_t domain, unsigned int logp)
{
    const unsigned int shift = PARAMS_LOGQ - logp;
    const uint16_t mask = (uint16_t)((1u << shift) - 1u);
    uint8_t in[1 + BYTES_SEED_A + BYTES_SALT] = {0};

    in[0] = domain;
    memcpy(&in[1], seed, seedlen);
    shake((uint8_t *)d, n * sizeof(uint16_t), in, 1 + seedlen);
    for (size_t i = 0; i < n; i++) {
        d[i] = LE_TO_UINT16(d[i]) & mask;
    }
    return 0;
}

static int frost_quantize_dithered_profile(uint16_t *out, const uint16_t *in, size_t n, const uint8_t *seed, size_t seedlen, uint8_t domain, unsigned int logp, unsigned long long *dither_cycles, unsigned long long *quant_cycles)
{
    uint16_t *d = (uint16_t *)malloc(n * sizeof(uint16_t));

    if (d == NULL) {
        return 1;
    }
    PROF_DECL();
    PROF_MARK((*dither_cycles));
    if (frost_expand_dither_local(d, n, seed, seedlen, domain, logp) != 0) {
        clear_bytes((uint8_t *)d, n * sizeof(uint16_t));
        free(d);
        return 1;
    }
    PROF_ADD((*dither_cycles));
    PROF_MARK((*quant_cycles));
    for (size_t i = 0; i < n; i++) {
        out[i] = frost_quantize_local(in[i], d[i], logp);
    }
    PROF_ADD((*quant_cycles));
    clear_bytes((uint8_t *)d, n * sizeof(uint16_t));
    free(d);
    return 0;
}

static int __attribute__((unused)) frost_quantize_dithered_local(uint16_t *out, const uint16_t *in, size_t n, const uint8_t *seed, size_t seedlen, uint8_t domain, unsigned int logp)
{
    unsigned long long ignored_dither = 0, ignored_quant = 0;
    return frost_quantize_dithered_profile(out, in, n, seed, seedlen, domain, logp, &ignored_dither, &ignored_quant);
}

static int frost_reconstruct_dithered_profile(uint16_t *normal, const uint16_t *split, size_t n, const uint8_t *seed, size_t seedlen, uint8_t domain, unsigned int logp, unsigned long long *dither_cycles, unsigned long long *reconstruct_cycles)
{
    uint16_t *d = (uint16_t *)malloc(n * sizeof(uint16_t));

    if (d == NULL) {
        return 1;
    }
    PROF_DECL();
    PROF_MARK((*dither_cycles));
    if (frost_expand_dither_local(d, n, seed, seedlen, domain, logp) != 0) {
        clear_bytes((uint8_t *)d, n * sizeof(uint16_t));
        free(d);
        return 1;
    }
    PROF_ADD((*dither_cycles));
    PROF_MARK((*reconstruct_cycles));
    for (size_t i = 0; i < n; i++) {
        normal[i] = (uint16_t)((frost_reconstruct_local(split[i], logp) - d[i]) & frost_q_mask_local());
    }
    PROF_ADD((*reconstruct_cycles));
    clear_bytes((uint8_t *)d, n * sizeof(uint16_t));
    free(d);
    return 0;
}

static int __attribute__((unused)) frost_reconstruct_dithered_local(uint16_t *normal, const uint16_t *split, size_t n, const uint8_t *seed, size_t seedlen, uint8_t domain, unsigned int logp)
{
    unsigned long long ignored_dither = 0, ignored_reconstruct = 0;
    return frost_reconstruct_dithered_profile(normal, split, n, seed, seedlen, domain, logp, &ignored_dither, &ignored_reconstruct);
}

int crypto_kem_keypair(unsigned char* pk, unsigned char* sk)
{ // MAMBA-Frost key generation with public dithered quantization
    PROF_DECL();
    unsigned long long __attribute__((unused)) c_rand = 0, c_seedexp = 0, c_cbd = 0, c_as = 0, c_a_expand = 0, c_a_mul = 0, c_pk_dither = 0, c_pk_quant = 0, c_pack = 0, c_hash = 0, c_total = 0;
    PROF_BEGIN();
    uint8_t *pk_seedA = &pk[0];
    uint8_t *pk_b = &pk[BYTES_SEED_A];
    uint8_t *sk_s = &sk[SK_OFFSET_S];
    uint8_t *sk_pk = &sk[SK_OFFSET_PK];
    uint8_t *sk_pkh = &sk[SK_OFFSET_PKH];
    uint8_t *sk_z = &sk[SK_OFFSET_Z];
    uint16_t B_raw[PARAMS_N*PARAMS_NBAR_R] = {0};
    uint16_t B_split[PARAMS_N*PARAMS_NBAR_R] = {0};
    uint16_t S[PARAMS_N*PARAMS_NBAR_R] = {0};                          // contains secret data
    uint16_t E_zero[PARAMS_N*PARAMS_NBAR_R] = {0};
    uint8_t randomness[CRYPTO_BYTES + BYTES_SEED_SE + BYTES_SEED_A]; // contains secret data via randomness_z and randomness_seedSE
    uint8_t *randomness_z = &randomness[0];                          // contains secret data
    uint8_t *randomness_seedSE = &randomness[CRYPTO_BYTES];          // contains secret data
    uint8_t *randomness_seedA = &randomness[CRYPTO_BYTES + BYTES_SEED_SE];
    uint8_t shake_input_seedSE[1 + BYTES_SEED_SE];                   // contains secret data

    PROF_MARK(c_rand);
    if (randombytes(randomness, CRYPTO_BYTES + BYTES_SEED_SE + BYTES_SEED_A) != 0)
        return 1;
    PROF_ADD(c_rand);
#ifdef DO_VALGRIND_CHECK
    VALGRIND_MAKE_MEM_UNDEFINED(randomness, CRYPTO_BYTES + BYTES_SEED_SE + BYTES_SEED_A);
#endif
    PROF_MARK(c_seedexp);
    shake(pk_seedA, BYTES_SEED_A, randomness_seedA, BYTES_SEED_A);

    shake_input_seedSE[0] = 0x5F;
    memcpy(&shake_input_seedSE[1], randomness_seedSE, BYTES_SEED_SE);
    shake((uint8_t*)S, PARAMS_N*PARAMS_NBAR_R*sizeof(uint16_t), shake_input_seedSE, 1 + BYTES_SEED_SE);
    PROF_ADD(c_seedexp);
    for (size_t i = 0; i < PARAMS_N * PARAMS_NBAR_R; i++) {
        S[i] = LE_TO_UINT16(S[i]);
    }
    PROF_MARK(c_cbd);
    frost_sample_n(S, PARAMS_N*PARAMS_NBAR_R);
    PROF_ADD(c_cbd);
    PROF_MARK(c_as);
#ifdef PROFILE_ALL_LEVELS
    frost_prof_mat_reset();
#endif
    frost_mul_add_as_plus_e(B_raw, S, E_zero, pk_seedA);
#ifdef PROFILE_ALL_LEVELS
    c_a_expand = frost_prof_a_expand_cycles;
    c_a_mul = frost_prof_a_mul_cycles;
#endif
    PROF_ADD(c_as);
    if (frost_quantize_dithered_profile(B_split, B_raw, PARAMS_N*PARAMS_NBAR_R, pk_seedA, BYTES_SEED_A, DITHER_DOMAIN_PK, PARAMS_PK_LOGP, &c_pk_dither, &c_pk_quant) != 0) {
        clear_bytes((uint8_t *)B_raw, PARAMS_N*PARAMS_NBAR_R*sizeof(uint16_t));
        clear_bytes((uint8_t *)B_split, PARAMS_N*PARAMS_NBAR_R*sizeof(uint16_t));
        clear_bytes((uint8_t *)S, PARAMS_N*PARAMS_NBAR_R*sizeof(uint16_t));
        clear_bytes(randomness, CRYPTO_BYTES + BYTES_SEED_SE + BYTES_SEED_A);
        clear_bytes(shake_input_seedSE, 1 + BYTES_SEED_SE);
        return 1;
    }

    PROF_MARK(c_pack);
    frost_pack(pk_b, PK_PACKED_BYTES, B_split, PARAMS_N*PARAMS_NBAR_R, PARAMS_PK_LOGP);
    PROF_ADD(c_pack);

    memset(sk, 0, CRYPTO_SECRETKEYBYTES);
    frost_pack(sk_s, SK_PKE_PACKED_BYTES, S, PARAMS_N*PARAMS_NBAR_R, PARAMS_SECRET_COEFF_BITS);
    memcpy(sk_pk, pk, CRYPTO_PUBLICKEYBYTES);
    memcpy(sk_z, randomness_z, CRYPTO_BYTES);

    PROF_MARK(c_hash);
    shake(sk_pkh, BYTES_PKHASH, pk, CRYPTO_PUBLICKEYBYTES);
    PROF_ADD(c_hash);

    clear_bytes((uint8_t *)B_raw, PARAMS_N*PARAMS_NBAR_R*sizeof(uint16_t));
    clear_bytes((uint8_t *)B_split, PARAMS_N*PARAMS_NBAR_R*sizeof(uint16_t));
    clear_bytes((uint8_t *)S, PARAMS_N*PARAMS_NBAR_R*sizeof(uint16_t));
    clear_bytes(randomness, CRYPTO_BYTES + BYTES_SEED_SE + BYTES_SEED_A);
    clear_bytes(shake_input_seedSE, 1 + BYTES_SEED_SE);
#ifdef DO_VALGRIND_CHECK
    VALGRIND_MAKE_MEM_DEFINED(randomness, CRYPTO_BYTES + BYTES_SEED_SE + BYTES_SEED_A);
#endif
    PROF_END(c_total);
#ifdef PROFILE_ALL_LEVELS
    if (prof_all_enabled()) {
        unsigned long long c_other = c_total - (c_rand + c_seedexp + c_cbd + c_as + c_pk_dither + c_pk_quant + c_pack + c_hash);
        fprintf(stderr,
                "[profile-all] level=%d api=keygen total=%llu random=%llu seedexp=%llu genpublic_a_expand=%llu genpublic_a_mul=%llu d_pk_gen=%llu cbd_s=%llu mul_as=%llu quant_pk=%llu pack_pk=%llu hash_aux=%llu other=%llu\n",
                PARAMS_N, c_total, c_rand, c_seedexp, c_a_expand, c_a_mul, c_pk_dither, c_cbd, c_as, c_pk_quant, c_pack, c_hash, c_other);
    }
#endif
    return 0;
}


int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk)
{ // MAMBA-Frost key encapsulation with public dithered quantization
    PROF_DECL();
    unsigned long long __attribute__((unused)) c_total = 0, c_hpk = 0, c_msg_sample = 0, c_g = 0, c_seed_r = 0, c_cbd_r = 0, c_atr = 0, c_a_expand = 0, c_a_mul = 0, c_unpack_pk = 0, c_dpk = 0, c_recon_pk = 0, c_btr = 0, c_msg = 0, c_du = 0, c_dv = 0, c_qu_apply = 0, c_qv_apply = 0, c_pack = 0, c_hash_ss = 0;
    PROF_BEGIN();
    const uint8_t *pk_seedA = &pk[0];
    const uint8_t *pk_b = &pk[BYTES_SEED_A];
    uint8_t *ct_c1 = &ct[0];
    uint8_t *ct_c2 = &ct[CT_C1_PACKED_BYTES];
    uint16_t B_split[PARAMS_N*PARAMS_NBAR_R] = {0};
    uint16_t B_norm[PARAMS_N*PARAMS_NBAR_R] = {0};
    uint16_t Bp_raw[PARAMS_N*PARAMS_NBAR_S] = {0};                     // contains secret data
    uint16_t Bp_split[PARAMS_N*PARAMS_NBAR_S] = {0};                   // contains secret data
    uint16_t V_raw[PARAMS_NBAR_R*PARAMS_NBAR_S] = {0};                   // contains secret data
    uint16_t C_split[PARAMS_NBAR_R*PARAMS_NBAR_S] = {0};                 // contains secret data
    uint16_t C_enc[PARAMS_NBAR_R*PARAMS_NBAR_S] = {0};                   // contains secret data
    uint16_t E_zero_nbar[PARAMS_NBAR_R*PARAMS_NBAR_S] = {0};
    ALIGN_HEADER(32) uint16_t Sp[PARAMS_N*PARAMS_NBAR_S] ALIGN_FOOTER(32) = {0};  // contains secret data
    uint16_t E_zero[PARAMS_N*PARAMS_NBAR_S] = {0};
    uint8_t G2in[BYTES_PKHASH + BYTES_MU + BYTES_SALT];              // contains secret data via mu
    uint8_t *pkh = &G2in[0];
    uint8_t *mu = &G2in[BYTES_PKHASH];                               // contains secret data
    uint8_t *salt = &G2in[BYTES_PKHASH + BYTES_MU];
    uint8_t G2out[BYTES_SEED_SE + CRYPTO_BYTES];                     // contains secret data
    uint8_t *seedSE = &G2out[0];                                     // contains secret data
    uint8_t *k = &G2out[BYTES_SEED_SE];                              // contains secret data
    uint8_t Fin[CRYPTO_CIPHERTEXTBYTES + CRYPTO_BYTES];              // contains secret data via Fin_k
    uint8_t *Fin_ct = &Fin[0];
    uint8_t *Fin_k = &Fin[CRYPTO_CIPHERTEXTBYTES];                   // contains secret data
    uint8_t shake_input_seedSE[1 + BYTES_SEED_SE];                   // contains secret data

    PROF_MARK(c_hpk);
    shake(pkh, BYTES_PKHASH, pk, CRYPTO_PUBLICKEYBYTES);
    PROF_ADD(c_hpk);
    PROF_MARK(c_msg_sample);
    if (randombytes(mu, BYTES_MU + BYTES_SALT) != 0)
        return 1;
    PROF_ADD(c_msg_sample);
#ifdef DO_VALGRIND_CHECK
    VALGRIND_MAKE_MEM_UNDEFINED(mu, BYTES_MU + BYTES_SALT);
    VALGRIND_MAKE_MEM_UNDEFINED(pk, CRYPTO_PUBLICKEYBYTES);
#endif
    PROF_MARK(c_g);
    shake(G2out, BYTES_SEED_SE + CRYPTO_BYTES, G2in, BYTES_PKHASH + BYTES_MU + BYTES_SALT);
    PROF_ADD(c_g);

    PROF_MARK(c_seed_r);
    shake_input_seedSE[0] = 0x96;
    memcpy(&shake_input_seedSE[1], seedSE, BYTES_SEED_SE);
    shake((uint8_t*)Sp, PARAMS_N*PARAMS_NBAR_S*sizeof(uint16_t), shake_input_seedSE, 1 + BYTES_SEED_SE);
    for (size_t i = 0; i < PARAMS_N * PARAMS_NBAR_S; i++) {
        Sp[i] = LE_TO_UINT16(Sp[i]);
    }
    PROF_ADD(c_seed_r);
    PROF_MARK(c_cbd_r);
    frost_sample_n(Sp, PARAMS_N*PARAMS_NBAR_S);
    PROF_ADD(c_cbd_r);
    PROF_MARK(c_atr);
#ifdef PROFILE_ALL_LEVELS
    frost_prof_mat_reset();
#endif
    frost_mul_add_sa_plus_e(Bp_raw, Sp, E_zero, pk_seedA);
#ifdef PROFILE_ALL_LEVELS
    c_a_expand = frost_prof_a_expand_cycles;
    c_a_mul = frost_prof_a_mul_cycles;
#endif
    PROF_ADD(c_atr);
    for (size_t i = 0; i < PARAMS_N * PARAMS_NBAR_S; i++) {
        Bp_raw[i] &= frost_q_mask_local();
    }
    if (frost_quantize_dithered_profile(Bp_split, Bp_raw, PARAMS_N*PARAMS_NBAR_S, salt, BYTES_SALT, DITHER_DOMAIN_U, PARAMS_U_LOGP, &c_du, &c_qu_apply) != 0) {
        return 1;
    }
    PROF_MARK(c_pack);
    frost_pack(ct_c1, CT_C1_PACKED_BYTES, Bp_split, PARAMS_N*PARAMS_NBAR_S, PARAMS_U_LOGP);
    PROF_ADD(c_pack);

    PROF_MARK(c_unpack_pk);
    frost_unpack(B_split, PARAMS_N*PARAMS_NBAR_R, pk_b, PK_PACKED_BYTES, PARAMS_PK_LOGP);
    PROF_ADD(c_unpack_pk);
    if (frost_reconstruct_dithered_profile(B_norm, B_split, PARAMS_N*PARAMS_NBAR_R, pk_seedA, BYTES_SEED_A, DITHER_DOMAIN_PK, PARAMS_PK_LOGP, &c_dpk, &c_recon_pk) != 0) {
        return 1;
    }
    PROF_MARK(c_btr);
    frost_mul_add_sb_plus_e(V_raw, B_norm, Sp, E_zero_nbar);
    PROF_ADD(c_btr);

    PROF_MARK(c_msg);
    frost_key_encode(C_enc, (uint16_t*)mu);
    frost_add(C_enc, V_raw, C_enc);
    PROF_ADD(c_msg);
    if (frost_quantize_dithered_profile(C_split, C_enc, PARAMS_NBAR_R*PARAMS_NBAR_S, salt, BYTES_SALT, DITHER_DOMAIN_V, PARAMS_V_LOGP, &c_dv, &c_qv_apply) != 0) {
        return 1;
    }
    PROF_MARK(c_pack);
    frost_pack(ct_c2, CT_C2_PACKED_BYTES, C_split, PARAMS_NBAR_R*PARAMS_NBAR_S, PARAMS_V_LOGP);
    PROF_ADD(c_pack);

    memcpy(&ct[CRYPTO_CIPHERTEXTBYTES - BYTES_SALT], salt, BYTES_SALT);
    memcpy(Fin_ct, ct, CRYPTO_CIPHERTEXTBYTES);
    memcpy(Fin_k, k, CRYPTO_BYTES);
    PROF_MARK(c_hash_ss);
    shake(ss, CRYPTO_BYTES, Fin, CRYPTO_CIPHERTEXTBYTES + CRYPTO_BYTES);
    PROF_ADD(c_hash_ss);

    clear_bytes((uint8_t *)B_norm, PARAMS_N*PARAMS_NBAR_R*sizeof(uint16_t));
    clear_bytes((uint8_t *)Bp_raw, PARAMS_N*PARAMS_NBAR_S*sizeof(uint16_t));
    clear_bytes((uint8_t *)Bp_split, PARAMS_N*PARAMS_NBAR_S*sizeof(uint16_t));
    clear_bytes((uint8_t *)V_raw, PARAMS_NBAR_R*PARAMS_NBAR_S*sizeof(uint16_t));
    clear_bytes((uint8_t *)C_split, PARAMS_NBAR_R*PARAMS_NBAR_S*sizeof(uint16_t));
    clear_bytes((uint8_t *)C_enc, PARAMS_NBAR_R*PARAMS_NBAR_S*sizeof(uint16_t));
    clear_bytes((uint8_t *)Sp, PARAMS_N*PARAMS_NBAR_S*sizeof(uint16_t));
    clear_bytes(mu, BYTES_MU);
    clear_bytes(G2out, BYTES_SEED_SE + CRYPTO_BYTES);
    clear_bytes(Fin_k, CRYPTO_BYTES);
    clear_bytes(shake_input_seedSE, 1 + BYTES_SEED_SE);
#ifdef DO_VALGRIND_CHECK
    VALGRIND_MAKE_MEM_DEFINED(mu, BYTES_MU);
    VALGRIND_MAKE_MEM_DEFINED(pk, CRYPTO_PUBLICKEYBYTES);
#endif
    PROF_END(c_total);
#ifdef PROFILE_ALL_LEVELS
    if (prof_all_enabled()) {
        unsigned long long c_other = c_total - (c_hpk + c_msg_sample + c_g + c_seed_r + c_cbd_r + c_atr + c_unpack_pk + c_dpk + c_recon_pk + c_btr + c_msg + c_du + c_dv + c_qu_apply + c_qv_apply + c_pack + c_hash_ss);
        fprintf(stderr,
                "[profile-all] level=%d api=encaps total=%llu h_pk=%llu message_sampling=%llu g_hash=%llu sigma_to_r_xof=%llu r_sampling=%llu genpublic_a_expand=%llu genpublic_a_mul=%llu d_pk_gen=%llu unpack_pk=%llu bhat_reconstruct=%llu gen_dither_du=%llu gen_dither_dv=%llu mul_atr=%llu quant_u=%llu mul_btr=%llu encode_msg=%llu quant_v=%llu pack_ct=%llu final_hash=%llu other=%llu\n",
                PARAMS_N, c_total, c_hpk, c_msg_sample, c_g, c_seed_r, c_cbd_r, c_a_expand, c_a_mul, c_dpk, c_unpack_pk, c_recon_pk, c_du, c_dv, c_atr, c_qu_apply, c_btr, c_msg, c_qv_apply, c_pack, c_hash_ss, c_other);
    }
#endif
    return 0;
}


int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk)
{ // MAMBA-Frost key decapsulation with public dithered quantization
    PROF_DECL();
    unsigned long long __attribute__((unused)) c_total = 0, c_unct = 0, c_du = 0, c_dv = 0, c_recon_u = 0, c_recon_v = 0, c_stu = 0, c_mudec = 0, c_s_seed = 0, c_g = 0, c_seed_r = 0, c_cbd_r = 0, c_reenc_atr = 0, c_reenc_a_expand = 0, c_reenc_a_mul = 0, c_reenc_btr = 0, c_reenc_du = 0, c_reenc_dv = 0, c_reenc_qu_apply = 0, c_reenc_pk_unpack = 0, c_reenc_dpk = 0, c_reenc_pk_recon = 0, c_reenc_qv_apply = 0, c_ctcmp = 0, c_hashss = 0;
    PROF_BEGIN();
    uint16_t B_split[PARAMS_N*PARAMS_NBAR_R] = {0};
    uint16_t B_norm[PARAMS_N*PARAMS_NBAR_R] = {0};
    uint16_t Bp_split[PARAMS_N*PARAMS_NBAR_S] = {0};
    uint16_t Bp_norm[PARAMS_N*PARAMS_NBAR_S] = {0};
    uint16_t BBp_raw[PARAMS_N*PARAMS_NBAR_S] = {0};                     // contains secret data
    uint16_t BBp_split[PARAMS_N*PARAMS_NBAR_S] = {0};                   // contains secret data
    uint16_t W[PARAMS_NBAR_R*PARAMS_NBAR_S] = {0};                        // contains secret data
    uint16_t C_split[PARAMS_NBAR_R*PARAMS_NBAR_S] = {0};
    uint16_t C_norm[PARAMS_NBAR_R*PARAMS_NBAR_S] = {0};                   // contains secret data
    uint16_t CC[PARAMS_NBAR_R*PARAMS_NBAR_S] = {0};                       // contains secret data
    uint16_t CC_split[PARAMS_NBAR_R*PARAMS_NBAR_S] = {0};                 // contains secret data
    uint16_t E_zero[PARAMS_N*PARAMS_NBAR_S] = {0};
    uint16_t E_zero_nbar[PARAMS_NBAR_R*PARAMS_NBAR_S] = {0};
    const uint8_t *ct_c1 = &ct[0];
    const uint8_t *ct_c2 = &ct[CT_C1_PACKED_BYTES];
    const uint8_t *salt = &ct[CRYPTO_CIPHERTEXTBYTES - BYTES_SALT];
    const uint8_t *sk_s = &sk[SK_OFFSET_S];
    const uint8_t *sk_pk = &sk[SK_OFFSET_PK];
    const uint8_t *sk_z = &sk[SK_OFFSET_Z];
    uint16_t S[PARAMS_N * PARAMS_NBAR_R];                               // contains secret data
    const uint8_t *sk_pkh = &sk[SK_OFFSET_PKH];
    const uint8_t *pk_seedA = &sk_pk[0];
    const uint8_t *pk_b = &sk_pk[BYTES_SEED_A];
    uint8_t G2in[BYTES_PKHASH + BYTES_MU + BYTES_SALT];               // contains secret data via muprime
    uint8_t *pkh = &G2in[0];
    uint8_t *muprime = &G2in[BYTES_PKHASH];                           // contains secret data
    uint8_t *G2in_salt = &G2in[BYTES_PKHASH + BYTES_MU];
    uint8_t G2out[BYTES_SEED_SE + CRYPTO_BYTES];                      // contains secret data
    uint8_t *seedSEprime = &G2out[0];                                 // contains secret data
    uint8_t *kprime = &G2out[BYTES_SEED_SE];                          // contains secret data
    uint8_t Fin[CRYPTO_CIPHERTEXTBYTES + CRYPTO_BYTES];               // contains secret data via Fin_k
    uint8_t *Fin_ct = &Fin[0];
    uint8_t *Fin_k = &Fin[CRYPTO_CIPHERTEXTBYTES];                    // contains secret data
    uint8_t shake_input_seedSEprime[1 + BYTES_SEED_SE];               // contains secret data
    ALIGN_HEADER(32) uint16_t Sp[PARAMS_N*PARAMS_NBAR_S] ALIGN_FOOTER(32) = {0};  // contains secret data

#ifdef DO_VALGRIND_CHECK
    VALGRIND_MAKE_MEM_UNDEFINED(sk, CRYPTO_SECRETKEYBYTES);
    VALGRIND_MAKE_MEM_UNDEFINED(ct, CRYPTO_CIPHERTEXTBYTES);
#endif

    PROF_MARK(c_s_seed);
    frost_unpack(S, PARAMS_N*PARAMS_NBAR_R, sk_s, SK_PKE_PACKED_BYTES, PARAMS_SECRET_COEFF_BITS);
    for (size_t i = 0; i < PARAMS_N * PARAMS_NBAR_R; i++) {
        if ((S[i] & (1u << (PARAMS_SECRET_COEFF_BITS - 1u))) != 0u) {
            S[i] = (uint16_t)(S[i] | (uint16_t)(~((1u << PARAMS_SECRET_COEFF_BITS) - 1u)));
        }
    }
    PROF_ADD(c_s_seed);

    PROF_MARK(c_unct);
    frost_unpack(Bp_split, PARAMS_N*PARAMS_NBAR_S, ct_c1, CT_C1_PACKED_BYTES, PARAMS_U_LOGP);
    frost_unpack(C_split, PARAMS_NBAR_R*PARAMS_NBAR_S, ct_c2, CT_C2_PACKED_BYTES, PARAMS_V_LOGP);
    PROF_ADD(c_unct);
    if (frost_reconstruct_dithered_profile(Bp_norm, Bp_split, PARAMS_N*PARAMS_NBAR_S, salt, BYTES_SALT, DITHER_DOMAIN_U, PARAMS_U_LOGP, &c_du, &c_recon_u) != 0 ||
        frost_reconstruct_dithered_profile(C_norm, C_split, PARAMS_NBAR_R*PARAMS_NBAR_S, salt, BYTES_SALT, DITHER_DOMAIN_V, PARAMS_V_LOGP, &c_dv, &c_recon_v) != 0) {
        return 1;
    }
    PROF_MARK(c_stu);
    frost_mul_bs(W, Bp_norm, S);
    PROF_ADD(c_stu);
    PROF_MARK(c_mudec);
    frost_sub(W, C_norm, W);
    frost_key_decode((uint16_t*)muprime, W);
    PROF_ADD(c_mudec);

    PROF_MARK(c_g);
    memcpy(pkh, sk_pkh, BYTES_PKHASH);
    memcpy(G2in_salt, salt, BYTES_SALT);
    shake(G2out, BYTES_SEED_SE + CRYPTO_BYTES, G2in, BYTES_PKHASH + BYTES_MU + BYTES_SALT);
    PROF_ADD(c_g);

    PROF_MARK(c_seed_r);
    shake_input_seedSEprime[0] = 0x96;
    memcpy(&shake_input_seedSEprime[1], seedSEprime, BYTES_SEED_SE);
    shake((uint8_t*)Sp, PARAMS_N*PARAMS_NBAR_S*sizeof(uint16_t), shake_input_seedSEprime, 1 + BYTES_SEED_SE);
    for (size_t i = 0; i < PARAMS_N * PARAMS_NBAR_S; i++) {
        Sp[i] = LE_TO_UINT16(Sp[i]);
    }
    PROF_ADD(c_seed_r);
    PROF_MARK(c_cbd_r);
    frost_sample_n(Sp, PARAMS_N*PARAMS_NBAR_S);
    PROF_ADD(c_cbd_r);
    PROF_MARK(c_reenc_atr);
#ifdef PROFILE_ALL_LEVELS
    frost_prof_mat_reset();
#endif
    frost_mul_add_sa_plus_e(BBp_raw, Sp, E_zero, pk_seedA);
#ifdef PROFILE_ALL_LEVELS
    c_reenc_a_expand = frost_prof_a_expand_cycles;
    c_reenc_a_mul = frost_prof_a_mul_cycles;
#endif
    PROF_ADD(c_reenc_atr);
    for (size_t i = 0; i < PARAMS_N * PARAMS_NBAR_S; i++) {
        BBp_raw[i] &= frost_q_mask_local();
    }
    if (frost_quantize_dithered_profile(BBp_split, BBp_raw, PARAMS_N*PARAMS_NBAR_S, salt, BYTES_SALT, DITHER_DOMAIN_U, PARAMS_U_LOGP, &c_reenc_du, &c_reenc_qu_apply) != 0) {
        return 1;
    }

    PROF_MARK(c_reenc_pk_unpack);
    frost_unpack(B_split, PARAMS_N*PARAMS_NBAR_R, pk_b, PK_PACKED_BYTES, PARAMS_PK_LOGP);
    PROF_ADD(c_reenc_pk_unpack);
    if (frost_reconstruct_dithered_profile(B_norm, B_split, PARAMS_N*PARAMS_NBAR_R, pk_seedA, BYTES_SEED_A, DITHER_DOMAIN_PK, PARAMS_PK_LOGP, &c_reenc_dpk, &c_reenc_pk_recon) != 0) {
        return 1;
    }
    PROF_MARK(c_reenc_btr);
    frost_mul_add_sb_plus_e(CC, B_norm, Sp, E_zero_nbar);
    PROF_ADD(c_reenc_btr);
    PROF_MARK(c_reenc_qv_apply);
    frost_key_encode(W, (uint16_t*)muprime);
    frost_add(CC, CC, W);
    PROF_ADD(c_reenc_qv_apply);
    if (frost_quantize_dithered_profile(CC_split, CC, PARAMS_NBAR_R*PARAMS_NBAR_S, salt, BYTES_SALT, DITHER_DOMAIN_V, PARAMS_V_LOGP, &c_reenc_dv, &c_reenc_qv_apply) != 0) {
        return 1;
    }

    memcpy(Fin_ct, ct, CRYPTO_CIPHERTEXTBYTES);

    PROF_MARK(c_ctcmp);
    int8_t selector = ct_verify(Bp_split, BBp_split, PARAMS_N*PARAMS_NBAR_S) | ct_verify(C_split, CC_split, PARAMS_NBAR_R*PARAMS_NBAR_S);
    ct_select((uint8_t*)Fin_k, (uint8_t*)kprime, (uint8_t*)sk_z, CRYPTO_BYTES, selector);
    PROF_ADD(c_ctcmp);
    PROF_MARK(c_hashss);
    shake(ss, CRYPTO_BYTES, Fin, CRYPTO_CIPHERTEXTBYTES + CRYPTO_BYTES);
    PROF_ADD(c_hashss);

    clear_bytes((uint8_t *)B_norm, PARAMS_N*PARAMS_NBAR_R*sizeof(uint16_t));
    clear_bytes((uint8_t *)Bp_norm, PARAMS_N*PARAMS_NBAR_S*sizeof(uint16_t));
    clear_bytes((uint8_t *)BBp_raw, PARAMS_N*PARAMS_NBAR_S*sizeof(uint16_t));
    clear_bytes((uint8_t *)BBp_split, PARAMS_N*PARAMS_NBAR_S*sizeof(uint16_t));
    clear_bytes((uint8_t *)W, PARAMS_NBAR_R*PARAMS_NBAR_S*sizeof(uint16_t));
    clear_bytes((uint8_t *)C_norm, PARAMS_NBAR_R*PARAMS_NBAR_S*sizeof(uint16_t));
    clear_bytes((uint8_t *)CC, PARAMS_NBAR_R*PARAMS_NBAR_S*sizeof(uint16_t));
    clear_bytes((uint8_t *)CC_split, PARAMS_NBAR_R*PARAMS_NBAR_S*sizeof(uint16_t));
    clear_bytes((uint8_t *)Sp, PARAMS_N*PARAMS_NBAR_S*sizeof(uint16_t));
    clear_bytes((uint8_t *)S, PARAMS_N*PARAMS_NBAR_R*sizeof(uint16_t));
    clear_bytes(muprime, BYTES_MU);
    clear_bytes(G2out, BYTES_SEED_SE + CRYPTO_BYTES);
    clear_bytes(Fin_k, CRYPTO_BYTES);
    clear_bytes(shake_input_seedSEprime, 1 + BYTES_SEED_SE);
#ifdef DO_VALGRIND_CHECK
    VALGRIND_MAKE_MEM_DEFINED(sk, CRYPTO_SECRETKEYBYTES);
    VALGRIND_MAKE_MEM_DEFINED(ct, CRYPTO_CIPHERTEXTBYTES);
#endif
    PROF_END(c_total);
#ifdef PROFILE_ALL_LEVELS
    if (prof_all_enabled()) {
        unsigned long long c_other = c_total - (c_unct + c_du + c_dv + c_recon_u + c_recon_v + c_stu + c_mudec + c_s_seed + c_g + c_seed_r + c_cbd_r + c_reenc_atr + c_reenc_btr + c_reenc_du + c_reenc_dv + c_reenc_qu_apply + c_reenc_pk_unpack + c_reenc_dpk + c_reenc_pk_recon + c_reenc_qv_apply + c_ctcmp + c_hashss);
        fprintf(stderr,
                "[profile-all] level=%d api=decaps total=%llu unpack_ct=%llu gen_dither_du=%llu gen_dither_dv=%llu reconstruct_u=%llu reconstruct_v=%llu mul_stu=%llu msg_decode=%llu sk_s_sampling=%llu g_hash=%llu sigma_to_r_xof=%llu r_sampling=%llu reenc_a_expand=%llu reenc_a_mul=%llu reenc_atr=%llu reenc_btr=%llu reenc_du=%llu reenc_dv=%llu reenc_quant_u=%llu reenc_unpack_pk=%llu reenc_d_pk=%llu reenc_bhat_reconstruct=%llu reenc_quant_v=%llu ct_compare=%llu final_hash=%llu other=%llu\n",
                PARAMS_N, c_total, c_unct, c_du, c_dv, c_recon_u, c_recon_v, c_stu, c_mudec, c_s_seed, c_g, c_seed_r, c_cbd_r, c_reenc_a_expand, c_reenc_a_mul, c_reenc_atr, c_reenc_btr, c_reenc_du, c_reenc_dv, c_reenc_qu_apply, c_reenc_pk_unpack, c_reenc_dpk, c_reenc_pk_recon, c_reenc_qv_apply, c_ctcmp, c_hashss, c_other);
    }
#endif
    return 0;
}
