/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements polynomial sampling routines for the optimized POLARLAC-512 instance.
*/

#include <stdint.h>
#include <string.h>
#include "params.h"
#include "symmetric.h"
#include "poly.h"
#include "arm_mlwe_sampler_fft.h"

#ifdef POLARLAC_B_TEST_HOOKS
static uint64_t polarlac_b_trace_ternary_bytes_value;
static uint64_t polarlac_b_trace_uniform_bytes_value;
static uint32_t polarlac_b_trace_q5_calls_value;
static uint32_t polarlac_b_trace_q4_calls_value;

void polarlac_b_test_trace_reset(void)
{
    polarlac_b_trace_ternary_bytes_value = 0;
    polarlac_b_trace_uniform_bytes_value = 0;
    polarlac_b_trace_q5_calls_value = 0;
    polarlac_b_trace_q4_calls_value = 0;
}

uint64_t polarlac_b_test_trace_ternary_bytes(void)
{
    return polarlac_b_trace_ternary_bytes_value;
}

uint64_t polarlac_b_test_trace_uniform_bytes(void)
{
    return polarlac_b_trace_uniform_bytes_value;
}

uint32_t polarlac_b_test_trace_q5_calls(void)
{
    return polarlac_b_trace_q5_calls_value;
}

uint32_t polarlac_b_test_trace_q4_calls(void)
{
    return polarlac_b_trace_q4_calls_value;
}

#define POLARLAC_B_TRACE_TERNARY_BYTES(n) \
    (polarlac_b_trace_ternary_bytes_value += (uint64_t)(n))
#define POLARLAC_B_TRACE_UNIFORM_BYTES(n) \
    (polarlac_b_trace_uniform_bytes_value += (uint64_t)(n))
#define POLARLAC_B_TRACE_Q5_CALL() (polarlac_b_trace_q5_calls_value++)
#define POLARLAC_B_TRACE_Q4_CALL() (polarlac_b_trace_q4_calls_value++)
#else
#define POLARLAC_B_TRACE_TERNARY_BYTES(n) ((void)(n))
#define POLARLAC_B_TRACE_UNIFORM_BYTES(n) ((void)(n))
#define POLARLAC_B_TRACE_Q5_CALL() ((void)0)
#define POLARLAC_B_TRACE_Q4_CALL() ((void)0)
#endif


#define GEN_A_Q5_BYTES 1368
#define GEN_A_Q4_BYTES 30

static inline uint64_t load_u48_le(const uint8_t *buf)
{
    return (uint64_t)buf[0]
         | ((uint64_t)buf[1] << 8)
         | ((uint64_t)buf[2] << 16)
         | ((uint64_t)buf[3] << 24)
         | ((uint64_t)buf[4] << 32)
         | ((uint64_t)buf[5] << 40);
}

static inline uint64_t load_u39_le(const uint8_t *buf)
{
    return (uint64_t)buf[0]
         | ((uint64_t)buf[1] << 8)
         | ((uint64_t)buf[2] << 16)
         | ((uint64_t)buf[3] << 24)
         | (((uint64_t)buf[4] & 0x7FULL) << 32);
}

static inline uint64_t divmod_769_exact(uint64_t x, uint64_t *remainder)
{
    /*
     * reciprocal = ceil(2^58 / 769).  For 0 <= x < 769^5, the product's
     * approximation error remains below one quotient unit, so the high
     * 58 bits equal floor(x / 769) without a data-dependent correction.
     */
    const uint64_t reciprocal = 374811932576999ULL;
    uint64_t q = (uint64_t)(((__uint128_t)x * reciprocal) >> 58);
    *remainder = x - q * 769ULL;
    return q;
}

#ifdef POLARLAC_B_TEST_HOOKS
uint64_t polarlac_b_test_divmod_769(uint64_t x, uint64_t *remainder)
{
    return divmod_769_exact(x, remainder);
}
#endif

#if !POLARLAC_COMPONENT_B_UNIFORM_SELECTED

static inline unsigned int rejection_uniformQ5_u48(
    uint64_t *r, uint32_t len, const uint8_t *buf, uint32_t buflen)
{
    unsigned int i = 0;
    unsigned int j = 0;
    const uint64_t q5 = 268925323054849ULL; /* 769^5 */

    while (i < len && (j + 6u) <= buflen) {
        uint64_t x = load_u48_le(buf + j);
        j += 6u;
        if (x < q5) {
            r[i++] = x;
        }
    }
    return i;
}

static inline unsigned int rejection_uniformQ4_u39(
    uint64_t *r, uint32_t len, const uint8_t *buf, uint32_t buflen)
{
    unsigned int i = 0;
    unsigned int j = 0;
    const uint64_t q4 = 349707832321ULL; /* 769^4 */

    while (i < len && (j + 5u) <= buflen) {
        uint64_t x = load_u39_le(buf + j);
        j += 5u;
        if (x < q4) {
            r[i++] = x;
        }
    }
    return i;
}

static inline void pack_uniformQ_tmp(uint8_t *pa, const uint64_t *tmp)
{
    int i;
    int idx = 0;
    uint64_t res[154];

    memset(res, 0, sizeof(res));
    for (i = 0; i < 204; i += 4) {
        uint64_t x0 = tmp[i + 0];
        uint64_t x1 = tmp[i + 1];
        uint64_t x2 = tmp[i + 2];
        uint64_t x3 = tmp[i + 3];
        res[idx++] = x0 | ((x3 & 0xFFFFULL) << 48);
        res[idx++] = x1 | (((x3 >> 16) & 0xFFFFULL) << 48);
        res[idx++] = x2 | (((x3 >> 32) & 0xFFFFULL) << 48);
    }
    res[idx++] = tmp[204];
    memcpy(pa, (const uint8_t *)res, PK_CRT_POLY_BYTES);
}

static inline void sample_uniformQ_packed(uint8_t *pa, xof128_stream *stream)
{
    unsigned int t = 0;
    uint8_t buf5[GEN_A_Q5_BYTES];
    uint8_t buf4[GEN_A_Q4_BYTES];
    uint64_t tmp[205];

    while (t < 204u) {
        bit_xof128_stream_squeeze(stream, buf5, GEN_A_Q5_BYTES);
        POLARLAC_B_TRACE_Q5_CALL();
        POLARLAC_B_TRACE_UNIFORM_BYTES(GEN_A_Q5_BYTES);
        t += rejection_uniformQ5_u48(tmp + t, 204u - t,
                                     buf5, GEN_A_Q5_BYTES);
    }

    t = 0;
    while (t < 1u) {
        bit_xof128_stream_squeeze(stream, buf4, GEN_A_Q4_BYTES);
        POLARLAC_B_TRACE_Q4_CALL();
        POLARLAC_B_TRACE_UNIFORM_BYTES(GEN_A_Q4_BYTES);
        t += rejection_uniformQ4_u39(tmp + 204, 1u,
                                     buf4, GEN_A_Q4_BYTES);
    }

    pack_uniformQ_tmp(pa, tmp);
}

static inline void poly_unpack(int16_t *a, const uint8_t *pa)
{
    int i;
    int idx;
    uint64_t res[154] = {0};
    uint64_t tmp[205] = {0};
    const uint64_t mask48 = 0x0000FFFFFFFFFFFFULL;
    const uint64_t mask39 = 0x0000007FFFFFFFFFULL;

    memcpy((uint8_t *)res, pa, PK_CRT_POLY_BYTES);
    idx = 153;
    tmp[204] = res[idx--] & mask39;

    for (i = 203; i > 0; i -= 4) {
        tmp[i - 1] = res[idx] & mask48;
        tmp[i] = ((res[idx--] >> 48) & 0xFFFFULL) << 32;
        tmp[i - 2] = res[idx] & mask48;
        tmp[i] |= ((res[idx--] >> 48) & 0xFFFFULL) << 16;
        tmp[i - 3] = res[idx] & mask48;
        tmp[i] |= ((res[idx--] >> 48) & 0xFFFFULL);
    }

    for (i = 0; i < 4; i++) {
        uint64_t r;
        tmp[204] = divmod_769_exact(tmp[204], &r);
        a[RL_KEM_N - 4 + i] = (int16_t)r;
    }

    idx = 0;
    for (i = 0; i < RL_KEM_N - 4; i += 5) {
        uint64_t x = tmp[idx++];
        for (int d = 0; d < 5; d++) {
            uint64_t r;
            x = divmod_769_exact(x, &r);
            a[i + d] = (int16_t)r;
        }
    }
}

#else /* Optional B3 direct public-matrix expansion (route-disabled). */

static inline void decode_q5_coefficients(int16_t *dst, uint64_t x)
{
    for (int d = 0; d < 5; d++) {
        uint64_t r;
        x = divmod_769_exact(x, &r);
        dst[d] = (int16_t)r;
    }
}

static inline void decode_q4_coefficients(int16_t *dst, uint64_t x)
{
    for (int d = 0; d < 4; d++) {
        uint64_t r;
        x = divmod_769_exact(x, &r);
        dst[d] = (int16_t)r;
    }
}

/*
 * Preserve the original XOF block sizes, rejection order and accepted values,
 * but decode accepted base-769 integers directly into coefficients.  This
 * removes the internal pack->unpack round trip without changing the public
 * matrix or the XOF stream position.
 */
static inline void sample_uniformQ_coeffs(int16_t *a, xof128_stream *stream)
{
    uint8_t buf5[GEN_A_Q5_BYTES];
    uint8_t buf4[GEN_A_Q4_BYTES];
    unsigned int groups = 0;
    const uint64_t q5 = 268925323054849ULL;
    const uint64_t q4 = 349707832321ULL;

    while (groups < 204u) {
        unsigned int j = 0;
        bit_xof128_stream_squeeze(stream, buf5, GEN_A_Q5_BYTES);
        POLARLAC_B_TRACE_Q5_CALL();
        POLARLAC_B_TRACE_UNIFORM_BYTES(GEN_A_Q5_BYTES);
        while (groups < 204u && (j + 6u) <= GEN_A_Q5_BYTES) {
            uint64_t x = load_u48_le(buf5 + j);
            j += 6u;
            if (x < q5) {
                decode_q5_coefficients(a + 5u * groups, x);
                groups++;
            }
        }
    }

    for (;;) {
        unsigned int j = 0;
        bit_xof128_stream_squeeze(stream, buf4, GEN_A_Q4_BYTES);
        POLARLAC_B_TRACE_Q4_CALL();
        POLARLAC_B_TRACE_UNIFORM_BYTES(GEN_A_Q4_BYTES);
        while ((j + 5u) <= GEN_A_Q4_BYTES) {
            uint64_t x = load_u39_le(buf4 + j);
            j += 5u;
            if (x < q4) {
                decode_q4_coefficients(a + RL_KEM_N - 4, x);
                return;
            }
        }
    }
}

#endif /* optional B3 */

void poly_generate_uniformQ(polarlac_polymat *a, const uint8_t *seed, int transposed)
{
    unsigned int i, j;
    uint8_t exseed[PK_SEED_LEN_BYTES + 2];
    xof128_stream stream;
#if !POLARLAC_COMPONENT_B_UNIFORM_SELECTED
    uint8_t pa[PK_CRT_POLY_BYTES];
#endif

    memcpy(exseed, seed, PK_SEED_LEN_BYTES);
    for (i = 0; i < RL_KEM_K; i++) {
        for (j = 0; j < RL_KEM_K; j++) {
            if (transposed) {
                exseed[PK_SEED_LEN_BYTES] = (uint8_t)i;
                exseed[PK_SEED_LEN_BYTES + 1] = (uint8_t)j;
            } else {
                exseed[PK_SEED_LEN_BYTES] = (uint8_t)j;
                exseed[PK_SEED_LEN_BYTES + 1] = (uint8_t)i;
            }

            bit_xof128_stream_init(&stream, exseed, PK_SEED_LEN_BYTES + 2, 0);
#if POLARLAC_COMPONENT_B_UNIFORM_SELECTED
            sample_uniformQ_coeffs(a->row[i].vec[j].coeffs, &stream);
#else
            sample_uniformQ_packed(pa, &stream);
            poly_unpack(a->row[i].vec[j].coeffs, pa);
#endif
            bit_xof128_stream_release(&stream);
        }
    }
}

// Ternary sampler: {-1:3/16; 0:5/8; 1:3/16}.
void poly_generate_tenary(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    enum { SOURCE_BYTES = RL_KEM_N_LEN_BYTES * 4 };
    uint8_t buf[SOURCE_BYTES];
    xof128_stream stream;

    bit_xof128_stream_init(&stream, seed, KEM_SEED_LEN_BYTES, nonce);
    bit_xof128_stream_squeeze(&stream, buf, SOURCE_BYTES);
    POLARLAC_B_TRACE_TERNARY_BYTES(SOURCE_BYTES);
#if POLARLAC_COMPONENT_B_TERNARY_SELECTED
    polarlac_ternary_4plane_from_bytes(a, RL_KEM_N, buf, SOURCE_BYTES);
#else
    {
        int16_t t[RL_KEM_N * 4];
        unsigned int i, j;
        for (i = 0; i < 8; i++) {
            for (j = 0; j < SOURCE_BYTES; j++) {
                t[i * SOURCE_BYTES + j] = (int16_t)(buf[j] & 1U);
                buf[j] >>= 1;
            }
        }
        for (i = 0; i < RL_KEM_N; i++) {
            a[i] = (int16_t)((t[i] & t[i + RL_KEM_N]) -
                             (t[i + 2 * RL_KEM_N] & t[i + 3 * RL_KEM_N]));
        }
    }
#endif
    bit_xof128_stream_release(&stream);
}
