#include <string.h>
#include <stdlib.h>

#include "gf2x.h"

#define K_SQR_THRESHOLD 0

#define INV_MAX_ITER 16

const size_t k0[] = {0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384};
const size_t t0[] = {0, 17682, 8841, 11051, 16162, 19126, 9004, 20020, 31521, 14593, 35026, 7480, 6134, 35087, 5450, 32943};
const size_t k1[] = {0, 0, 0, 0, 0, 1, 0, 0, 0, 33, 0, 545, 0, 0, 0, 2593};
const size_t t1[] = {0, 0, 0, 0, 0, 17682, 0, 0, 0, 4502, 0, 3435, 0, 0, 0, 29305};

// Multiplies two polynomials over GF2X using the schoolbook method.
static inline void schoolbook_mul(uint64_t *c, const uint64_t *a, const uint64_t *b, size_t n64)
{
    for (size_t i = 0; i < n64; i++)
    {
        uint64_t mask = - (a[i] & 1);
        for (size_t j = 0; j < n64; j++)
        {
            c[i + j] ^= b[j] & mask;
        }
        for (size_t bit = 1; bit < 64; bit++)
        {
            mask = - ((a[i] >> bit) & 1);
            size_t low_bits = 64 - bit;
            for (size_t j = 0; j < n64; j++)
            {
                c[i + j] ^= (b[j] << bit) & mask;
                c[i + j + 1] ^= (b[j] >> low_bits) & mask;
            }
        }
    }
}

// Adds two vectors of n64 * 64 bits.
static inline void gf2x_add_len(uint64_t *c, const uint64_t *a, const uint64_t *b, size_t n64)
{
    for (size_t i = 0; i < n64; i++)
    {
        c[i] = a[i] ^ b[i];
    }
}

// Adds three vectors of n64 * 64 bits.
static inline void gf2x_add3_len(uint64_t *c, const uint64_t *a, const uint64_t *b, const uint64_t *d, size_t n64)
{
    for (size_t i = 0; i < n64; i++)
    {
        c[i] = a[i] ^ b[i] ^ d[i];
    }
}

// Multiplies two polynomials with recursive Karatsuba splitting.
void karatsuba_mul(uint64_t *c, const uint64_t *a, const uint64_t *b, size_t n64, uint64_t *buf)
{
    if (n64 <= 1)
    {
        memset(c, 0, n64 * 2 * 8);
        schoolbook_mul(c, a, b, n64);
        return;
    }

    size_t m64 = n64 / 2;
    const uint64_t *a_low = a, *a_high = a + m64;
    const uint64_t *b_low = b, *b_high = b + m64;
    uint64_t *a_sum = buf, *b_sum = buf + m64;
    uint64_t *z0 = buf + n64, *z1 = buf + n64 * 2, *z2 = buf + n64 * 3;
    uint64_t *buf_next = buf + n64 * 4;

    gf2x_add_len(a_sum, a_low, a_high, m64);
    gf2x_add_len(b_sum, b_low, b_high, m64);

    karatsuba_mul(z0, a_low, b_low, m64, buf_next);
    karatsuba_mul(z2, a_high, b_high, m64, buf_next);
    karatsuba_mul(z1, a_sum, b_sum, m64, buf_next);

    gf2x_add3_len(z1, z1, z0, z2, n64);

    fast_cpy(c, z0, n64);
    fast_cpy(c + n64, z2, n64);
    gf2x_add_len(c + m64, z1, c + m64, n64);
}

// Reduces a double-length polynomial modulo x^r - 1.
static inline void gf2x_mod(uint64_t *c, const uint64_t *a)
{
    memcpy(c, a, R_SIZE_64 << 3);
    for (size_t i = 0; i < R_SIZE_64; i++)
    {
        c[i] ^= (a[i + R_SIZE_64 - 1] >> R_64_LAST_BITS) | (a[i + R_SIZE_64] << R_64_REST_BITS);
    }
    c[R_SIZE_64 - 1] &= R_64_LAST_MASK;
}

// Adds two vectors of r bits.
void gf2x_add(uint8_t *c, const uint8_t *a, const uint8_t *b)
{
    gf2x_add_len((uint64_t *)c, (const uint64_t *)a, (const uint64_t *)b, R_SIZE_64);
}

// Multiplies two vectors and reduces the result to r bits.
void gf2x_mul(uint8_t *c, const uint8_t *a, const uint8_t *b)
{
    uint64_t *temp = (uint64_t *)aligned_alloc(64, PADDED_R_SIZE_64 * 2 * sizeof(uint64_t));
    uint64_t *buf = (uint64_t *)aligned_alloc(64, PADDED_R_SIZE_64 * 8 * sizeof(uint64_t));

    trike_setz((uint8_t *)temp, PADDED_R_SIZE_BYTES * 2);
    trike_setz((uint8_t *)buf, PADDED_R_SIZE_BYTES * 8);

    karatsuba_mul(temp, (const uint64_t *)a, (const uint64_t *)b, PADDED_R_SIZE_64, buf);
    gf2x_mod((uint64_t *)c, temp);

    free(buf);
    free(temp);
}

// Squares a vector and reduces the result.
static inline void gf2x_sqr(uint8_t *c, const uint8_t *a)
{
    gf2x_mul(c, a, a);
}

// Computes repeated squaring.
static inline void gf2x_k_sqr(uint8_t *c, const uint8_t *a, size_t k, size_t t)
{
    if (k <= K_SQR_THRESHOLD)
    {
        fast_cpy((uint64_t *)c, (const uint64_t *)a, R_SIZE_ZMM_64);
        for (size_t i = 0; i < k; i++)
        {
            gf2x_sqr(c, c);
        }
    }
    else
    {
        uint64_t idx = 0;
        size_t pos = 0;
        for (size_t i = 0; i < R_SIZE_BYTES; i++)
        {
            uint8_t byte = 0;
            for (size_t bit = 0; bit < 8 && idx < PARAM_R; bit++, idx++)
            {
                byte |= (a[pos >> 3] >> (pos & 7) & 1) << bit;
                pos += t;
                pos -= -(pos >= PARAM_R) & PARAM_R;
            }
            c[i] = byte;
        }
    }
}

// Computes the inverse of a vector of r bits.
void gf2x_inv(uint8_t *inv_a, const uint8_t *a)
{
    uint8_t *f = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
    uint8_t *g = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);

    trike_assign(a, f, R_ZMM_SIZE_BYTES, PADDED_R_SIZE_BYTES);
    trike_assign(a, inv_a, R_ZMM_SIZE_BYTES, PADDED_R_SIZE_BYTES);
    trike_setz(g, PADDED_R_SIZE_BYTES);

    for (size_t i = 1; i < INV_MAX_ITER; i++)
    {
        gf2x_k_sqr(g, f, k0[i], t0[i]);
        gf2x_mul(f, f, g);
        if(k1[i])
        {
            gf2x_k_sqr(g, f, k1[i], t1[i]);
            gf2x_mul(inv_a, inv_a, g);
        }
    }
    gf2x_sqr(inv_a, inv_a);
    
    free(f);
    free(g);
}

// Shifts a vector of r bits to the right by a given number of bits.
void gf2x_shift(uint8_t *out, const uint8_t *in, uint32_t shift)
{
    uint8_t *temp = (uint8_t *)calloc(R_SIZE_BYTES * 2, sizeof(uint8_t));
    if (temp == NULL) return;

    memset(temp, 0, R_SIZE_BYTES * 2);

    uint32_t byte_shift = shift / 8, bit_shift = shift % 8, low_bits = 8 - bit_shift;
    temp[byte_shift] = in[0] << bit_shift;
    for (uint32_t i = 0; i < R_SIZE_BYTES - 1; i++)
    {
        temp[byte_shift + i + 1] = (in[i] >> low_bits) | (in[i + 1] << bit_shift);
    }
    temp[byte_shift + R_SIZE_BYTES] = (in[R_SIZE_BYTES - 1] >> low_bits);

    memcpy(out, temp, R_SIZE_BYTES);

    uint32_t high_bits = (PARAM_R - 1) % 8 + 1; low_bits = 8 - high_bits;

    for (uint32_t i = 0; i < R_SIZE_BYTES; i++)
    {
        out[i] ^= (temp[i + R_SIZE_BYTES - 1] >> high_bits) | (temp[i + R_SIZE_BYTES] << low_bits);
    }

    out[R_SIZE_BYTES - 1] &= (1U << high_bits) - 1;

    free(temp);
}