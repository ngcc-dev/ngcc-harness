/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-512 instance.
*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "params.h"
#include "auxfunc.h"
#include "sample.h"
#include "symmetric.h"

#include <arm_neon.h>
#define ZEN_CRYPTO_TARGET __attribute__((target("+crypto")))

static const uint64_t pack_table[] = 
{
    1, 769, 591361, 454756609, 349707832321
};

static inline uint64_t load64_le(const uint8_t *src)
{
    uint64_t x;

    memcpy(&x, src, sizeof(x));
    return x;
}

static inline uint16_t load16_le(const uint8_t *src)
{
    uint16_t x;

    memcpy(&x, src, sizeof(x));
    return x;
}

static inline void store64_le(uint8_t *dst, uint64_t x)
{
    memcpy(dst, &x, sizeof(x));
}

static inline void store16_le(uint8_t *dst, uint16_t x)
{
    memcpy(dst, &x, sizeof(x));
}

static inline uint64_t pack_base769_5(const int16_t *a)
{
    return (uint64_t)(uint16_t)a[0]
         + (uint64_t)(uint16_t)a[1] * pack_table[1]
         + (uint64_t)(uint16_t)a[2] * pack_table[2]
         + (uint64_t)(uint16_t)a[3] * pack_table[3]
         + (uint64_t)(uint16_t)a[4] * pack_table[4];
}

static inline void unpack_base769_5(int16_t *a, uint64_t x)
{
    const uint64_t DIV769_M = 374811932576999ULL;
    unsigned int i;

#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
    for(i = 0; i < 5; i++)
    {
        uint64_t q = (uint64_t)(((__uint128_t)x * DIV769_M) >> 58);
        uint64_t r = x - q * 769ULL;

        a[i] = (int16_t)r;
        x = q;
    }
}

static inline void unpack_base769_3(int16_t *a, uint64_t x)
{
    const uint64_t DIV769_M = 374811932576999ULL;
    unsigned int i;

#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
    for(i = 0; i < 3; i++)
    {
        uint64_t q = (uint64_t)(((__uint128_t)x * DIV769_M) >> 58);
        uint64_t r = x - q * 769ULL;

        a[i] = (int16_t)r;
        x = q;
    }
}

static const int16_t neon_shifts_0_7[8] = {0, 1, 2, 3, 4, 5, 6, 7};
static const int16_t neon_shifts_neg_0_7[8] = {0, -1, -2, -3, -4, -5, -6, -7};

static inline void pack_binary_128_neon(const int16_t *a, uint64_t out[2])
{
    unsigned int i;
    const int16x8_t pos_shifts = vld1q_s16(neon_shifts_0_7);
    const int16x8_t ones = vdupq_n_s16(1);

    out[0] = 0;
    out[1] = 0;

    for(i = 0; i < 128; i += 8)
    {
        int16x8_t v = vandq_s16(vld1q_s16(&a[i]), ones);
        int16x8_t r;

        v = vshlq_s16(v, pos_shifts);
        r = vpaddq_s16(v, v);
        r = vpaddq_s16(r, r);
        r = vpaddq_s16(r, r);
        out[i >> 6] |= (uint64_t)(uint8_t)vgetq_lane_s16(r, 0) << (i & 56);
    }
}

static inline void pack_binary_256_neon(const int16_t *a, uint64_t out[4])
{
    pack_binary_128_neon(a, out);
    pack_binary_128_neon(a + 128, out + 2);
}

static inline void pack_binary_512_neon(const int16_t *a, uint64_t out[8])
{
    pack_binary_256_neon(a, out);
    pack_binary_256_neon(a + 256, out + 4);
}

static inline void pack_binary_1024_neon(const int16_t *a, uint64_t out[16])
{
    pack_binary_512_neon(a, out);
    pack_binary_512_neon(a + 512, out + 8);
}

static inline void unpack_binary_words_neon(int16_t *res, const uint64_t *words,
                                            unsigned int nwords)
{
    unsigned int w, shift;
    const int16x8_t neg_shifts = vld1q_s16(neon_shifts_neg_0_7);
    const int16x8_t ones = vdupq_n_s16(1);

    for(w = 0; w < nwords; w++)
    {
        for(shift = 0; shift < 64; shift += 8)
        {
            int16x8_t bcast = vdupq_n_s16((int16_t)(uint8_t)(words[w] >> shift));
            vst1q_s16(&res[(w << 6) + shift],
                      vandq_s16(vshlq_s16(bcast, neg_shifts), ones));
        }
    }
}

static inline int16x8_t expand_byte_bits_neon(uint8_t byte,
                                              int16x8_t neg_shifts,
                                              int16x8_t ones)
{
    int16x8_t bcast = vdupq_n_s16((int16_t)byte);

    return vandq_s16(vshlq_s16(bcast, neg_shifts), ones);
}

static inline uint8_t pack_coeff_bits8_neon(const int16_t *a,
                                            int16x8_t bit_shift,
                                            int16x8_t pos_shifts,
                                            int16x8_t ones)
{
    int16x8_t v = vandq_s16(vshlq_s16(vld1q_s16(a), bit_shift), ones);

    v = vshlq_s16(v, pos_shifts);
    v = vpaddq_s16(v, v);
    v = vpaddq_s16(v, v);
    v = vpaddq_s16(v, v);
    return (uint8_t)vgetq_lane_s16(v, 0);
}

ZEN_CRYPTO_TARGET
static inline void clmul64_words(uint64_t a, uint64_t b,
                                 uint64_t *lo, uint64_t *hi)
{
    const __uint128_t p = (__uint128_t)vmull_p64((poly64_t)a, (poly64_t)b);

    *lo = (uint64_t)p;
    *hi = (uint64_t)(p >> 64);
}

ZEN_CRYPTO_TARGET
static inline void clmul2_karatsuba(uint64_t a0, uint64_t a1,
                                    uint64_t b0, uint64_t b1,
                                    uint64_t out[4])
{
    uint64_t z0_lo, z0_hi;
    uint64_t z1_lo, z1_hi;
    uint64_t z2_lo, z2_hi;

    clmul64_words(a0, b0, &z0_lo, &z0_hi);
    clmul64_words(a1, b1, &z2_lo, &z2_hi);
    clmul64_words(a0 ^ a1, b0 ^ b1, &z1_lo, &z1_hi);

    z1_lo ^= z0_lo ^ z2_lo;
    z1_hi ^= z0_hi ^ z2_hi;

    out[0] = z0_lo;
    out[1] = z0_hi ^ z1_lo;
    out[2] = z1_hi ^ z2_lo;
    out[3] = z2_hi;
}

ZEN_CRYPTO_TARGET
static inline void clmul4_karatsuba(const uint64_t a[4], const uint64_t b[4],
                                    uint64_t out[8])
{
    uint64_t low[4];
    uint64_t high[4];
    uint64_t cross[4];
    uint64_t mid[4];
    unsigned int i;

    clmul2_karatsuba(a[0], a[1], b[0], b[1], low);
    clmul2_karatsuba(a[2], a[3], b[2], b[3], high);
    clmul2_karatsuba(a[0] ^ a[2], a[1] ^ a[3],
                     b[0] ^ b[2], b[1] ^ b[3], cross);

    for(i = 0; i < 4; i++)
    {
        mid[i] = cross[i] ^ low[i] ^ high[i];
    }

    out[0] = low[0];
    out[1] = low[1];
    out[2] = low[2] ^ mid[0];
    out[3] = low[3] ^ mid[1];
    out[4] = high[0] ^ mid[2];
    out[5] = high[1] ^ mid[3];
    out[6] = high[2];
    out[7] = high[3];
}

ZEN_CRYPTO_TARGET
static inline void clmul8_karatsuba(const uint64_t a[8], const uint64_t b[8],
                                    uint64_t out[16])
{
    uint64_t low[8];
    uint64_t high[8];
    uint64_t cross[8];
    uint64_t mid[8];
    uint64_t ax[4];
    uint64_t bx[4];
    unsigned int i;

    clmul4_karatsuba(a, b, low);
    clmul4_karatsuba(a + 4, b + 4, high);

    for(i = 0; i < 4; i++)
    {
        ax[i] = a[i] ^ a[i + 4];
        bx[i] = b[i] ^ b[i + 4];
    }
    clmul4_karatsuba(ax, bx, cross);

    for(i = 0; i < 8; i++)
    {
        mid[i] = cross[i] ^ low[i] ^ high[i];
    }

    out[0] = low[0];
    out[1] = low[1];
    out[2] = low[2];
    out[3] = low[3];
    out[4] = low[4] ^ mid[0];
    out[5] = low[5] ^ mid[1];
    out[6] = low[6] ^ mid[2];
    out[7] = low[7] ^ mid[3];
    out[8] = high[0] ^ mid[4];
    out[9] = high[1] ^ mid[5];
    out[10] = high[2] ^ mid[6];
    out[11] = high[3] ^ mid[7];
    out[12] = high[4];
    out[13] = high[5];
    out[14] = high[6];
    out[15] = high[7];
}

ZEN_CRYPTO_TARGET
static inline void clmul16_karatsuba(const uint64_t a[16], const uint64_t b[16],
                                     uint64_t out[32])
{
    uint64_t low[16];
    uint64_t high[16];
    uint64_t cross[16];
    uint64_t mid[16];
    uint64_t ax[8];
    uint64_t bx[8];
    unsigned int i;

    clmul8_karatsuba(a, b, low);
    clmul8_karatsuba(a + 8, b + 8, high);

    for(i = 0; i < 8; i++)
    {
        ax[i] = a[i] ^ a[i + 8];
        bx[i] = b[i] ^ b[i + 8];
    }
    clmul8_karatsuba(ax, bx, cross);

    for(i = 0; i < 16; i++)
    {
        mid[i] = cross[i] ^ low[i] ^ high[i];
    }

    for(i = 0; i < 8; i++)
    {
        out[i] = low[i];
        out[i + 8] = low[i + 8] ^ mid[i];
        out[i + 16] = high[i] ^ mid[i + 8];
        out[i + 24] = high[i + 8];
    }
}

ZEN_CRYPTO_TARGET
static void mul_in_R2_512_words_pmull(const int16_t *a,
                                      const uint64_t b_words[8],
                                      int16_t *res)
{
    uint64_t aw[8];
    uint64_t product[16];
    uint64_t folded[8];
    unsigned int i;

    pack_binary_512_neon(a, aw);
    clmul8_karatsuba(aw, b_words, product);

    for(i = 0; i < 8; i++)
    {
        folded[i] = product[i] ^ product[i + 8];
    }
    unpack_binary_words_neon(res, folded, 8);
}

ZEN_CRYPTO_TARGET
static void mul_in_R2_pmull_256(const int16_t *a, const int16_t *b, int16_t *res)
{
    uint64_t aw[4];
    uint64_t bw[4];
    uint64_t product[8];
    uint64_t folded[4];
    unsigned int i;

    pack_binary_256_neon(a, aw);
    pack_binary_256_neon(b, bw);
    clmul4_karatsuba(aw, bw, product);

    for(i = 0; i < 4; i++)
    {
        folded[i] = product[i] ^ product[i + 4];
    }
    unpack_binary_words_neon(res, folded, 4);
}

ZEN_CRYPTO_TARGET
static void mul_in_R2_pmull_512(const int16_t *a, const int16_t *b, int16_t *res)
{
    uint64_t bw[8];

    pack_binary_512_neon(b, bw);
    mul_in_R2_512_words_pmull(a, bw, res);
}

ZEN_CRYPTO_TARGET
static void mul_in_R2_pmull_1024(const int16_t *a, const int16_t *b,
                                 int16_t *res)
{
    uint64_t aw[16];
    uint64_t bw[16];
    uint64_t product[32];
    uint64_t folded[16];
    unsigned int i;

    pack_binary_1024_neon(a, aw);
    pack_binary_1024_neon(b, bw);
    clmul16_karatsuba(aw, bw, product);

    for(i = 0; i < 16; i++)
    {
        folded[i] = product[i] ^ product[i + 16];
    }
    unpack_binary_words_neon(res, folded, 16);
}

ZEN_CRYPTO_TARGET
void mul_in_R2_1024_packed512(const int16_t *a, const uint8_t *b_packed,
                              int16_t *res)
{
    uint64_t aw[16];
    uint64_t bw[8];
    uint64_t lo[16];
    uint64_t hi[16];
    uint64_t folded[16];
    unsigned int i;

    pack_binary_1024_neon(a, aw);
    for(i = 0; i < 8; i++)
    {
        bw[i] = load64_le(b_packed + 8 * i);
    }

    clmul8_karatsuba(aw, bw, lo);
    clmul8_karatsuba(aw + 8, bw, hi);

    for(i = 0; i < 8; i++)
    {
        folded[i] = lo[i] ^ hi[i + 8];
        folded[i + 8] = lo[i + 8] ^ hi[i];
    }
    unpack_binary_words_neon(res, folded, 16);
}

int check_poly_inv_Zq(int16_t *a)
{
	unsigned int i;
    uint32_t acc = 0;

    for(i = 0; i < ZEN_N; i += 16)
	{
        int32_t s0 = vaddlvq_s16(vld1q_s16(&a[i]));
        int32_t s1 = vaddlvq_s16(vld1q_s16(&a[i + 8]));

		acc |= (uint32_t)((s0 + s1) == 0);
	}
	return (int)(acc & 1u);
}

int check_poly_inv_Z2(int16_t *a)
{
	unsigned int i;
    int16x8_t acc = vdupq_n_s16(0);
    uint16x4_t folded;
    uint16_t x;

    for(i = 0; i < ZEN_N4; i += 16)
    {
        acc = veorq_s16(acc, vld1q_s16(&a[i]));
        acc = veorq_s16(acc, vld1q_s16(&a[i + 8]));
    }

    folded = veor_u16(vget_low_u16(vreinterpretq_u16_s16(acc)),
                      vget_high_u16(vreinterpretq_u16_s16(acc)));
    x = vget_lane_u16(folded, 0) ^ vget_lane_u16(folded, 1) ^
        vget_lane_u16(folded, 2) ^ vget_lane_u16(folded, 3);
	return (int)(x == 0);
}

#define DEFINE_MUL_IN_R2_SMALL(N, MASKN)                              \
static void mul_in_R2_##N(int16_t *a, int16_t *b, int16_t *res)        \
{                                                                      \
    unsigned int i;                                                    \
    uint64_t B;                                                        \
    uint64_t R;                                                        \
    uint64_t mask;                                                     \
    uint64_t wrap;                                                     \
                                                                       \
    B = 0;                                                             \
    R = 0;                                                             \
                                                                       \
    for (i = 0; i < (N); i++) {                                        \
        B |= ((uint64_t)((uint16_t)b[i] & 1u)) << i;                   \
    }                                                                  \
                                                                       \
    B &= (MASKN);                                                      \
                                                                       \
    for (i = 0; i < (N); i++) {                                        \
        mask = 0ULL - ((uint64_t)((uint16_t)a[i] & 1u));               \
        R ^= B & mask;                                                 \
                                                                       \
        wrap = (B >> ((N) - 1)) & 1ULL;                                \
        B = ((B << 1) | wrap) & (MASKN);                               \
    }                                                                  \
                                                                       \
    for (i = 0; i < (N); i++) {                                        \
        res[i] = (int16_t)((R >> i) & 1ULL);                           \
    }                                                                  \
}
DEFINE_MUL_IN_R2_SMALL(2,  0x0000000000000003ULL)
DEFINE_MUL_IN_R2_SMALL(4,  0x000000000000000FULL)
DEFINE_MUL_IN_R2_SMALL(8,  0x00000000000000FFULL)
DEFINE_MUL_IN_R2_SMALL(16, 0x000000000000FFFFULL)
DEFINE_MUL_IN_R2_SMALL(32, 0x00000000FFFFFFFFULL)
DEFINE_MUL_IN_R2_SMALL(64, UINT64_MAX)

#define MUL_R2_512_STEP(AOFF)                                             \
    do {                                                                  \
        for (i = 0; i < 64; i++) {                                        \
            mask = 0ULL - ((uint64_t)((uint16_t)a[(AOFF) + i] & 1u));     \
                                                                          \
            r0 ^= b0 & mask;                                              \
            r1 ^= b1 & mask;                                              \
            r2 ^= b2 & mask;                                              \
            r3 ^= b3 & mask;                                              \
            r4 ^= b4 & mask;                                              \
            r5 ^= b5 & mask;                                              \
            r6 ^= b6 & mask;                                              \
            r7 ^= b7 & mask;                                              \
                                                                          \
            wrap = b7 >> 63;                                              \
            u0 = (b0 << 1) | wrap;                                        \
            u1 = (b1 << 1) | (b0 >> 63);                                  \
            u2 = (b2 << 1) | (b1 >> 63);                                  \
            u3 = (b3 << 1) | (b2 >> 63);                                  \
            u4 = (b4 << 1) | (b3 >> 63);                                  \
            u5 = (b5 << 1) | (b4 >> 63);                                  \
            u6 = (b6 << 1) | (b5 >> 63);                                  \
            u7 = (b7 << 1) | (b6 >> 63);                                  \
                                                                          \
            b0 = u0;                                                      \
            b1 = u1;                                                      \
            b2 = u2;                                                      \
            b3 = u3;                                                      \
            b4 = u4;                                                      \
            b5 = u5;                                                      \
            b6 = u6;                                                      \
            b7 = u7;                                                      \
        }                                                                 \
    } while (0)

#define MUL_R2_1024_STEP(AOFF)                                             \
    do {                                                                   \
        for (i = 0; i < 64; i++) {                                         \
            mask = 0ULL - ((uint64_t)((uint16_t)a[(AOFF) + i] & 1u));      \
                                                                           \
            r0  ^= b0  & mask;                                             \
            r1  ^= b1  & mask;                                             \
            r2  ^= b2  & mask;                                             \
            r3  ^= b3  & mask;                                             \
            r4  ^= b4  & mask;                                             \
            r5  ^= b5  & mask;                                             \
            r6  ^= b6  & mask;                                             \
            r7  ^= b7  & mask;                                             \
            r8  ^= b8  & mask;                                             \
            r9  ^= b9  & mask;                                             \
            r10 ^= b10 & mask;                                             \
            r11 ^= b11 & mask;                                             \
            r12 ^= b12 & mask;                                             \
            r13 ^= b13 & mask;                                             \
            r14 ^= b14 & mask;                                             \
            r15 ^= b15 & mask;                                             \
                                                                           \
            wrap = b15 >> 63;                                              \
            u0  = (b0  << 1) | wrap;                                       \
            u1  = (b1  << 1) | (b0  >> 63);                                \
            u2  = (b2  << 1) | (b1  >> 63);                                \
            u3  = (b3  << 1) | (b2  >> 63);                                \
            u4  = (b4  << 1) | (b3  >> 63);                                \
            u5  = (b5  << 1) | (b4  >> 63);                                \
            u6  = (b6  << 1) | (b5  >> 63);                                \
            u7  = (b7  << 1) | (b6  >> 63);                                \
            u8  = (b8  << 1) | (b7  >> 63);                                \
            u9  = (b9  << 1) | (b8  >> 63);                                \
            u10 = (b10 << 1) | (b9  >> 63);                                \
            u11 = (b11 << 1) | (b10 >> 63);                                \
            u12 = (b12 << 1) | (b11 >> 63);                                \
            u13 = (b13 << 1) | (b12 >> 63);                                \
            u14 = (b14 << 1) | (b13 >> 63);                                \
            u15 = (b15 << 1) | (b14 >> 63);                                \
                                                                           \
            b0  = u0;                                                      \
            b1  = u1;                                                      \
            b2  = u2;                                                      \
            b3  = u3;                                                      \
            b4  = u4;                                                      \
            b5  = u5;                                                      \
            b6  = u6;                                                      \
            b7  = u7;                                                      \
            b8  = u8;                                                      \
            b9  = u9;                                                      \
            b10 = u10;                                                     \
            b11 = u11;                                                     \
            b12 = u12;                                                     \
            b13 = u13;                                                     \
            b14 = u14;                                                     \
            b15 = u15;                                                     \
        }                                                                  \
    } while (0)

static inline void mul_in_R2_128(int16_t *a, int16_t *b, int16_t *res)
{
    unsigned int i;
    uint64_t b0, b1;
    uint64_t r0, r1;
    uint64_t u0, u1;
    uint64_t mask;
    uint64_t wrap;

    b0 = b1 = 0;
    r0 = r1 = 0;

    for (i = 0; i < 64; i++) {
        b0 |= ((uint64_t)((uint16_t)b[i] & 1u)) << i;
        b1 |= ((uint64_t)((uint16_t)b[i + 64] & 1u)) << i;
    }

    for (i = 0; i < 64; i++) {
        mask = 0ULL - ((uint64_t)((uint16_t)a[i] & 1u));

        r0 ^= b0 & mask;
        r1 ^= b1 & mask;

        wrap = b1 >> 63;
        u0 = (b0 << 1) | wrap;
        u1 = (b1 << 1) | (b0 >> 63);

        b0 = u0;
        b1 = u1;
    }

    for (i = 0; i < 64; i++) {
        mask = 0ULL - ((uint64_t)((uint16_t)a[i + 64] & 1u));

        r0 ^= b0 & mask;
        r1 ^= b1 & mask;

        wrap = b1 >> 63;
        u0 = (b0 << 1) | wrap;
        u1 = (b1 << 1) | (b0 >> 63);

        b0 = u0;
        b1 = u1;
    }

    for (i = 0; i < 64; i++) {
        res[i] = (int16_t)((r0 >> i) & 1ULL);
        res[i + 64] = (int16_t)((r1 >> i) & 1ULL);
    }
}

static void mul_in_R2_256(int16_t *a, int16_t *b, int16_t *res)
{
    mul_in_R2_pmull_256(a, b, res);
}

static void mul_in_R2_512(int16_t *a, int16_t *b, int16_t *res)
{
    mul_in_R2_pmull_512(a, b, res);
}

void mul_in_R2_1024(int16_t *a, int16_t *b, int16_t *res)
{
    mul_in_R2_pmull_1024(a, b, res);
}

/* Exported FastInversion is the packed-bit assembly implementation in fastinversion.S. */
static void FastInversion_c_reference(int16_t *f_inv, int16_t *f)
{
    unsigned int i, j;
    int16_t k[ZEN_N4];
    int16_t b[ZEN_N4] = {0};
    int16_t tmp[ZEN_N4];
    uint64_t f_words[8];
#define MUL_F_IN_R2_512(B, OUT) mul_in_R2_512_words_pmull((B), f_words, (OUT))

    pack_binary_512_neon(f, f_words);

    k[0] = f[0];
    for (i = 1; i < ZEN_N4; i++) 
    {
        k[i] = f[i] ^ k[i - 1];
    }

    memset(f_inv, 0, ZEN_N4 * sizeof(int16_t));
    f_inv[0] = 1;

    b[0] = 0;
    for (i = 0; i < ZEN_N4; i++) 
    {
        b[0] ^= k[i];
    }

    for (i = 0; i < ZEN_N4; i++) 
    {
        k[i] ^= (b[0] * f[i]);
    }

    for (i = 1; i < ZEN_N4; i++) 
    {
        k[i] = k[i] ^ k[i - 1];
    }

    f_inv[0] = !b[0];
    f_inv[1] = b[0];

    /*
     * Level n = 2
     */
    memset(tmp, 0, 2 * sizeof(int16_t));
    for (i = 0; i < 2; i++) 
    {
        for (j = i; j < ZEN_N4; j += 2) 
        {
            tmp[i] ^= k[j];
        }
    }

    mul_in_R2_2(f_inv, tmp, b);
    MUL_F_IN_R2_512(b, tmp);

    for (j = 0; j < ZEN_N4; j++) 
    {
        k[j] = k[j] ^ tmp[j];
    }

    for (i = 2; i < ZEN_N4; i += 2) 
    {
        for (j = i; j < i + 2; j++) 
        {
            k[j] = k[j] ^ k[j - 2];
        }
    }

    for (i = 0; i < 2; i++) 
    {
        tmp[i] = tmp[i + 2] = b[i];
    }

    for (i = 0; i < 4; i++) 
    {
        f_inv[i] ^= tmp[i];
    }

    /*
     * Level n = 4
     */
    memset(tmp, 0, 4 * sizeof(int16_t));
    for (i = 0; i < 4; i++) 
    {
        for (j = i; j < ZEN_N4; j += 4) 
        {
            tmp[i] ^= k[j];
        }
    }

    mul_in_R2_4(f_inv, tmp, b);
    MUL_F_IN_R2_512(b, tmp);

    for (j = 0; j < ZEN_N4; j++) 
    {
        k[j] = k[j] ^ tmp[j];
    }

    for (i = 4; i < ZEN_N4; i += 4) 
    {
        for (j = i; j < i + 4; j++) 
        {
            k[j] = k[j] ^ k[j - 4];
        }
    }

    for (i = 0; i < 4; i++) 
    {
        tmp[i] = tmp[i + 4] = b[i];
    }

    for (i = 0; i < 8; i++) 
    {
        f_inv[i] ^= tmp[i];
    }

    /*
     * Level n = 8
     */
    memset(tmp, 0, 8 * sizeof(int16_t));
    for (i = 0; i < 8; i++) 
    {
        for (j = i; j < ZEN_N4; j += 8) 
        {
            tmp[i] ^= k[j];
        }
    }

    mul_in_R2_8(f_inv, tmp, b);
    MUL_F_IN_R2_512(b, tmp);

    for (j = 0; j < ZEN_N4; j++) 
    {
        k[j] = k[j] ^ tmp[j];
    }

    for (i = 8; i < ZEN_N4; i += 8) 
    {
        for (j = i; j < i + 8; j++) 
        {
            k[j] = k[j] ^ k[j - 8];
        }
    }

    for (i = 0; i < 8; i++) 
    {
        tmp[i] = tmp[i + 8] = b[i];
    }

    for (i = 0; i < 16; i++) 
    {
        f_inv[i] ^= tmp[i];
    }

    /*
     * Level n = 16
     */
    memset(tmp, 0, 16 * sizeof(int16_t));
    for (i = 0; i < 16; i++) 
    {
        for (j = i; j < ZEN_N4; j += 16) 
        {
            tmp[i] ^= k[j];
        }
    }

    mul_in_R2_16(f_inv, tmp, b);
    MUL_F_IN_R2_512(b, tmp);

    for (j = 0; j < ZEN_N4; j++) 
    {
        k[j] = k[j] ^ tmp[j];
    }

    for (i = 16; i < ZEN_N4; i += 16) 
    {
        for (j = i; j < i + 16; j++) 
        {
            k[j] = k[j] ^ k[j - 16];
        }
    }

    for (i = 0; i < 16; i++) 
    {
        tmp[i] = tmp[i + 16] = b[i];
    }

    for (i = 0; i < 32; i++) 
    {
        f_inv[i] ^= tmp[i];
    }

    /*
     * Level n = 32
     */
    memset(tmp, 0, 32 * sizeof(int16_t));
    for (i = 0; i < 32; i++) 
    {
        for (j = i; j < ZEN_N4; j += 32) 
        {
            tmp[i] ^= k[j];
        }
    }

    mul_in_R2_32(f_inv, tmp, b);
    MUL_F_IN_R2_512(b, tmp);

    for (j = 0; j < ZEN_N4; j++) 
    {
        k[j] = k[j] ^ tmp[j];
    }

    for (i = 32; i < ZEN_N4; i += 32) 
    {
        for (j = i; j < i + 32; j++) 
        {
            k[j] = k[j] ^ k[j - 32];
        }
    }

    for (i = 0; i < 32; i++) 
    {
        tmp[i] = tmp[i + 32] = b[i];
    }

    for (i = 0; i < 64; i++) 
    {
        f_inv[i] ^= tmp[i];
    }

    /*
     * Level n = 64
     */
    memset(tmp, 0, 64 * sizeof(int16_t));
    for (i = 0; i < 64; i++) 
    {
        for (j = i; j < ZEN_N4; j += 64) 
        {
            tmp[i] ^= k[j];
        }
    }

    mul_in_R2_64(f_inv, tmp, b);
    MUL_F_IN_R2_512(b, tmp);

    for (j = 0; j < ZEN_N4; j++) 
    {
        k[j] = k[j] ^ tmp[j];
    }

    for (i = 64; i < ZEN_N4; i += 64) 
    {
        for (j = i; j < i + 64; j++) 
        {
            k[j] = k[j] ^ k[j - 64];
        }
    }

    for (i = 0; i < 64; i++) 
    {
        tmp[i] = tmp[i + 64] = b[i];
    }

    for (i = 0; i < 128; i++) 
    {
        f_inv[i] ^= tmp[i];
    }

    /*
     * Level n = 128
     */
    memset(tmp, 0, 128 * sizeof(int16_t));
    for (i = 0; i < 128; i++) 
    {
        for (j = i; j < ZEN_N4; j += 128) 
        {
            tmp[i] ^= k[j];
        }
    }

    mul_in_R2_128(f_inv, tmp, b);
    MUL_F_IN_R2_512(b, tmp);

    for (j = 0; j < ZEN_N4; j++) 
    {
        k[j] = k[j] ^ tmp[j];
    }

    for (i = 128; i < ZEN_N4; i += 128) 
    {
        for (j = i; j < i + 128; j++) 
        {
            k[j] = k[j] ^ k[j - 128];
        }
    }

    for (i = 0; i < 128; i++) 
    {
        tmp[i] = tmp[i + 128] = b[i];
    }

    for (i = 0; i < 256; i++) 
    {
        f_inv[i] ^= tmp[i];
    }

    /*
     * Level n = 256
     */
    memset(tmp, 0, 256 * sizeof(int16_t));
    for (i = 0; i < 256; i++) 
    {
        for (j = i; j < ZEN_N4; j += 256) 
        {
            tmp[i] ^= k[j];
        }
    }

    mul_in_R2_256(f_inv, tmp, b);
    MUL_F_IN_R2_512(b, tmp);

    for (j = 0; j < ZEN_N4; j++) 
    {
        k[j] = k[j] ^ tmp[j];
    }

    for (i = 256; i < ZEN_N4; i += 256) 
    {
        for (j = i; j < i + 256; j++) 
        {
            k[j] = k[j] ^ k[j - 256];
        }
    }

    for (i = 0; i < 256; i++) 
    {
        tmp[i] = tmp[i + 256] = b[i];
    }

    for (i = 0; i < 512; i++) 
    {
        f_inv[i] ^= tmp[i];
    }
#undef MUL_F_IN_R2_512
}

static inline void generate_gf_impl(int16_t *a, const uint8_t *seed,
                                    uint8_t nonce)
{
    uint8_t buf[ZEN_N_LEN_BYTES*5];

    zen_pseudoXOF(ZEN_N*5, seed, SEED_LEN_BYTES*8, buf, nonce);
    tenary3_32(a, buf);
}

static inline void generate_noise_impl(int16_t *a, const uint8_t *seed,
                                       uint8_t nonce)
{
    uint8_t buf[ZEN_N_LEN_BYTES*3];

    zen_pseudoXOF(ZEN_N*3, seed, SEED_LEN_BYTES*8, buf, nonce);
    tenary1_8(a, buf);
}

void poly_generate_gf(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    generate_gf_impl(a, seed, nonce);
}

void poly_generate_fg(int16_t *f, int16_t *g, const uint8_t *seed,
                      uint8_t nonce_f, uint8_t nonce_g)
{
    uint8_t buf_f[ZEN_N_LEN_BYTES*5];
    uint8_t buf_g[ZEN_N_LEN_BYTES*5];

    zen_pseudoXOF(ZEN_N*5, seed, SEED_LEN_BYTES*8,
                        buf_f, nonce_f);
    zen_pseudoXOF(ZEN_N*5, seed, SEED_LEN_BYTES*8,
                        buf_g, nonce_g);
    tenary3_32(f, buf_f);
    tenary3_32(g, buf_g);
}

void poly_generate_se(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    generate_noise_impl(a, seed, nonce);
}

void poly_generate_se_pair(int16_t *s, int16_t *e, const uint8_t *seed,
                      uint8_t nonce_s, uint8_t nonce_e)
{
    uint8_t buf_s[ZEN_N_LEN_BYTES*3];
    uint8_t buf_e[ZEN_N_LEN_BYTES*3];

    zen_pseudoXOF(ZEN_N*3, seed, SEED_LEN_BYTES*8,
                        buf_s, nonce_s);
    zen_pseudoXOF(ZEN_N*3, seed, SEED_LEN_BYTES*8,
                        buf_e, nonce_e);
    tenary1_8(s, buf_s);
    tenary1_8(e, buf_e);
}

void poly_bit2byte_pack(uint8_t *pa, const int16_t *a, const unsigned int n)
{
    unsigned int i;
    const int16x8_t shifts = vld1q_s16(neon_shifts_0_7);
    const int16x8_t ones = vdupq_n_s16(1);

    for(i = 0; i < n / 8; i++)
    {
        int16x8_t v = vandq_s16(vld1q_s16(&a[i * 8]), ones);
        int16x8_t s;

        v = vshlq_s16(v, shifts);
        s = vpaddq_s16(v, v);
        s = vpaddq_s16(s, s);
        s = vpaddq_s16(s, s);
        pa[i] = (uint8_t)vgetq_lane_s16(s, 0);
    }
}

void poly_byte2bit_unpack(int16_t *a, const uint8_t *pa, const unsigned int n)
{
    unsigned int i;
    const int16x8_t neg_shifts = vld1q_s16(neon_shifts_neg_0_7);
    const int16x8_t ones = vdupq_n_s16(1);

    for(i = 0; i < n / 8; i++)
    {
        int16x8_t bcast = vdupq_n_s16((int16_t)pa[i]);
        vst1q_s16(&a[i * 8], vandq_s16(vshlq_s16(bcast, neg_shifts), ones));
    }
}

void poly_secretkey_pack(uint8_t *ss, const int16_t *a)
{
    unsigned int i, k;
    const int16x8_t pos_shifts = vld1q_s16(neon_shifts_0_7);
    const int16x8_t ones = vdupq_n_s16(1);

    for(i = 0; i < ZEN_N; i += 32)
    {
        const int16_t *src = a + i;
        uint8_t *dst = ss + (i / 32) * 40;

#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
        for(k = 0; k < 10; k++)
        {
            const int16x8_t bit_shift = vdupq_n_s16(-(int16_t)k);

            dst[4 * k + 0] = pack_coeff_bits8_neon(src, bit_shift, pos_shifts, ones);
            dst[4 * k + 1] = pack_coeff_bits8_neon(src + 8, bit_shift, pos_shifts, ones);
            dst[4 * k + 2] = pack_coeff_bits8_neon(src + 16, bit_shift, pos_shifts, ones);
            dst[4 * k + 3] = pack_coeff_bits8_neon(src + 24, bit_shift, pos_shifts, ones);
        }
    }
}

void poly_secretkey_unpack(int16_t *a, const uint8_t *ss)
{
    unsigned int i, k;
    const int16x8_t neg_shifts = vld1q_s16(neon_shifts_neg_0_7);
    const int16x8_t ones = vdupq_n_s16(1);

    for(i = 0; i < ZEN_N; i += 32)
    {
        int16_t *dst = a + i;
        const uint8_t *src = ss + (i / 32) * 40;
        int16x8_t r0 = vdupq_n_s16(0);
        int16x8_t r1 = vdupq_n_s16(0);
        int16x8_t r2 = vdupq_n_s16(0);
        int16x8_t r3 = vdupq_n_s16(0);

#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
        for(k = 0; k < 10; k++)
        {
            const int16x8_t shift = vdupq_n_s16((int16_t)k);

            r0 = vorrq_s16(r0, vshlq_s16(
                expand_byte_bits_neon(src[4 * k + 0], neg_shifts, ones),
                shift));
            r1 = vorrq_s16(r1, vshlq_s16(
                expand_byte_bits_neon(src[4 * k + 1], neg_shifts, ones),
                shift));
            r2 = vorrq_s16(r2, vshlq_s16(
                expand_byte_bits_neon(src[4 * k + 2], neg_shifts, ones),
                shift));
            r3 = vorrq_s16(r3, vshlq_s16(
                expand_byte_bits_neon(src[4 * k + 3], neg_shifts, ones),
                shift));
        }

        vst1q_s16(dst, r0);
        vst1q_s16(dst + 8, r1);
        vst1q_s16(dst + 16, r2);
        vst1q_s16(dst + 24, r3);
    }
}

void poly_publickey_pack(uint8_t *pa, const int16_t *a)
{
    unsigned int i;
    uint8_t *dst = pa;

    for(i = 0; i < 2040; i += 20)
    {
        uint64_t x0 = pack_base769_5(a + i);
        uint64_t x1 = pack_base769_5(a + i + 5);
        uint64_t x2 = pack_base769_5(a + i + 10);
        uint64_t x3 = pack_base769_5(a + i + 15);

        store64_le(dst,      x0 | ((x3 & 0xFFFFULL) << 48));
        store64_le(dst + 8,  x1 | (((x3 >> 16) & 0xFFFFULL) << 48));
        store64_le(dst + 16, x2 | (((x3 >> 32) & 0xFFFFULL) << 48));
        dst += 24;
    }

    {
        uint64_t x0 = pack_base769_5(a + 2040);
        uint64_t x1 = (uint64_t)(uint16_t)a[2045]
                    + (uint64_t)(uint16_t)a[2046] * pack_table[1]
                    + (uint64_t)(uint16_t)a[2047] * pack_table[2];

        store64_le(dst, x0 | ((x1 & 0xFFFFULL) << 48));
        store16_le(dst + 8, (uint16_t)((x1 >> 16) & 0x1FFFULL));
    }
}

void poly_publickey_unpack(int16_t *a, const uint8_t *pa)
{
    unsigned int i;
    const uint8_t *src = pa;
    const uint64_t MASK48 = 0x0000FFFFFFFFFFFFULL;

    for(i = 0; i < 2040; i += 20)
    {
        uint64_t y0 = load64_le(src);
        uint64_t y1 = load64_le(src + 8);
        uint64_t y2 = load64_le(src + 16);
        uint64_t x0 = y0 & MASK48;
        uint64_t x1 = y1 & MASK48;
        uint64_t x2 = y2 & MASK48;
        uint64_t x3 = (y0 >> 48)
                    | (((y1 >> 48) & 0xFFFFULL) << 16)
                    | (((y2 >> 48) & 0xFFFFULL) << 32);

        unpack_base769_5(a + i, x0);
        unpack_base769_5(a + i + 5, x1);
        unpack_base769_5(a + i + 10, x2);
        unpack_base769_5(a + i + 15, x3);
        src += 24;
    }

    {
        uint64_t y0 = load64_le(src);
        uint64_t y1 = load16_le(src + 8) & 0x1FFFULL;
        uint64_t x0 = y0 & MASK48;
        uint64_t x1 = (y0 >> 48) | (y1 << 16);

        unpack_base769_5(a + 2040, x0);
        unpack_base769_3(a + 2045, x1);
    }
}

void poly_ciphertext_pack(uint8_t *pa, const int16_t *a)
{
    unsigned int i;

    for(i = 0; i < ZEN_N; i++)
    {
        pa[i] = (uint8_t)(a[i] & 0xFF);
    }
}

void poly_ciphertext_unpack(int16_t *a, const uint8_t *pa)
{
    unsigned int i;

    for(i = 0; i < ZEN_N; i++)
    {
        a[i] = (int16_t)pa[i];
    }
}

void poly_compress(int16_t *a)
{
    unsigned int i;
    uint32_t d;

    for(i = 0; i < ZEN_N; i++)
    {
        d = (uint32_t)a[i] << 8;
        d += 384;
        d *= 10908;
        d >>= 23;
        a[i] = (int16_t)(d & 255);
    }
}

void poly_decompress(int16_t *a)
{
    unsigned int i;

    for(i = 0; i < ZEN_N; i++)
    {
        a[i] = (int16_t)((((uint32_t)a[i] * ZEN_Q) + 128) >> 8);
    }
}
