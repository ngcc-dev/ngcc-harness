#ifndef POLARLAC_ARM_MLWE_ARITH_H
#define POLARLAC_ARM_MLWE_ARITH_H

#include <stdint.h>

#if defined(POLARLAC_USE_ARM_MLWE_ARITH) && !defined(POLARLAC_FORCE_SCALAR_ARITH) && defined(__aarch64__)
#include <arm_neon.h>
#define POLARLAC_HAVE_ARM_MLWE_NEON 1
#else
#define POLARLAC_HAVE_ARM_MLWE_NEON 0
#endif

/* The selected arithmetic backend is explicit NEON.  The whole-program
 * build still targets armv8.2-a+sve, so the compiler may use runtime-VL SVE
 * in unchanged portable C outside this header. */

#if POLARLAC_HAVE_ARM_MLWE_NEON
static inline int16x8_t polarlac_neon_wrap_add_s16x8(int16x8_t a, int16x8_t b)
{
    return vreinterpretq_s16_u16(vaddq_u16(vreinterpretq_u16_s16(a), vreinterpretq_u16_s16(b)));
}
static inline int16x8_t polarlac_neon_wrap_sub_s16x8(int16x8_t a, int16x8_t b)
{
    return vreinterpretq_s16_u16(vsubq_u16(vreinterpretq_u16_s16(a), vreinterpretq_u16_s16(b)));
}
static inline int16x4_t polarlac_neon_wrap_add_s16x4(int16x4_t a, int16x4_t b)
{
    return vreinterpret_s16_u16(vadd_u16(vreinterpret_u16_s16(a), vreinterpret_u16_s16(b)));
}
static inline int16x4_t polarlac_neon_wrap_sub_s16x4(int16x4_t a, int16x4_t b)
{
    return vreinterpret_s16_u16(vsub_u16(vreinterpret_u16_s16(a), vreinterpret_u16_s16(b)));
}
static inline int16x4_t polarlac_neon_wrap_neg_s16x4(int16x4_t a)
{
    return vreinterpret_s16_u16(vsub_u16(vdup_n_u16(0), vreinterpret_u16_s16(a)));
}
static inline int16x4_t polarlac_neon_reduce_s32x4(int32x4_t a)
{
    const uint32x4_t low = vmulq_n_u32(vreinterpretq_u32_s32(a), (uint32_t)(uint16_t)QINV);
    const int16x4_t u16 = vreinterpret_s16_u16(vmovn_u32(low));
    const int32x4_t u32 = vmovl_s16(u16);
    const int32x4_t t = vsubq_s32(a, vmulq_n_s32(u32, RL_KEM_Q));
    return vmovn_s32(vshrq_n_s32(t, 16));
}
static inline int16x8_t polarlac_neon_fqmul_s16x8(int16x8_t a, int16x8_t b)
{
    return vcombine_s16(polarlac_neon_reduce_s32x4(vmull_s16(vget_low_s16(a), vget_low_s16(b))),
                        polarlac_neon_reduce_s32x4(vmull_s16(vget_high_s16(a), vget_high_s16(b))));
}
static inline int16x4_t polarlac_neon_fqmul_s16x4(int16x4_t a, int16x4_t b)
{
    return polarlac_neon_reduce_s32x4(vmull_s16(a, b));
}

/* Exact vector form of the supplied source accumulation:
 *     mod_q(mod_q(first_product) + second_product)
 * The q=769 path first restores the standard residue with R mod q = 171,
 * exactly as rl_kem_mod_q() does in ntt.h. */
static inline int32x4_t polarlac_neon_canonical_q_s32x4(int32x4_t a)
{
#if RL_KEM_Q == 769
    int32x4_t t = vmovl_s16(polarlac_neon_reduce_s32x4(vmulq_n_s32(a, 171)));
#else
    int32x4_t t = vmovl_s16(polarlac_neon_reduce_s32x4(a));
#endif
    const int32x4_t q = vdupq_n_s32(RL_KEM_Q);
    const int32x4_t zero = vdupq_n_s32(0);
    t = vaddq_s32(t, vreinterpretq_s32_u32(vandq_u32(vcltq_s32(t, zero), vreinterpretq_u32_s32(q))));
    t = vsubq_s32(t, q);
    t = vaddq_s32(t, vreinterpretq_s32_u32(vandq_u32(vcltq_s32(t, zero), vreinterpretq_u32_s32(q))));
    return t;
}

static inline int16x8_t polarlac_neon_accumulate_products_s16x8(int16x8_t first,
                                                                int16x8_t second)
{
    int32x4_t lo = polarlac_neon_canonical_q_s32x4(vmovl_s16(vget_low_s16(first)));
    int32x4_t hi = polarlac_neon_canonical_q_s32x4(vmovl_s16(vget_high_s16(first)));
    lo = polarlac_neon_canonical_q_s32x4(vaddq_s32(lo, vmovl_s16(vget_low_s16(second))));
    hi = polarlac_neon_canonical_q_s32x4(vaddq_s32(hi, vmovl_s16(vget_high_s16(second))));
    return vcombine_s16(vmovn_s32(lo), vmovn_s32(hi));
}
static inline void polarlac_neon_ntt_group(int16_t *a, unsigned int start, unsigned int len, int16_t zeta)
{
    unsigned int j = start, end = start + len;
    const int16x8_t zv = vdupq_n_s16(zeta);
    for (; j + 8U <= end; j += 8U) {
        const int16x8_t lo = vld1q_s16(a + j), hi = vld1q_s16(a + j + len);
        const int16x8_t t = polarlac_neon_fqmul_s16x8(hi, zv);
        vst1q_s16(a + j, polarlac_neon_wrap_add_s16x8(lo, t));
        vst1q_s16(a + j + len, polarlac_neon_wrap_sub_s16x8(lo, t));
    }
    for (; j + 4U <= end; j += 4U) {
        const int16x4_t lo = vld1_s16(a + j), hi = vld1_s16(a + j + len);
        const int16x4_t t = polarlac_neon_fqmul_s16x4(hi, vdup_n_s16(zeta));
        vst1_s16(a + j, polarlac_neon_wrap_add_s16x4(lo, t));
        vst1_s16(a + j + len, polarlac_neon_wrap_sub_s16x4(lo, t));
    }
}
static inline void polarlac_neon_intt_group(int16_t *a, unsigned int start, unsigned int len, int16_t zeta)
{
    unsigned int j = start, end = start + len;
    const int16x8_t zv = vdupq_n_s16(zeta);
    for (; j + 8U <= end; j += 8U) {
        const int16x8_t lo = vld1q_s16(a + j), hi = vld1q_s16(a + j + len);
        vst1q_s16(a + j, polarlac_neon_wrap_add_s16x8(lo, hi));
        vst1q_s16(a + j + len, polarlac_neon_fqmul_s16x8(polarlac_neon_wrap_sub_s16x8(lo, hi), zv));
    }
    for (; j + 4U <= end; j += 4U) {
        const int16x4_t lo = vld1_s16(a + j), hi = vld1_s16(a + j + len);
        vst1_s16(a + j, polarlac_neon_wrap_add_s16x4(lo, hi));
        vst1_s16(a + j + len, polarlac_neon_fqmul_s16x4(polarlac_neon_wrap_sub_s16x4(lo, hi), vdup_n_s16(zeta)));
    }
}
static inline void polarlac_neon_ntt_two_groups4(int16_t *g0, int16_t z0, int16_t *g1, int16_t z1)
{
    const int16x8_t x0=vld1q_s16(g0), x1=vld1q_s16(g1);
    const int16x8_t lo=vcombine_s16(vget_low_s16(x0),vget_low_s16(x1));
    const int16x8_t hi=vcombine_s16(vget_high_s16(x0),vget_high_s16(x1));
    const int16x8_t z=vcombine_s16(vdup_n_s16(z0),vdup_n_s16(z1));
    const int16x8_t t=polarlac_neon_fqmul_s16x8(hi,z);
    const int16x8_t s=polarlac_neon_wrap_add_s16x8(lo,t), d=polarlac_neon_wrap_sub_s16x8(lo,t);
    vst1q_s16(g0,vcombine_s16(vget_low_s16(s),vget_low_s16(d)));
    vst1q_s16(g1,vcombine_s16(vget_high_s16(s),vget_high_s16(d)));
}
static inline void polarlac_neon_intt_two_groups4(int16_t *g0, int16_t z0, int16_t *g1, int16_t z1)
{
    const int16x8_t x0=vld1q_s16(g0), x1=vld1q_s16(g1);
    const int16x8_t lo=vcombine_s16(vget_low_s16(x0),vget_low_s16(x1));
    const int16x8_t hi=vcombine_s16(vget_high_s16(x0),vget_high_s16(x1));
    const int16x8_t z=vcombine_s16(vdup_n_s16(z0),vdup_n_s16(z1));
    const int16x8_t s=polarlac_neon_wrap_add_s16x8(lo,hi);
    const int16x8_t p=polarlac_neon_fqmul_s16x8(polarlac_neon_wrap_sub_s16x8(lo,hi),z);
    vst1q_s16(g0,vcombine_s16(vget_low_s16(s),vget_low_s16(p)));
    vst1q_s16(g1,vcombine_s16(vget_high_s16(s),vget_high_s16(p)));
}
static inline int16x8_t polarlac_neon_four_group_zetas(const int16_t zeta[4])
{
    int16x4_t lo = vdup_n_s16(zeta[0]);
    int16x4_t hi = vdup_n_s16(zeta[2]);

    lo = vset_lane_s16(zeta[1], lo, 2);
    lo = vset_lane_s16(zeta[1], lo, 3);
    hi = vset_lane_s16(zeta[3], hi, 2);
    hi = vset_lane_s16(zeta[3], hi, 3);
    return vcombine_s16(lo, hi);
}
static inline void polarlac_neon_ntt_four_groups2(int16_t *g, const int16_t zeta[4])
{
    const int16x4x4_t x=vld4_s16(g);
    const int16x4x2_t lz=vzip_s16(x.val[0],x.val[1]), hz=vzip_s16(x.val[2],x.val[3]);
    const int16x8_t lo=vcombine_s16(lz.val[0],lz.val[1]), hi=vcombine_s16(hz.val[0],hz.val[1]);
    const int16x8_t z=polarlac_neon_four_group_zetas(zeta);
    const int16x8_t t=polarlac_neon_fqmul_s16x8(hi,z);
    const int16x8_t s=polarlac_neon_wrap_add_s16x8(lo,t), d=polarlac_neon_wrap_sub_s16x8(lo,t);
    const int16x4x2_t su=vuzp_s16(vget_low_s16(s),vget_high_s16(s));
    const int16x4x2_t du=vuzp_s16(vget_low_s16(d),vget_high_s16(d));
    int16x4x4_t o={{su.val[0],su.val[1],du.val[0],du.val[1]}}; vst4_s16(g,o);
}
static inline void polarlac_neon_intt_four_groups2(int16_t *g, const int16_t zeta[4])
{
    const int16x4x4_t x=vld4_s16(g);
    const int16x4x2_t lz=vzip_s16(x.val[0],x.val[1]), hz=vzip_s16(x.val[2],x.val[3]);
    const int16x8_t lo=vcombine_s16(lz.val[0],lz.val[1]), hi=vcombine_s16(hz.val[0],hz.val[1]);
    const int16x8_t z=polarlac_neon_four_group_zetas(zeta);
    const int16x8_t s=polarlac_neon_wrap_add_s16x8(lo,hi);
    const int16x8_t p=polarlac_neon_fqmul_s16x8(polarlac_neon_wrap_sub_s16x8(lo,hi),z);
    const int16x4x2_t su=vuzp_s16(vget_low_s16(s),vget_high_s16(s));
    const int16x4x2_t pu=vuzp_s16(vget_low_s16(p),vget_high_s16(p));
    int16x4x4_t o={{su.val[0],su.val[1],pu.val[0],pu.val[1]}}; vst4_s16(g,o);
}
#endif

#endif /* POLARLAC_ARM_MLWE_ARITH_H */
