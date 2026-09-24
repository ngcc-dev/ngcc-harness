#include "bigint.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <immintrin.h>   /* _mulx_u64, _addcarry_u64, AVX2/AVX-512 */

/* Limb size ---------------------------------------------------------------- */
/* AVX-512 IFMA works on exactly 52-bit unsigned limbs.                      */
#define BASE      BIGINT_BASE
#define KARAT_THRESHOLD 32

#define LIMB_MASK ((1ULL << BASE) - 1)

/* Alignment required for limb arrays (bytes) ------------------------------- */
#if defined(__AVX512IFMA__) || defined(AVX512IFMA)
#define BIGINT_ALIGN 64
#elif defined(__AVX2__) || defined(AVX2)
#define BIGINT_ALIGN 32
#else
#define BIGINT_ALIGN 16
#endif

/* -------------------------------------------------------------------------- */
/*  Feature macros                                                            */
/* -------------------------------------------------------------------------- */
#if defined(__AVX512IFMA__) || defined(AVX512IFMA)
//#define USE_AVX512IFMA 1
#include <immintrin.h>
#else
#define USE_AVX512IFMA 0
#endif

#if defined(__AVX2__) || defined(AVX2)
#define USE_AVX2 1
#include <immintrin.h>
#else
#define USE_AVX2 0
#endif

/* C23: use standard attributes when available */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202311L)
#  define LIKELY(x)   ((x) ? (void)0 : (void)0), !!(x)   /* [[likely]] is on the if/else itself */
#  define UNLIKELY(x) ((x) ? (void)0 : (void)0), !!(x)

/* GCC / Clang / Intel */
#elif defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
#  define LIKELY(x)   __builtin_expect(!!(x), 1)
#  define UNLIKELY(x) __builtin_expect(!!(x), 0)

/* MSVC has __assume, but it is not a branch hint; leave it as identity */
#else
#  define LIKELY(x)   (!!(x))
#  define UNLIKELY(x) (!!(x))
#endif

/* -------------------------------------------------------------------------- */
/*  Helpers                                                                   */
/* -------------------------------------------------------------------------- */
static inline int max(int a, int b) { return a > b ? a : b; }
static inline int min(int a, int b) { return a < b ? a : b; }

static inline uint64_t
mul64_wide(uint64_t x, uint64_t y, uint64_t *hi)
{
#if defined(__SIZEOF_INT128__)
     __extension__ __uint128_t z = (__uint128_t)x * (__uint128_t)y;

    *hi = (uint64_t)(z >> 64);
    return (uint64_t)z;
#else
    uint64_t x0 = (uint32_t)x, x1 = x >> 32;
    uint64_t y0 = (uint32_t)y, y1 = y >> 32;
    uint64_t p0 = x0 * y0, p1 = x0 * y1;
    uint64_t p2 = x1 * y0, p3 = x1 * y1;
    uint64_t t = (p0 >> 32) + (uint32_t)p1 + (uint32_t)p2;

    *hi = p3 + (p1 >> 32) + (p2 >> 32) + (t >> 32);
    return (p0 & UINT64_C(0xFFFFFFFF)) | (t << 32);
#endif
}

static inline uint64_t *u64(const bigint *a)
{
    return (uint64_t *)a->d;
}

/* -------------------------------------------------------------------------- */
/*  Life-cycle                                                                */
/* -------------------------------------------------------------------------- */
int bigint_init(bigint *a, size_t n)
{
    void *p = NULL;
    if (posix_memalign(&p, BIGINT_ALIGN, n * sizeof(int64_t)) != 0) {
        a->d = NULL;
        return -1;
    }
    a->n = a->al=n;
    a->d = (uint64_t *)p;
    memset(a->d, 0, n * sizeof(int64_t));
    a->s=1;
    return 0;
}

void bigint_printx(const bigint *a){
    for(size_t i=0;i<a->n;i++)
        printf("%lu*x^%zu%c",a->d[i],i,i==a->n-1?'\n':'+');
    printf(" (s=%d)",a->s);
    printf("\n");
}

void bigint_printx_s(const bigint *a){
    for(size_t i=0;i<a->n;i++)
        printf("(%ld)*x^%zu%c",a->d[i],i,i==a->n-1?'\n':'+');
    printf(" (s=%d)",a->s);
    printf("\n");
}

void bigint_free(bigint *a)
{
    free(a->d);
    a->d = NULL;
    a->n = 0;
}

void bigint_zero(bigint *a)
{
    size_t n = a->n;
    uint64_t *d = u64(a);
    size_t i = 0;

#if USE_AVX512IFMA
    __m512i z = _mm512_setzero_si512();
    for (; i + 8 <= n; i += 8)
        _mm512_storeu_si512(d + i, z);
#elif USE_AVX2
    __m256i z = _mm256_setzero_si256();
    for (; i + 4 <= n; i += 4)
        _mm256_storeu_si256((__m256i *)(d + i), z);
#endif
    for (; i < n; i++) d[i] = 0;
}

void bigint_copy(bigint *dst, const bigint *src)
{
    size_t n = src->n;
    assert(dst->al>=n);
    uint64_t *dd = u64(dst);
    const uint64_t *sd = u64(src);
    size_t i = 0;

#if USE_AVX512IFMA
    for (; i + 8 <= n; i += 8) {
        __m512i v = _mm512_loadu_si512(sd + i);
        _mm512_storeu_si512(dd + i, v);
    }
#elif USE_AVX2
    for (; i + 4 <= n; i += 4) {
        __m256i v = _mm256_loadu_si256((__m256i const *)(sd + i));
        _mm256_storeu_si256((__m256i *)(dd + i), v);
    }
#endif
    for (; i < n; i++) dd[i] = sd[i];
    dst->n=n;
    dst->s=src->s;
}

/* -------------------------------------------------------------------------- */
/*  Addition / Subtraction                                                    */
/* -------------------------------------------------------------------------- */
static void bigint_add_positive(bigint *res, const bigint *a, const bigint *b)
{
    size_t na = a->n, nb = b->n;
    size_t n = max(na,nb);
    res->n = n;
    assert(res->al>=n);

    const uint64_t *ad = u64(a), *bd = u64(b);
    uint64_t *rd = u64(res);
    size_t i = 0;

    /* Only SIMD over the region where *both* a and b have valid limbs. */
#if USE_AVX512IFMA || USE_AVX2
    size_t common = min(na,nb);
#endif

#if USE_AVX512IFMA
    for (; i + 8 <= common; i += 8) {
        __m512i av = _mm512_loadu_si512((__m512i const *)(ad + i));
        __m512i bv = _mm512_loadu_si512((__m512i const *)(bd + i));
        _mm512_storeu_si512((__m512i *)(rd + i), _mm512_add_epi64(av, bv));
    }
#elif USE_AVX2
    for (; i + 4 <= common; i += 4) {
        __m256i av = _mm256_loadu_si256((__m256i const *)(ad + i));
        __m256i bv = _mm256_loadu_si256((__m256i const *)(bd + i));
        _mm256_storeu_si256((__m256i *)(rd + i), _mm256_add_epi64(av, bv));
    }
#endif

    /* Scalar tail: no carry management. */
    for (; i < n; i++) {
        uint64_t av = (i < na) ? ad[i] : 0;
        uint64_t bv = (i < nb) ? bd[i] : 0;
        rd[i] = av + bv;
    }

}

static void bigint_sub_positive(bigint *res, const bigint *a, const bigint *b)
{
    size_t na = a->n, nb = b->n;
    size_t n = (na < nb) ? na : nb;

    const uint64_t *ad = u64(a), *bd = u64(b);
    uint64_t *rd = u64(res);
    size_t i = 0;

#if USE_AVX512IFMA
    for (; i + 8 <= n; i += 8) {
        __m512i av = _mm512_loadu_si512((__m512i const *)(ad + i));
        __m512i bv = _mm512_loadu_si512((__m512i const *)(bd + i));
        _mm512_storeu_si512((__m512i *)(rd + i), _mm512_sub_epi64(av, bv));
    }
#elif USE_AVX2
    for (; i + 4 <= n; i += 4) {
        __m256i av = _mm256_loadu_si256((__m256i const *)(ad + i));
        __m256i bv = _mm256_loadu_si256((__m256i const *)(bd + i));
        _mm256_storeu_si256((__m256i *)(rd + i), _mm256_sub_epi64(av, bv));
    }
#endif

    /* Scalar tail: no borrow management. */
    for (; i < n; i++) {
        rd[i] = ad[i] - bd[i];
    }
    for (; i < na ; i++) {
        rd[i] = ad[i];
    }
    for (; i < nb ; i++) {
        rd[i] = -bd[i];
    }
    res->n=na+nb-n;
}


void bigint_add(bigint *res, const bigint *a, const bigint *b){
    assert(res->al>=a->n);
    assert(res->al>=b->n);
    if(a->s*b->s<0){
        bigint_sub_positive(res,a,b);
        res->s=a->s;
        bigint_fast_normalize_signed(res);
    }
    else{
        bigint_add_positive(res,a,b);
        res->s=a->s;
    }
}

void bigint_sub(bigint *res, const bigint *a, const bigint *b){
    assert(res->al>=a->n);
    assert(res->al>=b->n);
    if(a->s*b->s>0){
//        bigint_printx(a);
  //      bigint_printx(b);
        bigint_sub_positive(res,a,b);
        res->s=a->s;
        bigint_fast_normalize_signed(res);
//        puts("res");
//        bigint_printx(res);
    }
    else{
        bigint_add_positive(res,a,b);
        res->s=a->s;
    }
}

/* -------------------------------------------------------------------------- */
/*  Scalar multiplication                                                     */
/* -------------------------------------------------------------------------- */
void bigint_mul_scalar(bigint *res, const bigint *a, int64_t scalar)
{
    size_t n = a->n;
    const uint64_t *ad = a->d;
    uint64_t *rd = res->d;
    uint64_t carry = 0;
    size_t i = 0;
    assert(BASE > 40 && BASE < 52);
    if (scalar < 0) {
        res->s = -a->s;
        scalar = -scalar;
    } else {
        res->s = a->s;
    }
    uint64_t us = (uint64_t)scalar;
    assert(us <= LIMB_MASK);
    if (n == 0 || us == 0) {
        res->n = 0;
        res->s = 1;
        return;
    }
#if USE_AVX512IFMA
    const __m512i vs = _mm512_set1_epi64(us);
    const __m512i vmask = _mm512_set1_epi64(LIMB_MASK);
    const __m512i idx_shift = _mm512_setr_epi64(0,0,1,2,3,4,5,6);
    const __m512i idx_top = _mm512_setr_epi64(7,0,0,0,0,0,0,0);
    const __mmask8 mk_shift = 0xfe;
    const __mmask8 mk_lane0 = 0x01;
    __m512i vprev = _mm512_setzero_si512();
    for (; i + 8 <= n; i += 8) {
        __m512i va = _mm512_loadu_si512((const void *)(ad + i));
        __m512i vlo52 = _mm512_madd52lo_epu64(_mm512_setzero_si512(), va, vs);
        __m512i vhi52 = _mm512_madd52hi_epu64(_mm512_setzero_si512(), va, vs);
        __m512i vlimb = _mm512_and_si512(vlo52, vmask);
        __m512i vc = _mm512_add_epi64(
            _mm512_srli_epi64(vlo52, BASE),
            _mm512_slli_epi64(vhi52, 52 - BASE)
        );
        __m512i vcsh = _mm512_maskz_permutexvar_epi64(mk_shift, idx_shift, vc);
        __m512i vout = _mm512_add_epi64(_mm512_add_epi64(vlimb, vcsh), vprev);
        _mm512_storeu_si512((void *)(rd + i), vout);
        vprev = _mm512_maskz_permutexvar_epi64(mk_lane0, idx_top, vc);
    }
    carry = (uint64_t)_mm_cvtsi128_si64(_mm512_castsi512_si128(vprev));
#endif
    for (; i < n; i++) {
        uint64_t hi, lo;
        lo = mul64_wide(ad[i], us, &hi);
        rd[i] = (lo & LIMB_MASK) + carry;
        carry = (lo >> BASE) | (hi << (64 - BASE));
    }
    if (carry != 0) {
        assert(res->al > n);
        rd[n] = carry;
        res->n = n + 1;
    } else {
        res->n = n;
    }
}


/* --------------------------------------------------------------------------
  Quadratic multiplication
No normalization: output has size max(a.n,b.n)*
 -------------------------------------------------------------------------- */
void bigint_mul_quadratic(bigint *r, const bigint *a, const bigint *b)
{
    const size_t an = a->n;
    const size_t bn = b->n;
    const size_t rn = an + bn;
    const size_t outn = r->n;
    const size_t shift = rn > outn ? rn - outn : 0;
    memset(r->d, 0, outn * sizeof(uint64_t));
#if USE_AVX512IFMA
    const __m512i idx_shift = _mm512_setr_epi64(0,0,1,2,3,4,5,6);
    const __m512i idx_top = _mm512_setr_epi64(7,0,0,0,0,0,0,0);
    const __mmask8 mk_shift = 0xfe;
    const __mmask8 mk_lane0 = 0x01;
    for (size_t j = 0; j < bn; ++j) {
        const __m512i vb = _mm512_set1_epi64(b->d[j]);
        size_t i = shift > j ? shift - j : 0;
        int have_prev = 0;
        __m512i vprev = _mm512_setzero_si512();
        for (; i + 8 <= an; i += 8) {
            const size_t k = i + j - shift;
            __m512i va = _mm512_loadu_si512((const void *)&a->d[i]);
            __m512i vlo52 = _mm512_madd52lo_epu64(_mm512_setzero_si512(), va, vb);
            __m512i vhi52 = _mm512_madd52hi_epu64(_mm512_setzero_si512(), va, vb);
            __m512i vc = _mm512_slli_epi64(vhi52, 52 - BASE);
            __m512i vcsh = _mm512_maskz_permutexvar_epi64(mk_shift, idx_shift, vc);
            __m512i acc = _mm512_loadu_si512((const void *)&r->d[k]);
            acc = _mm512_add_epi64(acc, vlo52);
            acc = _mm512_add_epi64(acc, vcsh);
            acc = _mm512_add_epi64(acc, vprev);
            _mm512_storeu_si512((void *)&r->d[k], acc);
            vprev = _mm512_maskz_permutexvar_epi64(mk_lane0, idx_top, vc);
            have_prev = 1;
        }
        if (i < an) {
            const unsigned int rem = (unsigned int)(an - i);
            const __mmask8 mask = (__mmask8)((1U << rem) - 1);
            const size_t k = i + j - shift;
            __m512i va = _mm512_maskz_loadu_epi64(mask, &a->d[i]);
            __m512i vlo52 = _mm512_madd52lo_epu64(_mm512_setzero_si512(), va, vb);
            __m512i vhi52 = _mm512_madd52hi_epu64(_mm512_setzero_si512(), va, vb);
            __m512i vc = _mm512_slli_epi64(vhi52, 52 - BASE);
            __m512i vcsh = _mm512_maskz_permutexvar_epi64(mk_shift, idx_shift, vc);
            __m512i acc = _mm512_maskz_loadu_epi64(mask, &r->d[k]);
            acc = _mm512_add_epi64(acc, vlo52);
            acc = _mm512_add_epi64(acc, vcsh);
            acc = _mm512_add_epi64(acc, vprev);
            _mm512_mask_storeu_epi64(&r->d[k], mask, acc);
            uint64_t tmp[8];
            _mm512_storeu_si512((void *)tmp, vc);
            r->d[k + rem] += tmp[rem - 1];
        } else if (have_prev) {
            const size_t k = i + j - shift;
            r->d[k] += (uint64_t)_mm_cvtsi128_si64(_mm512_castsi512_si128(vprev));
        }
    }
#else
    for (size_t j = 0; j < bn; ++j) {
        const uint64_t bj = b->d[j];
        size_t i = shift > j ? shift - j : 0;
        for (; i < an; ++i) {
            __extension__ unsigned __int128 p = (unsigned __int128)a->d[i] * bj;
            r->d[i + j - shift] += (uint64_t)p & LIMB_MASK;
            r->d[i + j + 1 - shift] += (uint64_t)(p >> BASE);
        }
    }
#endif
    if (shift) {
        size_t t = shift - 1;
        size_t j0 = t >= an ? t - (an - 1) : 0;
        size_t j1 = t < bn ? t : bn - 1;
        for (size_t j = j0; j <= j1; ++j) {
            size_t i = t - j;
            __extension__ __int128 tmp = ((unsigned __int128)a->d[i] * b->d[j]) >> BASE;
             r->d[0] += (uint64_t)tmp;
        }
    }
    r->s=a->s*b->s;
}

void bigint_normalize(bigint *a)
{
    uint64_t carry = 0;

    for (size_t i = 0; i < a->n; i++) {
        uint64_t t = a->d[i] + carry;
        a->d[i] = t & LIMB_MASK;
        carry   = t >> BASE;
    }
    if(carry){
        assert(a->n<a->al);
        a->d[a->n]=carry;
        a->n++;
    }
}

void bigint_fast_normalize(bigint *a)
{
    if (!a || a->n == 0)
        return;

    uint64_t *d = a->d;
    size_t    n = a->n;
    size_t    i = 0;
    uint64_t prev_carry = 0;

    /*----- 8-limb vector loop -----------------------------------------*/
#if USE_AVX512IFMA
    const __m512i vmask     = _mm512_set1_epi64(LIMB_MASK);
    const __m512i perm_idx  = _mm512_set_epi64(6,5,4,3,2,1,0,0);
    const __m512i carry_idx = _mm512_set1_epi64(7);   /* read lane 7 */

    __m512i pending = _mm512_setzero_si512();   /* [carry,0,0,0,0,0,0,0] */

    for (; i + 8 <= n; i += 8) {
        __m512i v    = _mm512_loadu_si512((__m512i const *)(d + i));
        __m512i low  = _mm512_and_si512(v, vmask);
        __m512i high = _mm512_srli_epi64(v, BASE);

        /* shifted = [0, h0, h1, h2, h3, h4, h5, h6] */
        __m512i shifted = _mm512_maskz_permutexvar_epi64(0xFE, perm_idx, high);

        /* pending holds the carry from the previous 8-limb block in lane 0 */
        __m512i add = _mm512_or_si512(shifted, pending);
        __m512i out = _mm512_add_epi64(low, add);
        _mm512_storeu_si512((__m512i *)(d + i), out);

        /* isolate h7 into lane 0 for the next iteration */
        pending = _mm512_maskz_permutexvar_epi64(0x01, carry_idx, high);
    }
    /* pull the scalar out of the vector register once, outside the hot loop */
    __m128i pending_lo = _mm512_extracti64x2_epi64(pending, 0);
    prev_carry = (uint64_t)_mm_cvtsi128_si64(pending_lo);
#endif

    /*----- Scalar tail ------------------------------------------------*/

//    printf("%zu %zu\n",a->n,a->al);
    for (; i < n; i++) {
        uint64_t c = d[i] >> BASE;
//        printf("%zu %zu %zu\n",i,d[i],prev_carry);
        d[i] = (d[i] & LIMB_MASK) + prev_carry;
        prev_carry = c;
    }
    d[n-1] += prev_carry << BASE;
}


static void bigint_fast_normalize_signed_positive(bigint *a){
    int64_t *d = (int64_t *)a->d;
    size_t   n = a->n;
    size_t   i = 0;
    const uint64_t C = 1ULL << (64 - BASE);
    int64_t carryin = 0;
//    bigint_print(a);
#if USE_AVX512
    const __m512i vc        = _mm512_set1_epi64(C);
    const __m512i vone      = _mm512_set1_epi64(1);
    const __m512i perm_idx  = _mm512_set_epi64(6,5,4,3,2,1,0,0);
    const __m512i carry_idx = _mm512_set1_epi64(7);

    __m512i vpending = _mm512_setzero_si512();   /* [carry,0,0,0,0,0,0,0] */

    for (; i + 8 <= n; i += 8) {
        __m512i v = _mm512_loadu_si512((__m512i const *)(d + i));

        /* carryout = ((d[i] + C) >> BASE) - 1   (signed arithmetic) */
        __m512i v_plus_c   = _mm512_add_epi64(v, vc);          /* d[i]+C  (mod 2^64) */

        /* ----- signed (arithmetic) right shift by BASE ----- */
        __mmask8 neg      = _mm512_movepi64_mask(v_plus_c);   /* sign bits */
        __m512i sign_ext  = _mm512_maskz_set1_epi64(neg, -1ULL);
        sign_ext          = _mm512_slli_epi64(sign_ext, 64 - BASE);
        __m512i v_q       = _mm512_srli_epi64(v_plus_c, BASE);
        v_q               = _mm512_or_si512(v_q, sign_ext);   /* arithmetic >> BASE */
        /* --------------------------------------------------- */

        __m512i v_carryout = _mm512_sub_epi64(v_q, vone);      /* q - 1   (signed)   */

        /* Shift carryouts right by one lane for distance-1 propagation.
         * shifted = [0, c0, c1, c2, c3, c4, c5, c6] */
        __m512i v_shifted = _mm512_maskz_permutexvar_epi64(0xFE, perm_idx, v_carryout);

        /* Inject the carry that spilled from the previous 8-limb block */
        __m512i v_carryin = _mm512_or_si512(v_shifted, vpending);

        /* residual = d[i] + carryin - carryout * 2^BASE */
        __m512i v_out = _mm512_sub_epi64(v, _mm512_slli_epi64(v_carryout, BASE));
        v_out = _mm512_add_epi64(v_out, v_carryin);

        _mm512_storeu_si512((__m512i *)(d + i), v_out);

        /* Save carryout[7] into lane 0 for the next iteration */
        vpending = _mm512_maskz_permutexvar_epi64(0x01, carry_idx, v_carryout);
    }
    __m128i pending_lo = _mm512_extracti64x2_epi64(vpending, 0);
    carryin = _mm_cvtsi128_si64(pending_lo);
#elif USE_AVX2
    const __m256i vc          = _mm256_set1_epi64x(C);
    const __m256i vone        = _mm256_set1_epi64x(1);
    const __m256i zero_lane0  = _mm256_set_epi64x(-1ULL, -1ULL, -1ULL, 0);
    const __m256i keep_lane0  = _mm256_set_epi64x(0, 0, 0, -1ULL);

    __m256i vpending = _mm256_setzero_si256();   /* [carry,0,0,0] */

    for (; i + 4 <= n; i += 4) {
        __m256i v = _mm256_loadu_si256((__m256i const *)(d + i));

        /* carryout = ((d[i] + C) >> BASE) - 1   (signed bit-arithmetic) */
        __m256i v_plus_c   = _mm256_add_epi64(v, vc);

        /* ----- signed (arithmetic) right shift by BASE ----- */
        __m256i sign_ext  = _mm256_cmpgt_epi64(_mm256_setzero_si256(), v_plus_c);
        sign_ext          = _mm256_slli_epi64(sign_ext, 64 - BASE);
        __m256i v_q       = _mm256_srli_epi64(v_plus_c, BASE);
        v_q               = _mm256_or_si256(v_q, sign_ext);   /* arithmetic >> BASE */
        /* --------------------------------------------------- */

        __m256i v_carryout = _mm256_sub_epi64(v_q, vone);

        /* shifted = [0, c0, c1, c2] */
        __m256i perm    = _mm256_permute4x64_epi64(v_carryout, 0x90); /* [c0,c0,c1,c2] */
        __m256i shifted = _mm256_and_si256(perm, zero_lane0);           /* [0,c0,c1,c2] */

        __m256i v_carryin = _mm256_or_si256(shifted, vpending);

        /* residual = d[i] + carryin - carryout * 2^BASE */
        __m256i v_out = _mm256_sub_epi64(v, _mm256_slli_epi64(v_carryout, BASE));
        v_out = _mm256_add_epi64(v_out, v_carryin);

        _mm256_storeu_si256((__m256i *)(d + i), v_out);

        /* save carryout[3] into lane 0 for the next block */
        __m256i pc = _mm256_permute4x64_epi64(v_carryout, 0x03); /* [c3,c0,c0,c0] */
        vpending   = _mm256_and_si256(pc, keep_lane0);            /* [c3,0,0,0] */
    }
    __m128i pend_lo = _mm256_extracti128_si256(vpending, 0);
    carryin = _mm_cvtsi128_si64(pend_lo);
#endif

      /*----- scalar tail ------------------------------------------------*/

//    puts("signed norm");
//    printf("%zu\n",i);
	for (; i < n; i++) {
		int64_t t = (int64_t)d[i] + C;
		int64_t  carryout = (int64_t)(t >> BASE) - 1;
		d[i] = d[i] + (uint64_t)carryin
			- ((uint64_t)carryout << BASE);
//        printf("%zu %ld %ld\n",i,t,carryout);
        carryin = carryout;
    }
	d[n-1] += (uint64_t)carryin << BASE;
//    bigint_print(a);
  //  puts("renormalise");
    for(i=n-1;UNLIKELY((int64_t)d[i]<0 && i);i--){
        d[i]+=1;
//        printf("%zu new %lld\n",i,d[i-1]-(1ll<<BASE));
        d[i-1]-=1ll<<BASE;
    }
//    bigint_print(a);
}


/* ------------------------------------------------------------------ */
/*  Scratch needed in addition to the result array r (size na+nb).  */
/* ------------------------------------------------------------------ */
size_t bigint_karatsuba_work_size(size_t na, size_t nb)
{

    if (na > nb) { size_t t = na; na = nb; nb = t; }
    if (na <= KARAT_THRESHOLD) return 0;

    size_t m  = na / 2;
    size_t nh = na - m;
    size_t p1_len = nh + nb - m;          /* length of (a0+a1)*(b0+b1) */

    size_t rec0 = bigint_karatsuba_work_size(nh, nh);
    size_t rec1 = bigint_karatsuba_work_size(nh, nb - m);

    /* layout:  p1  |  save (m limbs)/  recursive work  */
    return max(2*p1_len+2*m,p1_len+max(rec0,rec1));
}

/* ------------------------------------------------------------------ */
/*  Recursive worker.                                                 */
/*  * a,b,r,work must be aligned to KARAT_ALIGN bytes.             */
/*  * r must not alias a or b.                                       */
/*  * All arithmetic inside the recursion is limb-wise (no carries). */
/* ------------------------------------------------------------------ */
static void karatsuba_mul_rec(uint64_t *a, size_t na,
                              uint64_t *b, size_t nb,
                              uint64_t *r, uint64_t *work,int nbadd)
{
    /* ensure na is the shorter length so we split the limiting operand */
    if (na > nb) {
        uint64_t *t = a; a = b; b = t;
        size_t tt = na; na = nb; nb = tt;
    }

    if (na <= KARAT_THRESHOLD) {
        bigint A={na,0,1,a},B={nb,0,1,b},R={na+nb,0,1,r};
        if(nbadd>=(1<<(52-BASE)) || 1){
            bigint_fast_normalize(&A);
            bigint_fast_normalize(&B);
        }
        bigint_mul_quadratic(&R,&A,&B);
//        bigint_fast_normalize(&R);
        return;
    }

    size_t m  = na / 2;
    size_t nh = na - m;
    size_t p1_len = nh + nb - m;
    uint64_t *p1   = work;
    uint64_t *nwork =p1+p1_len;
    uint64_t *p0 = r;
    uint64_t *p2=r+2*m;

    // nbadd is multiplied in worst case by 2*na/KARAT before mul_quad; normalize early so that everything is correct
    if(nbadd*na>(KARAT_THRESHOLD<<(61-BASE)) || 1){
        bigint A={na,0,1,a},B={nb,0,1,b};
        bigint_fast_normalize(&A);
        bigint_fast_normalize(&B);
        nbadd=2;
    }

    /* -------------------------------------------------------------- */
    /*  1. Form T1 = a0+a1 and T2 = b0+b1 in result (no carry)  */
    /* -------------------------------------------------------------- */
#pragma omp simd
    for (size_t i = 0; i < m; ++i) {
        r[nb-m+i]      = a[i] + a[i + m];      /* T1 */
        r[i] = b[i] + b[i + m];      /* T2 */
    }
    if(na&1)
        r[nb] = a[2*m];                  /* T1 high */
    for (size_t i = m; i < nb - m; ++i)
        r[i] = b[i + m];             /* T2 high */

    /* -------------------------------------------------------------- */
    /*  2. P1 = T1 * T2  ->  temporary storage                       */
    /* -------------------------------------------------------------- */

    karatsuba_mul_rec(r+nb-m, nh, r, nb - m, p1,nwork,nbadd*2);
    karatsuba_mul_rec(a, m, b, m, p0,nwork,nbadd);
    karatsuba_mul_rec(a + m, nh, b + m, nb - m, p2,nwork,nbadd); /* P2 */

    /* -------------------------------------------------------------- */
    /*  4. Combine:  R = P0 + (P1 - P0 - P2)*B^m + P2*B^(2m)          */
    /*     The middle term is added at offset m.                     */
    /*     r[m..2m-1] (high half of P0) is saved first because the   */
    /*     write position m+i overtakes it when i reaches m.           */
    /* -------------------------------------------------------------- */

    memcpy(nwork,p2,(p1_len)*8);
    p2=nwork;
    memcpy(nwork+p1_len,p0,(2*m)*8);
    p0=nwork+p1_len;
        /*  low part of P1: i in [0, m-1]  */
#pragma omp simd
    for (size_t i = 0; i < 2*m; ++i) {
        int64_t mid = p1[i] - p0[i] - p2[i];
//        if(nwork[0]==589130796865892ll) mid=p1[i];
//        printf("%lld (%lld %lld %lld)\n",mid,p1[i],p0[i],nwork[i]);
        r[m + i] += mid;
    }
  //  puts("");
    for (size_t i = 2 * m;i < p1_len; ++i) {
        int64_t mid = p1[i] - p2[i];
        r[m + i] += mid;
    }
//    bigint_printx_s(&R);

    if((na>=128 && na<=256) || (na>=(1<<11) && na<=(1<<12))){
        bigint R={na+nb,0,1,r};
        bigint_fast_normalize_signed_positive(&R);
    }
}

/* ------------------------------------------------------------------ */
/*  Public entry point.                                               */
/*  r  must have room for na+nb limbs.                                */
/*  work must have at least karatsuba_work_size(na,nb) limbs and be   */
/*  aligned to KARAT_ALIGN bytes.                                     */
/*                                                                    */
/*  After return, r[0..na+nb-1] holds the polynomial product. */
/*  Output is "fast normalized" */
/* ------------------------------------------------------------------ */
void bigint_mul_karatsuba(const bigint *a,const bigint *b,bigint *r, uint64_t *work,int nbadd)
{
    r->n=a->n+b->n;
    assert(a->n+b->n<=r->al);
    memset(r->d,0,r->n*8);
    if(a->n+r->n<3*KARAT_THRESHOLD){
        r->s=a->s*b->s;
        bigint_mul_quadratic(r,a,b);
        bigint_fast_normalize(r);
        return;
    }
    karatsuba_mul_rec((uint64_t*)a->d, a->n, (uint64_t*)b->d, b->n, r->d, work,nbadd);
    r->s=a->s*b->s;
    bigint_fast_normalize_signed_positive(r);
}

void bigint_fast_normalize_signed(bigint *a){
//    puts("plop");
//    bigint_print(a);
    bigint_fast_normalize_signed_positive(a);
  //  bigint_print(a);
    int64_t *d = (int64_t *)a->d;
    size_t   n = a->n;
    size_t   i = 0;
    int64_t carryin = 0;
    /*----- sign detection & signed normalization ----------------------*/

    /* Find most significant non-zero limb */
    size_t msb = n;
    while (msb > 0 && d[msb - 1] == 0) msb--;

    int sign = 1;
    if (msb > 0) {
        int64_t v = d[msb - 1];
        if (v > 2) {
            sign = 1;
        } else if (v < -2) {
            sign = -1;
        } else {
            /* |v| <= 2 : a carry coming from the left (i.e. further
               normalization) could still flip or cancel this limb.
               Run a second pass to fully stabilize before deciding. */
            carryin = 0;
            for (i = 0; i < n; i++) {
                int64_t t = (int64_t)d[i] + carryin;
                int64_t  carryout = (int64_t)(t >> BASE);
				d[i] = t - carryout * (INT64_C(1) << BASE);
  //              printf("%zu t=%ld c=%ld\n",i,t,carryout);
                carryin = carryout;
            }
			d[n - 1] += carryin * (INT64_C(1) << BASE);
//    bigint_print(a);

            msb=n;

            /* Re-evaluate the most significant non-zero limb */
            while (msb > 0 && d[msb - 1] == 0) msb--;
            if (msb > 0) {
                if ((int64_t)d[msb - 1] > 0) sign = 1;
                else if ((int64_t)d[msb - 1] < 0) sign = -1;
            }
        }
    }
//    printf("sign=%d\n",sign);

    /* If the value is negative, negate all limbs and re-normalize
       so that the redundant digit array becomes positive. */
    if (sign < 0) {
        carryin = 0;
        for (i = 0; i < n; i++) {
            int64_t t = -(int64_t)d[i]+carryin;
            int64_t  carryout = (int64_t)(t >> BASE) ;
			d[i] = t - carryout * (INT64_C(1) << BASE);
//            printf("%zu t=%ld c=%ld\n",i,t,carryout);
            carryin = carryout;
        }
		d[n - 1] += carryin * (INT64_C(1) << BASE);
    }
    a->s*=sign;
}


void bigint_print(const bigint *a){
    printf("%zu s=%d:",a->n,a->s);
    for(size_t i=0;i<a->n;i++)
        printf("%lu ",a->d[i]);
    printf("\n");
}


/*
 * bigint_reciprocal_newton
 *
 * Compute  res = floor( 2^(BIGINT_BASE*e) / a )  using Newton iteration.
 * All multiplications are performed with Karatsuba.
 *
 * a       – non-zero, normalized bigint.
 * e       – exponent (>= 0).
 * work    – contiguous uint64_t scratch buffer.
 * work_n  – number of uint64_t slots in work.
 *
 * Workspace layout (all bigint structs live on the stack, only d[] points here):
 *
 *   [0] x     : e+2 limbs   current iterate
 *   [1] ax    : e+2 limbs   a * x
 *   [2] r     : e+2 limbs   B^e - a*x
 *   [3] xerr  : 2e+4 limbs  x * r   (product before division by B^e)
 *   [6] kwork : bigint_karatsuba_work_size(e+2, e+2)  (Karatsuba scratch)
 *
 * Total need = 7*e + 14 + bigint_karatsuba_work_size(e+2, e+2) uint64_t.
 *
 * res must be allocated by the caller with at least e+2 limbs.
 */
void bigint_reciprocal(bigint *res, const bigint *a, int64_t e,uint64_t *work)
{

    size_t n       = a->n;
    res->s=a->s;
    while(n>1 && !a->d[n-1])
        n--;
    if (e < 0 || (n==1 && !a->d[0])) {
        bigint_zero(res);
        return;
    }
    size_t limb_n  = e+1;                 /* limbs for x,ax */
    size_t xerr_n  = e+1+n;                  /* 2*e+4, enough for x*r     */
    uint64_t *wp = work;

    /* --- stack bigints, d[] carved out of the single work buffer --- */
    bigint ax, xerr;

#define WS_INIT(b, sz) do { \
    (b).d  = wp;            \
    (b).al = (sz);          \
    (b).n  = 0;             \
    wp    += (sz);          \
} while (0)

    WS_INIT(ax,   limb_n);
    WS_INIT(xerr, xerr_n);

    uint64_t *kwork = wp;   /* remaining space, size >= ksz */

    /* ---------- trivial cases ---------- */
    if (e < (int64_t)n - 1) {          /* B^e < B^(n-1) <= a  =>  Q = 0 */
        bigint_zero(res);
        return;
    }

    if (e == (int64_t)n - 1) {         /* Q is 1 iff a == B^(n-1)       */
        int is_pow = (a->d[n - 1] == 1);
        for (size_t i = 0; is_pow && i < n - 1; ++i)
            if (a->d[i] != 0) is_pow = 0;

        bigint_zero(res);
        if (is_pow) {
            res->d[0] = 1;
            res->n    = 1;
        }
        return;
    }
    /* ---------- initial underestimate ----------
       x0 = floor( B / (a_top + 1) ) * B^(e-n)
       This is always <= the true quotient.                        */
    memset(res->d, 0, res->al * sizeof(uint64_t));
    double top=a->d[n-1];
    if(n>1)
        top+=a->d[n-2]/(double)(1ull<<BASE);
    double inva=(1ULL << BIGINT_BASE)/top;
    uint64_t q0  = inva;    /* floor(B/top) */

    if(e-n==0){
        res->d[0] = q0;
        res->n = 1;
    }
    else{
        res->d[1] = q0;
        inva=(inva-q0)*(1ull<<BASE);
        res->d[0]=inva;
        res->n = 2;

    }

    /* bigint x ~= B^(n+x.n-1)/a ~= B^(n+x.n-1-e)*x */

    /* ---------- Newton iteration ----------
       x_{k+1} = x_k + floor( x_k * (B^e - a*x_k) / B^e ) */
    int iter=0;
    while(res->n<=e-n || iter==0) {
//        printf("res=");bigint_printx(res);
        /* ax = a * x  (needs n + x.n limbs, fits in ax.al) */
        ax.n = n + res->n;
        assert(ax.al>=ax.n);
        bigint_mul_karatsuba(a, res, &ax, kwork, 2);   /* inputs clean */
//        printf("ax=");
  //      bigint_printx(&ax);

        for(size_t i=0;i<ax.n;i++)
            ax.d[i]=-ax.d[i];
        int expo=ax.n-1;
        ax.d[expo]++;
        bigint_fast_normalize_signed(&ax);
        bigint_trim(&ax);
  //      printf("num-ax=");
//        bigint_printx(&ax);
        int shr=max(-(n+res->n)+2*ax.n-1,0);
//        printf("%zu %zu %zu :%d\n",ax.n,res->n,n,shr);
        bigint_shr(&ax,shr);
//        printf("ax=");bigint_printx(&ax);

        /* xerr = x * r  (needs x.n + r.n limbs, fits in xerr.al) */
        xerr.n = res->n + ax.n;
        bigint_mul_karatsuba(res, &ax, &xerr, kwork, 2);
//        bigint_printx(&xerr);
        int shl=min(ax.n-1-(iter>0),e-res->n-n+1);
//        printf("shl:%d axn=%zu\n",shl,ax.n);
        bigint_shl(res,shl);
        bigint_shr(&xerr,expo-shl-shr);
//        printf("xerr=");bigint_printx(&xerr);
        bigint_add(res,res,&xerr);
        bigint_fast_normalize(res);
        iter++;
    }
  //      printf("res=");bigint_printx(res);

}

void bigint_div_pow2(bigint *a, uint64_t k)
{
    if (a->n == 0)
        return;

    uint64_t limb_shift = k / BIGINT_BASE;
    unsigned bit_shift = (unsigned)(k % BIGINT_BASE);

    /*
     * First remove whole base-B limbs.
     */
    if (limb_shift >= a->n) {
        bigint_zero(a);
        return;
    }

    if (limb_shift != 0) {
        memmove(
            a->d,
            a->d + limb_shift,
            (a->n - limb_shift) * sizeof(uint64_t)
        );
        a->n -= limb_shift;
    }

    /*
     * Then shift by the remaining bit count inside base-B limbs.
     */
    if (bit_shift != 0) {
        bigint_normalize(a);
        const uint64_t mask = (1ULL << bit_shift) - 1;
        uint64_t carry = 0;

        /*
         * Little-endian limbs:
         *
         *   a = d[0] + d[1]*B + d[2]*B^2 + ...
         *
         * For right shift, bits flow from high limbs to low limbs,
         * so we scan from most significant limb downwards.
         */
        for (size_t i = a->n-1; i<a->n;i--) {
            uint64_t x = a->d[i];
            uint64_t new_carry = x & mask;

//            printf("%zu:%lu %lu\n",i,x,new_carry);
            a->d[i] = (x >> bit_shift)
                    + (carry << (BIGINT_BASE - bit_shift));

            carry = new_carry;
        }

    }

    /*
     * Trim leading zero limbs.
     */
    while (a->n > 0 && a->d[a->n - 1] == 0)
        a->n--;
//    printf("%zu\n",a->n);

    if (a->n == 0)
        a->d[0] = 0;
}
