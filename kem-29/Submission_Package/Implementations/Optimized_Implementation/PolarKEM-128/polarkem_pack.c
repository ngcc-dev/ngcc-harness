/**
 * @file polarkem_pack.c
 * @brief Portable compile-time-specialized coefficient packing implementation.
 */
#include "polarkem_pack.h"

#include <stddef.h>

#define POLARKEM_D_RANGE (1U << POLARKEM_QUANT_BITS)
#define POLARKEM_D_MASK (POLARKEM_D_RANGE - 1U)

uint16_t polarkem_compress_coefficient(uint16_t a)
{
    const uint64_t scaled = (uint64_t)a * (uint64_t)POLARKEM_D_RANGE
                            + (uint64_t)(POLARKEM_Q / 2U);
    return (uint16_t)((scaled / (uint64_t)POLARKEM_Q)
                      & (uint64_t)POLARKEM_D_MASK);
}

uint16_t polarkem_decompress_coefficient(uint16_t t)
{
    const uint64_t scaled = (uint64_t)(t & POLARKEM_D_MASK)
                            * (uint64_t)POLARKEM_Q
                            + (uint64_t)(POLARKEM_D_RANGE / 2U);
    return (uint16_t)(scaled >> POLARKEM_QUANT_BITS);
}

void polarkem_pack_coefficients(
    unsigned char payload[POLARKEM_CT_PAYLOAD_BYTES],
    const uint16_t coefficients[POLARKEM_N])
{
    size_t i;
    size_t out = 0U;

#if POLARKEM_QUANT_BITS == 11U
    for (i = 0U; i < POLARKEM_N; i += 8U) {
        const uint16_t v0 = polarkem_compress_coefficient(coefficients[i]);
        const uint16_t v1 = polarkem_compress_coefficient(coefficients[i + 1U]);
        const uint16_t v2 = polarkem_compress_coefficient(coefficients[i + 2U]);
        const uint16_t v3 = polarkem_compress_coefficient(coefficients[i + 3U]);
        const uint16_t v4 = polarkem_compress_coefficient(coefficients[i + 4U]);
        const uint16_t v5 = polarkem_compress_coefficient(coefficients[i + 5U]);
        const uint16_t v6 = polarkem_compress_coefficient(coefficients[i + 6U]);
        const uint16_t v7 = polarkem_compress_coefficient(coefficients[i + 7U]);

        payload[out] = (unsigned char)v0;
        payload[out + 1U] = (unsigned char)((v0 >> 8U) | (v1 << 3U));
        payload[out + 2U] = (unsigned char)((v1 >> 5U) | (v2 << 6U));
        payload[out + 3U] = (unsigned char)(v2 >> 2U);
        payload[out + 4U] = (unsigned char)((v2 >> 10U) | (v3 << 1U));
        payload[out + 5U] = (unsigned char)((v3 >> 7U) | (v4 << 4U));
        payload[out + 6U] = (unsigned char)((v4 >> 4U) | (v5 << 7U));
        payload[out + 7U] = (unsigned char)(v5 >> 1U);
        payload[out + 8U] = (unsigned char)((v5 >> 9U) | (v6 << 2U));
        payload[out + 9U] = (unsigned char)((v6 >> 6U) | (v7 << 5U));
        payload[out + 10U] = (unsigned char)(v7 >> 3U);
        out += 11U;
    }
#elif POLARKEM_QUANT_BITS == 9U
    for (i = 0U; i < POLARKEM_N; i += 8U) {
        const uint16_t v0 = polarkem_compress_coefficient(coefficients[i]);
        const uint16_t v1 = polarkem_compress_coefficient(coefficients[i + 1U]);
        const uint16_t v2 = polarkem_compress_coefficient(coefficients[i + 2U]);
        const uint16_t v3 = polarkem_compress_coefficient(coefficients[i + 3U]);
        const uint16_t v4 = polarkem_compress_coefficient(coefficients[i + 4U]);
        const uint16_t v5 = polarkem_compress_coefficient(coefficients[i + 5U]);
        const uint16_t v6 = polarkem_compress_coefficient(coefficients[i + 6U]);
        const uint16_t v7 = polarkem_compress_coefficient(coefficients[i + 7U]);

        payload[out] = (unsigned char)v0;
        payload[out + 1U] = (unsigned char)((v0 >> 8U) | (v1 << 1U));
        payload[out + 2U] = (unsigned char)((v1 >> 7U) | (v2 << 2U));
        payload[out + 3U] = (unsigned char)((v2 >> 6U) | (v3 << 3U));
        payload[out + 4U] = (unsigned char)((v3 >> 5U) | (v4 << 4U));
        payload[out + 5U] = (unsigned char)((v4 >> 4U) | (v5 << 5U));
        payload[out + 6U] = (unsigned char)((v5 >> 3U) | (v6 << 6U));
        payload[out + 7U] = (unsigned char)((v6 >> 2U) | (v7 << 7U));
        payload[out + 8U] = (unsigned char)(v7 >> 1U);
        out += 9U;
    }
#elif POLARKEM_QUANT_BITS == 8U
    for (i = 0U; i < POLARKEM_N; i += 8U) {
        payload[out] = (unsigned char)polarkem_compress_coefficient(coefficients[i]);
        payload[out + 1U] = (unsigned char)polarkem_compress_coefficient(coefficients[i + 1U]);
        payload[out + 2U] = (unsigned char)polarkem_compress_coefficient(coefficients[i + 2U]);
        payload[out + 3U] = (unsigned char)polarkem_compress_coefficient(coefficients[i + 3U]);
        payload[out + 4U] = (unsigned char)polarkem_compress_coefficient(coefficients[i + 4U]);
        payload[out + 5U] = (unsigned char)polarkem_compress_coefficient(coefficients[i + 5U]);
        payload[out + 6U] = (unsigned char)polarkem_compress_coefficient(coefficients[i + 6U]);
        payload[out + 7U] = (unsigned char)polarkem_compress_coefficient(coefficients[i + 7U]);
        out += 8U;
    }
#else
#error "Optimized PolarKEM packing supports only d=11, d=9, or d=8"
#endif
}

void polarkem_unpack_coefficients(
    uint16_t coefficients[POLARKEM_N],
    const unsigned char payload[POLARKEM_CT_PAYLOAD_BYTES])
{
    size_t i;
    size_t in = 0U;

#if POLARKEM_QUANT_BITS == 11U
    for (i = 0U; i < POLARKEM_N; i += 8U) {
        uint16_t v[8];

        v[0] = (uint16_t)((uint16_t)payload[in]
                          | ((uint16_t)(payload[in + 1U] & 0x07U) << 8U));
        v[1] = (uint16_t)(((uint16_t)payload[in + 1U] >> 3U)
                          | ((uint16_t)(payload[in + 2U] & 0x3fU) << 5U));
        v[2] = (uint16_t)(((uint16_t)payload[in + 2U] >> 6U)
                          | ((uint16_t)payload[in + 3U] << 2U)
                          | ((uint16_t)(payload[in + 4U] & 0x01U) << 10U));
        v[3] = (uint16_t)(((uint16_t)payload[in + 4U] >> 1U)
                          | ((uint16_t)(payload[in + 5U] & 0x0fU) << 7U));
        v[4] = (uint16_t)(((uint16_t)payload[in + 5U] >> 4U)
                          | ((uint16_t)(payload[in + 6U] & 0x7fU) << 4U));
        v[5] = (uint16_t)(((uint16_t)payload[in + 6U] >> 7U)
                          | ((uint16_t)payload[in + 7U] << 1U)
                          | ((uint16_t)(payload[in + 8U] & 0x03U) << 9U));
        v[6] = (uint16_t)(((uint16_t)payload[in + 8U] >> 2U)
                          | ((uint16_t)(payload[in + 9U] & 0x1fU) << 6U));
        v[7] = (uint16_t)(((uint16_t)payload[in + 9U] >> 5U)
                          | ((uint16_t)payload[in + 10U] << 3U));

        coefficients[i] = polarkem_decompress_coefficient(v[0]);
        coefficients[i + 1U] = polarkem_decompress_coefficient(v[1]);
        coefficients[i + 2U] = polarkem_decompress_coefficient(v[2]);
        coefficients[i + 3U] = polarkem_decompress_coefficient(v[3]);
        coefficients[i + 4U] = polarkem_decompress_coefficient(v[4]);
        coefficients[i + 5U] = polarkem_decompress_coefficient(v[5]);
        coefficients[i + 6U] = polarkem_decompress_coefficient(v[6]);
        coefficients[i + 7U] = polarkem_decompress_coefficient(v[7]);
        in += 11U;
    }
#elif POLARKEM_QUANT_BITS == 9U
    for (i = 0U; i < POLARKEM_N; i += 8U) {
        uint16_t v[8];

        v[0] = (uint16_t)((uint16_t)payload[in]
                          | ((uint16_t)(payload[in + 1U] & 0x01U) << 8U));
        v[1] = (uint16_t)(((uint16_t)payload[in + 1U] >> 1U)
                          | ((uint16_t)(payload[in + 2U] & 0x03U) << 7U));
        v[2] = (uint16_t)(((uint16_t)payload[in + 2U] >> 2U)
                          | ((uint16_t)(payload[in + 3U] & 0x07U) << 6U));
        v[3] = (uint16_t)(((uint16_t)payload[in + 3U] >> 3U)
                          | ((uint16_t)(payload[in + 4U] & 0x0fU) << 5U));
        v[4] = (uint16_t)(((uint16_t)payload[in + 4U] >> 4U)
                          | ((uint16_t)(payload[in + 5U] & 0x1fU) << 4U));
        v[5] = (uint16_t)(((uint16_t)payload[in + 5U] >> 5U)
                          | ((uint16_t)(payload[in + 6U] & 0x3fU) << 3U));
        v[6] = (uint16_t)(((uint16_t)payload[in + 6U] >> 6U)
                          | ((uint16_t)(payload[in + 7U] & 0x7fU) << 2U));
        v[7] = (uint16_t)(((uint16_t)payload[in + 7U] >> 7U)
                          | ((uint16_t)payload[in + 8U] << 1U));

        coefficients[i] = polarkem_decompress_coefficient(v[0]);
        coefficients[i + 1U] = polarkem_decompress_coefficient(v[1]);
        coefficients[i + 2U] = polarkem_decompress_coefficient(v[2]);
        coefficients[i + 3U] = polarkem_decompress_coefficient(v[3]);
        coefficients[i + 4U] = polarkem_decompress_coefficient(v[4]);
        coefficients[i + 5U] = polarkem_decompress_coefficient(v[5]);
        coefficients[i + 6U] = polarkem_decompress_coefficient(v[6]);
        coefficients[i + 7U] = polarkem_decompress_coefficient(v[7]);
        in += 9U;
    }
#elif POLARKEM_QUANT_BITS == 8U
    for (i = 0U; i < POLARKEM_N; i += 8U) {
        coefficients[i] = polarkem_decompress_coefficient(payload[in]);
        coefficients[i + 1U] = polarkem_decompress_coefficient(payload[in + 1U]);
        coefficients[i + 2U] = polarkem_decompress_coefficient(payload[in + 2U]);
        coefficients[i + 3U] = polarkem_decompress_coefficient(payload[in + 3U]);
        coefficients[i + 4U] = polarkem_decompress_coefficient(payload[in + 4U]);
        coefficients[i + 5U] = polarkem_decompress_coefficient(payload[in + 5U]);
        coefficients[i + 6U] = polarkem_decompress_coefficient(payload[in + 6U]);
        coefficients[i + 7U] = polarkem_decompress_coefficient(payload[in + 7U]);
        in += 8U;
    }
#else
#error "Optimized PolarKEM unpacking supports only d=11, d=9, or d=8"
#endif
}

#undef POLARKEM_D_MASK
#undef POLARKEM_D_RANGE
