#include <string.h>
#include <stdlib.h>

#include "gf2x.h"

#ifdef VPCLMUL_AVAILABLE
#define K_SQR_THRESHOLD 64
#else
#define K_SQR_THRESHOLD 0
#endif

#define INV_MAX_ITER 17

const size_t k0[] = {0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768};
const size_t t0[] = {0, 57022, 28511, 92660, 34302, 45573, 61256, 54750, 56288, 110361, 100050, 106261, 2691, 56772, 90761, 5145, 13049};
const size_t k1[] = {0, 0, 0, 1, 9, 25, 57, 0, 121, 0, 377, 1401, 3449, 7545, 0, 15737, 48505};
const size_t t1[] = {0, 0, 0, 57022, 17151, 85844, 51377, 0, 20155, 0, 31283, 37499, 95797, 104700, 0, 43725, 10396};

#ifdef VPCLMUL_AVAILABLE

// Computes 512 * 512 product with AVX-512 VPCLMUL instructions.
static inline void vpclmul_mul(uint64_t *c, const uint64_t *a, const uint64_t *b)
{
    const __m512i Zero = _mm512_setzero_si512();

    const __m512i Mask0 = _mm512_set_epi64(1, 0, 1, 0, 1, 0, 1, 0);
    const __m512i Mask1 = _mm512_set_epi64(3, 2, 3, 2, 3, 2, 3, 2);
    const __m512i Mask2 = _mm512_set_epi64(5, 4, 5, 4, 5, 4, 5, 4);
    const __m512i Mask3 = _mm512_set_epi64(7, 6, 7, 6, 7, 6, 7, 6);

    const __m512i Perm0 = _mm512_set_epi64(5, 4, 3, 2, 1, 0, 7, 6);
    const __m512i Perm1 = _mm512_set_epi64(3, 2, 1, 0, 7, 6, 5, 4);
    const __m512i Perm2 = _mm512_set_epi64(1, 0, 7, 6, 5, 4, 3, 2);

    __m512i A0 = _mm512_load_si512((__m512i*)a);
    __m512i B = _mm512_load_si512((__m512i*)b);
    __m512i A1 = _mm512_permutexvar_epi64(Mask1, A0);
    __m512i A2 = _mm512_permutexvar_epi64(Mask2, A0);
    __m512i A3 = _mm512_permutexvar_epi64(Mask3, A0);
    A0 = _mm512_permutexvar_epi64(Mask0, A0);

    __m512i Even0 = _mm512_clmulepi64_epi128(A0, B, 0x00);
    __m512i Even1 = _mm512_clmulepi64_epi128(A0, B, 0x11);
    __m512i Even2 = _mm512_clmulepi64_epi128(A1, B, 0x00);
    __m512i Even3 = _mm512_clmulepi64_epi128(A1, B, 0x11);
    __m512i Even4 = _mm512_clmulepi64_epi128(A2, B, 0x00);
    __m512i Even5 = _mm512_clmulepi64_epi128(A2, B, 0x11);
    __m512i Even6 = _mm512_clmulepi64_epi128(A3, B, 0x00);
    __m512i Even7 = _mm512_clmulepi64_epi128(A3, B, 0x11);
    __m512i Odd0 = _mm512_clmulepi64_epi128(A0, B, 0x10);
    __m512i Odd1 = _mm512_clmulepi64_epi128(A0, B, 0x01);
    __m512i Odd2 = _mm512_clmulepi64_epi128(A1, B, 0x10);
    __m512i Odd3 = _mm512_clmulepi64_epi128(A1, B, 0x01);
    __m512i Odd4 = _mm512_clmulepi64_epi128(A2, B, 0x10);
    __m512i Odd5 = _mm512_clmulepi64_epi128(A2, B, 0x01);
    __m512i Odd6 = _mm512_clmulepi64_epi128(A3, B, 0x10);
    __m512i Odd7 = _mm512_clmulepi64_epi128(A3, B, 0x01);

    Even1 = _mm512_permutexvar_epi64(Perm0, Even1);
    Even2 = _mm512_permutexvar_epi64(Perm0, Even2);
    Even3 = _mm512_permutexvar_epi64(Perm1, Even3);
    Even4 = _mm512_permutexvar_epi64(Perm1, Even4);
    Even5 = _mm512_permutexvar_epi64(Perm2, Even5);
    Even6 = _mm512_permutexvar_epi64(Perm2, Even6);
    Odd2 = _mm512_permutexvar_epi64(Perm0, Odd2);
    Odd3 = _mm512_permutexvar_epi64(Perm0, Odd3);
    Odd4 = _mm512_permutexvar_epi64(Perm1, Odd4);
    Odd5 = _mm512_permutexvar_epi64(Perm1, Odd5);
    Odd6 = _mm512_permutexvar_epi64(Perm2, Odd6);
    Odd7 = _mm512_permutexvar_epi64(Perm2, Odd7);

    //low bits in 0 and high bits in 7
    Even0 = _mm512_mask_xor_epi64(Even0, 0xFC, Even0, Even1);
    Even0 = _mm512_mask_xor_epi64(Even0, 0xFC, Even0, Even2);
    Even0 = _mm512_mask_xor_epi64(Even0, 0xF0, Even0, Even3);
    Even0 = _mm512_mask_xor_epi64(Even0, 0xF0, Even0, Even4);
    Even0 = _mm512_mask_xor_epi64(Even0, 0xC0, Even0, Even5);
    Even0 = _mm512_mask_xor_epi64(Even0, 0xC0, Even0, Even6);
    Even7 = _mm512_mask_xor_epi64(Even7, 0x03, Even7, Even1);
    Even7 = _mm512_mask_xor_epi64(Even7, 0x03, Even7, Even2);
    Even7 = _mm512_mask_xor_epi64(Even7, 0x0F, Even7, Even3);
    Even7 = _mm512_mask_xor_epi64(Even7, 0x0F, Even7, Even4);
    Even7 = _mm512_mask_xor_epi64(Even7, 0x3F, Even7, Even5);
    Even7 = _mm512_mask_xor_epi64(Even7, 0x3F, Even7, Even6);

    Odd0 = _mm512_xor_si512(Odd0, Odd1);
    Odd0 = _mm512_mask_xor_epi64(Odd0, 0xFC, Odd0, Odd2);
    Odd0 = _mm512_mask_xor_epi64(Odd0, 0xFC, Odd0, Odd3);
    Odd0 = _mm512_mask_xor_epi64(Odd0, 0xF0, Odd0, Odd4);
    Odd0 = _mm512_mask_xor_epi64(Odd0, 0xF0, Odd0, Odd5);
    Odd0 = _mm512_mask_xor_epi64(Odd0, 0xC0, Odd0, Odd6);
    Odd0 = _mm512_mask_xor_epi64(Odd0, 0xC0, Odd0, Odd7);
    Odd7 = _mm512_mask_xor_epi64(Zero, 0x3F, Odd7, Odd6);
    Odd7 = _mm512_mask_xor_epi64(Odd7, 0x0F, Odd7, Odd5);
    Odd7 = _mm512_mask_xor_epi64(Odd7, 0x0F, Odd7, Odd4);
    Odd7 = _mm512_mask_xor_epi64(Odd7, 0x03, Odd7, Odd3);
    Odd7 = _mm512_mask_xor_epi64(Odd7, 0x03, Odd7, Odd2);

    Odd7 = _mm512_alignr_epi64(Odd7, Odd0, 7);
    Odd0 = _mm512_alignr_epi64(Odd0, Zero, 7);

    Even0 = _mm512_xor_si512(Even0, Odd0);
    Even7 = _mm512_xor_si512(Even7, Odd7);

    _mm512_store_si512((__m512i*)c, Even0);
    _mm512_store_si512((__m512i*)(c + 8), Even7);
}

// Squares a polynomial with VPCLMUL and returns the unreduced result.
static inline void gf2x_sqr_vpclmul(uint64_t *c, const uint64_t *a)
{
    const __m512i Perm = _mm512_set_epi64(7, 3, 6, 2, 5, 1, 4, 0);
    for (size_t i = 0; i < R_SIZE_ZMMS; i++)
    {
        __m512i A = _mm512_load_si512((__m512i*)(a + i * 8));
        A = _mm512_permutexvar_epi64(Perm, A);
        __m512i Result1 = _mm512_clmulepi64_epi128(A, A, 0x00);
        __m512i Result2 = _mm512_clmulepi64_epi128(A, A, 0x11);
        _mm512_store_si512((__m512i*)(c + (i << 4)), Result1);
        _mm512_store_si512((__m512i*)(c + (i << 4 ^ 8)), Result2);
    }
}
#else

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
#endif

// Adds two vectors of n64 * 64 bits.
static inline void gf2x_add_len(uint64_t *c, const uint64_t *a, const uint64_t *b, size_t n64)
{
#ifdef AVX512_AVAILABLE
    for (size_t i = 0; i < n64; i += 8)
    {
        __m512i A = _mm512_load_si512((__m512i*)(a + i));
        __m512i B = _mm512_load_si512((__m512i*)(b + i));
        __m512i C = _mm512_xor_si512(A, B);
        _mm512_store_si512((__m512i*)(c + i), C);
    }
#else
    for (size_t i = 0; i < n64; i++)
    {
        c[i] = a[i] ^ b[i];
    }
#endif
}

// Adds three vectors of n64 * 64 bits.
static inline void gf2x_add3_len(uint64_t *c, const uint64_t *a, const uint64_t *b, const uint64_t *d, size_t n64)
{
#ifdef AVX512_AVAILABLE
    for (size_t i = 0; i < n64; i += 8)
    {
        __m512i A = _mm512_load_si512((__m512i*)(a + i));
        __m512i B = _mm512_load_si512((__m512i*)(b + i));
        __m512i D = _mm512_load_si512((__m512i*)(d + i));
        __m512i C = _mm512_ternarylogic_epi64(A, B, D, 0x96);
        _mm512_store_si512((__m512i*)(c + i), C);
    }
#else
    for (size_t i = 0; i < n64; i++)
    {
        c[i] = a[i] ^ b[i] ^ d[i];
    }
#endif
}

// Multiplies two polynomials with recursive Karatsuba splitting.
void karatsuba_mul(uint64_t *c, const uint64_t *a, const uint64_t *b, size_t n64, uint64_t *buf)
{
#ifdef VPCLMUL_AVAILABLE
    if (n64 == 8)
    {
        vpclmul_mul(c, a, b);
        return;
    }
#else
    if (n64 <= 1)
    {
        memset(c, 0, n64 * 2 * 8);
        schoolbook_mul(c, a, b, n64);
        return;
    }
#endif

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
#ifdef AVX512_AVAILABLE
    for (size_t i = 0; i < R_SIZE_64; i += 8)
    {
        __m512i High0 = _mm512_loadu_si512((__m512i*)(a + R_SIZE_64 + i - 1));
        __m512i High1 = _mm512_loadu_si512((__m512i*)(a + R_SIZE_64 + i));
        __m512i Low = _mm512_load_si512((__m512i*)(a + i));
        High0 = _mm512_srli_epi64(High0, R_64_LAST_BITS);
        High1 = _mm512_slli_epi64(High1, R_64_REST_BITS);
        Low = _mm512_ternarylogic_epi64(Low, High0, High1, 0x96);
        _mm512_store_si512((__m512i*)(c + i), Low);
    }
    c[R_SIZE_64 - 1] &= R_64_LAST_MASK;
    memset(c + R_SIZE_64, 0, (R_SIZE_ZMM_64 - R_SIZE_64) * 8);
#else
    memcpy(c, a, R_SIZE_64 << 3);
    for (size_t i = 0; i < R_SIZE_64; i++)
    {
        c[i] ^= (a[i + R_SIZE_64 - 1] >> R_64_LAST_BITS) | (a[i + R_SIZE_64] << R_64_REST_BITS);
    }
    c[R_SIZE_64 - 1] &= R_64_LAST_MASK;
#endif
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
#ifdef VPCLMUL_AVAILABLE
    uint64_t *temp = (uint64_t *)aligned_alloc(64, PADDED_R_SIZE_64 * 2 * sizeof(uint64_t));
    trike_setz((uint8_t *)temp, PADDED_R_SIZE_64 * 2 * sizeof(uint64_t));
    gf2x_sqr_vpclmul(temp, (const uint64_t *)a);
    gf2x_mod((uint64_t *)c, temp);
    free(temp);
#else
    gf2x_mul(c, a, a);
#endif
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
#ifdef AVX512_AVAILABLE

    uint64_t *out_64 = (uint64_t *)out;
    const uint64_t *in_64 = (const uint64_t *)in;
    uint32_t shift_64 = shift >> 6;
    uint32_t shift_bits = shift & 63;
    for (uint32_t i = 0; i < R_SIZE_64; i += 8)
    {
        __m512i va = _mm512_loadu_si512((__m512i *)(in_64 + shift_64 + i));
        __m512i vb = _mm512_loadu_si512((__m512i *)(in_64 + shift_64 + i + 1));
        va = _mm512_srli_epi64(va, shift_bits);
        vb = _mm512_slli_epi64(vb, 64 - shift_bits);
        __m512i vc = _mm512_or_si512(va, vb);
        _mm512_store_si512((__m512i *)(out_64 + i), vc);
    }
    out[R_SIZE_BYTES - 1] &= R_8_LAST_MASK;
    memset(out + R_SIZE_BYTES, 0, R_ZMM_SIZE_BYTES - R_SIZE_BYTES);

#else

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
    
#endif
}