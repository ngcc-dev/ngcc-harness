/*
Copyright (c) 2026 Ying Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
File Description: Cycle-count benchmark units for the optimized POLARLAC-512-Star implementation.
*/

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "drng.h"
#include "fft.h"
#include "kem_random.h"
#include "KEM_AlgorithmInstance.h"
#include "ntt.h"
#include "params.h"
#include "pke.h"
#include "poly.h"
#include "sample.h"
#include "symmetric.h"

#if defined(__i386__) || defined(__x86_64__)
#include <x86intrin.h>
#define cpucycles() _rdtsc()
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <intrin.h>
#define cpucycles() __rdtsc()
#elif defined(__aarch64__)
static inline unsigned long long cpucycles(void)
{
    unsigned long long value;

    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(value));
    return value;
}
#else
static inline unsigned long long cpucycles(void)
{
    return (unsigned long long)clock();
}
#endif

#ifndef NTESTS
#define NTESTS 1000
#endif

#ifndef NBATCH
#define NBATCH 100
#endif

static unsigned long long g_cycle_overhead = 0;

DRNG_ctx drng_algorithm;

static uint8_t ct_verify(const uint8_t *a, const uint8_t *b, size_t len)
{
    uint8_t r = 0;

    for (size_t i = 0; i < len; i++) {
        r |= (uint8_t)(a[i] ^ b[i]);
    }

    return (uint8_t)((-(uint64_t)r) >> 63);
}

static int g_bench_microseconds = 0;

static unsigned long long wall_microseconds(void)
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (unsigned long long)tv.tv_sec * 1000000ULL + (unsigned long long)tv.tv_usec;
}

#define BENCH(label, code)                                                     \
    do {                                                                       \
        unsigned long long bench_total = 0;                                     \
        for (int bench_i = 0; bench_i < NTESTS; bench_i++) {                   \
            unsigned long long bench_start = g_bench_microseconds ?            \
                wall_microseconds() : cpucycles();                             \
            for (int bench_rep = 0; bench_rep < NBATCH; bench_rep++) {         \
                do {                                                           \
                    code                                                       \
                } while (0);                                                   \
            }                                                                  \
            bench_total += (g_bench_microseconds ? wall_microseconds() :       \
                cpucycles()) - bench_start;                                    \
        }                                                                      \
        print_average((label), bench_total);                                   \
    } while (0)

static void print_uint64(unsigned long long num)
{
    if (num >= 10ULL) {
        print_uint64(num / 10ULL);
    }
    printf("%d", (int)(num % 10ULL));
}

static void print_average(const char *label, unsigned long long total)
{
    unsigned long long denom = (unsigned long long)NTESTS * (unsigned long long)NBATCH;
    unsigned long long scaled_average = (total * 100ULL) / denom;

    if (!g_bench_microseconds) {
        unsigned long long scaled_overhead = (g_cycle_overhead * 100ULL) / (unsigned long long)NBATCH;
        if (scaled_average > scaled_overhead) {
            scaled_average -= scaled_overhead;
        }
    }

    printf("%-34s : ", label);
    print_uint64(scaled_average / 100ULL);
    printf(".%02d %s/op\n", (int)(scaled_average % 100ULL),
           g_bench_microseconds ? "microseconds" : "cpucycles");
}

static void calibrate_cycle_counter(void)
{
    unsigned long long total = 0;

    for (int i = 0; i < NTESTS; i++) {
        unsigned long long start = cpucycles();
        total += cpucycles() - start;
    }
    g_cycle_overhead = total / (unsigned long long)NTESTS;
}

static void fill_seed(uint8_t *seed, uint8_t base)
{
    for (int i = 0; i < KEM_SEED_LEN_BYTES; i++) {
        seed[i] = (uint8_t)(base + (uint8_t)i);
    }
}

static void fill_message(uint8_t *msg, uint8_t base)
{
    for (int i = 0; i < PKE_MESSAGE_BYTES; i++) {
        msg[i] = (uint8_t)(base + (uint8_t)(3 * i));
    }
}

static void fill_canonical_poly(int16_t *poly)
{
    for (int i = 0; i < RL_KEM_N; i++) {
        poly[i] = rl_kem_mod_q((int32_t)(i * 37) - 200);
    }
}

static void seed_drng_with_base(uint8_t base)
{
    uint8_t seed[KEM_SEED_LEN_BYTES];

    fill_seed(seed, base);
    if (init_random_number(&drng_algorithm, seed, sizeof(seed)) != 0) {
        fprintf(stderr, "init_random_number failed during cycle setup\n");
    }
}

static int spectral_bound(const int16_t *poly)
{
    return fft_within_bound_int16(poly, (int32_t)RL_KEM_T);
}

static volatile int g_reject_sink;

static void sample_screened_ternary(int16_t *poly, const uint8_t *seed, uint8_t *nonce)
{
    for (;;) {
        poly_generate_tenary(poly, seed, *nonce);
        *nonce = (uint8_t)(*nonce + 1U);
        if (spectral_bound(poly)) {
            return;
        }
    }
}

static void sample_screened_polyvec(polarlac_polyvec *vec, const uint8_t *seed, uint8_t *nonce)
{
    for (int k = 0; k < RL_KEM_K; k++) {
        sample_screened_ternary(vec->vec[k].coeffs, seed, nonce);
    }
}

static void sample_screened_polyvec_pair(polarlac_polyvec *vec0, polarlac_polyvec *vec1,
                                         const uint8_t *seed, uint8_t *nonce)
{
#if RL_KEM_K == 2
    polarlac_poly *targets[2 * RL_KEM_K] = {
        &vec0->vec[0], &vec0->vec[1],
        &vec1->vec[0], &vec1->vec[1]
    };
    polarlac_poly candidates[4];
    uint8_t n = *nonce;
    int filled = 0;

    while (filled < 2 * RL_KEM_K) {
        poly_generate_tenary_x4(candidates[0].coeffs, candidates[1].coeffs,
                                candidates[2].coeffs, candidates[3].coeffs,
                                seed, n, (uint8_t)(n + 1U),
                                (uint8_t)(n + 2U), (uint8_t)(n + 3U));

        for (int j = 0; j < 4 && filled < 2 * RL_KEM_K; j++) {
            if (spectral_bound(candidates[j].coeffs)) {
                *targets[filled] = candidates[j];
                filled++;
            }
            n = (uint8_t)(n + 1U);
        }
    }

    *nonce = n;
#else
    sample_screened_polyvec(vec0, seed, nonce);
    sample_screened_polyvec(vec1, seed, nonce);
#endif
}

static inline void poly_ntt_from_coeffs(int16_t *dst, const int16_t *src)
{
    memcpy(dst, src, RL_KEM_N * sizeof(int16_t));
    mq_poly_ntt(dst);
}

static inline void polyvec_ntt_from(polarlac_polyvec *dst, const polarlac_polyvec *src)
{
    for (int k = 0; k < RL_KEM_K; k++) {
        poly_ntt_from_coeffs(dst->vec[k].coeffs, src->vec[k].coeffs);
    }
}

static inline void mlwe_muladd2_ntt_products(int16_t *out,
    const int16_t *a0_ntt, const int16_t *b0_ntt,
    const int16_t *a1_ntt, const int16_t *b1_ntt)
{
    mq_poly_pointwise_muladd2_aligned(out, a0_ntt, b0_ntt, a1_ntt, b1_ntt);
}

static void mlwe_matvec_ntt(polarlac_polyvec *out_ntt,
    const polarlac_polymat *a_ntt, const polarlac_polyvec *s_ntt)
{
    for (int i = 0; i < RL_KEM_K; i++) {
        mlwe_muladd2_ntt_products(out_ntt->vec[i].coeffs,
            a_ntt->row[i].vec[0].coeffs, s_ntt->vec[0].coeffs,
            a_ntt->row[i].vec[1].coeffs, s_ntt->vec[1].coeffs);
    }
}

static inline void mlwe_matvec_intt_add_error(polarlac_polyvec *out,
    const polarlac_polymat *a_ntt, const polarlac_polyvec *r_ntt,
    const polarlac_polyvec *err)
{
    mlwe_matvec_ntt(out, a_ntt, r_ntt);
    for (int i = 0; i < RL_KEM_K; i++) {
        mq_poly_intt(out->vec[i].coeffs);
        poly_add_inplace_reduce_q_avx2(out->vec[i].coeffs, err->vec[i].coeffs);
    }
}

static void mlwe_inner_product_ntt(int16_t *out_ntt,
    const polarlac_polyvec *a_ntt, const polarlac_polyvec *b_ntt)
{
    mlwe_muladd2_ntt_products(out_ntt,
        a_ntt->vec[0].coeffs, b_ntt->vec[0].coeffs,
        a_ntt->vec[1].coeffs, b_ntt->vec[1].coeffs);
}

static void mlwe_inner_product_cached_secret(int16_t *out,
    const polarlac_polyvec *u, const polarlac_polyvec *s_ntt)
{
#if RL_KEM_Q != 769
    polarlac_poly u0_ntt;
    polarlac_poly u1_ntt;
#endif

#if RL_KEM_Q == 769
    mlwe_muladd2_ntt_products(out,
        u->vec[0].coeffs, s_ntt->vec[0].coeffs,
        u->vec[1].coeffs, s_ntt->vec[1].coeffs);
#else
    poly_ntt_from_coeffs(u0_ntt.coeffs, u->vec[0].coeffs);
    poly_ntt_from_coeffs(u1_ntt.coeffs, u->vec[1].coeffs);
    mlwe_muladd2_ntt_products(out,
        u0_ntt.coeffs, s_ntt->vec[0].coeffs,
        u1_ntt.coeffs, s_ntt->vec[1].coeffs);
#endif
    mq_poly_intt(out);
}

static inline void serialize_polyvec_ntt(uint8_t *dst, const polarlac_polyvec *vec)
{
    for (int k = 0; k < RL_KEM_K; k++) {
        memcpy(dst + k * RL_KEM_N * sizeof(int16_t),
            vec->vec[k].coeffs, RL_KEM_N * sizeof(int16_t));
    }
}

static void c1_vec_compress(uint8_t *dst, const polarlac_polyvec *src)
{
#if RL_KEM_Q == 769
    (void)polyvec_compress(dst, src);
#else
    polyvec_byte_compress(dst, src);
#endif
}

static void c1_vec_decompress(polarlac_polyvec *dst, const uint8_t *src)
{
#if RL_KEM_Q == 769
    polyvec_decompress(dst, src);
#else
    polyvec_byte_decompress(dst, src);
#endif
}

static int derive_g(uint8_t *ss, uint8_t *seed_enc, const uint8_t *m, const uint8_t *pk)
{
    uint8_t g_output[SS_KEY_BYTES + KEM_SEED_LEN_BYTES];

    if (bit_xof_concat2_bytes(g_output, sizeof(g_output),
            m, PKE_MESSAGE_BYTES, pk, PKE_PUBLIC_KEY_BYTES) != 0) {
        return -1;
    }

    memcpy(ss, g_output, SS_KEY_BYTES);
    memcpy(seed_enc, g_output + SS_KEY_BYTES, KEM_SEED_LEN_BYTES);
    return 0;
}

static int derive_reject_key(uint8_t *ss, const uint8_t *reject_seed, const uint8_t *ct)
{
    return bit_xof_concat2_bytes(ss, SS_KEY_BYTES,
        reject_seed, KEM_REJECT_SEED_BYTES, ct, PKE_CIPHERTEXT_BYTES);
}

static int sanity_check(void)
{
    uint8_t pk[PKE_PUBLIC_KEY_BYTES];
    uint8_t sk[PKE_SECRET_KEY_BYTES + PKE_PUBLIC_KEY_BYTES + KEM_REJECT_SEED_BYTES];
    uint8_t ct[PKE_CIPHERTEXT_BYTES];
    uint8_t ss[SS_KEY_BYTES];
    uint8_t ss_dec[SS_KEY_BYTES];
    unsigned long long pk_len = kem_get_pk_len_bytes();
    unsigned long long sk_len = kem_get_sk_len_bytes();
    unsigned long long ct_len = kem_get_ct_len_bytes();
    unsigned long long ss_len = kem_get_ss_len_bytes();

    seed_drng_with_base(0xA0U);
    if (kem_keygen(pk, &pk_len, sk, &sk_len) != 0) {
        return -1;
    }
    if (kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len) != 0) {
        return -2;
    }
    if (kem_dec(sk, sk_len, ct, ct_len, ss_dec, &ss_len) != 0) {
        return -3;
    }
    if (memcmp(ss, ss_dec, SS_KEY_BYTES) != 0) {
        return -4;
    }

    ct[0] ^= 1U;
    if (kem_dec(sk, sk_len, ct, ct_len, ss_dec, &ss_len) != 0) {
        return -5;
    }

    return 0;
}

static void benchmark_whole_api(void)
{
    uint8_t pk[PKE_PUBLIC_KEY_BYTES];
    uint8_t sk_pke[PKE_SECRET_KEY_BYTES];
    uint8_t sk_kem[PKE_SECRET_KEY_BYTES + PKE_PUBLIC_KEY_BYTES + KEM_REJECT_SEED_BYTES];
    uint8_t ct[PKE_CIPHERTEXT_BYTES];
    uint8_t ct_bad[PKE_CIPHERTEXT_BYTES];
    uint8_t msg[PKE_MESSAGE_BYTES];
    uint8_t dec[PKE_MESSAGE_BYTES];
    uint8_t ss[SS_KEY_BYTES];
    uint8_t ss_dec[SS_KEY_BYTES];
    uint8_t seed_kg[KEM_SEED_LEN_BYTES];
    uint8_t seed_enc[KEM_SEED_LEN_BYTES];
    unsigned long long pk_len = kem_get_pk_len_bytes();
    unsigned long long sk_len = kem_get_sk_len_bytes();
    unsigned long long ct_len = kem_get_ct_len_bytes();
    unsigned long long ss_len = kem_get_ss_len_bytes();

    fill_seed(seed_kg, 0x11U);
    fill_seed(seed_enc, 0x91U);
    fill_message(msg, 0x3CU);
    PKE_KeyGen(pk, sk_pke, seed_kg);
    PKE_Encrypt(ct, pk, msg, seed_enc);

    seed_drng_with_base(0x40U);
    kem_keygen(pk, &pk_len, sk_kem, &sk_len);
    kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len);
    memcpy(ct_bad, ct, sizeof(ct_bad));
    ct_bad[0] ^= 1U;

    printf("\n=========================== whole api cycles ===========================\n");

    BENCH("PKE_KeyGen total", {
        PKE_KeyGen(pk, sk_pke, seed_kg);
    });

    BENCH("PKE_Encrypt total", {
        PKE_Encrypt(ct, pk, msg, seed_enc);
    });

    BENCH("PKE_Decrypt total", {
        PKE_Decrypt(dec, ct, sk_pke);
    });

    seed_drng_with_base(0x41U);
    BENCH("kem_keygen total", {
        kem_keygen(pk, &pk_len, sk_kem, &sk_len);
    });

    seed_drng_with_base(0x51U);
    BENCH("kem_enc total", {
        kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len);
    });

    BENCH("kem_dec valid total", {
        kem_dec(sk_kem, sk_len, ct, ct_len, ss_dec, &ss_len);
    });

    // BENCH("kem_dec reject total", {
    //     kem_dec(sk_kem, sk_len, ct_bad, ct_len, ss_dec, &ss_len);
    // });
//     "kem_dec reject total" 覆盖的是 kem_dec() 的 reject 路径：
// 用 sk_pke 对坏密文 ct_bad 执行 PKE_Decrypt
// 根据解出的 m 和 stored pk 通过 G 一次派生 ss_valid || seed_enc
// 重新加密得到 ct_check
// 比较 ct_bad 和 ct_check
// 因为不一致，走 reject key：
// ss = XOF(z || ct_bad)，其中 z 是 sk 中预存的拒绝种子
// 它的意义是衡量 FO transform 里 密文验证失败时 的完整解封装成本。
}

static void benchmark_kem_units(void)
{
    uint8_t pk[PKE_PUBLIC_KEY_BYTES];
    uint8_t sk[PKE_SECRET_KEY_BYTES + PKE_PUBLIC_KEY_BYTES + KEM_REJECT_SEED_BYTES];
    uint8_t ct[PKE_CIPHERTEXT_BYTES];
    uint8_t ct_check[PKE_CIPHERTEXT_BYTES];
    uint8_t msg[PKE_MESSAGE_BYTES];
    uint8_t seed_enc[KEM_SEED_LEN_BYTES];
    uint8_t ss[SS_KEY_BYTES];
    uint8_t random_buf[KEM_SEED_LEN_BYTES];
    uint8_t m_dec[PKE_MESSAGE_BYTES];
    unsigned long long pk_len = kem_get_pk_len_bytes();
    unsigned long long sk_len = kem_get_sk_len_bytes();
    unsigned long long ct_len = kem_get_ct_len_bytes();
    unsigned long long ss_len = kem_get_ss_len_bytes();

    fill_message(msg, 0x70U);
    seed_drng_with_base(0x62U);
    kem_keygen(pk, &pk_len, sk, &sk_len);
    kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len);
    PKE_Decrypt(m_dec, ct, sk);
    derive_g(ss, seed_enc, m_dec, pk);

    printf("\n=========================== kem unit cycles ===========================\n");

    BENCH("KEM random seed_kg", {
        kem_get_random_number(&drng_algorithm, random_buf, KEM_SEED_LEN_BYTES * 8ULL);
    });

    BENCH("KEM random message", {
        kem_get_random_number(&drng_algorithm, random_buf, PKE_MESSAGE_BYTES * 8ULL);
    });

    BENCH("KEM G = XOF(m||pk)->ss||seed", {
        derive_g(ss, seed_enc, msg, pk);
    });

    BENCH("KEM H reject = XOF(z||ct)", {
        derive_reject_key(ss, sk + PKE_SECRET_KEY_BYTES + PKE_PUBLIC_KEY_BYTES, ct);
    });

    // BENCH("KEM reject key", {
    //     derive_reject_key(ss, sk + PKE_SECRET_KEY_BYTES + PKE_PUBLIC_KEY_BYTES, ct);
    // });

    BENCH("KEM dec PKE_Decrypt", {
        PKE_Decrypt(m_dec, ct, sk);
    });

    BENCH("KEM dec verify re-encrypt", {
        PKE_Encrypt(ct_check, pk, m_dec, seed_enc);
    });

    BENCH("KEM dec compare ct", {
        (void)ct_verify(ct, ct_check, PKE_CIPHERTEXT_BYTES);
    });
}

static void benchmark_pke_keygen_units(void)
{
    uint8_t seed_kg[KEM_SEED_LEN_BYTES];
    uint8_t expanded[PKE_EXPANDED_SEED_BYTES];
    uint8_t seed_se[KEM_SEED_LEN_BYTES];
    uint8_t pk_part[PK_LEN_BYTES];
    uint8_t sk_ntt_raw[SK_NTT_LEN_BYTES];
    polarlac_polymat a_ntt;
    polarlac_polyvec s;
    polarlac_polyvec s_ntt;
    polarlac_polyvec e;
    polarlac_polyvec e_ntt;
    polarlac_polyvec b_ntt;
    uint8_t nonce = 0;

    fill_seed(seed_kg, 0x22U);
    bit_xof_bytes(expanded, PKE_EXPANDED_SEED_BYTES, seed_kg, KEM_SEED_LEN_BYTES);
    memcpy(seed_se, expanded + PK_SEED_LEN_BYTES, KEM_SEED_LEN_BYTES);
    poly_generate_uniformQ(&a_ntt, expanded, 0);
    sample_screened_polyvec_pair(&s, &e, seed_se, &nonce);
    polyvec_ntt_from(&s_ntt, &s);
    polyvec_ntt_from(&e_ntt, &e);
    mlwe_matvec_ntt(&b_ntt, &a_ntt, &s_ntt);
    for (int i = 0; i < RL_KEM_K; i++) {
        poly_add_inplace_reduce_q_avx2(b_ntt.vec[i].coeffs, e_ntt.vec[i].coeffs);
    }

    printf("\n======================= pke keygen unit cycles ========================\n");

    BENCH("KeyGen expand seed", {
        bit_xof_bytes(expanded, PKE_EXPANDED_SEED_BYTES, seed_kg, KEM_SEED_LEN_BYTES);
    });

    BENCH("KeyGen sample A matrix", {
        poly_generate_uniformQ(&a_ntt, expanded, 0);
    });

    BENCH("KeyGen sample s+e vectors screened", {
        uint8_t n = (uint8_t)bench_i;
        sample_screened_polyvec_pair(&s, &e, seed_se, &n);
    });

    BENCH("KeyGen NTT(s vector)", {
        polyvec_ntt_from(&s_ntt, &s);
    });

    BENCH("KeyGen NTT(e vector)", {
        polyvec_ntt_from(&e_ntt, &e);
    });

    BENCH("KeyGen b_ntt=A*s+e", {
        mlwe_matvec_ntt(&b_ntt, &a_ntt, &s_ntt);
        for (int j = 0; j < RL_KEM_K; j++) {
            poly_add_inplace_reduce_q_avx2(b_ntt.vec[j].coeffs, e_ntt.vec[j].coeffs);
        }
    });

    BENCH("KeyGen compress b_ntt vector", {
        (void)polyvec_compress(pk_part, &b_ntt);
    });

    BENCH("KeyGen cache raw s_ntt vector", {
        serialize_polyvec_ntt(sk_ntt_raw, &s_ntt);
    });

}

static void benchmark_pke_encrypt_units(void)
{
    uint8_t pk[PKE_PUBLIC_KEY_BYTES];
    uint8_t sk[PKE_SECRET_KEY_BYTES];
    uint8_t seed_kg[KEM_SEED_LEN_BYTES];
    uint8_t seed_enc[KEM_SEED_LEN_BYTES];
    uint8_t c1_packed[C1_LEN_BYTES];
    uint8_t c2_code[RL_KEM_Lv];
    uint8_t c2_packed[C2_LEN_BYTES];
    uint8_t msg[PKE_MESSAGE_BYTES];
    uint8_t hatm[RL_KEM_Lv];
    polarlac_polymat a_t_ntt;
    polarlac_polyvec b_ntt;
    polarlac_polyvec r;
    polarlac_polyvec r_ntt;
    polarlac_polyvec e1;
#if RL_KEM_Q == 769
    polarlac_polyvec e1_ntt;
#endif
    polarlac_poly e2;
    polarlac_polyvec u;
    polarlac_poly v;
    uint8_t nonce = 0;

    fill_seed(seed_kg, 0x31U);
    fill_seed(seed_enc, 0xB1U);
    fill_message(msg, 0x45U);
    PKE_KeyGen(pk, sk, seed_kg);
    poly_generate_uniformQ(&a_t_ntt, pk, 1);
    polyvec_decompress(&b_ntt, pk + PK_SEED_LEN_BYTES);
    Encode_m(hatm, msg);
    sample_screened_polyvec_pair(&r, &e1, seed_enc, &nonce);
    poly_generate_tenary(e2.coeffs, seed_enc, nonce);
    polyvec_ntt_from(&r_ntt, &r);
#if RL_KEM_Q == 769
    polyvec_ntt_from(&e1_ntt, &e1);
    mlwe_matvec_ntt(&u, &a_t_ntt, &r_ntt);
    for (int i = 0; i < RL_KEM_K; i++) {
        poly_add_inplace_reduce_q_avx2(u.vec[i].coeffs, e1_ntt.vec[i].coeffs);
    }
#else
    mlwe_matvec_intt_add_error(&u, &a_t_ntt, &r_ntt, &e1);
#endif
    mlwe_inner_product_ntt(v.coeffs, &b_ntt, &r_ntt);
    mq_poly_intt(v.coeffs);
    poly_add_inplace_reduce_q_avx2(v.coeffs, e2.coeffs);
    poly_add_msg_inplace_reduce_q_avx2(v.coeffs, hatm);
    poly_compress_c2(c2_code, v.coeffs, D_C2_BITS);

    printf("\n======================= pke encrypt unit cycles =======================\n");

    BENCH("Encrypt unpack pk b_ntt vector", {
        polyvec_decompress(&b_ntt, pk + PK_SEED_LEN_BYTES);
    });

    BENCH("Encrypt regenerate A^T matrix", {
        poly_generate_uniformQ(&a_t_ntt, pk, 1);
    });

    BENCH("Encrypt Encode_m", {
        Encode_m(hatm, msg);
    });

    BENCH("Encrypt sample r+e1 vectors screened", {
        uint8_t n = (uint8_t)bench_i;
        sample_screened_polyvec_pair(&r, &e1, seed_enc, &n);
    });

    BENCH("Encrypt sample e2 direct", {
        poly_generate_tenary(e2.coeffs, seed_enc, (uint8_t)bench_i);
    });

    BENCH("Encrypt NTT(r vector)", {
        polyvec_ntt_from(&r_ntt, &r);
    });

#if RL_KEM_Q == 769
    BENCH("Encrypt NTT(e1 vector)", {
        polyvec_ntt_from(&e1_ntt, &e1);
    });

    BENCH("Encrypt u_ntt=A^T*r+e1_ntt", {
        mlwe_matvec_ntt(&u, &a_t_ntt, &r_ntt);
        for (int i = 0; i < RL_KEM_K; i++) {
            poly_add_inplace_reduce_q_avx2(u.vec[i].coeffs, e1_ntt.vec[i].coeffs);
        }
    });
#else
    BENCH("Encrypt u=A^T*r+e1", {
        mlwe_matvec_intt_add_error(&u, &a_t_ntt, &r_ntt, &e1);
    });
#endif

    BENCH("Encrypt v=<b,r>+e2+msg", {
        mlwe_inner_product_ntt(v.coeffs, &b_ntt, &r_ntt);
        mq_poly_intt(v.coeffs);
        poly_add_inplace_reduce_q_avx2(v.coeffs, e2.coeffs);
        poly_add_msg_inplace_reduce_q_avx2(v.coeffs, hatm);
    });

    BENCH("Encrypt compress c1 vector", {
        c1_vec_compress(c1_packed, &u);
    });

    BENCH("Encrypt compress c2", {
        poly_compress_c2(c2_code, v.coeffs, D_C2_BITS);
    });

    BENCH("Encrypt pack c2", {
        pack_c2_dbit(c2_packed, c2_code);
    });
}

static void benchmark_pke_decrypt_units(void)
{
    uint8_t pk[PKE_PUBLIC_KEY_BYTES];
    uint8_t sk[PKE_SECRET_KEY_BYTES];
    uint8_t ct[PKE_CIPHERTEXT_BYTES];
    uint8_t seed_kg[KEM_SEED_LEN_BYTES];
    uint8_t seed_enc[KEM_SEED_LEN_BYTES];
    uint8_t msg[PKE_MESSAGE_BYTES];
    uint8_t c2_code[RL_KEM_Lv];
    polarlac_polyvec u;
    polarlac_poly v;
    polarlac_polyvec s_ntt_vec;
    polarlac_poly su;
    int16_t hatm[RL_KEM_Lv];
    uint8_t dec[PKE_MESSAGE_BYTES];

    fill_seed(seed_kg, 0x51U);
    fill_seed(seed_enc, 0xC1U);
    fill_message(msg, 0x5CU);
    PKE_KeyGen(pk, sk, seed_kg);
    PKE_Encrypt(ct, pk, msg, seed_enc);
    c1_vec_decompress(&u, ct);
    unpack_c2_dbit(c2_code, ct + C1_LEN_BYTES);
    poly_decompress_c2(v.coeffs, c2_code, D_C2_BITS);
    memcpy(&s_ntt_vec, sk, sizeof(s_ntt_vec));
    mlwe_inner_product_cached_secret(su.coeffs, &u, &s_ntt_vec);
    poly_normalize_q_avx2(su.coeffs);
    poly_sub_reduce_q_avx2(hatm, v.coeffs, su.coeffs);

    printf("\n======================= pke decrypt unit cycles =======================\n");

    BENCH("Decrypt unpack c1 vector", {
        c1_vec_decompress(&u, ct);
    });

    BENCH("Decrypt unpack c2", {
        unpack_c2_dbit(c2_code, ct + C1_LEN_BYTES);
        poly_decompress_c2(v.coeffs, c2_code, D_C2_BITS);
    });

    BENCH("Decrypt load cached s_ntt", {
        memcpy(&s_ntt_vec, sk, sizeof(s_ntt_vec));
    });

    BENCH("Decrypt su=<c1,s_ntt>", {
        mlwe_inner_product_cached_secret(su.coeffs, &u, &s_ntt_vec);
    });

    BENCH("Decrypt c2-u", {
        poly_normalize_q_avx2(su.coeffs);
        poly_sub_reduce_q_avx2(hatm, v.coeffs, su.coeffs);
    });

    BENCH("Decrypt Decode_m", {
        Decode_m(dec, hatm);
    });
}

static void benchmark_core_primitives(void)
{
    uint8_t seed[KEM_SEED_LEN_BYTES];
    uint8_t c1_packed[C1_LEN_BYTES];
    uint8_t c2_code[RL_KEM_Lv];
    uint8_t c2_packed[C2_LEN_BYTES];
    int16_t poly[RL_KEM_N];
    int16_t poly2[RL_KEM_N];
    int16_t out[RL_KEM_N];
    polarlac_polymat mat;
    polarlac_polyvec vec;
    polarlac_polyvec vec_dec;
    ci16_t uniq[RL_KEM_N_Half];

    fill_seed(seed, 0xD0U);
    fill_canonical_poly(poly);
    fill_canonical_poly(poly2);
    for (int k = 0; k < RL_KEM_K; k++) {
        for (int i = 0; i < RL_KEM_N; i++) {
            vec.vec[k].coeffs[i] = rl_kem_mod_q((int32_t)poly[i] + k);
        }
    }
    mq_poly_ntt(poly2);
    poly_compress_c2(c2_code, poly, D_C2_BITS);
    pack_c2_dbit(c2_packed, c2_code);

    printf("\n======================== core primitive cycles ========================\n");

    BENCH("sample A matrix", {
        poly_generate_uniformQ(&mat, seed, 0);
    });

    BENCH("sample ternary direct", {
        poly_generate_tenary(out, seed, (uint8_t)bench_i);
    });

    BENCH("sample ternary screened", {
        uint8_t n = (uint8_t)bench_i;
        sample_screened_ternary(out, seed, &n);
    });

    BENCH("selected rejection check", {
        g_reject_sink ^= spectral_bound(poly);
    });

    BENCH("fft bound check", {
        g_reject_sink ^= fft_within_bound_int16(poly, (int32_t)RL_KEM_T);
    });

    BENCH("real half fft", {
        real_fft_unique_raw_i16(poly, 1, RL_KEM_N, uniq);
    });

    BENCH("ntt", {
        memcpy(out, poly, sizeof(out));
        mq_poly_ntt(out);
    });

    BENCH("intt", {
        memcpy(out, poly2, sizeof(out));
        mq_poly_intt(out);
    });

    BENCH("pointwise mul", {
        mq_poly_pointwise_mul(out, poly2, poly2);
    });

    BENCH("poly_mul", {
        poly_mul(out, poly, poly);
    });

    BENCH("poly_mul_with_cached_ntt", {
        poly_mul_with_cached_ntt(out, poly, poly2);
    });

    BENCH("poly_mul_add_with_ntt", {
        poly_mul_add_with_ntt(out, poly2, poly, poly);
    });

    BENCH("poly_mul_add_with_cached_ntt", {
        poly_mul_add_with_cached_ntt(out, poly2, poly2, poly);
    });

    BENCH("compress c1 vector", {
        c1_vec_compress(c1_packed, &vec);
    });

    BENCH("decompress c1 vector", {
        c1_vec_decompress(&vec_dec, c1_packed);
    });

    BENCH("compress c2", {
        poly_compress_c2(c2_code, poly, D_C2_BITS);
    });

    BENCH("decompress c2", {
        poly_decompress_c2(out, c2_code, D_C2_BITS);
    });

    BENCH("pack c2", {
        pack_c2_dbit(c2_packed, c2_code);
    });

    BENCH("unpack c2", {
        unpack_c2_dbit(c2_code, c2_packed);
    });
}

int main(void)
{
    int sanity;

    calibrate_cycle_counter();
    sanity = sanity_check();
    if (sanity != 0) {
        fprintf(stderr, "cycle-test sanity check failed: %d\n", sanity);
        return 1;
    }

    printf("ALGORITHM      : %s\n", ALGORITHM_INSTANCE);
    printf("RL_KEM_N       : %d\n", RL_KEM_N);
    printf("RL_KEM_Lv      : %d\n", RL_KEM_Lv);
    printf("MESSAGE_BYTES  : %d\n", MESSAGE_LEN_BYTES);
    printf("D_C2_BITS      : %d\n", D_C2_BITS);
    printf("NTESTS         : %d\n", NTESTS);
    printf("NBATCH         : %d\n", NBATCH);
    printf("cycle overhead : ");
    print_uint64(g_cycle_overhead);
    printf(" cpucycles\n");
    printf("sanity         : PASS\n");

    printf("\n=== cycles block ===\n");
    g_bench_microseconds = 0;
    benchmark_whole_api();
    benchmark_kem_units();
    benchmark_pke_keygen_units();
    benchmark_pke_encrypt_units();
    benchmark_pke_decrypt_units();
    benchmark_core_primitives();

    // printf("\n=== microseconds block ===\n");
    // g_bench_microseconds = 1;
    // benchmark_whole_api();
    // benchmark_kem_units();
    // benchmark_pke_keygen_units();
    // benchmark_pke_encrypt_units();
    // benchmark_pke_decrypt_units();
    // benchmark_core_primitives();

    return 0;
}
