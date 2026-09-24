/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-128 instance.

NEON Optimization (ARMv7): Added 8-way SIMD Montgomery multiplication and butterfly
operations via #ifdef __ARM_NEON__. Original X86 scalar code is preserved unmodified
in #else branches for full cross-platform compatibility.
*/
#include <stdint.h>
#include <stdio.h>
#include "params.h"
#include "ntt.h"

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

int16_t montgomery_reduce(int32_t a)
{
    int32_t t;
    int16_t u;

    u = a * QINV;
    t = (int32_t)u * ZEN_Q;
    t = a - t;
    t >>= 16;
    return t;
}

/// @brief Multiply two coefficients in the finite field modulo ZEN_Q using Montgomery reduction
/// @param[in] a First input coefficient
/// @param[in] b Second input coefficient
/// @return Product of a and b reduced modulo ZEN_Q
static inline int16_t fqmul(int16_t a, int16_t b) 
{
  return montgomery_reduce((int32_t)a * b);
}

/* ========================================================================
 * ARMv7 NEON SIMD 向量化核心函数
 * 利用 128位 Q寄存器实现8路int16_t并行Montgomery乘法与蝶形运算
 * ======================================================================== */
#ifdef __ARM_NEON__

/// @brief NEON 4路并行 Montgomery 约简 (int32x4_t → int32x4_t)
/// @note 使用无符号窄化+掩码避免 vmovn_s32 饱和溢出 (saturation→truncation)
static inline int32x4_t neon_mont_reduce_4way(int32x4_t a,
                                               int32x4_t qinv_vec,
                                               int32x4_t zenq_vec)
{
    int32x4_t u = vmulq_s32(a, qinv_vec);
    /* 提取低16位(无饱和): 掩码0xFFFF → vmovn_u32(值域0~65535无饱和) → 符号扩展 */
    uint32x4_t u_low = vandq_u32(vreinterpretq_u32_s32(u), vdupq_n_u32(0xFFFF));
    uint16x4_t u_narrow = vmovn_u32(u_low);
    u = vmovl_s16(vreinterpret_s16_u16(u_narrow));
    int32x4_t t = vmulq_s32(u, zenq_vec);
    t = vsubq_s32(a, t);
    t = vshrq_n_s32(t, 16);
    return t;
}

/// @brief NEON 8路并行有限域乘法: r[i]=montgomery_reduce(a[i]*b[i])
static inline int16x8_t neon_fqmul_8way(int16x8_t a, int16x8_t b,
                                         int32x4_t qinv_vec, int32x4_t zenq_vec)
{
    int16x4_t a_lo = vget_low_s16(a);
    int16x4_t a_hi = vget_high_s16(a);
    int16x4_t b_lo = vget_low_s16(b);
    int16x4_t b_hi = vget_high_s16(b);
    int32x4_t prod_lo = vmull_s16(a_lo, b_lo);
    int32x4_t prod_hi = vmull_s16(a_hi, b_hi);
    int32x4_t red_lo = neon_mont_reduce_4way(prod_lo, qinv_vec, zenq_vec);
    int32x4_t red_hi = neon_mont_reduce_4way(prod_hi, qinv_vec, zenq_vec);
    return vcombine_s16(vmovn_s32(red_lo), vmovn_s32(red_hi));
}

/// @brief NEON 8路前向 NTT 蝶形: t=fqmul(zeta,a[j+len]); a[j+len]=a[j]-t; a[j]=a[j]+t
static inline void neon_butterfly_ntt_8way(int16_t *a_j, int16_t *a_jlen,
                                            int16x8_t zeta_vec,
                                            int32x4_t qinv_vec, int32x4_t zenq_vec)
{
    int16x8_t aj  = vld1q_s16(a_j);
    int16x8_t ajl = vld1q_s16(a_jlen);
    int16x8_t t = neon_fqmul_8way(zeta_vec, ajl, qinv_vec, zenq_vec);
    vst1q_s16(a_j,    vaddq_s16(aj, t));
    vst1q_s16(a_jlen, vsubq_s16(aj, t));
}

/// @brief NEON 8路逆向 NTT 蝶形 (Gentleman-Sande)
static inline void neon_butterfly_intt_8way(int16_t *a_j, int16_t *a_jlen,
                                             int16x8_t zeta_vec,
                                             int32x4_t qinv_vec, int32x4_t zenq_vec)
{
    int16x8_t aj  = vld1q_s16(a_j);
    int16x8_t ajl = vld1q_s16(a_jlen);
    int16x8_t sum  = vaddq_s16(aj, ajl);
    int16x8_t diff = vsubq_s16(aj, ajl);
    vst1q_s16(a_j,    sum);
    vst1q_s16(a_jlen, neon_fqmul_8way(zeta_vec, diff, qinv_vec, zenq_vec));
}

#endif /* __ARM_NEON__ */


/* ========================================================================
 * poly_ntt — 前向 NTT 变换
 * NEON: 8路并行蝶形运算，len≥8时使用NEON，len=4尾部标量处理
 * ======================================================================== */
#ifdef __ARM_NEON__
void poly_ntt(int16_t *a)
{
    unsigned int len, start, j, k;
    int16_t t, zeta;

    int32x4_t qinv_vec = vdupq_n_s32(QINV);
    int32x4_t zenq_vec = vdupq_n_s32(ZEN_Q);

    k = 1;
    for(len = ZEN_N >> 1; len >= 8; len >>= 1)
    {
        for(start = 0; start < ZEN_N; start = j + len)
        {
            zeta = f[k++];
            int16x8_t zeta_vec = vdupq_n_s16(zeta);
            for(j = start; j < start + len; j += 8)
            {
                neon_butterfly_ntt_8way(&a[j], &a[j + len],
                                         zeta_vec, qinv_vec, zenq_vec);
            }
        }
    }
    /* 最后一轮 len=4 — 标量处理 (ZEN_N=512) */
    for(len = 4; len >= 4; len >>= 1)
    {
        for(start = 0; start < ZEN_N; start = j + len)
        {
            zeta = f[k++];
            for(j = start; j < start + len; j++)
            {
                t = fqmul(zeta, a[j + len]);
                a[j + len] = a[j] - t;
                a[j] = a[j] + t;
            }
        }
    }
}
#else
void poly_ntt(int16_t *a) 
{
    unsigned int len, start, j, k;
    int16_t t, zeta;

    k = 1;
    for(len = ZEN_N >> 1; len >= 4; len >>= 1) 
    {
        for(start = 0; start < ZEN_N; start = j + len) 
        {
            zeta = f[k++];
            for(j = start; j < start + len; j++) 
            {
                t = fqmul(zeta, a[j + len]);
                a[j + len] = a[j] - t;
                a[j] = a[j] + t;
            }
        }
    }
}
#endif


/* ========================================================================
 * poly_ntt_mq — 前向 NTT + Montgomery缩放
 * NEON: 8路并行蝶形 + NEON并行 MONT 缩放 + Barrett条件修正
 * ======================================================================== */
#ifdef __ARM_NEON__
void poly_ntt_mq(int16_t *a)
{
    unsigned int len, start, j, k;
    int16_t t, zeta;

    int32x4_t qinv_vec = vdupq_n_s32(QINV);
    int32x4_t zenq_vec = vdupq_n_s32(ZEN_Q);

    k = 1;
    for(len = ZEN_N >> 1; len >= 8; len >>= 1)
    {
        for(start = 0; start < ZEN_N; start = j + len)
        {
            zeta = f[k++];
            int16x8_t zeta_vec = vdupq_n_s16(zeta);
            for(j = start; j < start + len; j += 8)
            {
                neon_butterfly_ntt_8way(&a[j], &a[j + len],
                                         zeta_vec, qinv_vec, zenq_vec);
            }
        }
    }
    for(len = 4; len >= 4; len >>= 1)
    {
        for(start = 0; start < ZEN_N; start = j + len)
        {
            zeta = f[k++];
            for(j = start; j < start + len; j++)
            {
                t = fqmul(zeta, a[j + len]);
                a[j + len] = a[j] - t;
                a[j] = a[j] + t;
            }
        }
    }

    /* NEON 8路并行 MONT 缩放: a[j]=fqmul(a[j],171); a[j]+=(a[j]>>15)&Q */
    {
        int16x8_t mont_vec = vdupq_n_s16(171);
        int16x8_t q16_vec  = vdupq_n_s16(ZEN_Q);
        for(j = 0; j < ZEN_N; j += 8)
        {
            int16x8_t a_vec = vld1q_s16(&a[j]);
            a_vec = neon_fqmul_8way(a_vec, mont_vec, qinv_vec, zenq_vec);
            int16x8_t sign_bit = vshrq_n_s16(a_vec, 15);
            a_vec = vaddq_s16(a_vec, vandq_s16(sign_bit, q16_vec));
            vst1q_s16(&a[j], a_vec);
        }
    }
}
#else
void poly_ntt_mq(int16_t *a) 
{
    unsigned int len, start, j, k;
    int16_t t, zeta;

    k = 1;
    for(len = ZEN_N >> 1; len >= 4; len >>= 1) 
    {
        for(start = 0; start < ZEN_N; start = j + len) 
        {
            zeta = f[k++];
            for(j = start; j < start + len; j++) 
            {
                t = fqmul(zeta, a[j + len]);
                a[j + len] = a[j] - t;
                a[j] = a[j] + t;
            }
        }
    }

    for(j = 0; j < ZEN_N; j++)
    {
        a[j] = fqmul(a[j], 171);
        a[j] += (a[j] >> 15) & ZEN_Q;
    }
}
#endif


/* ========================================================================
 * poly_intt — 逆向 NTT 变换
 * NEON: 8路并行 Gentleman-Sande 蝶形 + NEON并行最终缩放
 * ======================================================================== */
#ifdef __ARM_NEON__
void poly_intt(int16_t *a)
{
    unsigned int start, len, j, k;
    int16_t t, zeta;

    int32x4_t qinv_vec = vdupq_n_s32(QINV);
    int32x4_t zenq_vec = vdupq_n_s32(ZEN_Q);

    k = 0;
    /* 先处理 len=4 标量: fn[0] 用于 len=4 (与x86路径fn索引顺序一致) */
    len = 4;
    for(start = 0; start < ZEN_N; start = j + len)
    {
        zeta = fn[k++];
        for(j = start; j < start + len; j++)
        {
            t = a[j];
            a[j] = (t + a[j + len]);
            a[j + len] = t - a[j + len];
            a[j + len] = fqmul(zeta, a[j + len]);
        }
    }
    /* len=8..256 使用 NEON 8路并行: fn[1..6] */
    for(len = 8; len <= ZEN_N >> 1; len <<= 1)
    {
        for(start = 0; start < ZEN_N; start = j + len)
        {
            zeta = fn[k++];
            int16x8_t zeta_vec = vdupq_n_s16(zeta);
            for(j = start; j < start + len; j += 8)
            {
                neon_butterfly_intt_8way(&a[j], &a[j + len],
                                          zeta_vec, qinv_vec, zenq_vec);
            }
        }
    }

    /* NEON 8路并行最终缩放: a[j] = fqmul(a[j], fn[127]) */
    {
        int16x8_t factor_vec = vdupq_n_s16(fn[127]);
        for(j = 0; j < ZEN_N; j += 8)
        {
            int16x8_t a_vec = vld1q_s16(&a[j]);
            a_vec = neon_fqmul_8way(a_vec, factor_vec, qinv_vec, zenq_vec);
            vst1q_s16(&a[j], a_vec);
        }
    }
}
#else
void poly_intt(int16_t *a) 
{
    unsigned int start, len, j, k;
    int16_t t, zeta;

    k = 0;
    for(len = 4; len <= ZEN_N >> 1; len <<= 1)
    {
        for(start = 0; start < ZEN_N; start =j + len)
        {
            zeta = fn[k++];
            for(j = start; j < start + len; j++) 
            {
                t = a[j];
                a[j] = (t + a[j + len]);
                a[j + len] = t - a[j + len];
                a[j + len] = fqmul(zeta, a[j + len]);
            }
        }
    }

    for(j = 0; j < ZEN_N; j++)
    {
        a[j] = fqmul(a[j], fn[127]);
    }
}
#endif


/* ========================================================================
 * 以下函数为标量实现，不做 NEON 修改 — 保持原始代码完全不变
 * ======================================================================== */

/// @brief Multiply two degree-3 polynomial blocks in the NTT domain with a given twiddle factor
/// @param[out] r Base address of output coefficient array of length 4
/// @param[in] a Base address of first input coefficient array of length 4
/// @param[in] b Base address of second input coefficient array of length 4
/// @param[in] zeta Twiddle factor used in the block multiplication
/// @return None
static inline void base_mul(int16_t *r, int16_t *a, int16_t *b, int16_t zeta)
{
    int16_t c0,  c1,  c2,  c3;
    c0 = fqmul(a[0], b[0]);
    c1 = fqmul(a[1], b[1]);
    c2 = fqmul(a[2], b[2]);
    c3 = fqmul(a[3], b[3]);

    r[0] = fqmul((a[1] + a[3]), (b[1] + b[3]));
    r[0] -= c1;
    r[0] -= c3;
    r[0] += c2;
    r[0] = fqmul(r[0], zeta);
    r[0] += c0;

    r[1] = fqmul((a[2] + a[3]), (b[2] + b[3]));
    r[1] -= c2;
    r[1] -= c3;
    r[1] = fqmul(r[1], zeta);
    r[1] += fqmul((a[0] + a[1]), (b[0] + b[1]));
    r[1] -= c0;
    r[1] -= c1;

    r[2] = fqmul(c3, zeta);
    r[2] += c1;
    r[2] += fqmul((a[0] + a[2]), (b[0] + b[2]));
    r[2] -= c0;
    r[2] -= c2;

    r[3] = fqmul((a[0] + a[3]), (b[0] + b[3]));
    r[3] -= c0;
    r[3] -= c3;
    r[3] += fqmul((a[1] + a[2]), (b[1] + b[2]));
    r[3] -= c1;
    r[3] -= c2;
}

void poly_basemul_ntt(int16_t *r,  int16_t *a,  int16_t *b)
{
    unsigned int i;
    for(i = 0; i < ZEN_N / 8; i++) 
    {
        base_mul(r + 8 * i, a + 8 * i, b + 8 * i, f[64 + i]);
        base_mul(r + 8 * i + 4, a + 8 * i + 4, b + 8 * i + 4, -f[64 + i]);
    }
}
/// @brief Multiply two degree-3 polynomial blocks in the NTT domain with a given twiddle factor and reduce coefficients modulo ZEN_Q
/// @param[out] r Base address of output coefficient array of length 4
/// @param[in] a Base address of first input coefficient array of length 4
/// @param[in] b Base address of second input coefficient array of length 4
/// @param[in] zeta Twiddle factor used in the block multiplication
/// @return None
static inline void base_mul_mq(int16_t *r, int16_t *a, int16_t *b, int16_t zeta)
{
    int16_t c0,  c1,  c2,  c3;
    c0 = fqmul(a[0], b[0]);
    c1 = fqmul(a[1], b[1]);
    c2 = fqmul(a[2], b[2]);
    c3 = fqmul(a[3], b[3]);

    r[0] = fqmul((a[1] + a[3]), (b[1] + b[3]));
    r[0] -= c1;
    r[0] -= c3;
    r[0] += c2;
    r[0] = fqmul(r[0], zeta);
    r[0] += c0;
    r[0] = fqmul(r[0], 19);
    r[0] += (r[0] >> 15) & ZEN_Q;

    r[1] = fqmul((a[2] + a[3]), (b[2] + b[3]));
    r[1] -= c2;
    r[1] -= c3;
    r[1] = fqmul(r[1], zeta);
    r[1] += fqmul((a[0] + a[1]), (b[0] + b[1]));
    r[1] -= c0;
    r[1] -= c1;
    r[1] = fqmul(r[1], 19);
    r[1] += (r[1] >> 15) & ZEN_Q;

    r[2] = fqmul(c3, zeta);
    r[2] += c1;
    r[2] += fqmul((a[0] + a[2]), (b[0] + b[2]));
    r[2] -= c0;
    r[2] -= c2;
    r[2] = fqmul(r[2], 19);
    r[2] += (r[2] >> 15) & ZEN_Q;

    r[3] = fqmul((a[0] + a[3]), (b[0] + b[3]));
    r[3] -= c0;
    r[3] -= c3;
    r[3] += fqmul((a[1] + a[2]), (b[1] + b[2]));
    r[3] -= c1;
    r[3] -= c2;
    r[3] = fqmul(r[3], 19);
    r[3] += (r[3] >> 15) & ZEN_Q;
}

void poly_basemul_ntt_mq(int16_t *r,  int16_t *a,  int16_t *b)
{
    unsigned int i;
    for(i = 0; i < ZEN_N / 8; i++) 
    {
        base_mul_mq(r + 8 * i, a + 8 * i, b + 8 * i, f[64 + i]);
        base_mul_mq(r + 8 * i + 4, a + 8 * i + 4, b + 8 * i + 4, -f[64 + i]);
    }
}

/// @brief Compute the inverse of a degree-3 polynomial block in the NTT domain with a given twiddle factor
/// @param[out] r Base address of output coefficient array of length 4
/// @param[in] a Base address of input coefficient array of length 4
/// @param[in] zeta Twiddle factor associated with the block
/// @return None
static void base_inv(int16_t *r, int16_t *a, int16_t zeta)
{
    unsigned int t, k, det;
    int16_t zeta2 = fqmul(zeta, zeta);

    r[0] = fqmul(a[2], a[2]);
    t = fqmul(a[1], a[3]);
    t = fqmul(t, 342);
    r[0] += t;
    r[0] = fqmul(r[0], a[0]);
    t = fqmul(a[1], a[1]);
    t = fqmul(t, a[2]);
    r[0] -= t;
    r[0] = fqmul(r[0], zeta);
    t = fqmul(a[3], a[3]);
    t = fqmul(t, a[2]);
    t = fqmul(t, zeta2);
    r[0] -= t;
    t = fqmul(a[0], a[0]);
    t = fqmul(t, a[0]);
    r[0] -= t;
    r[0] = fqmul(r[0], 173);

    r[1] = fqmul(a[1], a[2]);
    t = fqmul(a[0], a[3]);
    t = fqmul(t, 342);
    r[1] -= t;
    r[1] = fqmul(r[1], a[2]);
    t = fqmul(a[1], a[1]);
    t = fqmul(t, a[3]);
    r[1] -= t;
    r[1] = fqmul(r[1], zeta);
    t = fqmul(a[3], a[3]);
    t = fqmul(t, a[3]);
    t = fqmul(t, zeta2);
    r[1] += t;
    t = fqmul(a[0], a[0]);
    t = fqmul(t, a[1]);
    r[1] += t;
    r[1] = fqmul(r[1], 173);

    r[2] = fqmul(a[1], a[3]);
    r[2] = fqmul(r[2], 342);
    t = fqmul(a[2], a[2]);
    r[2] -= t;
    r[2] = fqmul(r[2], a[2]);
    t = fqmul(a[3], a[3]);
    t = fqmul(t, a[0]);
    r[2] -= t;
    r[2] = fqmul(r[2], zeta);
    t = fqmul(a[0], a[0]);
    t = fqmul(t, a[2]);
    r[2] += t;
    t = fqmul(a[1], a[1]);
    t = fqmul(t, a[0]);
    r[2] -= t;
    r[2] = fqmul(r[2], 173);

    r[3] = fqmul(a[2], a[2]);
    t = fqmul(a[1], a[3]);
    r[3] -= t;
    r[3] = fqmul(r[3], a[3]);
    r[3] = fqmul(r[3], zeta);
    t = fqmul(a[1], a[1]);
    t = fqmul(t, a[1]);
    r[3] += t;
    t = fqmul(a[0], a[2]);
    t = fqmul(t, a[1]);
    t = fqmul(t, 342);
    r[3] -= t;
    t = fqmul(a[0], a[0]);
    t = fqmul(t, a[3]);
    r[3] += t;
    r[3] = fqmul(r[3], 173);

    det = fqmul(a[2], a[2]);
    t = fqmul(a[1], a[3]);
    t = fqmul(t, 684);
    det -= t;
    det = fqmul(det, a[2]);
    det = fqmul(det, a[2]);
    t = fqmul(a[0], a[2]);
    t = fqmul(t, 342);
    t += fqmul(a[1], a[1]);
    t = fqmul(t, a[3]);
    t = fqmul(t, a[3]);
    t = fqmul(t, 342);
    det += t;
    det = fqmul(det, zeta2);
    t = fqmul(a[3], a[3]);
    t = fqmul(t, a[3]);
    t = fqmul(t, a[3]);
    t = fqmul(t, zeta2);
    t = fqmul(t, zeta);
    det = t-det;
    t = fqmul(a[0], a[0]);
    t = fqmul(t, a[0]);
    t = fqmul(t, a[0]);
    det -= t;
    t = fqmul(a[1], a[3]);
    t = fqmul(t, 342);
    t += fqmul(a[2], a[2]);
    t = fqmul(t, 342);
    t = fqmul(t, a[0]);
    t = fqmul(t, a[0]);
    k = fqmul(a[0], a[2]);
    k = fqmul(k, -684);
    k += fqmul(a[1], a[1]);
    k = fqmul(k, a[1]);
    k = fqmul(k, a[1]);
    t += k;
    t = fqmul(t, zeta);
    det += t;
    det = fqmul(det, 361);
    det += (det >> 15) & ZEN_Q;
    det = qinv[det];

    r[0] = fqmul(r[0], det);
    r[0] = fqmul(r[0], 19);
    r[1] = fqmul(r[1], det);
    r[1] = fqmul(r[1], 19);
    r[2] = fqmul(r[2], det);
    r[2] = fqmul(r[2], 19);
    r[3] = fqmul(r[3], det);
    r[3] = fqmul(r[3], 19);
}

void poly_baseinv_ntt(int16_t *r, int16_t *a)
{
    unsigned int i;
    for(i = 0; i < ZEN_N / 8; i++) 
    {
        base_inv(r + 8 * i, a + 8 * i, f[64 + i]);
        base_inv(r + 8 * i + 4, a + 8 * i + 4, -f[64 + i]);
    }
}
