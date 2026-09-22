#ifndef PORS_U256_H
#define PORS_U256_H

#include <stdint.h>
#include <stddef.h>

#ifndef PORS_U256_LIMBS
#error "PORS_U256_LIMBS must be defined before including pors_u256.h"
#endif

#if !defined(__SIZEOF_INT128__)
#error "pors_u256 requires unsigned __int128 support"
#endif

__extension__ typedef unsigned __int128 pors_u128;

typedef struct {
    uint64_t limb[PORS_U256_LIMBS];
} pors_u256;

static inline void pors_u256_zero(pors_u256 *x)
{
    size_t i;

    for (i = 0; i < PORS_U256_LIMBS; i++) {
        x->limb[i] = 0;
    }
}

static inline void pors_u256_set_u32(pors_u256 *x, uint32_t v)
{
    size_t i;

    x->limb[0] = v;
    for (i = 1; i < PORS_U256_LIMBS; i++) {
        x->limb[i] = 0;
    }
}

static inline void pors_u256_set_u64(pors_u256 *x, uint64_t v)
{
    size_t i;

    x->limb[0] = v;
    for (i = 1; i < PORS_U256_LIMBS; i++) {
        x->limb[i] = 0;
    }
}

static inline void pors_u256_load_limbs(pors_u256 *out,
                                         const uint64_t *src,
                                         uint32_t src_limbs)
{
    uint32_t i;

    for (i = 0; i < src_limbs; i++) {
        out->limb[i] = src[i];
    }
    for (; i < PORS_U256_LIMBS; i++) {
        out->limb[i] = 0;
    }
}

static inline int pors_u256_is_zero(const pors_u256 *x)
{
    size_t i;
    uint64_t acc = 0;

    for (i = 0; i < PORS_U256_LIMBS; i++) {
        acc |= x->limb[i];
    }

    return acc == 0;
}

static inline int pors_u256_to_u32_checked(uint32_t *out, const pors_u256 *x)
{
    size_t i;

    if (x->limb[0] > UINT32_MAX) {
        return -1;
    }
    for (i = 1; i < PORS_U256_LIMBS; i++) {
        if (x->limb[i] != 0) {
            return -1;
        }
    }

    *out = (uint32_t)x->limb[0];
    return 0;
}

static inline int pors_u256_to_u64_checked(uint64_t *out, const pors_u256 *x)
{
    size_t i;

    for (i = 1; i < PORS_U256_LIMBS; i++) {
        if (x->limb[i] != 0) {
            return -1;
        }
    }

    *out = x->limb[0];
    return 0;
}

static inline int pors_u256_cmp(const pors_u256 *a, const pors_u256 *b)
{
    size_t i = PORS_U256_LIMBS;

    while (i > 0) {
        i--;
        if (a->limb[i] < b->limb[i]) {
            return -1;
        }
        if (a->limb[i] > b->limb[i]) {
            return 1;
        }
    }

    return 0;
}

static inline uint64_t pors_u256_add_assign(pors_u256 *a, const pors_u256 *b)
{
    size_t i;
    uint64_t carry = 0;

    for (i = 0; i < PORS_U256_LIMBS; i++) {
        const uint64_t ai = a->limb[i];
        const uint64_t bi = b->limb[i];
        const uint64_t tmp = ai + carry;
        const uint64_t carry1 = (tmp < ai);
        const uint64_t res = tmp + bi;
        const uint64_t carry2 = (res < tmp);

        a->limb[i] = res;
        carry = carry1 | carry2;
    }

    return carry;
}

static inline uint64_t pors_u256_sub(pors_u256 *out,
                                      const pors_u256 *a,
                                      const pors_u256 *b)
{
    size_t i;
    uint64_t borrow = 0;

    for (i = 0; i < PORS_U256_LIMBS; i++) {
        const uint64_t ai = a->limb[i];
        const uint64_t bi = b->limb[i];
        const uint64_t tmp = ai - borrow;
        const uint64_t borrow1 = (ai < borrow);
        const uint64_t res = tmp - bi;
        const uint64_t borrow2 = (tmp < bi);

        out->limb[i] = res;
        borrow = borrow1 | borrow2;
    }

    return borrow;
}

static inline uint64_t pors_u256_sub_assign(pors_u256 *a, const pors_u256 *b)
{
    size_t i;
    uint64_t borrow = 0;

    for (i = 0; i < PORS_U256_LIMBS; i++) {
        const uint64_t ai = a->limb[i];
        const uint64_t bi = b->limb[i];
        const uint64_t tmp = ai - borrow;
        const uint64_t borrow1 = (ai < borrow);
        const uint64_t res = tmp - bi;
        const uint64_t borrow2 = (tmp < bi);

        a->limb[i] = res;
        borrow = borrow1 | borrow2;
    }

    return borrow;
}

/* acc = (acc + term) mod mod. Requires 0 <= acc, term < mod and mod < 2^(64*limbs). */
static inline void pors_u256_add_mod_assign(pors_u256 *acc,
                                             const pors_u256 *term,
                                             const pors_u256 *mod)
{
    const uint64_t carry = pors_u256_add_assign(acc, term);

    if (carry != 0u || pors_u256_cmp(acc, mod) >= 0) {
        (void)pors_u256_sub_assign(acc, mod);
    }
}

static inline void pors_u256_mul_u32(pors_u256 *x, uint32_t m)
{
    pors_u128 carry = 0;
    size_t i;

    for (i = 0; i < PORS_U256_LIMBS; i++) {
        const pors_u128 v = (pors_u128)x->limb[i] * m + carry;
        x->limb[i] = (uint64_t)v;
        carry = v >> 64;
    }
}

static inline uint32_t pors_u32_gcd(uint32_t a, uint32_t b)
{
    while (b != 0u) {
        const uint32_t t = a % b;
        a = b;
        b = t;
    }

    return a;
}

static inline uint32_t pors_u32_ctz(uint32_t x)
{
    uint32_t n = 0;

    while ((x & 1u) == 0u) {
        x >>= 1;
        n++;
    }

    return n;
}

static inline void pors_u256_rshift_bits(pors_u256 *x, uint32_t s)
{
    uint64_t carry = 0;
    size_t i = PORS_U256_LIMBS;

    if (s == 0u) {
        return;
    }

    while (i > 0) {
        const uint64_t limb = x->limb[i - 1u];
        x->limb[i - 1u] = (limb >> s) | carry;
        carry = limb << (64u - s);
        i--;
    }
}

static inline uint64_t pors_u256_inv_odd_u64(uint32_t d)
{
    uint64_t inv = d;

    inv *= 2u - (uint64_t)d * inv;
    inv *= 2u - (uint64_t)d * inv;
    inv *= 2u - (uint64_t)d * inv;
    inv *= 2u - (uint64_t)d * inv;
    inv *= 2u - (uint64_t)d * inv;
    inv *= 2u - (uint64_t)d * inv;

    return inv;
}

static inline void pors_u256_div_odd_u32_exact(pors_u256 *x, uint32_t d)
{
    const uint64_t inv = pors_u256_inv_odd_u64(d);
    uint64_t carry = 0;
    size_t i;

    for (i = 0; i < PORS_U256_LIMBS; i++) {
        const uint64_t ai = x->limb[i];
        const uint64_t qi = (ai - carry) * inv;
        const pors_u128 prod = (pors_u128)qi * d + carry;

        x->limb[i] = qi;
        carry = (uint64_t)((prod - ai) >> 64);
    }
}

/* Exact division by a non-zero 32-bit integer. The input must be divisible by d. */
static inline void pors_u256_div_u32_exact(pors_u256 *x, uint32_t d)
{
    uint32_t shift;

    if (d == 1u) {
        return;
    }

    shift = pors_u32_ctz(d);
    if (shift != 0u) {
        pors_u256_rshift_bits(x, shift);
        d >>= shift;
    }

    if (d != 1u) {
        pors_u256_div_odd_u32_exact(x, d);
    }
}

#endif
