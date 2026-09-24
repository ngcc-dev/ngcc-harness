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
* CHAMP-512 optimized implementation (NGCC ICCS submission)
*
* Hash function based on walks in the Cayley graph of GL(2, F_p) where
*     p = 2^128 - 15449  (a 128-bit safe prime; p-1 = 2q, q prime)
* and generators
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
typedef struct { u128 m[4]; } Mat2x2;
typedef struct { int32_t m[4]; } SmallMat32;
typedef struct { u128 mag; int neg; } s128;

static const u128 P = {{ 0xFFFFFFFFFFFFC3A7ULL, 0xFFFFFFFFFFFFFFFFULL }};

static const u128 GEN_A[4] = {
    {{0xFFFFFFFFFFFFC3A5ULL, 0xFFFFFFFFFFFFFFFFULL}}, {{4,0}}, {{1,0}}, {{0xFFFFFFFFFFFFC3A4ULL, 0xFFFFFFFFFFFFFFFFULL}}
};
static const u128 GEN_B[4] = {
    {{0xFFFFFFFFFFFFC3A2ULL, 0xFFFFFFFFFFFFFFFFULL}},
    {{0xFFFFFFFFFFFFC3A1ULL, 0xFFFFFFFFFFFFFFFFULL}},
    {{2, 0}},
    {{2, 0}}
};
static const u128 MAT_IDENTITY[4] = {
    {{1,0}}, {{0,0}}, {{0,0}}, {{1,0}}
};
static const u128 ZERO_U128 = {{0,0}};

static const int32_t GEN_A_SMALL[4] = { -2, 4, 1, -3 };
static const int32_t GEN_B_SMALL[4] = { -5, -6, 2, 2 };

static SmallMat32 small_byte_table[256];
static int small_byte_table_ready = 0;

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

static inline uint64_t u128_sub_borrow(u128 *r, const u128 *a, const u128 *b)
{
    uint64_t borrow = 0;
    int i;
    for (i = 0; i < 2; i++) {
        uint64_t t = a->w[i] - borrow;
        uint64_t b0 = (a->w[i] < borrow) ? 1 : 0;
        uint64_t t2 = t - b->w[i];
        uint64_t b1 = (t < b->w[i]) ? 1 : 0;
        r->w[i] = t2;
        borrow = b0 + b1;
    }
    return borrow;
}

static inline void u128_sub(u128 *r, const u128 *a, const u128 *b)
{
    (void)u128_sub_borrow(r, a, b);
}

static inline uint64_t u128_sub_masked_c(u128 *r, uint64_t sub_c)
{
    uint64_t sub = 15449ULL & (0ULL - sub_c);
    uint64_t old0 = r->w[0];
    uint64_t borrow;
    r->w[0] = old0 - sub;
    borrow = (old0 < sub) ? 1 : 0;
    {
        uint64_t old1 = r->w[1];
        r->w[1] = old1 - borrow;
        borrow = (old1 < borrow) ? 1 : 0;
    }
    return borrow;
}

static inline void submod_u128_lazy(u128 *r, const u128 *a, const u128 *b)
{
    uint64_t borrow0 = u128_sub_borrow(r, a, b);
    uint64_t borrow1 = u128_sub_masked_c(r, borrow0);
    (void)u128_sub_masked_c(r, borrow0 & borrow1);
}

static inline void reduce256_lazy(u128 *r, const uint64_t a[4])
{
    static const uint64_t c = 15449ULL;
    uint64_t hi_c[3];
    uint64_t t[3];

    {
        uint64_t carry = 0;
        int i;
        for (i = 0; i < 2; i++) {
            uint64_t lo, hi;
            mul_u64_wide(a[2 + i], c, &lo, &hi);
            {
                uint64_t s = lo + carry;
                uint64_t c0 = (s < lo) ? 1 : 0;
                hi_c[i] = s;
                carry = hi + c0;
            }
        }
        hi_c[2] = carry;
    }

    {
        uint64_t carry = 0;
        int i;
        for (i = 0; i < 2; i++) {
            uint64_t s = a[i] + hi_c[i];
            uint64_t c0 = (s < a[i]) ? 1 : 0;
            s += carry;
            t[i] = s;
            carry = c0 + ((s < carry) ? 1 : 0);
        }
        t[2] = hi_c[2] + carry;
    }

    {
        uint64_t extra = t[2] * c;
        uint64_t carry;
        r->w[0] = t[0] + extra;
        carry = (r->w[0] < t[0]) ? 1 : 0;
        r->w[1] = t[1] + carry;
        carry = (r->w[1] < t[1]) ? 1 : 0;
        if (carry) {
            uint64_t old0 = r->w[0];
            r->w[0] += c;
            r->w[1] += (r->w[0] < old0) ? 1 : 0;
        }
    }
}

static inline void canonicalize_lazy_u128(u128 *a)
{
    u128 tmp;
    uint64_t borrow = u128_sub_borrow(&tmp, a, &P);
    uint64_t mask = 0ULL - borrow;
    a->w[0] = (tmp.w[0] & ~mask) | (a->w[0] & mask);
    a->w[1] = (tmp.w[1] & ~mask) | (a->w[1] & mask);
}

static inline void reduce256(u128 *r, const uint64_t a[4])
{
    reduce256_lazy(r, a);
    canonicalize_lazy_u128(r);
}

static inline void addmod(u128 *r, const u128 *a, const u128 *b)
{
    uint64_t carry = u128_add(r, a, b);
    if (carry || u128_gte(r, &P)) {
        u128_sub(r, r, &P);
    }
}

static inline void mulmod(u128 *r, const u128 *a, const u128 *b)
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
        {
            int k = i + 2;
            while (carry != 0 && k < 4) {
                uint64_t s = prod[k] + carry;
                carry = (s < prod[k]) ? 1 : 0;
                prod[k] = s;
                k++;
            }
        }
    }
    reduce256(r, prod);
}

static inline void mul_u128_raw_accumulate(uint64_t acc[4], const u128 *a, const u128 *b)
{
#if defined(__SIZEOF_INT128__)
    int i;
    for (i = 0; i < 2; i++) {
        wide_u128 carry = 0;
        int j;
        for (j = 0; j < 2; j++) {
            carry += (wide_u128)a->w[i] * (wide_u128)b->w[j] + (wide_u128)acc[i + j];
            acc[i + j] = (uint64_t)carry;
            carry >>= 64;
        }
        {
            int k = i + 2;
            while (carry != 0 && k < 4) {
                carry += (wide_u128)acc[k];
                acc[k] = (uint64_t)carry;
                carry >>= 64;
                k++;
            }
        }
    }
#else
    int i;
    for (i = 0; i < 2; i++) {
        uint64_t carry = 0;
        int j;
        for (j = 0; j < 2; j++) {
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
            int k = i + 2;
            while (carry != 0 && k < 4) {
                uint64_t s = acc[k] + carry;
                carry = (s < acc[k]) ? 1 : 0;
                acc[k] = s;
                k++;
            }
        }
    }
#endif
}

static inline void mat2x2_mul(u128 r[4], const u128 x[4], const u128 y[4])
{
    u128 t0, t1;
    mulmod(&t0, &x[0], &y[0]); mulmod(&t1, &x[2], &y[1]); addmod(&r[0], &t0, &t1);
    mulmod(&t0, &x[1], &y[0]); mulmod(&t1, &x[3], &y[1]); addmod(&r[1], &t0, &t1);
    mulmod(&t0, &x[0], &y[2]); mulmod(&t1, &x[2], &y[3]); addmod(&r[2], &t0, &t1);
    mulmod(&t0, &x[1], &y[2]); mulmod(&t1, &x[3], &y[3]); addmod(&r[3], &t0, &t1);
}

static inline void invmod(u128 *r, const u128 *a)
{
    u128 exp = {{ 0xFFFFFFFFFFFFC3A5ULL, 0xFFFFFFFFFFFFFFFFULL }};
    u128 base = *a;
    u128 result = {{ 1, 0 }};
    int limb;
    for (limb = 0; limb < 2; limb++) {
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

static inline void field_from_i64(u128 *r, int64_t x)
{
    if (x < 0) {
        u128 mag = {{ 0ULL - (uint64_t)x, 0 }};
        u128_sub(r, &P, &mag);
    } else {
        r->w[0] = (uint64_t)x;
        r->w[1] = 0;
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

static inline void apply_small32_matrix(u128 state[4], const int32_t m[4])
{
    u128 field_mat[4];
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
 * The exact 4-byte chunk coefficients stay below 2^128, so each raw product
 * fits in 256 bits and a single fold modulo 2^128 - c is enough before the
 * sign correction.
 */
static inline void linear_combo_lazy(u128 *r,
                                     const u128 *x0, const s128 *y0,
                                     const u128 *x1, const s128 *y1)
{
    uint64_t pos[4] = {0};
    uint64_t neg[4] = {0};
    int pos_used = 0;
    int neg_used = 0;
    u128 pos_rep = {{0,0}};
    u128 neg_rep = {{0,0}};

    if (!u128_is_zero(&y0->mag)) {
        if (y0->neg) {
            mul_u128_raw_accumulate(neg, x0, &y0->mag);
            neg_used = 1;
        } else {
            mul_u128_raw_accumulate(pos, x0, &y0->mag);
            pos_used = 1;
        }
    }

    if (!u128_is_zero(&y1->mag)) {
        if (y1->neg) {
            mul_u128_raw_accumulate(neg, x1, &y1->mag);
            neg_used = 1;
        } else {
            mul_u128_raw_accumulate(pos, x1, &y1->mag);
            pos_used = 1;
        }
    }

    if (!neg_used) {
        reduce256_lazy(r, pos);
        return;
    }

    if (!pos_used) {
        reduce256_lazy(&neg_rep, neg);
        submod_u128_lazy(r, &ZERO_U128, &neg_rep);
        return;
    }

    reduce256_lazy(&pos_rep, pos);
    reduce256_lazy(&neg_rep, neg);
    submod_u128_lazy(r, &pos_rep, &neg_rep);
}

static inline void apply_small128_matrix_lazy(u128 state[4], const s128 m[4])
{
    u128 next[4];
    linear_combo_lazy(&next[0], &state[0], &m[0], &state[2], &m[1]);
    linear_combo_lazy(&next[1], &state[1], &m[0], &state[3], &m[1]);
    linear_combo_lazy(&next[2], &state[0], &m[2], &state[2], &m[3]);
    linear_combo_lazy(&next[3], &state[1], &m[2], &state[3], &m[3]);
    memcpy(state, next, sizeof(next));
}

static void process_chunk_aligned_bytes(u128 state[4], const unsigned char *msg,
                                        unsigned long long start_byte,
                                        unsigned long long end_byte)
{
    unsigned long long i;

    for (i = start_byte; i < end_byte; i += 4ULL) {
        int64_t p12[4], p34[4];
        s128 chunk[4];

        small_mat32_mul_to_i64(p12, small_byte_table[msg[i + 0]].m, small_byte_table[msg[i + 1]].m);
        small_mat32_mul_to_i64(p34, small_byte_table[msg[i + 2]].m, small_byte_table[msg[i + 3]].m);
        small_mat64_mul_to_s128(chunk, p12, p34);
        apply_small128_matrix_lazy(state, chunk);
    }
}

static void process_scalar_bytes(u128 state[4], const unsigned char *msg,
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
    u128 state[4];
    u128 u1, u2, u3, u4;
    const u128 *out[4];
    int ci, limb;

    (void)digest_len_bits;

    build_small_byte_table();
    memcpy(state, MAT_IDENTITY, sizeof(MAT_IDENTITY));

    full_bytes = msg_len_bits >> 3;
    full_chunk_bytes = full_bytes & ~3ULL;

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

    canonicalize_lazy_u128(&state[0]);
    canonicalize_lazy_u128(&state[1]);
    canonicalize_lazy_u128(&state[2]);
    canonicalize_lazy_u128(&state[3]);

    if (u128_is_zero(&state[0])) memset(&u1, 0, sizeof(u1)); else invmod(&u1, &state[0]);
    if (u128_is_zero(&state[1])) memset(&u2, 0, sizeof(u2)); else invmod(&u2, &state[1]);
    if (u128_is_zero(&state[2])) memset(&u3, 0, sizeof(u3)); else invmod(&u3, &state[2]);
    if (u128_is_zero(&state[3])) memset(&u4, 0, sizeof(u4)); else invmod(&u4, &state[3]);

    out[0] = &u1;
    out[1] = &u2;
    out[2] = &u3;
    out[3] = &u4;

    for (ci = 0; ci < 4; ci++) {
        for (limb = 0; limb < 2; limb++) {
            uint64_t val = out[ci]->w[limb];
            digest[ci * 16 + limb * 8 + 0] = (unsigned char)(val);
            digest[ci * 16 + limb * 8 + 1] = (unsigned char)(val >> 8);
            digest[ci * 16 + limb * 8 + 2] = (unsigned char)(val >> 16);
            digest[ci * 16 + limb * 8 + 3] = (unsigned char)(val >> 24);
            digest[ci * 16 + limb * 8 + 4] = (unsigned char)(val >> 32);
            digest[ci * 16 + limb * 8 + 5] = (unsigned char)(val >> 40);
            digest[ci * 16 + limb * 8 + 6] = (unsigned char)(val >> 48);
            digest[ci * 16 + limb * 8 + 7] = (unsigned char)(val >> 56);
        }
    }

    return 0;
}
