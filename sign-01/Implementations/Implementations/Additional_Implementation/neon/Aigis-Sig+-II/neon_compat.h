/*
 * neon_compat.h — AVX2 intrinsics emulation on ARM NEON (AArch64)
 *
 * __m256i = struct { int32x4_t lo, hi; }  (two 128-bit halves)
 * sizeof(__m256i) = 32 bytes, identical to x86.
 * Memory layout: lo at lower address, hi at higher — matches x86 lane order.
 *
 * Guarantees bit-exact output vs x86 AVX2 for KAT correctness.
 */
#ifndef NEON_COMPAT_H
#define NEON_COMPAT_H

#include <arm_neon.h>
#include <stdint.h>
#include <string.h>

/* ────────── Types ────────── */
typedef struct { int32x4_t lo, hi; } __m256i;
typedef int32x4_t __m128i;

/* reinterpret-cast helpers (zero-cost on all compilers) */
#define _V8(x)    vreinterpretq_u8_s32(x)
#define _S8(x)    vreinterpretq_s8_s32(x)
#define _V16(x)   vreinterpretq_u16_s32(x)
#define _S16(x)   vreinterpretq_s16_s32(x)
#define _V32(x)   vreinterpretq_u32_s32(x)
#define _V64(x)   vreinterpretq_u64_s32(x)
#define _S64(x)   vreinterpretq_s64_s32(x)
#define _FU8(x)   vreinterpretq_s32_u8(x)
#define _FS8(x)   vreinterpretq_s32_s8(x)
#define _FU16(x)  vreinterpretq_s32_u16(x)
#define _FS16(x)  vreinterpretq_s32_s16(x)
#define _FU32(x)  vreinterpretq_s32_u32(x)
#define _FU64(x)  vreinterpretq_s32_u64(x)
#define _FS64(x)  vreinterpretq_s32_s64(x)

/* ────────── Load / Store ────────── */
static inline __m256i _mm256_loadu_si256(const void *p) {
    __m256i r;
    const int32_t *q = (const int32_t *)p;
    r.lo = vld1q_s32(q);
    r.hi = vld1q_s32(q + 4);
    return r;
}
static inline __m256i _mm256_load_si256(const void *p) {
    return _mm256_loadu_si256(p);
}
static inline void _mm256_storeu_si256(void *p, __m256i a) {
    int32_t *q = (int32_t *)p;
    vst1q_s32(q,     a.lo);
    vst1q_s32(q + 4, a.hi);
}
static inline void _mm256_store_si256(void *p, __m256i a) {
    _mm256_storeu_si256(p, a);
}

static inline __m128i _mm_loadu_si128(const void *p) {
    return vld1q_s32((const int32_t *)p);
}
static inline __m128i _mm_load_si128(const void *p) {
    return vld1q_s32((const int32_t *)p);
}
static inline void _mm_storeu_si128(void *p, __m128i a) {
    vst1q_s32((int32_t *)p, a);
}
static inline void _mm_store_si128(void *p, __m128i a) {
    vst1q_s32((int32_t *)p, a);
}
static inline void _mm_storel_epi64(void *p, __m128i a) {
    vst1_s32((int32_t *)p, vget_low_s32(a));
}

/* ────────── Broadcast / Set ────────── */
static inline __m256i _mm256_setzero_si256(void) {
    __m256i r; r.lo = r.hi = vdupq_n_s32(0); return r;
}
static inline __m256i _mm256_set1_epi8(int8_t x) {
    __m256i r; r.lo = r.hi = _FS8(vdupq_n_s8(x)); return r;
}
static inline __m256i _mm256_set1_epi16(int16_t x) {
    __m256i r; r.lo = r.hi = _FS16(vdupq_n_s16(x)); return r;
}
static inline __m256i _mm256_set1_epi32(int32_t x) {
    __m256i r; r.lo = r.hi = vdupq_n_s32(x); return r;
}
static inline __m256i _mm256_set1_epi64x(int64_t x) {
    __m256i r; r.lo = r.hi = _FS64(vdupq_n_s64(x)); return r;
}
static inline __m128i _mm_set1_epi32(int32_t x) { return vdupq_n_s32(x); }
static inline __m128i _mm_set1_epi64x(int64_t x) { return _FS64(vdupq_n_s64(x)); }

/* set: e7=highest … e0=lowest */
static inline __m256i _mm256_set_epi32(int e7,int e6,int e5,int e4,
                                       int e3,int e2,int e1,int e0) {
    __m256i r;
    int32_t lo[4]={e0,e1,e2,e3}, hi[4]={e4,e5,e6,e7};
    r.lo = vld1q_s32(lo); r.hi = vld1q_s32(hi);
    return r;
}
static inline __m256i _mm256_set_epi64x(int64_t e3,int64_t e2,int64_t e1,int64_t e0){
    __m256i r;
    int64_t lo[2]={e0,e1}, hi[2]={e2,e3};
    r.lo = _FS64(vld1q_s64(lo)); r.hi = _FS64(vld1q_s64(hi));
    return r;
}
static inline __m256i _mm256_set_epi8(
    int8_t b31,int8_t b30,int8_t b29,int8_t b28,int8_t b27,int8_t b26,int8_t b25,int8_t b24,
    int8_t b23,int8_t b22,int8_t b21,int8_t b20,int8_t b19,int8_t b18,int8_t b17,int8_t b16,
    int8_t b15,int8_t b14,int8_t b13,int8_t b12,int8_t b11,int8_t b10,int8_t b9, int8_t b8,
    int8_t b7, int8_t b6, int8_t b5, int8_t b4, int8_t b3, int8_t b2, int8_t b1, int8_t b0)
{
    __m256i r;
    int8_t lo[16]={b0,b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15};
    int8_t hi[16]={b16,b17,b18,b19,b20,b21,b22,b23,b24,b25,b26,b27,b28,b29,b30,b31};
    r.lo = _FS8(vld1q_s8(lo)); r.hi = _FS8(vld1q_s8(hi));
    return r;
}
static inline __m256i _mm256_setr_epi8(
    int8_t b0, int8_t b1, int8_t b2, int8_t b3, int8_t b4, int8_t b5, int8_t b6, int8_t b7,
    int8_t b8, int8_t b9, int8_t b10,int8_t b11,int8_t b12,int8_t b13,int8_t b14,int8_t b15,
    int8_t b16,int8_t b17,int8_t b18,int8_t b19,int8_t b20,int8_t b21,int8_t b22,int8_t b23,
    int8_t b24,int8_t b25,int8_t b26,int8_t b27,int8_t b28,int8_t b29,int8_t b30,int8_t b31)
{
    return _mm256_set_epi8(b31,b30,b29,b28,b27,b26,b25,b24,
                           b23,b22,b21,b20,b19,b18,b17,b16,
                           b15,b14,b13,b12,b11,b10,b9,b8,
                           b7,b6,b5,b4,b3,b2,b1,b0);
}

/* ────────── Integer Arithmetic (32-bit) ────────── */
static inline __m256i _mm256_add_epi32(__m256i a,__m256i b){
    __m256i r; r.lo=vaddq_s32(a.lo,b.lo); r.hi=vaddq_s32(a.hi,b.hi); return r;
}
static inline __m256i _mm256_sub_epi32(__m256i a,__m256i b){
    __m256i r; r.lo=vsubq_s32(a.lo,b.lo); r.hi=vsubq_s32(a.hi,b.hi); return r;
}
static inline __m256i _mm256_mullo_epi32(__m256i a,__m256i b){
    __m256i r; r.lo=vmulq_s32(a.lo,b.lo); r.hi=vmulq_s32(a.hi,b.hi); return r;
}
static inline __m256i _mm256_abs_epi32(__m256i a){
    __m256i r; r.lo=vabsq_s32(a.lo); r.hi=vabsq_s32(a.hi); return r;
}
static inline __m256i _mm256_cmpgt_epi32(__m256i a,__m256i b){
    __m256i r; r.lo=_FU32(vcgtq_s32(a.lo,b.lo)); r.hi=_FU32(vcgtq_s32(a.hi,b.hi)); return r;
}
static inline __m256i _mm256_cmpeq_epi32(__m256i a,__m256i b){
    __m256i r; r.lo=_FU32(vceqq_s32(a.lo,b.lo)); r.hi=_FU32(vceqq_s32(a.hi,b.hi)); return r;
}
static inline __m128i _mm_cmpgt_epi32(__m128i a,__m128i b){ return _FU32(vcgtq_s32(a,b)); }
static inline __m128i _mm_sub_epi32(__m128i a,__m128i b){ return vsubq_s32(a,b); }

/* ────────── Integer Arithmetic (64-bit) ────────── */
static inline __m256i _mm256_add_epi64(__m256i a,__m256i b){
    __m256i r;
    r.lo=_FS64(vaddq_s64(_S64(a.lo),_S64(b.lo)));
    r.hi=_FS64(vaddq_s64(_S64(a.hi),_S64(b.hi)));
    return r;
}
static inline __m256i _mm256_sub_epi64(__m256i a,__m256i b){
    __m256i r;
    r.lo=_FS64(vsubq_s64(_S64(a.lo),_S64(b.lo)));
    r.hi=_FS64(vsubq_s64(_S64(a.hi),_S64(b.hi)));
    return r;
}
/* mul EVEN 32-bit elements → 64-bit results (low 32 of each 64-bit lane) */
static inline __m256i _mm256_mul_epi32(__m256i a,__m256i b){
    __m256i r;
    int32x2_t al=vmovn_s64(_S64(a.lo)), bl=vmovn_s64(_S64(b.lo));
    int32x2_t ah=vmovn_s64(_S64(a.hi)), bh=vmovn_s64(_S64(b.hi));
    r.lo=_FS64(vmull_s32(al,bl));
    r.hi=_FS64(vmull_s32(ah,bh));
    return r;
}
static inline __m128i _mm_add_epi64(__m128i a,__m128i b){
    return _FS64(vaddq_s64(_S64(a),_S64(b)));
}

/* ────────── Integer Arithmetic (16-bit, 8-bit) ────────── */
static inline __m256i _mm256_add_epi16(__m256i a,__m256i b){
    __m256i r; r.lo=_FS16(vaddq_s16(_S16(a.lo),_S16(b.lo)));
    r.hi=_FS16(vaddq_s16(_S16(a.hi),_S16(b.hi))); return r;
}
static inline __m256i _mm256_add_epi8(__m256i a,__m256i b){
    __m256i r; r.lo=_FS8(vaddq_s8(_S8(a.lo),_S8(b.lo)));
    r.hi=_FS8(vaddq_s8(_S8(a.hi),_S8(b.hi))); return r;
}
static inline __m256i _mm256_sub_epi8(__m256i a,__m256i b){
    __m256i r; r.lo=_FS8(vsubq_s8(_S8(a.lo),_S8(b.lo)));
    r.hi=_FS8(vsubq_s8(_S8(a.hi),_S8(b.hi))); return r;
}
static inline __m256i _mm256_sub_epi16(__m256i a,__m256i b){
    __m256i r; r.lo=_FS16(vsubq_s16(_S16(a.lo),_S16(b.lo)));
    r.hi=_FS16(vsubq_s16(_S16(a.hi),_S16(b.hi))); return r;
}

/* ────────── Shifts ────────── */
static inline __m256i _mm256_slli_epi32(__m256i a,int n){
    __m256i r; int32x4_t sh=vdupq_n_s32(n);
    r.lo=vshlq_s32(a.lo,sh); r.hi=vshlq_s32(a.hi,sh); return r;
}
static inline __m256i _mm256_srai_epi32(__m256i a,int n){
    __m256i r; int32x4_t sh=vdupq_n_s32(-n);
    r.lo=vshlq_s32(a.lo,sh); r.hi=vshlq_s32(a.hi,sh); return r;
}
static inline __m256i _mm256_srli_epi32(__m256i a,int n){
    __m256i r; int32x4_t sh=vdupq_n_s32(-n);
    r.lo=_FU32(vshlq_u32(_V32(a.lo),sh)); r.hi=_FU32(vshlq_u32(_V32(a.hi),sh)); return r;
}
static inline __m256i _mm256_slli_epi64(__m256i a,int n){
    __m256i r; int64x2_t sh=vdupq_n_s64(n);
    r.lo=_FS64(vshlq_s64(_S64(a.lo),sh)); r.hi=_FS64(vshlq_s64(_S64(a.hi),sh)); return r;
}
static inline __m256i _mm256_srli_epi64(__m256i a,int n){
    __m256i r; int64x2_t sh=vdupq_n_s64(-n);
    r.lo=_FU64(vshlq_u64(_V64(a.lo),sh)); r.hi=_FU64(vshlq_u64(_V64(a.hi),sh)); return r;
}
static inline __m256i _mm256_srli_epi16(__m256i a,int n){
    __m256i r; int16x8_t sh=vdupq_n_s16(-n);
    r.lo=_FU16(vshlq_u16(_V16(a.lo),sh)); r.hi=_FU16(vshlq_u16(_V16(a.hi),sh)); return r;
}
/* Variable shift (per-element) */
static inline __m256i _mm256_srlv_epi32(__m256i a,__m256i cnt){
    __m256i r;
    r.lo=_FU32(vshlq_u32(_V32(a.lo),vnegq_s32(cnt.lo)));
    r.hi=_FU32(vshlq_u32(_V32(a.hi),vnegq_s32(cnt.hi)));
    return r;
}
static inline __m128i _mm_srlv_epi32(__m128i a,__m128i cnt){
    return _FU32(vshlq_u32(_V32(a),vnegq_s32(cnt)));
}

/* ────────── Bitwise ────────── */
static inline __m256i _mm256_and_si256(__m256i a,__m256i b){
    __m256i r; r.lo=vandq_s32(a.lo,b.lo); r.hi=vandq_s32(a.hi,b.hi); return r;
}
static inline __m256i _mm256_or_si256(__m256i a,__m256i b){
    __m256i r; r.lo=vorrq_s32(a.lo,b.lo); r.hi=vorrq_s32(a.hi,b.hi); return r;
}
static inline __m256i _mm256_xor_si256(__m256i a,__m256i b){
    __m256i r; r.lo=veorq_s32(a.lo,b.lo); r.hi=veorq_s32(a.hi,b.hi); return r;
}
static inline __m256i _mm256_andnot_si256(__m256i a,__m256i b){
    __m256i r; r.lo=vbicq_s32(b.lo,a.lo); r.hi=vbicq_s32(b.hi,a.hi); return r;
}
static inline __m128i _mm_and_si128(__m128i a,__m128i b){ return vandq_s32(a,b); }
static inline __m128i _mm_or_si128(__m128i a,__m128i b) { return vorrq_s32(a,b); }

/* ────────── Extract / Cast ────────── */
static inline __m128i _mm256_castsi256_si128(__m256i a){ return a.lo; }
static inline __m128i _mm256_extracti128_si256(__m256i a,int imm){ return imm?a.hi:a.lo; }
static inline __m128i _mm256_extractf128_si256(__m256i a,int imm){ return imm?a.hi:a.lo; }
static inline int32_t _mm_cvtsi128_si32(__m128i a){ return vgetq_lane_s32(a,0); }
static inline int32_t _mm_extract_epi32(__m128i a,int imm){
    int32_t t[4]; vst1q_s32(t,a); return t[imm];
}

/* ────────── Sign / Zero extend ────────── */
static inline __m256i _mm256_cvtepi32_epi64(__m128i a){
    __m256i r;
    r.lo=_FS64(vmovl_s32(vget_low_s32(a)));
    r.hi=_FS64(vmovl_s32(vget_high_s32(a)));
    return r;
}
static inline __m256i _mm256_cvtepi8_epi32(__m128i a){
    __m256i r;
    int16x8_t w=vmovl_s8(vget_low_s8(_S8(a)));
    r.lo=vmovl_s16(vget_low_s16(w));
    r.hi=vmovl_s16(vget_high_s16(w));
    return r;
}
static inline __m256i _mm256_cvtepi16_epi32(__m128i a){
    __m256i r;
    r.lo=vmovl_s16(vget_low_s16(_S16(a)));
    r.hi=vmovl_s16(vget_high_s16(_S16(a)));
    return r;
}
static inline __m256i _mm256_cvtepi8_epi16(__m128i a){
    __m256i r;
    r.lo=_FS16(vmovl_s8(vget_low_s8(_S8(a))));
    r.hi=_FS16(vmovl_s8(vget_high_s8(_S8(a))));
    return r;
}

/* ────────── Pack (with saturation) ────────── */
/* AVX2 packing is per-128-bit-lane: pack(a.lo,b.lo) → r.lo, pack(a.hi,b.hi) → r.hi */
static inline __m256i _mm256_packs_epi32(__m256i a,__m256i b){
    __m256i r;
    r.lo=_FS16(vcombine_s16(vqmovn_s32(a.lo),vqmovn_s32(b.lo)));
    r.hi=_FS16(vcombine_s16(vqmovn_s32(a.hi),vqmovn_s32(b.hi)));
    return r;
}
static inline __m256i _mm256_packs_epi16(__m256i a,__m256i b){
    __m256i r;
    r.lo=_FS8(vcombine_s8(vqmovn_s16(_S16(a.lo)),vqmovn_s16(_S16(b.lo))));
    r.hi=_FS8(vcombine_s8(vqmovn_s16(_S16(a.hi)),vqmovn_s16(_S16(b.hi))));
    return r;
}
static inline __m256i _mm256_packus_epi32(__m256i a,__m256i b){
    __m256i r;
    r.lo=_FU16(vcombine_u16(vqmovun_s32(a.lo),vqmovun_s32(b.lo)));
    r.hi=_FU16(vcombine_u16(vqmovun_s32(a.hi),vqmovun_s32(b.hi)));
    return r;
}
static inline __m256i _mm256_packus_epi16(__m256i a,__m256i b){
    __m256i r;
    r.lo=_FU8(vcombine_u8(vqmovun_s16(_S16(a.lo)),vqmovun_s16(_S16(b.lo))));
    r.hi=_FU8(vcombine_u8(vqmovun_s16(_S16(a.hi)),vqmovun_s16(_S16(b.hi))));
    return r;
}

/* ────────── Unpack (interleave) ────────── */
static inline __m256i _mm256_unpacklo_epi8(__m256i a,__m256i b){
    __m256i r; r.lo=_FS8(vzip1q_s8(_S8(a.lo),_S8(b.lo)));
    r.hi=_FS8(vzip1q_s8(_S8(a.hi),_S8(b.hi))); return r;
}
static inline __m256i _mm256_unpackhi_epi8(__m256i a,__m256i b){
    __m256i r; r.lo=_FS8(vzip2q_s8(_S8(a.lo),_S8(b.lo)));
    r.hi=_FS8(vzip2q_s8(_S8(a.hi),_S8(b.hi))); return r;
}
static inline __m256i _mm256_unpacklo_epi16(__m256i a,__m256i b){
    __m256i r; r.lo=_FS16(vzip1q_s16(_S16(a.lo),_S16(b.lo)));
    r.hi=_FS16(vzip1q_s16(_S16(a.hi),_S16(b.hi))); return r;
}
static inline __m256i _mm256_unpackhi_epi16(__m256i a,__m256i b){
    __m256i r; r.lo=_FS16(vzip2q_s16(_S16(a.lo),_S16(b.lo)));
    r.hi=_FS16(vzip2q_s16(_S16(a.hi),_S16(b.hi))); return r;
}
static inline __m256i _mm256_unpacklo_epi64(__m256i a,__m256i b){
    __m256i r; r.lo=_FS64(vzip1q_s64(_S64(a.lo),_S64(b.lo)));
    r.hi=_FS64(vzip1q_s64(_S64(a.hi),_S64(b.hi))); return r;
}
static inline __m256i _mm256_unpackhi_epi64(__m256i a,__m256i b){
    __m256i r; r.lo=_FS64(vzip2q_s64(_S64(a.lo),_S64(b.lo)));
    r.hi=_FS64(vzip2q_s64(_S64(a.hi),_S64(b.hi))); return r;
}
static inline __m128i _mm_unpacklo_epi8(__m128i a,__m128i b){
    return _FS8(vzip1q_s8(_S8(a),_S8(b)));
}
static inline __m128i _mm_unpackhi_epi8(__m128i a,__m128i b){
    return _FS8(vzip2q_s8(_S8(a),_S8(b)));
}

/* ────────── Shuffle / Permute ────────── */
/* Per-lane byte shuffle (like vtbl). High bit of index → zero result byte. */
static inline __m128i _mm_shuffle_epi8(__m128i a,__m128i idx){
    uint8x16_t hi = vdupq_n_u8(0x80);
    uint8x16_t i8 = _V8(idx);
    uint8x16_t neg = vcgeq_u8(i8, hi);
    return _FU8(vbicq_u8(vqtbl1q_u8(_V8(a), vandq_u8(i8, vdupq_n_u8(0x0f))), neg));
}
static inline __m256i _mm256_shuffle_epi8(__m256i a,__m256i idx){
    __m256i r;
    r.lo = _mm_shuffle_epi8(a.lo, idx.lo);
    r.hi = _mm_shuffle_epi8(a.hi, idx.hi);
    return r;
}

/* Cross-lane 32-bit variable permute */
static inline __m256i _mm256_permutevar8x32_epi32(__m256i a,__m256i idx){
    __m256i r;
    int32_t s[8],ix[8],d[8];
    vst1q_s32(s,a.lo); vst1q_s32(s+4,a.hi);
    vst1q_s32(ix,idx.lo); vst1q_s32(ix+4,idx.hi);
    for(int i=0;i<8;i++) d[i]=s[ix[i]&7];
    r.lo=vld1q_s32(d); r.hi=vld1q_s32(d+4);
    return r;
}

/* Per-lane 32-bit shuffle (same control for both lanes) */
static inline __m256i _mm256_shuffle_epi32_x(__m256i a,const int imm){
    __m256i r;
    int32_t ls[4],hs[4],ld[4],hd[4];
    vst1q_s32(ls,a.lo); vst1q_s32(hs,a.hi);
    ld[0]=ls[(imm>>0)&3]; ld[1]=ls[(imm>>2)&3]; ld[2]=ls[(imm>>4)&3]; ld[3]=ls[(imm>>6)&3];
    hd[0]=hs[(imm>>0)&3]; hd[1]=hs[(imm>>2)&3]; hd[2]=hs[(imm>>4)&3]; hd[3]=hs[(imm>>6)&3];
    r.lo=vld1q_s32(ld); r.hi=vld1q_s32(hd);
    return r;
}
#define _mm256_shuffle_epi32(a,imm) _mm256_shuffle_epi32_x(a,imm)

/* Select 128-bit halves */
static inline __m256i _mm256_permute2x128_si256(__m256i a,__m256i b,int imm){
    __m256i r;
    int32x4_t p[4]={a.lo,a.hi,b.lo,b.hi};
    r.lo=p[imm&3]; r.hi=p[(imm>>4)&3];
    return r;
}

/* Permute 64-bit elements across full width */
static inline __m256i _mm256_permute4x64_epi64_x(__m256i a,const int imm){
    __m256i r;
    int64_t e[4],d[4];
    memcpy(e,&a,32);
    d[0]=e[(imm>>0)&3]; d[1]=e[(imm>>2)&3]; d[2]=e[(imm>>4)&3]; d[3]=e[(imm>>6)&3];
    memcpy(&r,d,32);
    return r;
}
#define _mm256_permute4x64_epi64(a,imm) _mm256_permute4x64_epi64_x(a,imm)

/* Blend 32-bit elements by immediate mask */
static inline __m256i _mm256_blend_epi32_x(__m256i a,__m256i b,const int imm){
    __m256i r;
    int32_t ra[8],rb[8],rd[8];
    vst1q_s32(ra,a.lo); vst1q_s32(ra+4,a.hi);
    vst1q_s32(rb,b.lo); vst1q_s32(rb+4,b.hi);
    for(int i=0;i<8;i++) rd[i]=((imm>>i)&1)?rb[i]:ra[i];
    r.lo=vld1q_s32(rd); r.hi=vld1q_s32(rd+4);
    return r;
}
#define _mm256_blend_epi32(a,b,imm) _mm256_blend_epi32_x(a,b,imm)

/* Byte shift right per 128-bit lane */
static inline __m256i _mm256_bsrli_epi128_x(__m256i a,const int n){
    __m256i r;
    uint8_t tl[16],th[16],rl[16]={0},rh[16]={0};
    vst1q_u8(tl,_V8(a.lo)); vst1q_u8(th,_V8(a.hi));
    if(n<16) { memcpy(rl,tl+n,16-n); memcpy(rh,th+n,16-n); }
    r.lo=_FU8(vld1q_u8(rl)); r.hi=_FU8(vld1q_u8(rh));
    return r;
}
#define _mm256_bsrli_epi128(a,n) _mm256_bsrli_epi128_x(a,n)

/* ────────── Movemask ────────── */
static inline int _mm256_movemask_epi8(__m256i a){
    /* Optimised: extract MSB of each byte using NEON shifts */
    static const uint8_t mask_bits[16]={1,2,4,8,16,32,64,128,1,2,4,8,16,32,64,128};
    uint8x16_t mb = vld1q_u8(mask_bits);
    uint8x16_t lo_msb = vshrq_n_u8(_V8(a.lo), 7);
    uint8x16_t hi_msb = vshrq_n_u8(_V8(a.hi), 7);
    lo_msb = vmulq_u8(lo_msb, mb);
    hi_msb = vmulq_u8(hi_msb, mb);
    /* Sum pairs to get 8-bit chunks */
    uint8_t tl[16], th[16];
    vst1q_u8(tl, lo_msb); vst1q_u8(th, hi_msb);
    int mask = 0;
    mask |= (tl[0]|tl[1]|tl[2]|tl[3]|tl[4]|tl[5]|tl[6]|tl[7]);
    mask |= (tl[8]|tl[9]|tl[10]|tl[11]|tl[12]|tl[13]|tl[14]|tl[15]) << 8;
    mask |= (th[0]|th[1]|th[2]|th[3]|th[4]|th[5]|th[6]|th[7]) << 16;
    mask |= (th[8]|th[9]|th[10]|th[11]|th[12]|th[13]|th[14]|th[15]) << 24;
    return mask;
}

static inline int _mm256_movemask_ps_emu(__m256i a){
    int32_t t[8]; vst1q_s32(t,a.lo); vst1q_s32(t+4,a.hi);
    int m=0;
    for(int i=0;i<8;i++) if(t[i]<0) m|=(1<<i);
    return m;
}
static inline int _mm_movemask_ps_emu(__m128i a){
    int32_t t[4]; vst1q_s32(t,a);
    int m=0;
    for(int i=0;i<4;i++) if(t[i]<0) m|=(1<<i);
    return m;
}

/* "Cast" macros — identity, since we unify all types as __m256i / __m128i */
#define _mm256_castsi256_ps(x) (x)
#define _mm256_castps_si256(x) (x)
#define _mm_castsi128_ps(x) (x)
#define _mm_castps_si128(x) (x)
#define _mm256_movemask_ps(x) _mm256_movemask_ps_emu(x)
#define _mm_movemask_ps(x) _mm_movemask_ps_emu(x)

static inline int _mm_popcnt_u32(unsigned int x){ return __builtin_popcount(x); }

/* SSE per-element variable permute (used as _mm_permutevar_ps in sample.c) */
static inline __m128i _mm_permutevar_ps_emu(__m128i a,__m128i idx){
    int32_t s[4],ix[4],d[4];
    vst1q_s32(s,a); vst1q_s32(ix,idx);
    for(int i=0;i<4;i++) d[i]=s[ix[i]&3];
    return vld1q_s32(d);
}
#define _mm_permutevar_ps(a,idx) _mm_permutevar_ps_emu(a,idx)

/* ────────── SAD ────────── */
static inline __m256i _mm256_sad_epu8(__m256i a,__m256i b){
    __m256i r;
    /* NEON: compute abs diff and horizontal add per 8-byte group */
    uint8x16_t ad_lo = vabdq_u8(_V8(a.lo), _V8(b.lo));
    uint8x16_t ad_hi = vabdq_u8(_V8(a.hi), _V8(b.hi));
    /* Sum each 8-byte half into a 64-bit value */
    uint16x8_t p_lo = vpaddlq_u8(ad_lo);
    uint32x4_t q_lo = vpaddlq_u16(p_lo);
    uint64x2_t s_lo = vpaddlq_u32(q_lo);  /* not quite right — need per-8-byte sum */
    uint16x8_t p_hi = vpaddlq_u8(ad_hi);
    uint32x4_t q_hi = vpaddlq_u16(p_hi);
    uint64x2_t s_hi = vpaddlq_u32(q_hi);
    /* Each 64-bit lane should be the sum of 8 abs-diffs.
       vpaddlq cascades correctly: u8→u16(pairwise) →u32→u64.
       But vpaddlq_u32 sums pairs of u32 into u64, where each u32 was sum of 4 bytes.
       So each u64 lane = sum of 8 bytes. ✓ */
    r.lo = _FU64(s_lo);
    r.hi = _FU64(s_hi);
    return r;
}

/* ────────── Keccak support: broadcast 64-bit (used as _mm256_broadcast_sd) ────────── */
static inline __m256i _mm256_broadcast_sd_emu(const void *p){
    int64_t v; memcpy(&v,p,8);
    return _mm256_set1_epi64x(v);
}
#define _mm256_broadcast_sd(p) _mm256_broadcast_sd_emu(p)
#define _mm256_castpd_si256(x) (x)
#define _mm256_castsi256_pd(x) (x)

/* shuffle_pd: shuffle 64-bit elements with per-lane control */
static inline __m256i _mm256_shuffle_pd_emu(__m256i a,__m256i b,int imm){
    __m256i r;
    int64_t aa[4],bb[4],rr[4];
    memcpy(aa,&a,32); memcpy(bb,&b,32);
    rr[0]=(imm&1)?aa[1]:aa[0];
    rr[1]=(imm&2)?bb[1]:bb[0];
    rr[2]=(imm&4)?aa[3]:aa[2];
    rr[3]=(imm&8)?bb[3]:bb[2];
    memcpy(&r,rr,32);
    return r;
}
#define _mm256_shuffle_pd(a,b,imm) _mm256_shuffle_pd_emu(a,b,imm)

/* permute2f128: same as permute2x128 (identity cast from ps) */
#define _mm256_permute2f128_ps(a,b,c) _mm256_permute2x128_si256(a,b,c)

/* storeu2_m128d: store hi/lo halves to two locations */
static inline void _mm256_storeu2_m128d_emu(void *hi,void *lo,__m256i v){
    vst1q_s32((int32_t*)lo, v.lo);
    vst1q_s32((int32_t*)hi, v.hi);
}
#define _mm256_storeu2_m128d(hi,lo,v) _mm256_storeu2_m128d_emu(hi,lo,v)

/* ────────── ALIGN macro ────────── */
#ifndef ALIGN
#define ALIGN(n) __attribute__((aligned(n)))
#endif

#endif /* NEON_COMPAT_H */
