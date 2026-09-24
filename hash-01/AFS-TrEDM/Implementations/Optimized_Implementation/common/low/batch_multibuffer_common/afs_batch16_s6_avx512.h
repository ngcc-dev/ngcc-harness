#ifndef AFS_BATCH16_S6_AVX512_H
#define AFS_BATCH16_S6_AVX512_H

#include <stdint.h>

#if defined(AFS_TREDM_USE_AVX512_BATCH) && defined(__AVX512F__)
#include <immintrin.h>
#define AFS_TREDM_HAVE_BATCH16_AVX512_TRUE 1

typedef struct {
    __m512i hi;
    __m512i lo;
} v64p;

static inline __m512i vxor32(__m512i a, __m512i b)
{
    return _mm512_xor_si512(a, b);
}

static inline __m512i vrot32_shift(__m512i x, int r)
{
    r &= 31;
    if (r == 0) {
        return x;
    }
    return _mm512_or_si512(_mm512_srli_epi32(x, r),
                           _mm512_slli_epi32(x, 32 - r));
}

#if defined(__GNUC__) || defined(__clang__)
#define vrot32(x, r) _mm512_ror_epi32((x), (r))
#else
#define vrot32(x, r) vrot32_shift((x), (r))
#endif

static inline v64p v64p_zero(void)
{
    v64p out;
    out.hi = _mm512_setzero_si512();
    out.lo = _mm512_setzero_si512();
    return out;
}

static inline v64p v64p_set1_u64(uint64_t x)
{
    v64p out;
    out.hi = _mm512_set1_epi32((int)(uint32_t)(x >> 32));
    out.lo = _mm512_set1_epi32((int)(uint32_t)x);
    return out;
}

static inline v64p v64p_xor(v64p a, v64p b)
{
    v64p out;
    out.hi = vxor32(a.hi, b.hi);
    out.lo = vxor32(a.lo, b.lo);
    return out;
}

static inline v64p v64p_rotr64(v64p a, unsigned r)
{
    v64p out;
    r &= 63U;
    if (r == 0U) {
        return a;
    }
    if (r == 32U) {
        out.hi = a.lo;
        out.lo = a.hi;
        return out;
    }
    if (r < 32U) {
        out.hi = _mm512_or_si512(_mm512_srli_epi32(a.hi, (int)r),
                                 _mm512_slli_epi32(a.lo, (int)(32U - r)));
        out.lo = _mm512_or_si512(_mm512_srli_epi32(a.lo, (int)r),
                                 _mm512_slli_epi32(a.hi, (int)(32U - r)));
        return out;
    }
    r -= 32U;
    out.hi = _mm512_or_si512(_mm512_srli_epi32(a.lo, (int)r),
                             _mm512_slli_epi32(a.hi, (int)(32U - r)));
    out.lo = _mm512_or_si512(_mm512_srli_epi32(a.hi, (int)r),
                             _mm512_slli_epi32(a.lo, (int)(32U - r)));
    return out;
}

static inline v64p v64p_mu(v64p a)
{
    return v64p_xor(v64p_xor(a, v64p_rotr64(a, 17U)), v64p_rotr64(a, 32U));
}

#else
#define AFS_TREDM_HAVE_BATCH16_AVX512_TRUE 0
#endif

int afs_tredm_hash_batch16_avx512_same_len(
    const uint8_t * const msg[16],
    uint64_t msg_len_bits,
    uint8_t * const digest[16]);

int afs_tredm512_hash_batch16_avx512_same_len(
    const uint8_t * const msg[16],
    uint64_t msg_len_bits,
    uint8_t * const digest[16]);

int afs_batch16_avx512_selftest_primitives(void);

#endif /* AFS_BATCH16_S6_AVX512_H */
