/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements polynomial sampling routines for the optimized POLARLAC-Light instance.
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

/// @param[out] r Base address of output coefficient array
/// @param[in] len Total number of coefficients required in the output array
/// @param[in] buf Base address of input byte array used for sampling
/// @param[in] buflen Total bytes of the input byte array
/// @return Number of sampled coefficients written into the output array r
unsigned int rejection_uniformQ(int16_t *r, uint32_t len, const uint8_t *buf, uint32_t buflen)
{
    unsigned int i, j;
    uint16_t val0, val1, val2, val3, val4, val5, val6, val7;
    i = j = 0;
    while(i < len && (j+9) < buflen)
    {
        val0 = (((buf[j] >> 0) & 0x1) | (buf[j+1] << 1));
        val1 = (((buf[j] >> 1) & 0x1) | (buf[j+2] << 1));
        val2 = (((buf[j] >> 2) & 0x1) | (buf[j+3] << 1));
        val3 = (((buf[j] >> 3) & 0x1) | (buf[j+4] << 1));
        val4 = (((buf[j] >> 4) & 0x1) | (buf[j+5] << 1));
        val5 = (((buf[j] >> 5) & 0x1) | (buf[j+6] << 1));
        val6 = (((buf[j] >> 6) & 0x1) | (buf[j+7] << 1));
        val7 = (((buf[j] >> 7) & 0x1) | (buf[j+8] << 1));
        j += 9;

        if(val0 < RL_KEM_Q)
            r[i++] = val0;
        if(i < len && val1 < RL_KEM_Q)
            r[i++] = val1;
        if(i < len && val2 < RL_KEM_Q)
            r[i++] = val2;
        if(i < len && val3 < RL_KEM_Q)
            r[i++] = val3;
        if(i < len && val4 < RL_KEM_Q)
            r[i++] = val4;
        if(i < len && val5 < RL_KEM_Q)
            r[i++] = val5;
        if(i < len && val6 < RL_KEM_Q)
            r[i++] = val6;
        if(i < len && val7 < RL_KEM_Q)
            r[i++] = val7;
    }
    return i;
}

void poly_generate_uniformQ(polarlac_polymat *a, const uint8_t *seed, int transposed)
{
    unsigned int i, j;
    uint8_t buf[RL_KEM_N], exseed[PK_SEED_LEN_BYTES+2];
    xof128_stream stream;
    memcpy(exseed, seed, PK_SEED_LEN_BYTES);

    for(i = 0; i < RL_KEM_K; i++)
    {
        for(j = 0; j < RL_KEM_K; j++)
        {
            
            if(transposed)
            {
                exseed[PK_SEED_LEN_BYTES] = i;
                exseed[PK_SEED_LEN_BYTES+1] = j;
            }
            else
            {
                exseed[PK_SEED_LEN_BYTES] = j;
                exseed[PK_SEED_LEN_BYTES+1] = i;
            }
            bit_xof128_stream_init(&stream, exseed, PK_SEED_LEN_BYTES+2, 0);
            bit_xof128_stream_squeeze(&stream, buf, RL_KEM_N);
            POLARLAC_B_TRACE_UNIFORM_BYTES(RL_KEM_N);
#if POLARLAC_COMPONENT_B_UNIFORM_SELECTED
            polarlac_expand_u8_to_i16(a->row[i].vec[j].coeffs, buf, RL_KEM_N);
#else
            for (unsigned int t = 0; t < RL_KEM_N; t++) {
                a->row[i].vec[j].coeffs[t] = (int16_t)buf[t];
            }
#endif
        }
    }
    bit_xof128_stream_release(&stream);
}


// Ternary sampler: {-1:1/8; 0:3/4; 1:1/8}.
void poly_generate_tenary(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    enum { SOURCE_BYTES = RL_KEM_N_LEN_BYTES * 3 };
    uint8_t buf[SOURCE_BYTES];
    xof128_stream stream;

    bit_xof128_stream_init(&stream, seed, KEM_SEED_LEN_BYTES, nonce);
    bit_xof128_stream_squeeze(&stream, buf, SOURCE_BYTES);
    POLARLAC_B_TRACE_TERNARY_BYTES(SOURCE_BYTES);
#if POLARLAC_COMPONENT_B_TERNARY_SELECTED
    polarlac_ternary_3plane_from_bytes(a, RL_KEM_N, buf, SOURCE_BYTES);
#else
    {
        int16_t t[RL_KEM_N * 3];
        unsigned int i, j;
        for (i = 0; i < 8; i++) {
            for (j = 0; j < SOURCE_BYTES; j++) {
                t[i * SOURCE_BYTES + j] = (int16_t)(buf[j] & 1U);
                buf[j] >>= 1;
            }
        }
        for (i = 0; i < RL_KEM_N; i++) {
            a[i] = (int16_t)((t[i] - t[i + RL_KEM_N]) *
                             t[i + 2 * RL_KEM_N]);
        }
    }
#endif
    bit_xof128_stream_release(&stream);
}
