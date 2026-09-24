/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

/*
* CHAMP-1024 optimized implementation (NGCC ICCS submission)
*
* Hash function based on walks in the Cayley graph of GL(2, F_p) where
*     p = 2^256 - 36113  (a 256-bit safe prime; p-1 = 2q, q prime)
* and generator matrices
*     A = [[-2, 1], [4, -3]]
*     B = [[-5, 2], [-6, 2]]  (entries reduced mod p)
* 
* Each input BIT is mapped to a matrix (0 -> A, 1 -> B), MSB-first within 
* a byte. The product of these matrices is accumulated modulo p, and the 
* final result is obtained by inverting and concatenating its entries.
*/

#include <stdint.h>
#include <string.h>
#include "CryptHash_AlgorithmInstance.h"

#if defined(__SIZEOF_INT128__)
__extension__ typedef unsigned __int128 wide_u128;
__extension__ typedef __int128 wide_s128;
#endif

typedef struct { uint64_t w[2]; } u128;
typedef struct { uint64_t w[4]; } u256;
typedef struct { u256 m[4]; } Mat2x2;
typedef struct { int32_t m[4]; } SmallMat32;
typedef struct { u128 mag; int neg; } s128;
typedef struct { u256 mag; int neg; } s256;

static const u256 P = {{
    0xFFFFFFFFFFFF72EFULL,
    0xFFFFFFFFFFFFFFFFULL,
    0xFFFFFFFFFFFFFFFFULL,
    0xFFFFFFFFFFFFFFFFULL
}};

static const u256 GEN_A[4] = {
        {{0xFFFFFFFFFFFF72EDULL, 0xFFFFFFFFFFFFFFFFULL,
            0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}},
        {{4,0,0,0}},
        {{1,0,0,0}},
        {{0xFFFFFFFFFFFF72ECULL, 0xFFFFFFFFFFFFFFFFULL,
            0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}}
};
static const u256 GEN_B[4] = {
        {{0xFFFFFFFFFFFF72EAULL, 0xFFFFFFFFFFFFFFFFULL,
      0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}},
        {{0xFFFFFFFFFFFF72E9ULL, 0xFFFFFFFFFFFFFFFFULL,
            0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}},
        {{2,0,0,0}},
        {{2,0,0,0}}
};
static const u256 MAT_IDENTITY[4] = {
    {{1,0,0,0}}, {{0,0,0,0}}, {{0,0,0,0}}, {{1,0,0,0}}
};
static const u256 ZERO_U256 = {{0,0,0,0}};

static const int32_t GEN_A_SMALL[4] = { -2, 4, 1, -3 };
static const int32_t GEN_B_SMALL[4] = { -5, -6, 2, 2 };

static SmallMat32 small_byte_table[256];
static int small_byte_table_ready = 0;

static inline void reduce512_lazy(u256 *r, const uint64_t a[8]);
static inline void canonicalize_lazy_u256(u256 *a);

#if defined(__SIZEOF_INT128__)
static inline uint64_t u64_mulhi(uint64_t a, uint64_t b)
{
    return (uint64_t)(((wide_u128)a * (wide_u128)b) >> 64);
}

static inline void mul_u64_wide(uint64_t a, uint64_t b, uint64_t *lo, uint64_t *hi)
{
    wide_u128 prod = (wide_u128)a * (wide_u128)b;
    *lo = (uint64_t)prod;
    *hi = (uint64_t)(prod >> 64);
}
#else
static inline uint64_t u64_mulhi(uint64_t a, uint64_t b)
{
    uint32_t a0 = (uint32_t)a;
    uint32_t a1 = (uint32_t)(a >> 32);
    uint32_t b0 = (uint32_t)b;
    uint32_t b1 = (uint32_t)(b >> 32);
    uint64_t p00 = (uint64_t)a0 * b0;
    uint64_t p01 = (uint64_t)a0 * b1;
    uint64_t p10 = (uint64_t)a1 * b0;
    uint64_t p11 = (uint64_t)a1 * b1;
    uint64_t mid = (p00 >> 32) + (uint32_t)p01 + (uint32_t)p10;
    return p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32);
}

static inline void mul_u64_wide(uint64_t a, uint64_t b, uint64_t *lo, uint64_t *hi)
{
    *lo = a * b;
    *hi = u64_mulhi(a, b);
}
#endif

static inline int u128_gte(const u128 *a, const u128 *b)
{
    if (a->w[1] != b->w[1]) return a->w[1] > b->w[1];
    return a->w[0] >= b->w[0];
}

static inline int u128_is_zero(const u128 *a)
{
    return (a->w[0] | a->w[1]) == 0;
}

static inline uint64_t u128_add(u128 *r, const u128 *a, const u128 *b)
{
    uint64_t s0 = a->w[0] + b->w[0];
    uint64_t c0 = (s0 < a->w[0]) ? 1 : 0;
    uint64_t s1 = a->w[1] + b->w[1];
    uint64_t c1 = (s1 < a->w[1]) ? 1 : 0;
    s1 += c0;
    r->w[0] = s0;
    r->w[1] = s1;
    return c1 + ((s1 < c0) ? 1 : 0);
}

static inline void u128_sub(u128 *r, const u128 *a, const u128 *b)
{
    uint64_t a0 = a->w[0];
    uint64_t a1 = a->w[1];
    uint64_t w0 = a0 - b->w[0];
    uint64_t borrow = (a0 < b->w[0]) ? 1 : 0;
    r->w[0] = w0;
    r->w[1] = a1 - b->w[1] - borrow;
}

static inline int u256_gte(const u256 *a, const u256 *b)
{
    int i;
    for (i = 3; i >= 0; i--) {
        if (a->w[i] > b->w[i]) return 1;
        if (a->w[i] < b->w[i]) return 0;
    }
    return 1;
}

static inline int u256_is_zero(const u256 *a)
{
    return (a->w[0] | a->w[1] | a->w[2] | a->w[3]) == 0;
}

static inline uint64_t u256_add(u256 *r, const u256 *a, const u256 *b)
{
    uint64_t carry = 0;
    int i;
    for (i = 0; i < 4; i++) {
        uint64_t s = a->w[i] + b->w[i];
        uint64_t c0 = (s < a->w[i]) ? 1 : 0;
        s += carry;
        r->w[i] = s;
        carry = c0 + ((s < carry) ? 1 : 0);
    }
    return carry;
}

static inline uint64_t u256_sub_borrow(u256 *r, const u256 *a, const u256 *b)
{
    uint64_t borrow = 0;
    int i;
    for (i = 0; i < 4; i++) {
        uint64_t t = a->w[i] - borrow;
        uint64_t b0 = (a->w[i] < borrow) ? 1 : 0;
        uint64_t t2 = t - b->w[i];
        uint64_t b1 = (t < b->w[i]) ? 1 : 0;
        r->w[i] = t2;
        borrow = b0 + b1;
    }
    return borrow;
}

static inline void u256_sub(u256 *r, const u256 *a, const u256 *b)
{
    (void)u256_sub_borrow(r, a, b);
}

static inline uint64_t u256_sub_masked_c(u256 *r, uint64_t sub_c)
{
    uint64_t sub = 36113ULL & (0ULL - sub_c);
    uint64_t old0 = r->w[0];
    uint64_t borrow;
    int i;
    r->w[0] = old0 - sub;
    borrow = (old0 < sub) ? 1 : 0;
    for (i = 1; i < 4; i++) {
        uint64_t w = r->w[i] - borrow;
        borrow = (r->w[i] < borrow) ? 1 : 0;
        r->w[i] = w;
    }
    return borrow;
}

static inline void submod_u256_lazy(u256 *r, const u256 *a, const u256 *b)
{
    uint64_t borrow0 = u256_sub_borrow(r, a, b);
    uint64_t borrow1 = u256_sub_masked_c(r, borrow0);
    (void)u256_sub_masked_c(r, borrow0 & borrow1);
}

static inline void reduce512(u256 *r, const uint64_t a[8])
{
    reduce512_lazy(r, a);
    canonicalize_lazy_u256(r);
}

static inline void reduce512_lazy(u256 *r, const uint64_t a[8])
{
    static const uint64_t c = 36113ULL;
    uint64_t hi_c[5];
    uint64_t t[5];

    {
        uint64_t carry = 0;
        int i;
        for (i = 0; i < 4; i++) {
            uint64_t lo, hi;
            mul_u64_wide(a[4 + i], c, &lo, &hi);
            {
                uint64_t s = lo + carry;
                uint64_t c0 = (s < lo) ? 1 : 0;
                hi_c[i] = s;
                carry = hi + c0;
            }
        }
        hi_c[4] = carry;
    }

    {
        uint64_t carry = 0;
        int i;
        for (i = 0; i < 4; i++) {
            uint64_t s = a[i] + hi_c[i];
            uint64_t c0 = (s < a[i]) ? 1 : 0;
            s += carry;
            t[i] = s;
            carry = c0 + ((s < carry) ? 1 : 0);
        }
        t[4] = hi_c[4] + carry;
    }

    {
        uint64_t extra = t[4] * c;
        uint64_t carry;
        r->w[0] = t[0] + extra;
        carry = (r->w[0] < t[0]) ? 1 : 0;
        r->w[1] = t[1] + carry;
        carry = (r->w[1] < t[1]) ? 1 : 0;
        r->w[2] = t[2] + carry;
        carry = (r->w[2] < t[2]) ? 1 : 0;
        r->w[3] = t[3] + carry;
        carry = (r->w[3] < t[3]) ? 1 : 0;
        r->w[0] += carry * c;
    }
}

static inline void canonicalize_lazy_u256(u256 *a)
{
    u256 tmp;
    uint64_t borrow = u256_sub_borrow(&tmp, a, &P);
    uint64_t mask = 0ULL - borrow;
    int i;
    for (i = 0; i < 4; i++) {
        a->w[i] = (tmp.w[i] & ~mask) | (a->w[i] & mask);
    }
}

static inline void addmod(u256 *r, const u256 *a, const u256 *b)
{
    uint64_t carry = u256_add(r, a, b);
    if (carry || u256_gte(r, &P)) {
        u256_sub(r, r, &P);
    }
}

static void mulmod(u256 *r, const u256 *a, const u256 *b)
{
    uint64_t prod[8] = {0};
    int i;
    for (i = 0; i < 4; i++) {
        uint64_t carry = 0;
        int j;
        for (j = 0; j < 4; j++) {
            uint64_t lo, hi;
            mul_u64_wide(a->w[i], b->w[j], &lo, &hi);
            uint64_t s = prod[i + j] + carry;
            uint64_t c0 = (s < prod[i + j]) ? 1 : 0;
            uint64_t s2 = s + lo;
            uint64_t c1 = (s2 < s) ? 1 : 0;
            prod[i + j] = s2;
            carry = hi + c0 + c1;
        }
        prod[i + 4] += carry;
    }
    reduce512(r, prod);
}

static inline void mul_u256_raw_accumulate(uint64_t acc[8], const u256 *a, const u256 *b)
{
#if defined(__SIZEOF_INT128__)
    int i;
    for (i = 0; i < 4; i++) {
        wide_u128 carry = 0;
        int j;
        for (j = 0; j < 4; j++) {
            carry += (wide_u128)a->w[i] * (wide_u128)b->w[j] + (wide_u128)acc[i + j];
            acc[i + j] = (uint64_t)carry;
            carry >>= 64;
        }
        {
            int k = i + 4;
            while (carry != 0) {
                carry += (wide_u128)acc[k];
                acc[k] = (uint64_t)carry;
                carry >>= 64;
                k++;
            }
        }
    }
#else
    int i;
    for (i = 0; i < 4; i++) {
        uint64_t carry = 0;
        int j;
        for (j = 0; j < 4; j++) {
            uint64_t lo, hi;
            mul_u64_wide(a->w[i], b->w[j], &lo, &hi);
            {
                uint64_t s = acc[i + j] + carry;
                uint64_t c0 = (s < acc[i + j]) ? 1 : 0;
                uint64_t s2 = s + lo;
                uint64_t c1 = (s2 < s) ? 1 : 0;
                acc[i + j] = s2;
                carry = hi + c0 + c1;
            }
        }
        {
            int k = i + 4;
            while (carry != 0) {
                uint64_t s = acc[k] + carry;
                carry = (s < acc[k]) ? 1 : 0;
                acc[k] = s;
                k++;
            }
        }
    }
#endif
}

static inline void mat2x2_mul(u256 r[4], const u256 x[4], const u256 y[4])
{
    u256 t0, t1;
    mulmod(&t0, &x[0], &y[0]); mulmod(&t1, &x[2], &y[1]); addmod(&r[0], &t0, &t1);
    mulmod(&t0, &x[1], &y[0]); mulmod(&t1, &x[3], &y[1]); addmod(&r[1], &t0, &t1);
    mulmod(&t0, &x[0], &y[2]); mulmod(&t1, &x[2], &y[3]); addmod(&r[2], &t0, &t1);
    mulmod(&t0, &x[1], &y[2]); mulmod(&t1, &x[3], &y[3]); addmod(&r[3], &t0, &t1);
}

static void invmod(u256 *r, const u256 *a)
{
    u256 exp = {{
        0xFFFFFFFFFFFF72EDULL,
        0xFFFFFFFFFFFFFFFFULL,
        0xFFFFFFFFFFFFFFFFULL,
        0xFFFFFFFFFFFFFFFFULL
    }};
    u256 base = *a;
    u256 result = {{1,0,0,0}};
    int limb;
    for (limb = 0; limb < 4; limb++) {
        uint64_t bits = exp.w[limb];
        int bit;
        for (bit = 0; bit < 64; bit++) {
            if (bits & 1ULL) mulmod(&result, &result, &base);
            mulmod(&base, &base, &base);
            bits >>= 1;
        }
    }
    *r = result;
}

static inline s128 s128_add_signed(const s128 *a, const s128 *b)
{
    s128 r;
    if (a->neg == b->neg) {
        u128_add(&r.mag, &a->mag, &b->mag);
        r.neg = a->neg;
    } else if (u128_gte(&a->mag, &b->mag)) {
        u128_sub(&r.mag, &a->mag, &b->mag);
        r.neg = a->neg;
    } else {
        u128_sub(&r.mag, &b->mag, &a->mag);
        r.neg = b->neg;
    }
    if (u128_is_zero(&r.mag)) r.neg = 0;
    return r;
}

static inline s128 mul_i64_i64(int64_t a, int64_t b)
{
    s128 r;
#if defined(__SIZEOF_INT128__)
    wide_s128 prod = (wide_s128)a * (wide_s128)b;
    wide_u128 mag = (prod < 0) ? (wide_u128)(-prod) : (wide_u128)prod;
    r.mag.w[0] = (uint64_t)mag;
    r.mag.w[1] = (uint64_t)(mag >> 64);
    r.neg = (prod < 0) && !u128_is_zero(&r.mag);
#else
    uint64_t ua = (a < 0) ? (0ULL - (uint64_t)a) : (uint64_t)a;
    uint64_t ub = (b < 0) ? (0ULL - (uint64_t)b) : (uint64_t)b;
    r.mag.w[0] = ua * ub;
    r.mag.w[1] = u64_mulhi(ua, ub);
    r.neg = ((a < 0) != (b < 0)) && !u128_is_zero(&r.mag);
#endif
    return r;
}

static inline void u128_mul_abs_to_u256(u256 *r, const u128 *a, const u128 *b)
{
    uint64_t prod[4] = {0};
    int i;
    for (i = 0; i < 2; i++) {
        uint64_t carry = 0;
        int j;
        for (j = 0; j < 2; j++) {
            uint64_t lo, hi;
            mul_u64_wide(a->w[i], b->w[j], &lo, &hi);
            uint64_t s = prod[i + j] + carry;
            uint64_t c0 = (s < prod[i + j]) ? 1 : 0;
            uint64_t s2 = s + lo;
            uint64_t c1 = (s2 < s) ? 1 : 0;
            prod[i + j] = s2;
            carry = hi + c0 + c1;
        }
        prod[i + 2] += carry;
    }
    r->w[0] = prod[0];
    r->w[1] = prod[1];
    r->w[2] = prod[2];
    r->w[3] = prod[3];
}

static inline s256 s256_add_signed(const s256 *a, const s256 *b)
{
    s256 r;
    if (a->neg == b->neg) {
        u256_add(&r.mag, &a->mag, &b->mag);
        r.neg = a->neg;
    } else if (u256_gte(&a->mag, &b->mag)) {
        u256_sub(&r.mag, &a->mag, &b->mag);
        r.neg = a->neg;
    } else {
        u256_sub(&r.mag, &b->mag, &a->mag);
        r.neg = b->neg;
    }
    if (u256_is_zero(&r.mag)) r.neg = 0;
    return r;
}

static inline s256 mul_s128_s128(const s128 *a, const s128 *b)
{
    s256 r;
    u128_mul_abs_to_u256(&r.mag, &a->mag, &b->mag);
    r.neg = (a->neg != b->neg) && !u256_is_zero(&r.mag);
    return r;
}

static inline void field_from_i64(u256 *r, int64_t x)
{
    if (x < 0) {
        u256 mag = {{ 0ULL - (uint64_t)x, 0, 0, 0 }};
        u256_sub(r, &P, &mag);
    } else {
        r->w[0] = (uint64_t)x;
        r->w[1] = 0;
        r->w[2] = 0;
        r->w[3] = 0;
    }
}

static inline void small_mat32_mul(int32_t r[4], const int32_t x[4], const int32_t y[4])
{
    r[0] = (int32_t)((int64_t)x[0] * y[0] + (int64_t)x[2] * y[1]);
    r[1] = (int32_t)((int64_t)x[1] * y[0] + (int64_t)x[3] * y[1]);
    r[2] = (int32_t)((int64_t)x[0] * y[2] + (int64_t)x[2] * y[3]);
    r[3] = (int32_t)((int64_t)x[1] * y[2] + (int64_t)x[3] * y[3]);
}

static inline void small_mat32_mul_to_i64(int64_t r[4], const int32_t x[4], const int32_t y[4])
{
    r[0] = (int64_t)x[0] * y[0] + (int64_t)x[2] * y[1];
    r[1] = (int64_t)x[1] * y[0] + (int64_t)x[3] * y[1];
    r[2] = (int64_t)x[0] * y[2] + (int64_t)x[2] * y[3];
    r[3] = (int64_t)x[1] * y[2] + (int64_t)x[3] * y[3];
}

static inline void small_mat64_mul_to_s128(s128 r[4], const int64_t x[4], const int64_t y[4])
{
    s128 t0, t1;
    t0 = mul_i64_i64(x[0], y[0]); t1 = mul_i64_i64(x[2], y[1]); r[0] = s128_add_signed(&t0, &t1);
    t0 = mul_i64_i64(x[1], y[0]); t1 = mul_i64_i64(x[3], y[1]); r[1] = s128_add_signed(&t0, &t1);
    t0 = mul_i64_i64(x[0], y[2]); t1 = mul_i64_i64(x[2], y[3]); r[2] = s128_add_signed(&t0, &t1);
    t0 = mul_i64_i64(x[1], y[2]); t1 = mul_i64_i64(x[3], y[3]); r[3] = s128_add_signed(&t0, &t1);
}

static inline void small_mat128_mul_to_s256(s256 r[4], const s128 x[4], const s128 y[4])
{
    s256 t0, t1;
    t0 = mul_s128_s128(&x[0], &y[0]); t1 = mul_s128_s128(&x[2], &y[1]); r[0] = s256_add_signed(&t0, &t1);
    t0 = mul_s128_s128(&x[1], &y[0]); t1 = mul_s128_s128(&x[3], &y[1]); r[1] = s256_add_signed(&t0, &t1);
    t0 = mul_s128_s128(&x[0], &y[2]); t1 = mul_s128_s128(&x[2], &y[3]); r[2] = s256_add_signed(&t0, &t1);
    t0 = mul_s128_s128(&x[1], &y[2]); t1 = mul_s128_s128(&x[3], &y[3]); r[3] = s256_add_signed(&t0, &t1);
}

static void populate_small_byte_table(void)
{
    int i;

    for (i = 0; i < 256; i++) {
        int32_t acc[4] = { 1, 0, 0, 1 };
        int b;
        for (b = 7; b >= 0; b--) {
            int32_t tmp[4];
            small_mat32_mul(tmp, acc, ((i >> b) & 1) ? GEN_B_SMALL : GEN_A_SMALL);
            memcpy(acc, tmp, sizeof(tmp));
        }
        memcpy(small_byte_table[i].m, acc, sizeof(acc));
    }

    small_byte_table_ready = 1;
}

static void build_small_byte_table(void)
{
    if (small_byte_table_ready) return;

    populate_small_byte_table();
}

static void apply_small32_matrix(u256 state[4], const int32_t m[4])
{
    u256 field_mat[4];
    Mat2x2 tmp;
    field_from_i64(&field_mat[0], m[0]);
    field_from_i64(&field_mat[1], m[1]);
    field_from_i64(&field_mat[2], m[2]);
    field_from_i64(&field_mat[3], m[3]);
    mat2x2_mul(tmp.m, state, field_mat);
    memcpy(state, tmp.m, sizeof(tmp.m));
}

/*
 * Reduce each output entry only once per linear combination.
 * The exact 8-byte chunk coefficients are small enough that the raw products
 * and their sums fit below 2^512, so a single fold back modulo 2^256 - c is
 * sufficient before handling the sign.
 */
static inline void linear_combo_lazy(u256 *r,
                                     const u256 *x0, const s256 *y0,
                                     const u256 *x1, const s256 *y1)
{
    uint64_t pos[8] = {0};
    uint64_t neg[8] = {0};
    int pos_used = 0;
    int neg_used = 0;
    u256 pos_rep = {{0,0,0,0}};
    u256 neg_rep = {{0,0,0,0}};

    if (!u256_is_zero(&y0->mag)) {
        if (y0->neg) {
            mul_u256_raw_accumulate(neg, x0, &y0->mag);
            neg_used = 1;
        } else {
            mul_u256_raw_accumulate(pos, x0, &y0->mag);
            pos_used = 1;
        }
    }

    if (!u256_is_zero(&y1->mag)) {
        if (y1->neg) {
            mul_u256_raw_accumulate(neg, x1, &y1->mag);
            neg_used = 1;
        } else {
            mul_u256_raw_accumulate(pos, x1, &y1->mag);
            pos_used = 1;
        }
    }

    if (!neg_used) {
        reduce512_lazy(r, pos);
        return;
    }

    if (!pos_used) {
        reduce512_lazy(&neg_rep, neg);
        submod_u256_lazy(r, &ZERO_U256, &neg_rep);
        return;
    }

    reduce512_lazy(&pos_rep, pos);
    reduce512_lazy(&neg_rep, neg);
    submod_u256_lazy(r, &pos_rep, &neg_rep);
}

static inline void apply_small256_matrix_lazy(u256 state[4], const s256 m[4])
{
    u256 next[4];
    linear_combo_lazy(&next[0], &state[0], &m[0], &state[2], &m[1]);
    linear_combo_lazy(&next[1], &state[1], &m[0], &state[3], &m[1]);
    linear_combo_lazy(&next[2], &state[0], &m[2], &state[2], &m[3]);
    linear_combo_lazy(&next[3], &state[1], &m[2], &state[3], &m[3]);
    memcpy(state, next, sizeof(next));
}

static void process_chunk_aligned_bytes(u256 state[4], const unsigned char *msg,
                                        unsigned long long start_byte,
                                        unsigned long long end_byte)
{
    unsigned long long i;

    for (i = start_byte; i < end_byte; i += 8ULL) {
        int64_t p12[4], p34[4], p56[4], p78[4];
        s128 q14[4], q58[4];
        s256 chunk[4];

        small_mat32_mul_to_i64(p12, small_byte_table[msg[i + 0]].m, small_byte_table[msg[i + 1]].m);
        small_mat32_mul_to_i64(p34, small_byte_table[msg[i + 2]].m, small_byte_table[msg[i + 3]].m);
        small_mat32_mul_to_i64(p56, small_byte_table[msg[i + 4]].m, small_byte_table[msg[i + 5]].m);
        small_mat32_mul_to_i64(p78, small_byte_table[msg[i + 6]].m, small_byte_table[msg[i + 7]].m);

        small_mat64_mul_to_s128(q14, p12, p34);
        small_mat64_mul_to_s128(q58, p56, p78);
        small_mat128_mul_to_s256(chunk, q14, q58);

        apply_small256_matrix_lazy(state, chunk);
    }
}

static void process_scalar_bytes(u256 state[4], const unsigned char *msg,
                                 unsigned long long start_byte,
                                 unsigned long long end_byte)
{
    unsigned long long i;

    for (i = start_byte; i < end_byte; i++) {
        apply_small32_matrix(state, small_byte_table[msg[i]].m);
    }
}

int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest)
{
    unsigned long long full_bytes, full_chunk_bytes;
    unsigned int remaining_bits;
    u256 state[4];
    u256 u1, u2, u3, u4;
    const u256 *out[4];
    int ci, limb;

    (void)digest_len_bits;

    build_small_byte_table();
    memcpy(state, MAT_IDENTITY, sizeof(MAT_IDENTITY));

    full_bytes = msg_len_bits >> 3;
    full_chunk_bytes = full_bytes & ~7ULL;

    if (full_chunk_bytes > 0) {
        process_chunk_aligned_bytes(state, msg, 0, full_chunk_bytes);
    }

    process_scalar_bytes(state, msg, full_chunk_bytes, full_bytes);

    remaining_bits = (unsigned int)(msg_len_bits & 7u);
    if (remaining_bits > 0) {
        unsigned char byte = msg[full_bytes];
        int b;
        for (b = 7; b >= (int)(8 - remaining_bits); b--) {
            Mat2x2 tmp;
            int bit = (byte >> b) & 1;
            mat2x2_mul(tmp.m, state, bit ? GEN_B : GEN_A);
            memcpy(state, tmp.m, sizeof(tmp.m));
        }
    }

    canonicalize_lazy_u256(&state[0]);
    canonicalize_lazy_u256(&state[1]);
    canonicalize_lazy_u256(&state[2]);
    canonicalize_lazy_u256(&state[3]);

    if (u256_is_zero(&state[0])) memset(&u1, 0, sizeof(u1)); else invmod(&u1, &state[0]);
    if (u256_is_zero(&state[1])) memset(&u2, 0, sizeof(u2)); else invmod(&u2, &state[1]);
    if (u256_is_zero(&state[2])) memset(&u3, 0, sizeof(u3)); else invmod(&u3, &state[2]);
    if (u256_is_zero(&state[3])) memset(&u4, 0, sizeof(u4)); else invmod(&u4, &state[3]);

    out[0] = &u1;
    out[1] = &u2;
    out[2] = &u3;
    out[3] = &u4;

    for (ci = 0; ci < 4; ci++) {
        for (limb = 0; limb < 4; limb++) {
            uint64_t val = out[ci]->w[limb];
            digest[ci * 32 + limb * 8 + 0] = (unsigned char)(val);
            digest[ci * 32 + limb * 8 + 1] = (unsigned char)(val >> 8);
            digest[ci * 32 + limb * 8 + 2] = (unsigned char)(val >> 16);
            digest[ci * 32 + limb * 8 + 3] = (unsigned char)(val >> 24);
            digest[ci * 32 + limb * 8 + 4] = (unsigned char)(val >> 32);
            digest[ci * 32 + limb * 8 + 5] = (unsigned char)(val >> 40);
            digest[ci * 32 + limb * 8 + 6] = (unsigned char)(val >> 48);
            digest[ci * 32 + limb * 8 + 7] = (unsigned char)(val >> 56);
        }
    }

    return 0;
}
