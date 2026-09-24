/*
 * avx2_neon.h — AVX2 intrinsics -> ARM NEON compatibility layer for Aigis-enc
 *
 * Maps __m256i (256-bit) to a pair of ARM NEON int8x16_t (128-bit).
 * Bit-exact with AVX2 semantics to preserve KAT correctness.
 */
#ifndef AVX2_NEON_H
#define AVX2_NEON_H

#include <arm_neon.h>
#include <stdint.h>
#include <string.h>

/* ================================================================== */
/*  Core 256-bit type                                                  */
/* ================================================================== */
typedef struct { int8x16_t lo, hi; } __m256i;
typedef int8x16_t __m128i;

#ifndef ALIGN
#define ALIGN(n) __attribute__((aligned(n)))
#endif

/* ================================================================== */
/*  Reinterpret helpers (zero-cost on ARM)                             */
/* ================================================================== */
#define AS_S16(x) vreinterpretq_s16_s8(x)
#define AS_U16(x) vreinterpretq_u16_s8(x)
#define AS_S32(x) vreinterpretq_s32_s8(x)
#define AS_U32(x) vreinterpretq_u32_s8(x)
#define AS_S64(x) vreinterpretq_s64_s8(x)
#define AS_U64(x) vreinterpretq_u64_s8(x)
#define AS_U8(x)  vreinterpretq_u8_s8(x)
#define FR_S16(x) vreinterpretq_s8_s16(x)
#define FR_U16(x) vreinterpretq_s8_u16(x)
#define FR_S32(x) vreinterpretq_s8_s32(x)
#define FR_U32(x) vreinterpretq_s8_u32(x)
#define FR_S64(x) vreinterpretq_s8_s64(x)
#define FR_U64(x) vreinterpretq_s8_u64(x)
#define FR_U8(x)  vreinterpretq_s8_u8(x)

/* ================================================================== */
/*  SET / SPLAT                                                        */
/* ================================================================== */
static inline __m256i _mm256_setzero_si256(void) {
    __m256i r; r.lo = r.hi = vdupq_n_s8(0); return r;
}
static inline __m256i _mm256_set1_epi8(int8_t a) {
    __m256i r; r.lo = r.hi = vdupq_n_s8(a); return r;
}
static inline __m256i _mm256_set1_epi16(int16_t a) {
    __m256i r; r.lo = r.hi = FR_S16(vdupq_n_s16(a)); return r;
}
static inline __m256i _mm256_set1_epi32(int32_t a) {
    __m256i r; r.lo = r.hi = FR_S32(vdupq_n_s32(a)); return r;
}
static inline __m256i _mm256_set1_epi64x(int64_t a) {
    __m256i r; r.lo = r.hi = FR_S64(vdupq_n_s64(a)); return r;
}
static inline __m256i _mm256_set_epi8(
    int8_t e31,int8_t e30,int8_t e29,int8_t e28,int8_t e27,int8_t e26,int8_t e25,int8_t e24,
    int8_t e23,int8_t e22,int8_t e21,int8_t e20,int8_t e19,int8_t e18,int8_t e17,int8_t e16,
    int8_t e15,int8_t e14,int8_t e13,int8_t e12,int8_t e11,int8_t e10,int8_t e9, int8_t e8,
    int8_t e7, int8_t e6, int8_t e5, int8_t e4, int8_t e3, int8_t e2, int8_t e1, int8_t e0) {
    __m256i r;
    int8_t L[16]={e0,e1,e2,e3,e4,e5,e6,e7,e8,e9,e10,e11,e12,e13,e14,e15};
    int8_t H[16]={e16,e17,e18,e19,e20,e21,e22,e23,e24,e25,e26,e27,e28,e29,e30,e31};
    r.lo = vld1q_s8(L); r.hi = vld1q_s8(H); return r;
}
static inline __m256i _mm256_set_epi32(int32_t e7,int32_t e6,int32_t e5,int32_t e4,
                                        int32_t e3,int32_t e2,int32_t e1,int32_t e0) {
    __m256i r;
    int32_t L[4]={e0,e1,e2,e3}, H[4]={e4,e5,e6,e7};
    r.lo = FR_S32(vld1q_s32(L)); r.hi = FR_S32(vld1q_s32(H)); return r;
}
static inline __m256i _mm256_set_epi64x(int64_t e3,int64_t e2,int64_t e1,int64_t e0) {
    __m256i r;
    int64_t L[2]={e0,e1}, H[2]={e2,e3};
    r.lo = FR_S64(vld1q_s64(L)); r.hi = FR_S64(vld1q_s64(H)); return r;
}

/* ================================================================== */
/*  LOAD / STORE                                                       */
/* ================================================================== */
static inline __m256i _mm256_load_si256(const void *p) {
    __m256i r; const int8_t *q=(const int8_t*)p;
    r.lo=vld1q_s8(q); r.hi=vld1q_s8(q+16); return r;
}
static inline __m256i _mm256_loadu_si256(const void *p) { return _mm256_load_si256(p); }
static inline void _mm256_store_si256(void *p, __m256i a) {
    int8_t *q=(int8_t*)p; vst1q_s8(q,a.lo); vst1q_s8(q+16,a.hi);
}
static inline void _mm256_storeu_si256(void *p, __m256i a) { _mm256_store_si256(p,a); }

static inline __m128i _mm_loadu_si128(const void *p) { return vld1q_s8((const int8_t*)p); }
static inline __m128i _mm_load_si128(const void *p)  { return vld1q_s8((const int8_t*)p); }
static inline void _mm_storeu_si128(void *p, __m128i a) { vst1q_s8((int8_t*)p,a); }

/* storeu2: store lo to lo_ptr, hi to hi_ptr (Intel convention) */
#define _mm256_storeu2_m128d(hi_ptr, lo_ptr, v) do { \
    vst1q_s8((int8_t*)(lo_ptr), (v).lo); \
    vst1q_s8((int8_t*)(hi_ptr), (v).hi); \
} while(0)

/* ================================================================== */
/*  ARITHMETIC: 8-bit                                                  */
/* ================================================================== */
static inline __m256i _mm256_add_epi8(__m256i a, __m256i b) {
    __m256i r; r.lo=vaddq_s8(a.lo,b.lo); r.hi=vaddq_s8(a.hi,b.hi); return r;
}
static inline __m256i _mm256_sub_epi8(__m256i a, __m256i b) {
    __m256i r; r.lo=vsubq_s8(a.lo,b.lo); r.hi=vsubq_s8(a.hi,b.hi); return r;
}

/* ================================================================== */
/*  ARITHMETIC: 16-bit                                                 */
/* ================================================================== */
static inline __m256i _mm256_add_epi16(__m256i a, __m256i b) {
    __m256i r;
    r.lo = FR_S16(vaddq_s16(AS_S16(a.lo),AS_S16(b.lo)));
    r.hi = FR_S16(vaddq_s16(AS_S16(a.hi),AS_S16(b.hi)));
    return r;
}
static inline __m256i _mm256_sub_epi16(__m256i a, __m256i b) {
    __m256i r;
    r.lo = FR_S16(vsubq_s16(AS_S16(a.lo),AS_S16(b.lo)));
    r.hi = FR_S16(vsubq_s16(AS_S16(a.hi),AS_S16(b.hi)));
    return r;
}
static inline __m256i _mm256_mullo_epi16(__m256i a, __m256i b) {
    __m256i r;
    r.lo = FR_S16(vmulq_s16(AS_S16(a.lo),AS_S16(b.lo)));
    r.hi = FR_S16(vmulq_s16(AS_S16(a.hi),AS_S16(b.hi)));
    return r;
}
/* mulhi_epi16: signed (a*b) >> 16 per 16-bit lane */
static inline __m256i _mm256_mulhi_epi16(__m256i a, __m256i b) {
    __m256i r;
    int16x8_t al=AS_S16(a.lo), bl=AS_S16(b.lo);
    int16x8_t ah=AS_S16(a.hi), bh=AS_S16(b.hi);
    int32x4_t p0 = vmull_s16(vget_low_s16(al),  vget_low_s16(bl));
    int32x4_t p1 = vmull_high_s16(al, bl);
    int32x4_t p2 = vmull_s16(vget_low_s16(ah),  vget_low_s16(bh));
    int32x4_t p3 = vmull_high_s16(ah, bh);
    r.lo = FR_S16(vcombine_s16(vshrn_n_s32(p0,16), vshrn_n_s32(p1,16)));
    r.hi = FR_S16(vcombine_s16(vshrn_n_s32(p2,16), vshrn_n_s32(p3,16)));
    return r;
}
/* mulhrs: (a*b + 0x4000) >> 15 — NEON vqrdmulh is equivalent for in-range values */
static inline __m256i _mm256_mulhrs_epi16(__m256i a, __m256i b) {
    __m256i r;
    r.lo = FR_S16(vqrdmulhq_s16(AS_S16(a.lo), AS_S16(b.lo)));
    r.hi = FR_S16(vqrdmulhq_s16(AS_S16(a.hi), AS_S16(b.hi)));
    return r;
}

/* ================================================================== */
/*  ARITHMETIC: 32-bit                                                 */
/* ================================================================== */
static inline __m256i _mm256_add_epi32(__m256i a, __m256i b) {
    __m256i r;
    r.lo = FR_S32(vaddq_s32(AS_S32(a.lo),AS_S32(b.lo)));
    r.hi = FR_S32(vaddq_s32(AS_S32(a.hi),AS_S32(b.hi)));
    return r;
}
static inline __m256i _mm256_sub_epi32(__m256i a, __m256i b) {
    __m256i r;
    r.lo = FR_S32(vsubq_s32(AS_S32(a.lo),AS_S32(b.lo)));
    r.hi = FR_S32(vsubq_s32(AS_S32(a.hi),AS_S32(b.hi)));
    return r;
}
/* mul_epi32: multiply low 32 of each 64-bit lane -> 64-bit */
static inline __m256i _mm256_mul_epi32(__m256i a, __m256i b) {
    __m256i r;
    int32x4_t al=AS_S32(a.lo), bl=AS_S32(b.lo);
    int32x4_t ah=AS_S32(a.hi), bh=AS_S32(b.hi);
    int32x2_t ae0 = vuzp1_s32(vget_low_s32(al), vget_high_s32(al));
    int32x2_t be0 = vuzp1_s32(vget_low_s32(bl), vget_high_s32(bl));
    int32x2_t ae1 = vuzp1_s32(vget_low_s32(ah), vget_high_s32(ah));
    int32x2_t be1 = vuzp1_s32(vget_low_s32(bh), vget_high_s32(bh));
    r.lo = FR_S64(vmull_s32(ae0, be0));
    r.hi = FR_S64(vmull_s32(ae1, be1));
    return r;
}

/* ================================================================== */
/*  BITWISE                                                            */
/* ================================================================== */
static inline __m256i _mm256_and_si256(__m256i a, __m256i b) {
    __m256i r; r.lo=vandq_s8(a.lo,b.lo); r.hi=vandq_s8(a.hi,b.hi); return r;
}
static inline __m256i _mm256_or_si256(__m256i a, __m256i b) {
    __m256i r; r.lo=vorrq_s8(a.lo,b.lo); r.hi=vorrq_s8(a.hi,b.hi); return r;
}
static inline __m256i _mm256_xor_si256(__m256i a, __m256i b) {
    __m256i r; r.lo=veorq_s8(a.lo,b.lo); r.hi=veorq_s8(a.hi,b.hi); return r;
}
static inline __m256i _mm256_andnot_si256(__m256i a, __m256i b) {
    __m256i r; r.lo=vbicq_s8(b.lo,a.lo); r.hi=vbicq_s8(b.hi,a.hi); return r;
}
/* 128-bit bitwise */
static inline __m128i _mm_xor_si128(__m128i a, __m128i b) { return veorq_s8(a,b); }
static inline __m128i _mm_and_si128(__m128i a, __m128i b) { return vandq_s8(a,b); }
static inline __m128i _mm_set1_epi8(int8_t a) { return vdupq_n_s8(a); }

/* ================================================================== */
/*  SHIFTS: 16-bit                                                     */
/* ================================================================== */
static inline __m256i _mm256_srli_epi16(__m256i a, int imm) {
    __m256i r;
    int16x8_t sh = vdupq_n_s16(-(int16_t)imm);
    r.lo = FR_U16(vshlq_u16(AS_U16(a.lo), sh));
    r.hi = FR_U16(vshlq_u16(AS_U16(a.hi), sh));
    return r;
}
static inline __m256i _mm256_slli_epi16(__m256i a, int imm) {
    __m256i r;
    int16x8_t sh = vdupq_n_s16((int16_t)imm);
    r.lo = FR_S16(vshlq_s16(AS_S16(a.lo), sh));
    r.hi = FR_S16(vshlq_s16(AS_S16(a.hi), sh));
    return r;
}
static inline __m256i _mm256_srai_epi16(__m256i a, int imm) {
    __m256i r;
    int16x8_t sh = vdupq_n_s16(-(int16_t)imm);
    r.lo = FR_S16(vshlq_s16(AS_S16(a.lo), sh));
    r.hi = FR_S16(vshlq_s16(AS_S16(a.hi), sh));
    return r;
}

/* ================================================================== */
/*  SHIFTS: 32-bit                                                     */
/* ================================================================== */
static inline __m256i _mm256_srli_epi32(__m256i a, int imm) {
    __m256i r;
    int32x4_t sh = vdupq_n_s32(-imm);
    r.lo = FR_U32(vshlq_u32(AS_U32(a.lo), sh));
    r.hi = FR_U32(vshlq_u32(AS_U32(a.hi), sh));
    return r;
}
static inline __m256i _mm256_slli_epi32(__m256i a, int imm) {
    __m256i r;
    int32x4_t sh = vdupq_n_s32(imm);
    r.lo = FR_S32(vshlq_s32(AS_S32(a.lo), sh));
    r.hi = FR_S32(vshlq_s32(AS_S32(a.hi), sh));
    return r;
}

/* ================================================================== */
/*  SHIFTS: 64-bit                                                     */
/* ================================================================== */
static inline __m256i _mm256_srli_epi64(__m256i a, int imm) {
    __m256i r;
    int64x2_t sh = vdupq_n_s64(-(int64_t)imm);
    r.lo = FR_U64(vshlq_u64(AS_U64(a.lo), sh));
    r.hi = FR_U64(vshlq_u64(AS_U64(a.hi), sh));
    return r;
}
static inline __m256i _mm256_slli_epi64(__m256i a, int imm) {
    __m256i r;
    int64x2_t sh = vdupq_n_s64((int64_t)imm);
    r.lo = FR_S64(vshlq_s64(AS_S64(a.lo), sh));
    r.hi = FR_S64(vshlq_s64(AS_S64(a.hi), sh));
    return r;
}
/* Variable shift: per-element 32-bit unsigned right shift */
static inline __m256i _mm256_srlv_epi32(__m256i a, __m256i count) {
    __m256i r;
    r.lo = FR_U32(vshlq_u32(AS_U32(a.lo), vnegq_s32(AS_S32(count.lo))));
    r.hi = FR_U32(vshlq_u32(AS_U32(a.hi), vnegq_s32(AS_S32(count.hi))));
    return r;
}

/* ================================================================== */
/*  COMPARISON                                                         */
/* ================================================================== */
static inline __m256i _mm256_cmpgt_epi16(__m256i a, __m256i b) {
    __m256i r;
    r.lo = FR_U16(vcgtq_s16(AS_S16(a.lo),AS_S16(b.lo)));
    r.hi = FR_U16(vcgtq_s16(AS_S16(a.hi),AS_S16(b.hi)));
    return r;
}

/* ================================================================== */
/*  UNPACK / INTERLEAVE                                                */
/* ================================================================== */
static inline __m256i _mm256_unpacklo_epi8(__m256i a, __m256i b) {
    __m256i r;
    r.lo = vzip1q_s8(a.lo, b.lo);
    r.hi = vzip1q_s8(a.hi, b.hi);
    return r;
}
static inline __m256i _mm256_unpackhi_epi8(__m256i a, __m256i b) {
    __m256i r;
    r.lo = vzip2q_s8(a.lo, b.lo);
    r.hi = vzip2q_s8(a.hi, b.hi);
    return r;
}
static inline __m256i _mm256_unpacklo_epi16(__m256i a, __m256i b) {
    __m256i r;
    r.lo = FR_S16(vzip1q_s16(AS_S16(a.lo),AS_S16(b.lo)));
    r.hi = FR_S16(vzip1q_s16(AS_S16(a.hi),AS_S16(b.hi)));
    return r;
}
static inline __m256i _mm256_unpackhi_epi16(__m256i a, __m256i b) {
    __m256i r;
    r.lo = FR_S16(vzip2q_s16(AS_S16(a.lo),AS_S16(b.lo)));
    r.hi = FR_S16(vzip2q_s16(AS_S16(a.hi),AS_S16(b.hi)));
    return r;
}
static inline __m256i _mm256_unpacklo_epi64(__m256i a, __m256i b) {
    __m256i r;
    r.lo = FR_S64(vzip1q_s64(AS_S64(a.lo),AS_S64(b.lo)));
    r.hi = FR_S64(vzip1q_s64(AS_S64(a.hi),AS_S64(b.hi)));
    return r;
}
static inline __m256i _mm256_unpackhi_epi64(__m256i a, __m256i b) {
    __m256i r;
    r.lo = FR_S64(vzip2q_s64(AS_S64(a.lo),AS_S64(b.lo)));
    r.hi = FR_S64(vzip2q_s64(AS_S64(a.hi),AS_S64(b.hi)));
    return r;
}

/* ================================================================== */
/*  PACK                                                               */
/* ================================================================== */
static inline __m256i _mm256_packs_epi16(__m256i a, __m256i b) {
    __m256i r;
    r.lo = vcombine_s8(vqmovn_s16(AS_S16(a.lo)), vqmovn_s16(AS_S16(b.lo)));
    r.hi = vcombine_s8(vqmovn_s16(AS_S16(a.hi)), vqmovn_s16(AS_S16(b.hi)));
    return r;
}
static inline __m256i _mm256_packus_epi32(__m256i a, __m256i b) {
    __m256i r;
    r.lo = FR_U16(vcombine_u16(vqmovun_s32(AS_S32(a.lo)), vqmovun_s32(AS_S32(b.lo))));
    r.hi = FR_U16(vcombine_u16(vqmovun_s32(AS_S32(a.hi)), vqmovun_s32(AS_S32(b.hi))));
    return r;
}

/* ================================================================== */
/*  SIGN / ZERO EXTEND                                                 */
/* ================================================================== */
static inline __m256i _mm256_cvtepi8_epi16(__m128i a) {
    __m256i r;
    r.lo = FR_S16(vmovl_s8(vget_low_s8(a)));
    r.hi = FR_S16(vmovl_s8(vget_high_s8(a)));
    return r;
}
static inline __m256i _mm256_cvtepu8_epi32(__m128i a) {
    __m256i r;
    uint8x8_t u8 = vget_low_u8(AS_U8(a));
    uint16x8_t u16 = vmovl_u8(u8);
    r.lo = FR_U32(vmovl_u16(vget_low_u16(u16)));
    r.hi = FR_U32(vmovl_u16(vget_high_u16(u16)));
    return r;
}
static inline __m256i _mm256_cvtepi16_epi32(__m128i a) {
    __m256i r;
    int16x8_t v = AS_S16(a);
    r.lo = FR_S32(vmovl_s16(vget_low_s16(v)));
    r.hi = FR_S32(vmovl_s16(vget_high_s16(v)));
    return r;
}

/* ================================================================== */
/*  128 <-> 256 EXTRACT / CAST / INSERT                                */
/* ================================================================== */
static inline __m128i _mm256_castsi256_si128(__m256i a) { return a.lo; }
static inline __m128i _mm256_extracti128_si256(__m256i a, int imm) {
    return imm ? a.hi : a.lo;
}
static inline __m256i _mm256_castsi128_si256(__m128i a) {
    __m256i r; r.lo=a; r.hi=vdupq_n_s8(0); return r;
}
static inline __m256i _mm256_inserti128_si256(__m256i a, __m128i b, int imm) {
    __m256i r=a; if(imm) r.hi=b; else r.lo=b; return r;
}

/* ================================================================== */
/*  PERMUTE / SHUFFLE                                                  */
/* ================================================================== */
static inline __m256i _mm256_permute2x128_si256(__m256i a, __m256i b, int imm) {
    __m256i r;
    __m128i src[4] = { a.lo, a.hi, b.lo, b.hi };
    r.lo = (imm & 0x08) ? vdupq_n_s8(0) : src[imm & 0x3];
    r.hi = (imm & 0x80) ? vdupq_n_s8(0) : src[(imm >> 4) & 0x3];
    return r;
}
static inline __m256i _mm256_permute4x64_epi64(__m256i a, int imm) {
    __m256i r;
    int64_t e[4];
    vst1q_s64(e+0, AS_S64(a.lo));
    vst1q_s64(e+2, AS_S64(a.hi));
    int64_t o[4];
    o[0] = e[(imm>>0)&3]; o[1] = e[(imm>>2)&3];
    o[2] = e[(imm>>4)&3]; o[3] = e[(imm>>6)&3];
    r.lo = FR_S64(vld1q_s64(o+0));
    r.hi = FR_S64(vld1q_s64(o+2));
    return r;
}
static inline __m256i _mm256_permutevar8x32_epi32(__m256i a, __m256i idx) {
    __m256i r;
    int32_t src[8], ind[8], out[8];
    vst1q_s32(src+0, AS_S32(a.lo));   vst1q_s32(src+4, AS_S32(a.hi));
    vst1q_s32(ind+0, AS_S32(idx.lo)); vst1q_s32(ind+4, AS_S32(idx.hi));
    for (int i=0;i<8;i++) out[i] = src[ind[i]&7];
    r.lo = FR_S32(vld1q_s32(out+0));
    r.hi = FR_S32(vld1q_s32(out+4));
    return r;
}
/* shuffle_epi8 (PSHUFB): per-128-bit-lane byte shuffle */
static inline __m256i _mm256_shuffle_epi8(__m256i a, __m256i b) {
    __m256i r;
    uint8x16_t alo = AS_U8(a.lo), ahi = AS_U8(a.hi);
    uint8x16_t blo = AS_U8(b.lo), bhi = AS_U8(b.hi);
    uint8x16_t idx_lo = vandq_u8(blo, vdupq_n_u8(0x0F));
    uint8x16_t idx_hi = vandq_u8(bhi, vdupq_n_u8(0x0F));
    uint8x16_t rlo = vqtbl1q_u8(alo, idx_lo);
    uint8x16_t rhi = vqtbl1q_u8(ahi, idx_hi);
    /* Zero where high bit of index byte is set */
    int8x16_t mlo = vshrq_n_s8(vreinterpretq_s8_u8(blo), 7);
    int8x16_t mhi = vshrq_n_s8(vreinterpretq_s8_u8(bhi), 7);
    rlo = vbicq_u8(rlo, vreinterpretq_u8_s8(mlo));
    rhi = vbicq_u8(rhi, vreinterpretq_u8_s8(mhi));
    r.lo = FR_U8(rlo); r.hi = FR_U8(rhi);
    return r;
}

/* ================================================================== */
/*  BLEND                                                              */
/* ================================================================== */
static inline __m256i _mm256_blend_epi16(__m256i a, __m256i b, int imm) {
    __m256i r;
    uint16_t av[16], bv[16];
    memcpy(av, &a, 32); memcpy(bv, &b, 32);
    for (int i=0;i<8;i++) {
        if (imm & (1<<i)) { av[i] = bv[i]; av[i+8] = bv[i+8]; }
    }
    memcpy(&r, av, 32);
    return r;
}
static inline __m256i _mm256_blend_epi32(__m256i a, __m256i b, int imm) {
    __m256i r;
    int32_t av[8], bv[8];
    memcpy(av, &a, 32); memcpy(bv, &b, 32);
    for (int i=0;i<8;i++) if (imm & (1<<i)) av[i] = bv[i];
    memcpy(&r, av, 32);
    return r;
}

/* ================================================================== */
/*  BROADCAST                                                          */
/* ================================================================== */
static inline __m256i _mm256_broadcastd_epi32(__m128i a) {
    __m256i r;
    r.lo = r.hi = FR_S32(vdupq_laneq_s32(AS_S32(a), 0));
    return r;
}
static inline __m256i _mm256_broadcastq_epi64(__m128i a) {
    __m256i r;
    r.lo = r.hi = FR_S64(vdupq_laneq_s64(AS_S64(a), 0));
    return r;
}

/* ================================================================== */
/*  MOVEMASK / TESTZ                                                   */
/* ================================================================== */
static inline int _mm256_movemask_epi8(__m256i a) {
    uint8_t buf[32];
    vst1q_u8(buf,    AS_U8(a.lo));
    vst1q_u8(buf+16, AS_U8(a.hi));
    int r = 0;
    for (int i=0;i<32;i++) r |= ((buf[i]>>7)<<i);
    return r;
}
static inline int _mm256_testz_si256(__m256i a, __m256i b) {
    __m256i c = _mm256_and_si256(a, b);
    return vmaxvq_u8(vorrq_u8(AS_U8(c.lo), AS_U8(c.hi))) == 0 ? 1 : 0;
}

/* ================================================================== */
/*  SSE2 128-bit helpers                                               */
/* ================================================================== */
static inline __m128i _mm_cvtsi64_si128(int64_t a) {
    int64x2_t v = vdupq_n_s64(0);
    v = vsetq_lane_s64(a, v, 0);
    return FR_S64(v);
}
/* byte shift right — requires compile-time constant */
#define _mm_srli_si128(a, imm) \
    ( (imm) >= 16 ? vdupq_n_s8(0) : \
      FR_U8(vextq_u8(AS_U8(a), vdupq_n_u8(0), (imm))) )

/* ================================================================== */
/*  Keccak-specific helpers                                            */
/* ================================================================== */

/* _mm256_shuffle_pd equivalent for integer types */
static inline __m256i _mm256_shuffle_pd_epi64(__m256i a, __m256i b, int imm) {
    __m256i r;
    int64_t al[2], ah[2], bl[2], bh[2], rl[2], rh[2];
    vst1q_s64(al, AS_S64(a.lo)); vst1q_s64(ah, AS_S64(a.hi));
    vst1q_s64(bl, AS_S64(b.lo)); vst1q_s64(bh, AS_S64(b.hi));
    rl[0] = (imm & 1) ? al[1] : al[0];
    rl[1] = (imm & 2) ? bl[1] : bl[0];
    rh[0] = (imm & 4) ? ah[1] : ah[0];
    rh[1] = (imm & 8) ? bh[1] : bh[0];
    r.lo = FR_S64(vld1q_s64(rl)); r.hi = FR_S64(vld1q_s64(rh));
    return r;
}

/* Macro aliases so Keccak code compiles unchanged */
#define _mm256_shuffle_pd(a, b, c)       _mm256_shuffle_pd_epi64(a, b, c)
#define _mm256_permute2f128_ps(a, b, c)  _mm256_permute2x128_si256(a, b, c)

/* broadcast_sd: broadcast one double (=uint64_t) to all 4 64-bit lanes */
static inline __m256i _mm256_broadcast_sd_impl(const void *p) {
    __m256i r; uint64_t v; memcpy(&v, p, 8);
    r.lo = r.hi = FR_U64(vdupq_n_u64(v));
    return r;
}
#define _mm256_broadcast_sd(p) _mm256_broadcast_sd_impl(p)

/* ================================================================== */
/*  BMI2 scalar fallbacks                                              */
/* ================================================================== */
static inline uint64_t _pdep_u64(uint64_t val, uint64_t mask) {
    uint64_t res = 0;
    for (uint64_t bb = 1; mask; bb += bb) {
        if (val & bb) res |= mask & (uint64_t)(-(int64_t)mask);
        mask &= mask - 1;
    }
    return res;
}
static inline uint64_t _pext_u64(uint64_t val, uint64_t mask) {
    uint64_t res = 0, bb = 1;
    while (mask) {
        if (val & (mask & (uint64_t)(-(int64_t)mask))) res |= bb;
        mask &= mask - 1; bb += bb;
    }
    return res;
}
static inline uint32_t _pext_u32(uint32_t val, uint32_t mask) {
    return (uint32_t)_pext_u64(val, mask);
}
static inline int _mm_popcnt_u32(uint32_t a) {
    return __builtin_popcount(a);
}

#endif /* AVX2_NEON_H */
