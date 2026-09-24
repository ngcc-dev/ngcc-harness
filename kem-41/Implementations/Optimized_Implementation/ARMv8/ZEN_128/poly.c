/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-128 instance.
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

static const int16_t neon_shifts_0_7[8] = {0, 1, 2, 3, 4, 5, 6, 7};
static const int16_t neon_shifts_neg_0_7[8] = {0, -1, -2, -3, -4, -5, -6, -7};

static inline uint64_t load64_le(const uint8_t *src)
{
    uint64_t x;
    memcpy(&x, src, sizeof(x));
    return x;
}

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
static void mul_in_R2_128_words_pmull(const int16_t *a,
                                      const uint64_t b_words[2],
                                      int16_t *res)
{
    uint64_t aw[2];
    uint64_t product[4];
    uint64_t folded[2];

    pack_binary_128_neon(a, aw);
    clmul2_karatsuba(aw[0], aw[1], b_words[0], b_words[1], product);

    folded[0] = product[0] ^ product[2];
    folded[1] = product[1] ^ product[3];
    unpack_binary_words_neon(res, folded, 2);
}

int check_poly_inv_Zq(int16_t *a)
{
    unsigned int i;
    uint32x4_t acc = vdupq_n_u32(0);

    for(i = 0; i < ZEN_N; i += 16)
    {
        int16x8_t v0 = vld1q_s16(&a[i]);
        int16x8_t v1 = vld1q_s16(&a[i + 8]);
        int32x4_t s0 = vpaddlq_s16(v0);
        int32x4_t s1 = vpaddlq_s16(v1);
        int32x4_t g = vcombine_s32(
            vpadd_s32(vget_low_s32(s0), vget_high_s32(s0)),
            vpadd_s32(vget_low_s32(s1), vget_high_s32(s1)));

        acc = vorrq_u32(acc, vceqq_s32(g, vdupq_n_s32(0)));
    }

    return (vgetq_lane_u64(vreinterpretq_u64_u32(acc), 0) |
            vgetq_lane_u64(vreinterpretq_u64_u32(acc), 1)) ? 1 : 0;
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

ZEN_CRYPTO_TARGET
static void mul_in_R2_pmull_256(const int16_t *a, const int16_t *b, int16_t *res)
{
    uint64_t aw[4];
    uint64_t bw[4];
    uint64_t low[4];
    uint64_t high[4];
    uint64_t cross[4];
    uint64_t mid[4];
    uint64_t folded[4];
    unsigned int i;

    pack_binary_128_neon(a, aw);
    pack_binary_128_neon(a + 128, aw + 2);
    pack_binary_128_neon(b, bw);
    pack_binary_128_neon(b + 128, bw + 2);

    clmul2_karatsuba(aw[0], aw[1], bw[0], bw[1], low);
    clmul2_karatsuba(aw[2], aw[3], bw[2], bw[3], high);
    clmul2_karatsuba(aw[0] ^ aw[2], aw[1] ^ aw[3],
                     bw[0] ^ bw[2], bw[1] ^ bw[3], cross);

    for(i = 0; i < 4; i++)
    {
        mid[i] = cross[i] ^ low[i] ^ high[i];
    }

    folded[0] = low[0] ^ high[0] ^ mid[2];
    folded[1] = low[1] ^ high[1] ^ mid[3];
    folded[2] = low[2] ^ high[2] ^ mid[0];
    folded[3] = low[3] ^ high[3] ^ mid[1];
    unpack_binary_words_neon(res, folded, 4);
}

ZEN_CRYPTO_TARGET
void mul_in_R2_256_packed128(const int16_t *a, const uint8_t *b_packed,
                             int16_t *res)
{
    uint64_t aw[4];
    uint64_t bw[2];
    uint64_t lo[4];
    uint64_t hi[4];
    uint64_t folded[4];

    pack_binary_128_neon(a, aw);
    pack_binary_128_neon(a + 128, aw + 2);
    bw[0] = load64_le(b_packed);
    bw[1] = load64_le(b_packed + 8);

    clmul2_karatsuba(aw[0], aw[1], bw[0], bw[1], lo);
    clmul2_karatsuba(aw[2], aw[3], bw[0], bw[1], hi);

    folded[0] = lo[0] ^ hi[2];
    folded[1] = lo[1] ^ hi[3];
    folded[2] = lo[2] ^ hi[0];
    folded[3] = lo[3] ^ hi[1];
    unpack_binary_words_neon(res, folded, 4);
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


void mul_in_R2_256(int16_t *a, int16_t *b, int16_t *res)
{
    mul_in_R2_pmull_256(a, b, res);
    return;
}

/* Exported FastInversion is the packed-bit assembly implementation in fastinversion.S. */
static void FastInversion_c_reference(int16_t *f_inv, int16_t *f)
{
    unsigned int i, j;
    int16_t k[ZEN_N4];
    int16_t b[ZEN_N4] = {0};
    int16_t tmp[ZEN_N4];
    uint64_t f_words[2];
#define MUL_F_IN_R2_128(B, OUT) mul_in_R2_128_words_pmull((B), f_words, (OUT))

    pack_binary_128_neon(f, f_words);

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
    MUL_F_IN_R2_128(b, tmp);

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
    MUL_F_IN_R2_128(b, tmp);

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
    MUL_F_IN_R2_128(b, tmp);

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
    MUL_F_IN_R2_128(b, tmp);

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
    MUL_F_IN_R2_128(b, tmp);

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
    MUL_F_IN_R2_128(b, tmp);

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
#undef MUL_F_IN_R2_128
}

void poly_generate_g(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    unsigned int i;
    uint8_t buf[ZEN_N_LEN_BYTES*5];
    int16_t t[ZEN_N];
    zen_pseudoXOF(ZEN_N*5, seed, SEED_LEN_BYTES*8, buf, nonce);
    cbd1(a, buf);
    tenary1_8(t, buf+ZEN_N_LEN_BYTES*2);
    for(i = 0; i < ZEN_N; i += 8)
    {
        vst1q_s16(&a[i], vaddq_s16(vld1q_s16(&a[i]), vld1q_s16(&t[i])));
    }
}

void poly_generate_f(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    uint8_t buf[ZEN_N_LEN_BYTES*2];

    zen_pseudoXOF(ZEN_N*2, seed, SEED_LEN_BYTES*8, buf, nonce);
    cbd1(a, buf);
}

void poly_generate_fg(int16_t *f, int16_t *g, const uint8_t *seed,
                      uint8_t nonce_f, uint8_t nonce_g)
{
    unsigned int i;
    uint8_t buf_f[ZEN_N_LEN_BYTES*2];
    uint8_t buf_g[ZEN_N_LEN_BYTES*5];
    int16_t t[ZEN_N];

    zen_pseudoXOF(ZEN_N*2, seed, SEED_LEN_BYTES*8,
                        buf_f, nonce_f);
    zen_pseudoXOF(ZEN_N*5, seed, SEED_LEN_BYTES*8,
                        buf_g, nonce_g);
    cbd1(f, buf_f);
    cbd1(g, buf_g);
    tenary1_8(t, buf_g+ZEN_N_LEN_BYTES*2);
    for(i = 0; i < ZEN_N; i += 8)
    {
        vst1q_s16(&g[i], vaddq_s16(vld1q_s16(&g[i]), vld1q_s16(&t[i])));
    }
}

void poly_generate_s(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    unsigned int i;
    uint8_t buf[ZEN_N_LEN_BYTES*7];
    int16_t t[ZEN_N];
    zen_pseudoXOF(ZEN_N*7, seed, SEED_LEN_BYTES*8, buf, nonce);
    cbd1(a, buf);
    tenary3_32(t, buf+ZEN_N_LEN_BYTES*2);
    for(i = 0; i < ZEN_N; i += 8)
    {
        vst1q_s16(&a[i], vaddq_s16(vld1q_s16(&a[i]), vld1q_s16(&t[i])));
    }
}

void poly_generate_e(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    uint8_t buf[ZEN_N_LEN_BYTES*4];

    zen_pseudoXOF(ZEN_N*4, seed, SEED_LEN_BYTES*8, buf, nonce);
    cbd2(a, buf);
}

void poly_generate_se_pair(int16_t *s, int16_t *e, const uint8_t *seed,
                      uint8_t nonce_s, uint8_t nonce_e)
{
    unsigned int i;
    uint8_t buf_s[ZEN_N_LEN_BYTES*7];
    uint8_t buf_e[ZEN_N_LEN_BYTES*4];
    int16_t t[ZEN_N];

    zen_pseudoXOF(ZEN_N*7, seed, SEED_LEN_BYTES*8,
                        buf_s, nonce_s);
    zen_pseudoXOF(ZEN_N*4, seed, SEED_LEN_BYTES*8,
                        buf_e, nonce_e);
    cbd1(s, buf_s);
    tenary3_32(t, buf_s+ZEN_N_LEN_BYTES*2);
    for(i = 0; i < ZEN_N; i += 8)
    {
        vst1q_s16(&s[i], vaddq_s16(vld1q_s16(&s[i]), vld1q_s16(&t[i])));
    }
    cbd2(e, buf_e);
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
            const int16x8_t scale = vdupq_n_s16((int16_t)(1u << k));

            r0 = vaddq_s16(r0, vmulq_s16(
                expand_byte_bits_neon(src[4 * k + 0], neg_shifts, ones),
                scale));
            r1 = vaddq_s16(r1, vmulq_s16(
                expand_byte_bits_neon(src[4 * k + 1], neg_shifts, ones),
                scale));
            r2 = vaddq_s16(r2, vmulq_s16(
                expand_byte_bits_neon(src[4 * k + 2], neg_shifts, ones),
                scale));
            r3 = vaddq_s16(r3, vmulq_s16(
                expand_byte_bits_neon(src[4 * k + 3], neg_shifts, ones),
                scale));
        }

        vst1q_s16(dst, r0);
        vst1q_s16(dst + 8, r1);
        vst1q_s16(dst + 16, r2);
        vst1q_s16(dst + 24, r3);
    }
}

static const uint64_t pack_table[] = 
{
    1, 769, 591361, 454756609, 349707832321
};


void poly_publickey_pack(uint8_t *pa, const int16_t *a)
{
    int i, idx;
    uint64_t tmp[103] = {0};
    uint64_t res[77]  = {0};

    idx = 0;
    for(i = 0; i < ZEN_N - 2; i += 5)
    {
        tmp[idx] =
              (uint64_t)(uint16_t)a[i]
            + (uint64_t)(uint16_t)a[i + 1] * pack_table[1]
            + (uint64_t)(uint16_t)a[i + 2] * pack_table[2]
            + (uint64_t)(uint16_t)a[i + 3] * pack_table[3]
            + (uint64_t)(uint16_t)a[i + 4] * pack_table[4];
        idx++;
    }

    tmp[102] =
          (uint64_t)(uint16_t)a[ZEN_N - 2]
        + (uint64_t)(uint16_t)a[ZEN_N - 1] * pack_table[1];

    idx = 0;
    for(i = 0; i < 100; i += 4)
    {
        uint64_t x0 = tmp[i];
        uint64_t x1 = tmp[i + 1];
        uint64_t x2 = tmp[i + 2];
        uint64_t x3 = tmp[i + 3];

        res[idx++] = x0 | ((x3 & 0xFFFFULL) << 48);
        res[idx++] = x1 | (((x3 >> 16) & 0xFFFFULL) << 48);
        res[idx++] = x2 | (((x3 >> 32) & 0xFFFFULL) << 48);
    }

    res[idx++] = tmp[100] | ((tmp[102] & 0xFFFFULL) << 48);
    res[idx++] = tmp[101] | (((tmp[102] >> 16) & 0xFULL) << 48);

    memcpy(pa, (const uint8_t *)res, ZEN_INDCPA_PUBLICKEY_LEN_BYTES);
}

void poly_publickey_unpack(int16_t *a, const uint8_t *pa)
{
    int i, idx;
    uint64_t res[77]  = {0};
    uint64_t tmp[103] = {0};
    const uint64_t MASK48   = 0x0000FFFFFFFFFFFFULL;
    const uint64_t DIV769_M = 374811932576999ULL;

    memcpy((uint8_t *)res, pa, ZEN_INDCPA_PUBLICKEY_LEN_BYTES);

    idx = 76;

    tmp[101] = res[idx] & MASK48;
    tmp[102] = ((res[idx--] >> 48) & 0xFULL) << 16;

    tmp[100] = res[idx] & MASK48;
    tmp[102] |= ((res[idx--] >> 48) & 0xFFFFULL);

    for(i = 99; i > 0; i -= 4)
    {
        tmp[i - 1] = res[idx] & MASK48;
        tmp[i]     = ((res[idx--] >> 48) & 0xFFFFULL) << 32;

        tmp[i - 2] = res[idx] & MASK48;
        tmp[i]    |= ((res[idx--] >> 48) & 0xFFFFULL) << 16;

        tmp[i - 3] = res[idx] & MASK48;
        tmp[i]    |= ((res[idx--] >> 48) & 0xFFFFULL);
    }

    for(i = 0; i < 2; i++)
    {
        uint64_t x = tmp[102];
        uint64_t q = (uint64_t)(((__uint128_t)x * DIV769_M) >> 58);
        uint64_t r = x - q * 769ULL;

        a[ZEN_N - 2 + i] = (int16_t)r;
        tmp[102] = q;
    }

    idx = 0;
    for(i = 0; i < ZEN_N - 2; i += 5)
    {
        uint64_t x, q, r;

        x = tmp[idx];

        q = (uint64_t)(((__uint128_t)x * DIV769_M) >> 58);
        r = x - q * 769ULL;
        a[i] = (int16_t)r;
        x = q;

        q = (uint64_t)(((__uint128_t)x * DIV769_M) >> 58);
        r = x - q * 769ULL;
        a[i + 1] = (int16_t)r;
        x = q;

        q = (uint64_t)(((__uint128_t)x * DIV769_M) >> 58);
        r = x - q * 769ULL;
        a[i + 2] = (int16_t)r;
        x = q;

        q = (uint64_t)(((__uint128_t)x * DIV769_M) >> 58);
        r = x - q * 769ULL;
        a[i + 3] = (int16_t)r;
        x = q;

        q = (uint64_t)(((__uint128_t)x * DIV769_M) >> 58);
        r = x - q * 769ULL;
        a[i + 4] = (int16_t)r;

        idx++;
    }
}

void poly_ciphertext_pack(uint8_t *pa, const int16_t *a)
{
    unsigned int i;

    for(i = 0; i < ZEN_N; i += 16)
    {
        int16x8_t v0 = vld1q_s16(&a[i]);
        int16x8_t v1 = vld1q_s16(&a[i + 8]);
        uint8x8_t lo = vmovn_u16(vreinterpretq_u16_s16(v0));
        uint8x8_t hi = vmovn_u16(vreinterpretq_u16_s16(v1));

        vst1_u8(&pa[i], lo);
        vst1_u8(&pa[i + 8], hi);
    }
}

void poly_ciphertext_unpack(int16_t *a, const uint8_t *pa)
{
    unsigned int i;

    for(i = 0; i < ZEN_N; i += 16)
    {
        uint8x8_t lo = vld1_u8(&pa[i]);
        uint8x8_t hi = vld1_u8(&pa[i + 8]);
        int16x8_t v0 = vreinterpretq_s16_u16(vmovl_u8(lo));
        int16x8_t v1 = vreinterpretq_s16_u16(vmovl_u8(hi));

        vst1q_s16(&a[i], v0);
        vst1q_s16(&a[i + 8], v1);
    }
}

void poly_compress(int16_t *a)
{
    unsigned int i;

    for(i = 0; i < ZEN_N; i += 8)
    {
        int16x8_t v = vld1q_s16(&a[i]);
        uint16x8_t uv = vreinterpretq_u16_s16(v);
        uint32x4_t d0 = vmovl_u16(vget_low_u16(uv));
        uint32x4_t d1 = vmovl_u16(vget_high_u16(uv));
        uint16x4_t n0;
        uint16x4_t n1;

        d0 = vmulq_n_u32(d0, 10908);
        d1 = vmulq_n_u32(d1, 10908);
        d0 = vaddq_u32(d0, vdupq_n_u32(16362));
        d1 = vaddq_u32(d1, vdupq_n_u32(16362));
        d0 = vshrq_n_u32(d0, 15);
        d1 = vshrq_n_u32(d1, 15);

        n0 = vmovn_u32(d0);
        n1 = vmovn_u32(d1);
        vst1q_s16(&a[i], vreinterpretq_s16_u16(vcombine_u16(n0, n1)));
    }
}

void poly_decompress(int16_t *a)
{
    unsigned int i;

    for(i = 0; i < ZEN_N; i += 8)
    {
        int16x8_t v = vld1q_s16(&a[i]);
        uint16x8_t uv = vreinterpretq_u16_s16(v);
        uint32x4_t d0 = vmovl_u16(vget_low_u16(uv));
        uint32x4_t d1 = vmovl_u16(vget_high_u16(uv));
        uint16x4_t n0;
        uint16x4_t n1;

        d0 = vaddq_u32(vmulq_n_u32(d0, ZEN_Q), vdupq_n_u32(128));
        d1 = vaddq_u32(vmulq_n_u32(d1, ZEN_Q), vdupq_n_u32(128));
        d0 = vshrq_n_u32(d0, 8);
        d1 = vshrq_n_u32(d1, 8);

        n0 = vmovn_u32(d0);
        n1 = vmovn_u32(d1);
        vst1q_s16(&a[i], vreinterpretq_s16_u16(vcombine_u16(n0, n1)));
    }
}
