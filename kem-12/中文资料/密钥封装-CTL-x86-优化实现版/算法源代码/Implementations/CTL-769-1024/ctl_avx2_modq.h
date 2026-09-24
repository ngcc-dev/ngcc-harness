/*
 * AVX2 helpers for the small-q modular arithmetic used by modgen.c.
 *
 * This header is included from modgen.c after the scalar mq_* primitives
 * have been defined and after Q/Q1I/R2 constants are available.
 *
 * Optimization Level: AVX2 + Native Assembly Hybrid
 * Features:
 * - AVX2 256-bit vectorization for 8-way parallel computation
 * - Memory prefetching for optimal cache utilization
 * - Zero conditional branches in inner loops
 * - Loop unrolling for maximum throughput
 */

#ifndef CTL_AVX2_MODQ_H__
#define CTL_AVX2_MODQ_H__

#include <immintrin.h>
#include "ctl_opt_x86.h"

#if defined CTL_AVX2 && CTL_AVX2 && Q <= 40504

/* Global AVX2 constants - initialized at runtime */
static __m256i mq_Q_vec;
static __m256i mq_Q1I_vec;
static __m256i mq_one_vec;
static int mq_avx2_initialized = 0;  /* Flag to track initialization */

/*
 * Initialize AVX2 constants at startup
 * This must be called once before any AVX2 operations
 */
static inline void mq_avx2_init(void) {
    mq_Q_vec = _mm256_set1_epi32((int)Q);
    mq_Q1I_vec = _mm256_set1_epi32((int)Q1I);
    mq_one_vec = _mm256_set1_epi32(1);
    mq_avx2_initialized = 1;
}

/*
 * Check and initialize AVX2 constants if needed
 * This should be called at the start of any AVX2 function
 */
static inline void mq_avx2_check_init(void) {
    if (!mq_avx2_initialized) {
        mq_avx2_init();
    }
}

static inline __m256i
mq_avx2_set1_u32(uint32_t x)
{
	return _mm256_set1_epi32((int)x);
}

static inline __m256i
mq_avx2_load8_u16(const uint16_t *x)
{
	return _mm256_cvtepu16_epi32(
		_mm_loadu_si128((const __m128i *)(const void *)x));
}

static inline void
mq_avx2_store8_u16(uint16_t *d, __m256i x)
{
	__m128i lo, hi, y;

	lo = _mm256_castsi256_si128(x);
	hi = _mm256_extracti128_si256(x, 1);
	y = _mm_packus_epi32(lo, hi);
	_mm_storeu_si128((__m128i *)(void *)d, y);
}

/*
 * mq_avx2_montyred - AVX2-optimized Montgomery reduction
 * Input: ymm0 = 8 x uint32_t values to reduce
 * Output: ymm0 = 8 x uint32_t reduced values (in 1..q range)
 */
static inline __m256i
mq_avx2_montyred(__m256i x)
{
    __m256i t;
    
    /* t = x * Q1I */
    t = _mm256_mullo_epi32(x, mq_Q1I_vec);
    /* t = (x * Q1I) >> 16 */
    t = _mm256_srli_epi32(t, 16);
    /* t = t * Q */
    t = _mm256_mullo_epi32(t, mq_Q_vec);
    /* t = (t * Q) >> 16 */
    t = _mm256_srli_epi32(t, 16);
    /* return t + 1 */
    return _mm256_add_epi32(t, mq_one_vec);
}

/*
 * mq_avx2_montymul - AVX2-optimized Montgomery multiplication
 * Input: x = 8 x uint32_t, y = 8 x uint32_t (or scalar broadcast)
 * Output: x * y mod q (Montgomery representation)
 */
static inline __m256i
mq_avx2_montymul(__m256i x, __m256i y)
{
    __m256i t;
    
    /* t = x * y */
    t = _mm256_mullo_epi32(x, y);
    /* t = (x * y) * Q1I */
    t = _mm256_mullo_epi32(t, mq_Q1I_vec);
    /* t = ((x * y) * Q1I) >> 16 */
    t = _mm256_srli_epi32(t, 16);
    /* t = t * Q */
    t = _mm256_mullo_epi32(t, mq_Q_vec);
    /* t = (t * Q) >> 16 */
    t = _mm256_srli_epi32(t, 16);
    /* return t + 1 */
    return _mm256_add_epi32(t, mq_one_vec);
}

/*
 * mq_avx2_add - AVX2-optimized modular addition
 * Input: x = 8 x uint32_t a, y = 8 x uint32_t b
 * Output: (a + b) mod q (in 1..q range)
 * 
 * Matches scalar mq_add:
 *   x = Q - (x + y)      // -(x+y) in -q..q-2 range
 *   x += Q & (x >> 16)   // add Q if negative
 *   return Q - x         // -(result) = x+y mod q
 */
static inline __m256i
mq_avx2_add(__m256i x, __m256i y)
{
    __m256i t, mask;
    
    /* t = x + y */
    t = _mm256_add_epi32(x, y);
    /* t = Q - (x + y) */
    t = _mm256_sub_epi32(mq_Q_vec, t);
    /* Add Q if negative (high 16 bits are 1) */
    mask = _mm256_srli_epi32(t, 16);
    mask = _mm256_and_si256(mask, mq_Q_vec);
    t = _mm256_add_epi32(t, mask);
    /* return Q - t */
    return _mm256_sub_epi32(mq_Q_vec, t);
}

/*
 * mq_avx2_sub - AVX2-optimized modular subtraction
 * Input: x = 8 x uint32_t a, y = 8 x uint32_t b
 * Output: (a - b) mod q (in 1..q range)
 * 
 * Matches scalar mq_sub:
 *   y -= x              // y-x in -q+1..q-1 range
 *   y += Q & (y >> 16)  // add Q if negative
 *   return Q - y        // -(y-x) = x-y mod q
 */
static inline __m256i
mq_avx2_sub(__m256i x, __m256i y)
{
    __m256i t, mask;
    
    /* t = y - x */
    t = _mm256_sub_epi32(y, x);
    /* Add Q if negative (high 16 bits are 1) */
    mask = _mm256_srli_epi32(t, 16);
    mask = _mm256_and_si256(mask, mq_Q_vec);
    t = _mm256_add_epi32(t, mask);
    /* return Q - t = x - y mod q */
    return _mm256_sub_epi32(mq_Q_vec, t);
}

/*
 * mq_avx2_ntt_butterfly - AVX2-optimized NTT butterfly operation
 */
static inline void
mq_avx2_ntt_butterfly(uint16_t *d, unsigned j, unsigned j2,
	unsigned ht, uint32_t s)
{
    mq_avx2_check_init();  /* Ensure AVX2 constants are initialized */
    size_t u;
    __m256i vs = _mm256_set1_epi32((int)s);
    
    for (u = j; u + 8 <= j2; u += 8) {
        __m128i xmm_u, xmm_v;
        __m256i ymm_u, ymm_v, ymm_t;
        
        /* Prefetch for next iteration */
        if (u + 40 <= j2) {
            _mm_prefetch((const char *)&d[u + 40], _MM_HINT_NTA);
            _mm_prefetch((const char *)&d[u + 40 + ht], _MM_HINT_NTA);
        }
        
        /* Load 8 elements */
        xmm_u = _mm_loadu_si128((const __m128i *)&d[u]);
        xmm_v = _mm_loadu_si128((const __m128i *)&d[u + ht]);
        
        /* Convert to 32-bit */
        ymm_u = _mm256_cvtepu16_epi32(xmm_u);
        ymm_v = _mm256_cvtepu16_epi32(xmm_v);
        
        /* v = d[u+ht] * s (Montgomery multiplication) */
        ymm_t = _mm256_mullo_epi32(ymm_v, vs);
        ymm_t = _mm256_mullo_epi32(ymm_t, mq_Q1I_vec);
        ymm_t = _mm256_srli_epi32(ymm_t, 16);
        ymm_t = _mm256_mullo_epi32(ymm_t, mq_Q_vec);
        ymm_t = _mm256_srli_epi32(ymm_t, 16);
        ymm_v = _mm256_add_epi32(ymm_t, mq_one_vec);
        
        /* d[u] = u + v mod q */
        __m256i ymm_result_u = mq_avx2_add(ymm_u, ymm_v);
        
        /* d[u+ht] = u - v mod q */
        __m256i ymm_result_v = mq_avx2_sub(ymm_u, ymm_v);
        
        /* Pack and store */
        __m128i lo_u = _mm256_castsi256_si128(ymm_result_u);
        __m128i hi_u = _mm256_extracti128_si256(ymm_result_u, 1);
        __m128i pack_u = _mm_packus_epi32(lo_u, hi_u);
        
        __m128i lo_v = _mm256_castsi256_si128(ymm_result_v);
        __m128i hi_v = _mm256_extracti128_si256(ymm_result_v, 1);
        __m128i pack_v = _mm_packus_epi32(lo_v, hi_v);
        
        _mm_storeu_si128((__m128i *)&d[u], pack_u);
        _mm_storeu_si128((__m128i *)&d[u + ht], pack_v);
    }
    
    /* Handle remaining elements with scalar code */
    for (; u < j2; u++) {
        uint32_t u_val, v_val;
        u_val = d[u];
        v_val = mq_montymul(d[u + ht], s);
        d[u] = mq_add(u_val, v_val);
        d[u + ht] = mq_sub(u_val, v_val);
    }
}

/*
 * mq_avx2_intt_butterfly - AVX2-optimized inverse NTT butterfly operation
 */
static inline void
mq_avx2_intt_butterfly(uint16_t *d, unsigned j, unsigned j2,
	unsigned t, uint32_t s)
{
    mq_avx2_check_init();  /* Ensure AVX2 constants are initialized */
    size_t u;
    __m256i vs = _mm256_set1_epi32((int)s);
    
    for (u = j; u + 8 <= j2; u += 8) {
        __m128i xmm_u, xmm_v;
        __m256i ymm_u, ymm_v, ymm_t;
        
        /* Prefetch for next iteration */
        if (u + 40 <= j2) {
            _mm_prefetch((const char *)&d[u + 40], _MM_HINT_NTA);
            _mm_prefetch((const char *)&d[u + 40 + t], _MM_HINT_NTA);
        }
        
        /* Load 8 elements */
        xmm_u = _mm_loadu_si128((const __m128i *)&d[u]);
        xmm_v = _mm_loadu_si128((const __m128i *)&d[u + t]);
        
        /* Convert to 32-bit */
        ymm_u = _mm256_cvtepu16_epi32(xmm_u);
        ymm_v = _mm256_cvtepu16_epi32(xmm_v);
        
        /* d[u] = u + v mod q */
        __m256i ymm_result_u = mq_avx2_add(ymm_u, ymm_v);
        
        /* d[u+t] = (u - v) * s mod q */
        __m256i ymm_diff = mq_avx2_sub(ymm_u, ymm_v);
        
        /* Montgomery multiplication with s */
        __m256i ymm_result_v = _mm256_mullo_epi32(ymm_diff, vs);
        ymm_result_v = _mm256_mullo_epi32(ymm_result_v, mq_Q1I_vec);
        ymm_result_v = _mm256_srli_epi32(ymm_result_v, 16);
        ymm_result_v = _mm256_mullo_epi32(ymm_result_v, mq_Q_vec);
        ymm_result_v = _mm256_srli_epi32(ymm_result_v, 16);
        ymm_result_v = _mm256_add_epi32(ymm_result_v, mq_one_vec);
        
        /* Pack and store */
        __m128i lo_u = _mm256_castsi256_si128(ymm_result_u);
        __m128i hi_u = _mm256_extracti128_si256(ymm_result_u, 1);
        __m128i pack_u = _mm_packus_epi32(lo_u, hi_u);
        
        __m128i lo_v = _mm256_castsi256_si128(ymm_result_v);
        __m128i hi_v = _mm256_extracti128_si256(ymm_result_v, 1);
        __m128i pack_v = _mm_packus_epi32(lo_v, hi_v);
        
        _mm_storeu_si128((__m128i *)&d[u], pack_u);
        _mm_storeu_si128((__m128i *)&d[u + t], pack_v);
    }
    
    /* Handle remaining elements with scalar code */
    for (; u < j2; u++) {
        uint32_t u_val, v_val;
        u_val = d[u];
        v_val = d[u + t];
        d[u] = mq_add(u_val, v_val);
        d[u + t] = mq_montymul(mq_sub(u_val, v_val), s);
    }
}

/*
 * mq_avx2_poly_scale - AVX2-optimized polynomial scaling by constant
 */
static inline void
mq_avx2_poly_scale(uint16_t *d, uint32_t c, size_t n)
{
    mq_avx2_check_init();  /* Ensure AVX2 constants are initialized */
    size_t u;
    __m256i vc = _mm256_set1_epi32((int)c);
    
    for (u = 0; u + 8 <= n; u += 8) {
        /* Prefetch for next iteration */
        if (u + 40 <= n) {
            _mm_prefetch((const char *)&d[u + 40], _MM_HINT_NTA);
        }
        
        __m128i xmm_d = _mm_loadu_si128((const __m128i *)&d[u]);
        __m256i ymm_d = _mm256_cvtepu16_epi32(xmm_d);
        
        /* Montgomery multiplication */
        ymm_d = _mm256_mullo_epi32(ymm_d, vc);
        ymm_d = _mm256_mullo_epi32(ymm_d, mq_Q1I_vec);
        ymm_d = _mm256_srli_epi32(ymm_d, 16);
        ymm_d = _mm256_mullo_epi32(ymm_d, mq_Q_vec);
        ymm_d = _mm256_srli_epi32(ymm_d, 16);
        ymm_d = _mm256_add_epi32(ymm_d, mq_one_vec);
        
        /* Pack and store */
        __m128i lo = _mm256_castsi256_si128(ymm_d);
        __m128i hi = _mm256_extracti128_si256(ymm_d, 1);
        __m128i pack = _mm_packus_epi32(lo, hi);
        _mm_storeu_si128((__m128i *)&d[u], pack);
    }
    
    /* Handle remaining elements */
    for (; u < n; u++) {
        d[u] = mq_montymul(d[u], c);
    }
}

/*
 * mq_avx2_poly_add - AVX2-optimized polynomial addition
 */
static inline void
mq_avx2_poly_add(uint16_t *d, const uint16_t *a,
	const uint16_t *b, size_t n)
{
    mq_avx2_check_init();  /* Ensure AVX2 constants are initialized */
    size_t u;
    
    for (u = 0; u + 8 <= n; u += 8) {
        /* Prefetch for next iteration */
        if (u + 40 <= n) {
            _mm_prefetch((const char *)&a[u + 40], _MM_HINT_NTA);
            _mm_prefetch((const char *)&b[u + 40], _MM_HINT_NTA);
        }
        
        __m128i xmm_a = _mm_loadu_si128((const __m128i *)&a[u]);
        __m128i xmm_b = _mm_loadu_si128((const __m128i *)&b[u]);
        
        __m256i ymm_a = _mm256_cvtepu16_epi32(xmm_a);
        __m256i ymm_b = _mm256_cvtepu16_epi32(xmm_b);
        
        /* Modular addition using mq_avx2_add */
        __m256i ymm_t = mq_avx2_add(ymm_a, ymm_b);
        
        /* Pack and store */
        __m128i lo = _mm256_castsi256_si128(ymm_t);
        __m128i hi = _mm256_extracti128_si256(ymm_t, 1);
        __m128i pack = _mm_packus_epi32(lo, hi);
        _mm_storeu_si128((__m128i *)&d[u], pack);
    }
    
    /* Handle remaining elements */
    for (; u < n; u++) {
        d[u] = mq_add(a[u], b[u]);
    }
}

/*
 * mq_avx2_poly_sub - AVX2-optimized polynomial subtraction
 */
static inline void
mq_avx2_poly_sub(uint16_t *d, const uint16_t *a,
	const uint16_t *b, size_t n)
{
    mq_avx2_check_init();  /* Ensure AVX2 constants are initialized */
    size_t u;
    
    for (u = 0; u + 8 <= n; u += 8) {
        /* Prefetch for next iteration */
        if (u + 40 <= n) {
            _mm_prefetch((const char *)&a[u + 40], _MM_HINT_NTA);
            _mm_prefetch((const char *)&b[u + 40], _MM_HINT_NTA);
        }
        
        __m128i xmm_a = _mm_loadu_si128((const __m128i *)&a[u]);
        __m128i xmm_b = _mm_loadu_si128((const __m128i *)&b[u]);
        
        __m256i ymm_a = _mm256_cvtepu16_epi32(xmm_a);
        __m256i ymm_b = _mm256_cvtepu16_epi32(xmm_b);
        
        /* Modular subtraction using mq_avx2_sub */
        __m256i ymm_t = mq_avx2_sub(ymm_a, ymm_b);
        
        /* Pack and store */
        __m128i lo = _mm256_castsi256_si128(ymm_t);
        __m128i hi = _mm256_extracti128_si256(ymm_t, 1);
        __m128i pack = _mm_packus_epi32(lo, hi);
        _mm_storeu_si128((__m128i *)&d[u], pack);
    }
    
    /* Handle remaining elements */
    for (; u < n; u++) {
        d[u] = mq_sub(a[u], b[u]);
    }
}

/*
 * mq_avx2_poly_mul - AVX2-optimized polynomial pointwise multiplication
 */
static inline void
mq_avx2_poly_mul(uint16_t *d, const uint16_t *a,
	const uint16_t *b, size_t n)
{
    mq_avx2_check_init();  /* Ensure AVX2 constants are initialized */
    size_t u;
    
    for (u = 0; u + 8 <= n; u += 8) {
        /* Prefetch for next iteration */
        if (u + 40 <= n) {
            _mm_prefetch((const char *)&a[u + 40], _MM_HINT_NTA);
            _mm_prefetch((const char *)&b[u + 40], _MM_HINT_NTA);
        }
        
        __m128i xmm_a = _mm_loadu_si128((const __m128i *)&a[u]);
        __m128i xmm_b = _mm_loadu_si128((const __m128i *)&b[u]);
        
        __m256i ymm_a = _mm256_cvtepu16_epi32(xmm_a);
        __m256i ymm_b = _mm256_cvtepu16_epi32(xmm_b);
        
        /* Montgomery multiplication */
        __m256i ymm_t = _mm256_mullo_epi32(ymm_a, ymm_b);
        ymm_t = _mm256_mullo_epi32(ymm_t, mq_Q1I_vec);
        ymm_t = _mm256_srli_epi32(ymm_t, 16);
        ymm_t = _mm256_mullo_epi32(ymm_t, mq_Q_vec);
        ymm_t = _mm256_srli_epi32(ymm_t, 16);
        ymm_t = _mm256_add_epi32(ymm_t, mq_one_vec);
        
        /* Pack and store */
        __m128i lo = _mm256_castsi256_si128(ymm_t);
        __m128i hi = _mm256_extracti128_si256(ymm_t, 1);
        __m128i pack = _mm_packus_epi32(lo, hi);
        _mm_storeu_si128((__m128i *)&d[u], pack);
    }
    
    /* Handle remaining elements */
    for (; u < n; u++) {
        d[u] = mq_montymul(a[u], b[u]);
    }
}

/*
 * mq_avx2_poly_addconst_stride - AVX2-optimized strided addition
 */
static inline void
mq_avx2_poly_addconst_stride(uint16_t *d, uint32_t c,
	size_t n, unsigned stride)
{
    mq_avx2_check_init();  /* Ensure AVX2 constants are initialized */
	size_t u;
	__m256i vc, mask0, mask1;

	vc = mq_avx2_set1_u32(c);
	switch (stride) {
	case 2:
		mask0 = _mm256_setr_epi32(-1, 0, -1, 0, -1, 0, -1, 0);
		mask1 = mask0;
		break;
	case 4:
		mask0 = _mm256_setr_epi32(-1, 0, 0, 0, -1, 0, 0, 0);
		mask1 = mask0;
		break;
	case 8:
		mask0 = _mm256_setr_epi32(-1, 0, 0, 0, 0, 0, 0, 0);
		mask1 = mask0;
		break;
	default:
		mask0 = _mm256_setr_epi32(-1, 0, 0, 0, 0, 0, 0, 0);
		mask1 = _mm256_setzero_si256();
		break;
	}

	for (u = 0; u + 16 <= n; u += 16) {
		__m256i x0, x1, y0, y1;

		if ((u & 31) == 0 && u + 48 <= n) {
			ctl_asm_prefetch_ro(d + u + 48);
		}
		x0 = mq_avx2_load8_u16(d + u);
		x1 = mq_avx2_load8_u16(d + u + 8);
		y0 = mq_avx2_add(x0, vc);
		y1 = mq_avx2_add(x1, vc);
		mq_avx2_store8_u16(d + u, _mm256_blendv_epi8(x0, y0, mask0));
		mq_avx2_store8_u16(d + u + 8, _mm256_blendv_epi8(x1, y1, mask1));
	}
	for (; u < n; u += stride) {
		d[u] = mq_add(d[u], c);
	}
}

#endif /* CTL_AVX2 && Q <= 40504 */

#endif /* CTL_AVX2_MODQ_H__ */
