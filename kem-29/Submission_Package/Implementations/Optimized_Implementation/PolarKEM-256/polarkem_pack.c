/**
 * @file polarkem_pack.c
 * @brief Portable nine-bit coefficient packing for PolarKEM-256.
 */
#include "polarkem_pack.h"

#include <stddef.h>

#define POLARKEM_D_RANGE (1U << POLARKEM_QUANT_BITS)
#define POLARKEM_D_MASK (POLARKEM_D_RANGE - 1U)

typedef char polarkem_assert_specialized_d9[
    (POLARKEM_QUANT_BITS == 9U) ? 1 : -1];

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
}

void polarkem_unpack_coefficients(
    uint16_t coefficients[POLARKEM_N],
    const unsigned char payload[POLARKEM_CT_PAYLOAD_BYTES])
{
    size_t i;
    size_t in = 0U;

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
}

#undef POLARKEM_D_MASK
#undef POLARKEM_D_RANGE
