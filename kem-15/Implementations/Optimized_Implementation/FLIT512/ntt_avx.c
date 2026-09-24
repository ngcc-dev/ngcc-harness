#include <stdint.h>
#include <immintrin.h>
#include "params.h"
#include "consts.h"
#include "ntt.h"
#include "ntt_avx.h"

/* Montgomery multiply with precomputed zeta pair (used by ntt_avx only) */
static inline __m256i fqmul_precomp(__m256i a, __m256i zeta_lo, __m256i zeta_hi, __m256i q) {
    __m256i lo = _mm256_mullo_epi16(zeta_lo, a);
    __m256i t = _mm256_mulhi_epi16(zeta_hi, a);
    lo = _mm256_mulhi_epi16(q, lo);
    return _mm256_sub_epi16(t, lo);
}

/* SSE2 version for len < 16 fallback */
static inline __m128i fqmul_precomp_sse(__m128i a, __m128i zeta_lo, __m128i zeta_hi, __m128i q) {
    __m128i lo = _mm_mullo_epi16(zeta_lo, a);
    __m128i t = _mm_mulhi_epi16(zeta_hi, a);
    lo = _mm_mulhi_epi16(q, lo);
    return _mm_sub_epi16(t, lo);
}

void ntt_avx(int16_t r[N], const int16_t *qdata) {
    __m256i q = _mm256_load_si256((const __m256i *)(qdata + _16XQ));
    __m128i q128 = _mm256_castsi256_si128(q);
    unsigned int len, start, j;
    int k;

#if KEM_MODE == 128
    k = 1;
    for (len = N/2; len >= 4; len >>= 1) {
        for (start = 0; start < N; start = j + len) {
            __m256i zeta_lo = _mm256_set1_epi64x(*(const int64_t *)(qdata + _ZETAS_EXP + 8*k));
            __m256i zeta_hi = _mm256_set1_epi64x(*(const int64_t *)(qdata + _ZETAS_EXP + 8*k + 4));
            k++;

            if (len >= 16) {
                for (j = start; j < start + len; j += 16) {
                    __m256i rj = _mm256_load_si256((const __m256i *)(r + j));
                    __m256i rjlen = _mm256_load_si256((const __m256i *)(r + j + len));
                    __m256i t = fqmul_precomp(rjlen, zeta_lo, zeta_hi, q);
                    __m256i new_rj = _mm256_add_epi16(rj, t);
                    __m256i new_rjlen = _mm256_sub_epi16(rj, t);
                    _mm256_store_si256((__m256i *)(r + j), new_rj);
                    _mm256_store_si256((__m256i *)(r + j + len), new_rjlen);
                }
            } else {
                __m128i zeta_lo_sse = _mm256_castsi256_si128(zeta_lo);
                __m128i zeta_hi_sse = _mm256_castsi256_si128(zeta_hi);
                for (j = start; j < start + len; j += 4) {
                    __m128i rj = _mm_loadl_epi64((const __m128i *)(r + j));
                    __m128i rjlen = _mm_loadl_epi64((const __m128i *)(r + j + len));
                    __m128i t = fqmul_precomp_sse(rjlen, zeta_lo_sse, zeta_hi_sse, q128);
                    __m128i new_rj = _mm_add_epi16(rj, t);
                    __m128i new_rjlen = _mm_sub_epi16(rj, t);
                    _mm_storel_epi64((__m128i *)(r + j), new_rj);
                    _mm_storel_epi64((__m128i *)(r + j + len), new_rjlen);
                }
            }
        }
    }
#elif KEM_MODE == 256
    k = 1;
    for (len = N/2; len >= 8; len >>= 1) {
        for (start = 0; start < N; start = j + len) {
            __m256i zeta_lo = _mm256_set1_epi64x(*(const int64_t *)(qdata + _ZETAS_EXP + 8*k));
            __m256i zeta_hi = _mm256_set1_epi64x(*(const int64_t *)(qdata + _ZETAS_EXP + 8*k + 4));
            k++;

            if (len >= 16) {
                for (j = start; j < start + len; j += 16) {
                    __m256i rj = _mm256_load_si256((const __m256i *)(r + j));
                    __m256i rjlen = _mm256_load_si256((const __m256i *)(r + j + len));
                    __m256i t = fqmul_precomp(rjlen, zeta_lo, zeta_hi, q);
                    __m256i new_rj = _mm256_add_epi16(rj, t);
                    __m256i new_rjlen = _mm256_sub_epi16(rj, t);
                    _mm256_store_si256((__m256i *)(r + j), new_rj);
                    _mm256_store_si256((__m256i *)(r + j + len), new_rjlen);
                }
            } else {
                __m128i zeta_lo_sse = _mm256_castsi256_si128(zeta_lo);
                __m128i zeta_hi_sse = _mm256_castsi256_si128(zeta_hi);
                for (j = start; j < start + len; j += 8) {
                    __m128i rj = _mm_load_si128((const __m128i *)(r + j));
                    __m128i rjlen = _mm_load_si128((const __m128i *)(r + j + len));
                    __m128i t = fqmul_precomp_sse(rjlen, zeta_lo_sse, zeta_hi_sse, q128);
                    __m128i new_rj = _mm_add_epi16(rj, t);
                    __m128i new_rjlen = _mm_sub_epi16(rj, t);
                    _mm_store_si128((__m128i *)(r + j), new_rj);
                    _mm_store_si128((__m128i *)(r + j + len), new_rjlen);
                }
            }
        }
    }
#elif KEM_MODE == 512
    k = 1;
    for (len = N/2; len >= 16; len >>= 1) {
        for (start = 0; start < N; start = j + len) {
            __m256i zeta_lo = _mm256_set1_epi64x(*(const int64_t *)(qdata + _ZETAS_EXP + 8*k));
            __m256i zeta_hi = _mm256_set1_epi64x(*(const int64_t *)(qdata + _ZETAS_EXP + 8*k + 4));
            k++;
            for (j = start; j < start + len; j += 16) {
                __m256i rj = _mm256_load_si256((const __m256i *)(r + j));
                __m256i rjlen = _mm256_load_si256((const __m256i *)(r + j + len));
                __m256i t = fqmul_precomp(rjlen, zeta_lo, zeta_hi, q);
                __m256i new_rj = _mm256_add_epi16(rj, t);
                __m256i new_rjlen = _mm256_sub_epi16(rj, t);
                _mm256_store_si256((__m256i *)(r + j), new_rj);
                _mm256_store_si256((__m256i *)(r + j + len), new_rjlen);
            }
        }
    }
#endif
}

void invntt_avx(int16_t r[N], const int16_t *qdata) {
    __m256i q = _mm256_load_si256((const __m256i *)(qdata + _16XQ));
    __m128i q128 = _mm256_castsi256_si128(q);
    /* qm1 = Q-1 for conditional subtraction (sum < 2Q guaranteed in INTT) */
    __m256i qm1 = _mm256_set1_epi16(Q - 1);
    __m128i qm1_128 = _mm_set1_epi16(Q - 1);
    unsigned int start, len, j;
    int k;

#if KEM_MODE == 128
    k = 0;
    for (len = 4; len <= N/2; len <<= 1) {
        for (start = 0; start < N; start = j + len) {
            __m256i zeta_lo = _mm256_set1_epi64x(*(const int64_t *)(qdata_inv + 8*(k)));
            __m256i zeta    = _mm256_set1_epi64x(*(const int64_t *)(qdata_inv + 8*(k) + 4));
            k++;

            if (len >= 16) {
                for (j = start; j < start + len; j += 16) {
                    __m256i rj = _mm256_load_si256((const __m256i *)(r + j));
                    __m256i rjlen = _mm256_load_si256((const __m256i *)(r + j + len));
                    __m256i t = rj;
                    rj = _mm256_add_epi16(t, rjlen);
                    { __m256i _m = _mm256_cmpgt_epi16(rj, qm1);
                      rj = _mm256_sub_epi16(rj, _mm256_and_si256(_m, q)); }
                    rjlen = _mm256_sub_epi16(t, rjlen);
                    rjlen = fqmul_precomp(rjlen, zeta_lo, zeta, q);
                    _mm256_store_si256((__m256i *)(r + j), rj);
                    _mm256_store_si256((__m256i *)(r + j + len), rjlen);
                }
            } else if (len == 8) {
                __m128i zeta_sse = _mm256_castsi256_si128(zeta);
                __m128i zeta_lo_sse = _mm256_castsi256_si128(zeta_lo);
                for (j = start; j < start + len; j += 8) {
                    __m128i rj = _mm_load_si128((const __m128i *)(r + j));
                    __m128i rjlen = _mm_load_si128((const __m128i *)(r + j + len));
                    __m128i t = rj;
                    rj = _mm_add_epi16(t, rjlen);
                    { __m128i _m = _mm_cmpgt_epi16(rj, qm1_128);
                      rj = _mm_sub_epi16(rj, _mm_and_si128(_m, q128)); }
                    rjlen = _mm_sub_epi16(t, rjlen);
                    rjlen = fqmul_precomp_sse(rjlen, zeta_lo_sse, zeta_sse, q128);
                    _mm_store_si128((__m128i *)(r + j), rj);
                    _mm_store_si128((__m128i *)(r + j + len), rjlen);
                }
            } else {
                __m128i zeta_sse = _mm256_castsi256_si128(zeta);
                __m128i zeta_lo_sse = _mm256_castsi256_si128(zeta_lo);
                for (j = start; j < start + len; j += 4) {
                    __m128i rj = _mm_loadl_epi64((const __m128i *)(r + j));
                    __m128i rjlen = _mm_loadl_epi64((const __m128i *)(r + j + len));
                    __m128i t = rj;
                    rj = _mm_add_epi16(t, rjlen);
                    { __m128i _m = _mm_cmpgt_epi16(rj, qm1_128);
                      rj = _mm_sub_epi16(rj, _mm_and_si128(_m, q128)); }
                    rjlen = _mm_sub_epi16(t, rjlen);
                    rjlen = fqmul_precomp_sse(rjlen, zeta_lo_sse, zeta_sse, q128);
                    _mm_storel_epi64((__m128i *)(r + j), rj);
                    _mm_storel_epi64((__m128i *)(r + j + len), rjlen);
                }
            }
        }
    }

    {
        __m256i zf_lo = _mm256_set1_epi64x(*(const int64_t *)(qdata_inv + 8*(N/4 - 1)));
        __m256i zf    = _mm256_set1_epi64x(*(const int64_t *)(qdata_inv + 8*(N/4 - 1) + 4));
        for (j = 0; j < N; j += 16) {
            __m256i rj = _mm256_load_si256((const __m256i *)(r + j));
            rj = fqmul_precomp(rj, zf_lo, zf, q);
            _mm256_store_si256((__m256i *)(r + j), rj);
        }
    }
#elif KEM_MODE == 256
    k = 0;
    for (len = 8; len <= N/2; len <<= 1) {
        for (start = 0; start < N; start = j + len) {
            __m256i zeta_lo = _mm256_set1_epi64x(*(const int64_t *)(qdata_inv + 8*(k)));
            __m256i zeta    = _mm256_set1_epi64x(*(const int64_t *)(qdata_inv + 8*(k) + 4));
            k++;

            if (len >= 16) {
                for (j = start; j < start + len; j += 16) {
                    __m256i rj = _mm256_load_si256((const __m256i *)(r + j));
                    __m256i rjlen = _mm256_load_si256((const __m256i *)(r + j + len));
                    __m256i t = rj;
                    rj = _mm256_add_epi16(t, rjlen);
                    { __m256i _m = _mm256_cmpgt_epi16(rj, qm1);
                      rj = _mm256_sub_epi16(rj, _mm256_and_si256(_m, q)); }
                    rjlen = _mm256_sub_epi16(t, rjlen);
                    rjlen = fqmul_precomp(rjlen, zeta_lo, zeta, q);
                    _mm256_store_si256((__m256i *)(r + j), rj);
                    _mm256_store_si256((__m256i *)(r + j + len), rjlen);
                }
            } else {
                __m128i zeta_sse = _mm256_castsi256_si128(zeta);
                __m128i zeta_lo_sse = _mm256_castsi256_si128(zeta_lo);
                for (j = start; j < start + len; j += 8) {
                    __m128i rj = _mm_load_si128((const __m128i *)(r + j));
                    __m128i rjlen = _mm_load_si128((const __m128i *)(r + j + len));
                    __m128i t = rj;
                    rj = _mm_add_epi16(t, rjlen);
                    { __m128i _m = _mm_cmpgt_epi16(rj, qm1_128);
                      rj = _mm_sub_epi16(rj, _mm_and_si128(_m, q128)); }
                    rjlen = _mm_sub_epi16(t, rjlen);
                    rjlen = fqmul_precomp_sse(rjlen, zeta_lo_sse, zeta_sse, q128);
                    _mm_store_si128((__m128i *)(r + j), rj);
                    _mm_store_si128((__m128i *)(r + j + len), rjlen);
                }
            }
        }
    }

    {
        __m256i zf_lo = _mm256_set1_epi64x(*(const int64_t *)(qdata_inv + 8*(N/8 - 1)));
        __m256i zf    = _mm256_set1_epi64x(*(const int64_t *)(qdata_inv + 8*(N/8 - 1) + 4));
        for (j = 0; j < N; j += 16) {
            __m256i rj = _mm256_load_si256((const __m256i *)(r + j));
            rj = fqmul_precomp(rj, zf_lo, zf, q);
            _mm256_store_si256((__m256i *)(r + j), rj);
        }
    }
#elif KEM_MODE == 512
    k = 0;
    for (len = 16; len <= N/2; len <<= 1) {
        for (start = 0; start < N; start = j + len) {
            __m256i zeta_lo = _mm256_set1_epi64x(*(const int64_t *)(qdata_inv + 8*(k)));
            __m256i zeta    = _mm256_set1_epi64x(*(const int64_t *)(qdata_inv + 8*(k) + 4));
            k++;
            for (j = start; j < start + len; j += 16) {
                __m256i rj = _mm256_load_si256((const __m256i *)(r + j));
                __m256i rjlen = _mm256_load_si256((const __m256i *)(r + j + len));
                __m256i t = rj;
                rj = _mm256_add_epi16(t, rjlen);
                { __m256i _m = _mm256_cmpgt_epi16(rj, qm1);
                  rj = _mm256_sub_epi16(rj, _mm256_and_si256(_m, q)); }
                rjlen = _mm256_sub_epi16(t, rjlen);
                rjlen = fqmul_precomp(rjlen, zeta_lo, zeta, q);
                _mm256_store_si256((__m256i *)(r + j), rj);
                _mm256_store_si256((__m256i *)(r + j + len), rjlen);
            }
        }
    }

    {
        __m256i zf_lo = _mm256_set1_epi64x(*(const int64_t *)(qdata_inv + 8*(N/16 - 1)));
        __m256i zf    = _mm256_set1_epi64x(*(const int64_t *)(qdata_inv + 8*(N/16 - 1) + 4));
        for (j = 0; j < N; j += 16) {
            __m256i rj = _mm256_load_si256((const __m256i *)(r + j));
            rj = fqmul_precomp(rj, zf_lo, zf, q);
            _mm256_store_si256((__m256i *)(r + j), rj);
        }
    }
#endif
}
