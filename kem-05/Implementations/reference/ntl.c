/******************************************************************************
 * BIKE_MLThre
 ******************************************************************************/

#include <string.h>

#include "ntl.h"

#define WORD_BITS 64U
#define POLY_WORDS DIVIDE_AND_CEIL((R_BITS + 1ULL), WORD_BITS)

static void mask_unused_bits(OUT uint8_t a[R_SIZE])
{
    const uint32_t bits_in_last_byte = R_BITS % 8ULL;

    if (bits_in_last_byte != 0)
    {
        a[R_SIZE - 1] &= (uint8_t)((1U << bits_in_last_byte) - 1U);
    }
}

static uint8_t get_bit(IN const uint8_t *in, IN const uint32_t bit_idx)
{
    return (uint8_t)((in[bit_idx / 8U] >> (bit_idx % 8U)) & 1U);
}

static void set_bit(OUT uint8_t *out, IN const uint32_t bit_idx, IN const uint8_t bit)
{
    if (bit)
    {
        out[bit_idx / 8U] |= (uint8_t)(1U << (bit_idx % 8U));
    }
}

static void toggle_bit(OUT uint8_t *out, IN const uint32_t bit_idx)
{
    out[bit_idx / 8U] ^= (uint8_t)(1U << (bit_idx % 8U));
}

static uint32_t count_ones(IN const uint8_t in[R_SIZE])
{
    uint32_t count = 0;

    for (uint32_t i = 0; i < R_SIZE; i++)
    {
        count += (uint32_t)__builtin_popcount((unsigned int)in[i]);
    }

    return count;
}

static void xor_cyclic_shift(OUT uint8_t res[R_SIZE],
        IN const uint8_t poly[R_SIZE],
        IN const uint32_t shift)
{
    for (uint32_t i = 0; i < R_BITS; i++)
    {
        if (get_bit(poly, i))
        {
            toggle_bit(res, (i + shift) % R_BITS);
        }
    }
}

static void poly_mask(OUT uint64_t a[POLY_WORDS])
{
    const uint32_t used_bits = (R_BITS + 1ULL) % WORD_BITS;

    if (used_bits != 0)
    {
        a[POLY_WORDS - 1] &= (1ULL << used_bits) - 1ULL;
    }
}

static uint8_t poly_get_bit(IN const uint64_t a[POLY_WORDS], IN const uint32_t bit_idx)
{
    return (uint8_t)((a[bit_idx / WORD_BITS] >> (bit_idx % WORD_BITS)) & 1ULL);
}

static void poly_set_bit(OUT uint64_t a[POLY_WORDS], IN const uint32_t bit_idx)
{
    a[bit_idx / WORD_BITS] |= 1ULL << (bit_idx % WORD_BITS);
}

static void poly_from_bytes(OUT uint64_t out[POLY_WORDS], IN const uint8_t in[R_SIZE])
{
    memset(out, 0, sizeof(uint64_t) * POLY_WORDS);

    for (uint32_t i = 0; i < R_BITS; i++)
    {
        if (get_bit(in, i))
        {
            poly_set_bit(out, i);
        }
    }
}

static int32_t poly_degree(IN const uint64_t a[POLY_WORDS])
{
    for (int32_t i = (int32_t)POLY_WORDS - 1; i >= 0; i--)
    {
        uint64_t word = a[i];

        if (i == (int32_t)POLY_WORDS - 1)
        {
            const uint32_t used_bits = (R_BITS + 1ULL) % WORD_BITS;

            if (used_bits != 0)
            {
                word &= (1ULL << used_bits) - 1ULL;
            }
        }

        if (word != 0)
        {
            return (int32_t)(i * WORD_BITS + (WORD_BITS - 1U - __builtin_clzll(word)));
        }
    }

    return -1;
}

static int poly_is_one(IN const uint64_t a[POLY_WORDS])
{
    if ((a[0] & 1ULL) == 0)
    {
        return 0;
    }

    if ((a[0] & ~1ULL) != 0)
    {
        return 0;
    }

    for (uint32_t i = 1; i < POLY_WORDS; i++)
    {
        if (a[i] != 0)
        {
            return 0;
        }
    }

    return 1;
}

static void poly_xor_shift(OUT uint64_t dest[POLY_WORDS],
        IN const uint64_t src[POLY_WORDS],
        IN const uint32_t shift)
{
    const uint32_t word_shift = shift / WORD_BITS;
    const uint32_t bit_shift = shift % WORD_BITS;

    for (int32_t i = (int32_t)POLY_WORDS - 1; i >= 0; i--)
    {
        const uint64_t word = src[i];

        if (word == 0)
        {
            continue;
        }

        if ((uint32_t)i + word_shift < POLY_WORDS)
        {
            dest[i + word_shift] ^= word << bit_shift;
        }

        if ((bit_shift != 0) && ((uint32_t)i + word_shift + 1U < POLY_WORDS))
        {
            dest[i + word_shift + 1U] ^= word >> (WORD_BITS - bit_shift);
        }
    }

    poly_mask(dest);
}

static void swap_poly(IN OUT uint64_t a[POLY_WORDS], IN OUT uint64_t b[POLY_WORDS])
{
    uint64_t tmp[POLY_WORDS];

    memcpy(tmp, a, sizeof(tmp));
    memcpy(a, b, sizeof(tmp));
    memcpy(b, tmp, sizeof(tmp));
}

static void swap_bytes(IN OUT uint8_t a[R_SIZE], IN OUT uint8_t b[R_SIZE])
{
    uint8_t tmp[R_SIZE];

    memcpy(tmp, a, sizeof(tmp));
    memcpy(a, b, sizeof(tmp));
    memcpy(b, tmp, sizeof(tmp));
}

void ntl_add(OUT uint8_t res_bin[R_SIZE],
        IN const uint8_t a_bin[R_SIZE],
        IN const uint8_t b_bin[R_SIZE])
{
    for (uint32_t i = 0; i < R_SIZE; i++)
    {
        res_bin[i] = a_bin[i] ^ b_bin[i];
    }

    mask_unused_bits(res_bin);
}

void ntl_mod_inv(OUT uint8_t res_bin[R_SIZE],
        IN const uint8_t a_bin[R_SIZE])
{
    uint64_t u[POLY_WORDS];
    uint64_t v[POLY_WORDS];
    uint8_t g1[R_SIZE] = {0};
    uint8_t g2[R_SIZE] = {0};

    memset(res_bin, 0, R_SIZE);
    poly_from_bytes(u, a_bin);
    memset(v, 0, sizeof(v));
    poly_set_bit(v, 0);
    poly_set_bit(v, R_BITS);
    g1[0] = 1;

    while (!poly_is_one(u))
    {
        int32_t deg_u = poly_degree(u);
        int32_t deg_v = poly_degree(v);

        if (deg_u < 0)
        {
            memset(res_bin, 0, R_SIZE);
            return;
        }

        if (deg_u < deg_v)
        {
            swap_poly(u, v);
            swap_bytes(g1, g2);
            deg_u = poly_degree(u);
            deg_v = poly_degree(v);
        }

        const uint32_t shift = (uint32_t)(deg_u - deg_v);
        poly_xor_shift(u, v, shift);
        xor_cyclic_shift(g1, g2, shift);
    }

    memcpy(res_bin, g1, R_SIZE);
    mask_unused_bits(res_bin);
}

void ntl_mod_mul(OUT uint8_t res_bin[R_SIZE],
        IN const uint8_t a_bin[R_SIZE],
        IN const uint8_t b_bin[R_SIZE])
{
    const uint8_t *sparse = a_bin;
    const uint8_t *poly = b_bin;

    memset(res_bin, 0, R_SIZE);

    if (count_ones(b_bin) < count_ones(a_bin))
    {
        sparse = b_bin;
        poly = a_bin;
    }

    for (uint32_t i = 0; i < R_BITS; i++)
    {
        if (get_bit(sparse, i))
        {
            xor_cyclic_shift(res_bin, poly, i);
        }
    }

    mask_unused_bits(res_bin);
}

void ntl_split_polynomial(OUT uint8_t e0[R_SIZE],
        OUT uint8_t e1[R_SIZE],
        IN const uint8_t e[N_SIZE])
{
    memset(e0, 0, R_SIZE);
    memset(e1, 0, R_SIZE);

    for (uint32_t i = 0; i < R_BITS; i++)
    {
        set_bit(e0, i, get_bit(e, i));
        set_bit(e1, i, get_bit(e, R_BITS + i));
    }

    mask_unused_bits(e0);
    mask_unused_bits(e1);
}
