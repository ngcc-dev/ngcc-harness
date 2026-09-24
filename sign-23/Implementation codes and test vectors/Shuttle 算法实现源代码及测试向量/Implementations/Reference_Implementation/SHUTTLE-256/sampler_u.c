/*
 * sampler_u.c -- SHUTTLE SamplerU reference (scalar, integer-only, CT).
 *
 * Draws U ~ Uniform(0,1] as an exponent+mantissa pair off the IRS XOF
 * stream and returns (frac_q62 = log2(b), a) so the caller can form
 * log2(U) = frac/2^62 - a (see sampler_u.h for the full contract, the
 * MSB-first bit schedule, and the (a,frac) return rationale).  The
 * ApproxLog kernel is reused from approx_log.h; this file owns only
 * the MSB-first extraction, the (j, x_q64) split, and the byte schedule.
 *
 * Constant-time: the two bit-extraction helpers are branchless; clz over
 * the 80-bit string uses __builtin_clzll on known-nonzero limbs plus
 * arithmetic masking (no data-dependent branch).  __builtin_bswap64 /
 * __builtin_clzll are GCC/Clang builtins (also available under -Wpedantic;
 * they are not the
 * __int128 extension).  NO float, NO division.
 */
#include "sampler_u.h"

#include <stdint.h>
#include <string.h>

#include "approx_log.h" /* approx_log2_frac_q62{,_x2} (ApproxLog kernel) */
#include "params.h"
#include "test/prof.h" /* PT_SAMPLERU / PT_APPROXLOG -- ((void)0) unless PROF_TIME */

/* Compile-time pins of the SamplerU byte/bit schedule. */
_Static_assert(KAPPA_A == 80,
               "SamplerU exponent block is 80 bits = 10 bytes");
_Static_assert(KAPPA_B == 57,
               "SamplerU mantissa block is 57 bits of 8 bytes");
#define SAMPLER_U_RHO_A_BYTES ((KAPPA_A + 7) / 8) /* 10 */
#define SAMPLER_U_RHO_B_BYTES ((KAPPA_B + 7) / 8) /* 8  */
_Static_assert(SAMPLER_U_RHO_A_BYTES == 10, "rho_a is 10 bytes");
_Static_assert(SAMPLER_U_RHO_B_BYTES == 8, "rho_b is 8 bytes");
/* The g=2, kappa_b=57 ApproxLog caller split:  j = m >> 55,
 * x_q64 = (m mod 2^55) << 9.  Pin the shift arithmetic. */
#define SAMPLER_U_G 2 /* = SHUTTLE_LOG_POLY_G */
#define SAMPLER_U_REDUCED_BITS (KAPPA_B - SAMPLER_U_G)   /* 55 */
#define SAMPLER_U_XQ_SHIFT (64 - SAMPLER_U_REDUCED_BITS) /* 9 */
_Static_assert(SHUTTLE_LOG_POLY_G == SAMPLER_U_G,
               "SamplerU g must match the ApproxLog table g");
_Static_assert(SAMPLER_U_REDUCED_BITS == 55 && SAMPLER_U_XQ_SHIFT == 9,
               "kappa_b=57,g=2 => reduced 55 bits, x_q64 left-shift 9");

/* be64: big-endian (MSB-first) 64-bit load of 8 bytes with no aliasing UB
 * (memcpy + bswap).  buf[0] becomes the most significant byte. */
static uint64_t be64(const uint8_t buf[8])
{
    uint64_t v;
    memcpy(&v, buf, 8);
    return __builtin_bswap64(v);
}

/*
 * clz80_msb_first -- count leading zeros over the 80-bit MSB-first string
 * held in rho_a[0..9] (rho_a[0]'s MSB is R_0).  Returns the 0-based index
 * of the first 1-bit, or 80 if the whole string is zero.  Then a =
 * clz80+1.
 *
 * The 80-bit field is split as a high 64-bit limb (bytes 0..7, big-endian)
 * and a low 16-bit limb (bytes 8..9, big-endian, occupying bit positions
 * 15..0 of the field).  Leading zeros = CLZ(hi) if hi != 0, else 64 +
 * CLZ16(lo) if lo != 0, else 80.  Branchless via sign-bit masks:
 *   hnz = (hi != 0) ? -1 : 0,  lnz = (lo != 0) ? -1 : 0   (as masks)
 *   clz = hnz ?  clz64(hi)
 *             : lnz ? 64 + (16 - bitlen(lo)) ... -> 64 + clz_over_16(lo)
 *                   : 80
 * __builtin_clzll is UNDEFINED on 0, so we OR a sentinel bit into each
 * limb before calling it and select the real result with the masks (the
 * sentinel never affects the chosen branch because that branch's mask is 0
 * there).
 */
static unsigned int clz80_msb_first(const uint8_t rho_a[10])
{
    uint64_t hi = be64(rho_a); /* bytes 0..7, R_0 = MSB of hi */
    uint32_t lo =
        ((uint32_t)rho_a[8] << 8) | (uint32_t)rho_a[9]; /* bits 15..0 */

    /* nonzero masks (0 or 0xFFFFFFFF) -- branchless, no compare-jump. */
    uint32_t hnz =
        (uint32_t)(((hi | (~hi + 1)) >> 63) & 1u); /* 1 iff hi!=0 */
    uint32_t lnz = (lo != 0u);                     /* 1 iff lo!=0 */
    uint32_t hmask = (uint32_t)(0u - hnz);         /* all-ones iff hi!=0 */
    uint32_t lmask = (uint32_t)(0u - lnz);         /* all-ones iff lo!=0 */

    /* CLZ of each limb, made safe on zero by OR'ing a sentinel low bit
     * (that sentinel changes CLZ only when the limb would otherwise be 0,
     * and in that case the limb's mask is 0 so the value is discarded). */
    unsigned int clz_hi = (unsigned int)__builtin_clzll(hi | 1u);
    /* lo lives in the low 16 bits; place it at the top of a 16-bit field
     * so a 32-bit clz gives the in-field leading-zero count: lo<<16 puts
     * bit15 at bit31. clz32(lo<<16 | sentinel) in [0..16]. */
    unsigned int clz_lo16 = (unsigned int)__builtin_clz(
        (lo << 16) | 1u); /* 0..16 (16 if lo==0) */

    /* Select: hi-branch -> clz_hi; else lo-branch -> 64 + clz_lo16;
     * else 80. Build via masks (exactly one of hmask/lmask/none describes
     * the result).
     */
    unsigned int res_hi = clz_hi;         /* used iff hmask */
    unsigned int res_lo = 64u + clz_lo16; /* used iff !hmask & lmask */
    unsigned int res_zero = 80u; /* used iff !hmask & !lmask        */

    unsigned int r = (res_hi & hmask) | (res_lo & (~hmask & lmask)) |
                     (res_zero & (~hmask & ~lmask));
    return r;
}

/*
 * mantissa57_msb_first -- the top kappa_b=57 MSB-first bits of an 8-byte
 * squeeze as the integer m = sum_{i=0..56} M_i 2^{56-i}.  This is exactly
 * be64(rho_b) >> 7: the big-endian load puts M_0 in bit 63, the top 57
 * bits become m in {0..2^57-1}, the low 7 bits are discarded.
 */
static uint64_t mantissa57_msb_first(const uint8_t rho_b[8])
{
    return be64(rho_b) >>
           (8u * SAMPLER_U_RHO_B_BYTES - KAPPA_B); /* >> 7 */
}

/* (j, x_q64) from the MSB-first mantissa integer m (g=2, kappa_b=57). */
static void mantissa_split(uint64_t m, uint32_t *j, uint64_t *x_q64)
{
    *j = (uint32_t)(m >>
                    SAMPLER_U_REDUCED_BITS); /* top g=2 bits, in {0..3} */
    *x_q64 = (m & ((UINT64_C(1) << SAMPLER_U_REDUCED_BITS) - 1))
             << SAMPLER_U_XQ_SHIFT; /* (m mod 2^55) << 9 */
}

/*
 * sampler_u_decode -- the PURE (no-XOF) SamplerU decode of one 18-byte
 * block (10 exponent bytes rho_a + 8 mantissa bytes rho_b) into ell = (a,
 * frac_q62). Factored out of sampler_u() so the bulk-buffer IRS path
 * (reject_sample) can squeeze the whole TAU*18 stream ONCE and decode
 * each transition's 18-byte slice here, with NO byte-cursor dependence on
 * the call structure.  This is the decode oracle shared by
 * ref/avx2/avx512: it touches no ctx, so the bytes are pinned entirely by
 * the (public-length) bulk squeeze upstream. */
sampler_u_res sampler_u_decode(const uint8_t rho_a[SAMPLER_U_RHO_A_BYTES],
                               const uint8_t rho_b[SAMPLER_U_RHO_B_BYTES])
{
    sampler_u_res r;
    uint64_t m;
    uint32_t j;
    uint64_t xq;

    {
        PROF_START(t_su);
        r.a = clz80_msb_first(rho_a) + 1u; /* a in {1..81} */
        m = mantissa57_msb_first(rho_b);   /* m in {0..2^57-1}   */
        mantissa_split(m, &j, &xq);
        PROF_STOP(PT_SAMPLERU, t_su);
    }
    {
        PROF_START(t_al);
        r.frac_q62 = approx_log2_frac_q62(j, xq); /* log2(b) in Q62 */
        PROF_STOP(PT_APPROXLOG, t_al);
    }
    return r;
}

sampler_u_res sampler_u(xof_ctx *ctx)
{
    uint8_t rho_a[SAMPLER_U_RHO_A_BYTES];
    uint8_t rho_b[SAMPLER_U_RHO_B_BYTES];

    xof256_squeeze(ctx, rho_a,
                   SAMPLER_U_RHO_A_BYTES); /* 10 exponent bytes  */
    xof256_squeeze(ctx, rho_b,
                   SAMPLER_U_RHO_B_BYTES); /* 8 mantissa bytes   */
    return sampler_u_decode(rho_a, rho_b);
}

void sampler_u_x2(xof_ctx *ctx, sampler_u_res out[2])
{
    uint8_t rho_a0[SAMPLER_U_RHO_A_BYTES], rho_b0[SAMPLER_U_RHO_B_BYTES];
    uint8_t rho_a1[SAMPLER_U_RHO_A_BYTES], rho_b1[SAMPLER_U_RHO_B_BYTES];
    uint32_t sel[2];
    uint64_t xq[2];
    int64_t frac[2];
    uint64_t m0, m1;

    /* Squeeze in the EXACT order of two sequential sampler_u() calls so
     * the ctx byte cursor is bit-identical: a0, b0, a1, b1. */
    xof256_squeeze(ctx, rho_a0, SAMPLER_U_RHO_A_BYTES);
    xof256_squeeze(ctx, rho_b0, SAMPLER_U_RHO_B_BYTES);
    xof256_squeeze(ctx, rho_a1, SAMPLER_U_RHO_A_BYTES);
    xof256_squeeze(ctx, rho_b1, SAMPLER_U_RHO_B_BYTES);

    out[0].a = clz80_msb_first(rho_a0) + 1u;
    out[1].a = clz80_msb_first(rho_a1) + 1u;
    m0 = mantissa57_msb_first(rho_b0);
    m1 = mantissa57_msb_first(rho_b1);
    mantissa_split(m0, &sel[0], &xq[0]);
    mantissa_split(m1, &sel[1], &xq[1]);

    approx_log2_frac_q62_x2(sel, xq,
                            frac); /* both log2(b) at once          */
    out[0].frac_q62 = frac[0];
    out[1].frac_q62 = frac[1];
}
