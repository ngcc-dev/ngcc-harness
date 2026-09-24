#ifndef FROST_E8_H
#define FROST_E8_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if defined(__GNUC__)
#define FROST_E8_UNUSED __attribute__((unused))
#else
#define FROST_E8_UNUSED
#endif

#ifdef FROST_CODEC_TRACE
extern volatile unsigned long frost_e8_encode_call_count;
extern volatile unsigned long frost_e8_decode_call_count;
#endif

#ifndef PARAMS_NBAR_R
#define PARAMS_NBAR_R PARAMS_NBAR
#endif
#ifndef PARAMS_NBAR_S
#define PARAMS_NBAR_S PARAMS_NBAR
#endif
#if (PARAMS_NBAR_R * PARAMS_NBAR_S) % 8 != 0
#error FROST_USE_E8_CODE requires PARAMS_NBAR_R*PARAMS_NBAR_S to be divisible by 8.
#endif

#if PARAMS_EXTRACTED_BITS < 2
#error FROST_USE_E8_CODE requires PARAMS_EXTRACTED_BITS >= 2.
#endif

static const int16_t E8_BASIS_2[8][8] = {
    { 4, -2,  0,  0,  0,  0,  0,  1 },
    { 0,  2, -2,  0,  0,  0,  0,  1 },
    { 0,  0,  2, -2,  0,  0,  0,  1 },
    { 0,  0,  0,  2, -2,  0,  0,  1 },
    { 0,  0,  0,  0,  2, -2,  0,  1 },
    { 0,  0,  0,  0,  0,  2, -2,  1 },
    { 0,  0,  0,  0,  0,  0,  2,  1 },
    { 0,  0,  0,  0,  0,  0,  0,  1 }
};

static uint64_t frost_e8_ct_mask_u64_lt(uint64_t a, uint64_t b)
{
    return (uint64_t)0 - ((a - b) >> 63);
}

static int64_t frost_e8_ct_select_i64(int64_t a, int64_t b, uint64_t mask)
{
    return (int64_t)(((uint64_t)a & ~mask) | ((uint64_t)b & mask));
}

static uint32_t frost_e8_ct_select_u32(uint32_t a, uint32_t b, uint64_t mask)
{
    return (uint32_t)(((uint64_t)a & ~mask) | ((uint64_t)b & mask));
}

static uint64_t frost_e8_abs_i64(int64_t x)
{
    int64_t mask = x >> 63;
    return (uint64_t)((x ^ mask) - mask);
}

static uint32_t frost_e8_read_bits(const uint8_t *in, size_t *bitpos, unsigned bits)
{
    uint32_t v = 0;
    for (unsigned b = 0; b < bits; b++) {
        v |= ((uint32_t)((in[*bitpos >> 3] >> (*bitpos & 7)) & 1u)) << b;
        (*bitpos)++;
    }
    return v;
}

static void frost_e8_write_bits(uint8_t *out, size_t *bitpos, uint32_t v, unsigned bits)
{
    for (unsigned b = 0; b < bits; b++) {
        out[*bitpos >> 3] |= (uint8_t)(((v >> b) & 1u) << (*bitpos & 7));
        (*bitpos)++;
    }
}

static void frost_e8_label_block(uint32_t x[8], const uint32_t z[8])
{
    const uint32_t qmask = ((uint32_t)1u << PARAMS_LOGQ) - 1u;
    const uint32_t alpha_half = (uint32_t)1u << (PARAMS_LOGQ - PARAMS_EXTRACTED_BITS - 1);

    for (size_t i = 0; i < 8; i++) {
        int64_t acc = 0;
        for (size_t j = 0; j < 8; j++) {
            acc += (int64_t)E8_BASIS_2[i][j] * (int64_t)z[j];
        }
        x[i] = (uint32_t)((acc * (int64_t)alpha_half) & (int64_t)qmask);
    }
}

static uint32_t frost_e8_reduce_2p(int64_t x)
{
    const uint32_t mask = ((uint32_t)1u << (PARAMS_EXTRACTED_BITS + 1)) - 1u;
    return (uint32_t)x & mask;
}

static void frost_e8_delabel_block(uint32_t z[8], const uint32_t x[8])
{
    const uint32_t p_mask = ((uint32_t)1u << PARAMS_EXTRACTED_BITS) - 1u;
    const uint32_t z0_mask = ((uint32_t)1u << (PARAMS_EXTRACTED_BITS - 1)) - 1u;
    uint32_t v[8];

    for (size_t i = 0; i < 8; i++) {
        v[i] = (x[i] >> (PARAMS_LOGQ - PARAMS_EXTRACTED_BITS - 1)) & ((2u << PARAMS_EXTRACTED_BITS) - 1u);
    }

    z[7] = v[7];
    z[6] = frost_e8_reduce_2p((int64_t)v[6] - z[7]) >> 1;
    z[5] = frost_e8_reduce_2p((int64_t)v[5] + 2u * z[6] - z[7]) >> 1;
    z[4] = frost_e8_reduce_2p((int64_t)v[4] + 2u * z[5] - z[7]) >> 1;
    z[3] = frost_e8_reduce_2p((int64_t)v[3] + 2u * z[4] - z[7]) >> 1;
    z[2] = frost_e8_reduce_2p((int64_t)v[2] + 2u * z[3] - z[7]) >> 1;
    z[1] = frost_e8_reduce_2p((int64_t)v[1] + 2u * z[2] - z[7]) >> 1;
    z[0] = frost_e8_reduce_2p((int64_t)v[0] + 2u * z[1] - z[7]) >> 2;

    z[0] &= z0_mask;
    for (size_t i = 1; i < 7; i++) z[i] &= p_mask;
    z[7] &= ((2u << PARAMS_EXTRACTED_BITS) - 1u);
}

static void frost_e8_d8_nn_decode_scaled(int64_t cand[8], int64_t y[8])
{
    const int64_t alpha = (int64_t)1 << (PARAMS_LOGQ - PARAMS_EXTRACTED_BITS);
    const int64_t alpha_half = alpha >> 1;
    int64_t u[8];
    int64_t residual[8];
    uint32_t parity = 0;
    uint64_t best_abs = 0;
    uint32_t best_idx = 0;

    for (size_t i = 0; i < 8; i++) {
        u[i] = (y[i] + alpha_half) >> (PARAMS_LOGQ - PARAMS_EXTRACTED_BITS);
        residual[i] = y[i] - alpha * u[i];
        parity ^= (uint32_t)u[i] & 1u;

        uint64_t a = frost_e8_abs_i64(residual[i]);
        uint64_t update = frost_e8_ct_mask_u64_lt(best_abs, a);
        best_abs = ((best_abs & ~update) | (a & update));
        best_idx = frost_e8_ct_select_u32(best_idx, (uint32_t)i, update);
    }

    uint64_t odd_mask = (uint64_t)0 - (uint64_t)(parity & 1u);
    for (size_t i = 0; i < 8; i++) {
        uint64_t eq = (uint64_t)0 - (uint64_t)((uint32_t)i == best_idx);
        uint64_t update = odd_mask & eq;
        int64_t sign = residual[i] >> 63;
        int64_t delta = 1 + (sign & -2);
        u[i] = frost_e8_ct_select_i64(u[i], u[i] + delta, update);
        cand[i] = alpha * u[i];
    }
}

static void frost_e8_nn_decode(uint32_t out[8], const uint32_t in[8])
{
    const uint32_t qmask = ((uint32_t)1u << PARAMS_LOGQ) - 1u;
    const int64_t alpha_half = (int64_t)1 << (PARAMS_LOGQ - PARAMS_EXTRACTED_BITS - 1);
    int64_t y0[8], y1[8], cand0[8], cand1[8];
    uint64_t dist0 = 0, dist1 = 0;

    for (size_t i = 0; i < 8; i++) {
        y0[i] = (int64_t)(in[i] & qmask);
        y1[i] = y0[i] - alpha_half;
    }

    frost_e8_d8_nn_decode_scaled(cand0, y0);
    frost_e8_d8_nn_decode_scaled(cand1, y1);

    for (size_t i = 0; i < 8; i++) {
        cand1[i] += alpha_half;
        int64_t r0 = y0[i] - cand0[i];
        int64_t r1 = y0[i] - cand1[i];
        dist0 += (uint64_t)(r0 * r0);
        dist1 += (uint64_t)(r1 * r1);
    }

    uint64_t use_cand1 = frost_e8_ct_mask_u64_lt(dist1, dist0);
    for (size_t i = 0; i < 8; i++) {
        int64_t selected = frost_e8_ct_select_i64(cand0[i], cand1[i], use_cand1);
        out[i] = (uint32_t)selected & qmask;
    }
}

static void frost_e8_encode_u32(uint32_t *out, const uint8_t *in)
{
#ifdef FROST_CODEC_TRACE
    frost_e8_encode_call_count++;
#endif
    static const unsigned bits[8] = {
        PARAMS_EXTRACTED_BITS - 1, PARAMS_EXTRACTED_BITS, PARAMS_EXTRACTED_BITS,
        PARAMS_EXTRACTED_BITS, PARAMS_EXTRACTED_BITS, PARAMS_EXTRACTED_BITS,
        PARAMS_EXTRACTED_BITS, PARAMS_EXTRACTED_BITS + 1
    };
    size_t bitpos = 0;
    for (size_t block = 0; block < (PARAMS_NBAR_R * PARAMS_NBAR_S) / 8; block++) {
        uint32_t z[8], x[8];
        for (size_t i = 0; i < 8; i++) z[i] = frost_e8_read_bits(in, &bitpos, bits[i]);
        frost_e8_label_block(x, z);
        for (size_t i = 0; i < 8; i++) out[8 * block + i] = x[i];
    }
}

static void frost_e8_decode_u32(uint8_t *out, const uint32_t *in)
{
#ifdef FROST_CODEC_TRACE
    frost_e8_decode_call_count++;
#endif
    static const unsigned bits[8] = {
        PARAMS_EXTRACTED_BITS - 1, PARAMS_EXTRACTED_BITS, PARAMS_EXTRACTED_BITS,
        PARAMS_EXTRACTED_BITS, PARAMS_EXTRACTED_BITS, PARAMS_EXTRACTED_BITS,
        PARAMS_EXTRACTED_BITS, PARAMS_EXTRACTED_BITS + 1
    };
    memset(out, 0, BYTES_MU);
    size_t bitpos = 0;
    for (size_t block = 0; block < (PARAMS_NBAR_R * PARAMS_NBAR_S) / 8; block++) {
        uint32_t decoded[8], z[8];
        frost_e8_nn_decode(decoded, &in[8 * block]);
        frost_e8_delabel_block(z, decoded);
        for (size_t i = 0; i < 8; i++) frost_e8_write_bits(out, &bitpos, z[i], bits[i]);
    }
}

static FROST_E8_UNUSED void frost_e8_encode_u16(uint16_t *out, const uint8_t *in)
{
    uint32_t tmp[PARAMS_NBAR_R * PARAMS_NBAR_S];
    frost_e8_encode_u32(tmp, in);
    for (size_t i = 0; i < PARAMS_NBAR_R * PARAMS_NBAR_S; i++) out[i] = (uint16_t)tmp[i];
}

static FROST_E8_UNUSED void frost_e8_decode_u16(uint8_t *out, const uint16_t *in)
{
    uint32_t tmp[PARAMS_NBAR_R * PARAMS_NBAR_S];
    for (size_t i = 0; i < PARAMS_NBAR_R * PARAMS_NBAR_S; i++) tmp[i] = in[i];
    frost_e8_decode_u32(out, tmp);
}

#endif
