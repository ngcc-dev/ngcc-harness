/*
Copyright (c) 2026 Yu Zhang.
File Description: POLARLAC-512 q=257 sampling with selected ARM/SVE component-B helpers.
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

uint64_t polarlac_b_test_trace_ternary_bytes(void) { return polarlac_b_trace_ternary_bytes_value; }
uint64_t polarlac_b_test_trace_uniform_bytes(void) { return polarlac_b_trace_uniform_bytes_value; }
uint32_t polarlac_b_test_trace_q5_calls(void) { return polarlac_b_trace_q5_calls_value; }
uint32_t polarlac_b_test_trace_q4_calls(void) { return polarlac_b_trace_q4_calls_value; }

#define POLARLAC_B_TRACE_TERNARY_BYTES(n) (polarlac_b_trace_ternary_bytes_value += (uint64_t)(n))
#define POLARLAC_B_TRACE_UNIFORM_BYTES(n) (polarlac_b_trace_uniform_bytes_value += (uint64_t)(n))
#define POLARLAC_B_TRACE_Q5_CALL() (polarlac_b_trace_q5_calls_value++)
#define POLARLAC_B_TRACE_Q4_CALL() (polarlac_b_trace_q4_calls_value++)
#else
#define POLARLAC_B_TRACE_TERNARY_BYTES(n) ((void)(n))
#define POLARLAC_B_TRACE_UNIFORM_BYTES(n) ((void)(n))
#define POLARLAC_B_TRACE_Q5_CALL() ((void)0)
#define POLARLAC_B_TRACE_Q4_CALL() ((void)0)
#endif


void poly_generate_uniformQ(polarlac_polymat *a, const uint8_t *seed, int transposed)
{
    unsigned int i, j;
    uint8_t buf[RL_KEM_N], exseed[PK_SEED_LEN_BYTES + 2];
    xof128_stream stream;
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
            bit_xof128_stream_squeeze(&stream, buf, RL_KEM_N);
            POLARLAC_B_TRACE_UNIFORM_BYTES(RL_KEM_N);
#if POLARLAC_COMPONENT_B_UNIFORM_SELECTED
            polarlac_expand_u8_to_i16(a->row[i].vec[j].coeffs, buf, RL_KEM_N);
#else
            for (unsigned int t = 0; t < RL_KEM_N; t++) {
                a->row[i].vec[j].coeffs[t] = (int16_t)buf[t];
            }
#endif
            bit_xof128_stream_release(&stream);
        }
    }
}

// Ternary sampler: {-1:11/128; 0:106/128; 1:11/128}.
void poly_generate_tenary(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    enum { SOURCE_BYTES = RL_KEM_N_LEN_BYTES * 7 };
    uint8_t buf[SOURCE_BYTES];
    xof128_stream stream;

    bit_xof128_stream_init(&stream, seed, KEM_SEED_LEN_BYTES, nonce);
    bit_xof128_stream_squeeze(&stream, buf, SOURCE_BYTES);
    POLARLAC_B_TRACE_TERNARY_BYTES(SOURCE_BYTES);
#if POLARLAC_COMPONENT_B_TERNARY_SELECTED
    polarlac_ternary_7byte_groups_from_bytes(a, RL_KEM_N, buf, SOURCE_BYTES);
#else
    {
        uint8_t x0, x1, x2, x3, x4, x5, x6, k;
        uint16_t mneg, mpos;
        unsigned int i = 0, j = 0;
        while (j < RL_KEM_N) {
            x0 = buf[i++]; x1 = buf[i++]; x2 = buf[i++]; x3 = buf[i++];
            x4 = buf[i++]; x5 = buf[i++]; x6 = buf[i++];
#define MAP_K(KVAL) do { \
    k = (uint8_t)(KVAL); \
    mneg = (uint16_t)(0u - (((uint16_t)k - 11u) >> 15)); \
    mpos = (uint16_t)(0u - (((uint16_t)116u - (uint16_t)k) >> 15)); \
    a[j++] = (int16_t)((mpos & 1u) - (mneg & 1u)); \
} while (0)
            MAP_K(x0 & 127u); MAP_K(x1 & 127u); MAP_K(x2 & 127u); MAP_K(x3 & 127u);
            MAP_K(x4 & 127u); MAP_K(x5 & 127u); MAP_K(x6 & 127u);
            MAP_K((uint8_t)((((uint16_t)(x0 >> 7) & 1u) << 6) |
                            (((uint16_t)(x1 >> 7) & 1u) << 5) |
                            (((uint16_t)(x2 >> 7) & 1u) << 4) |
                            (((uint16_t)(x3 >> 7) & 1u) << 3) |
                            (((uint16_t)(x4 >> 7) & 1u) << 2) |
                            (((uint16_t)(x5 >> 7) & 1u) << 1) |
                             ((uint16_t)(x6 >> 7) & 1u)));
#undef MAP_K
        }
    }
#endif
    bit_xof128_stream_release(&stream);
}
