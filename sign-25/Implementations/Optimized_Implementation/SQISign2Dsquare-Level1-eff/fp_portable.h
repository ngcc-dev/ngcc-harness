#ifndef SQISIGN_FP_PORTABLE_H
#define SQISIGN_FP_PORTABLE_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifndef SQISIGN_HAS_UINT128
#if defined(__SIZEOF_INT128__) && !defined(SQISIGN_FORCE_PORTABLE_FP)
#define SQISIGN_HAS_UINT128 1
#else
#define SQISIGN_HAS_UINT128 0
#endif
#endif

#define SQISIGN_MAX_FIELD_WORDS 18

static inline void
sqisign_mul64wide(uint64_t x, uint64_t y, uint64_t *hi, uint64_t *lo)
{
    const uint64_t x0 = (uint32_t)x;
    const uint64_t x1 = x >> 32;
    const uint64_t y0 = (uint32_t)y;
    const uint64_t y1 = y >> 32;

    const uint64_t p00 = x0 * y0;
    const uint64_t p01 = x0 * y1;
    const uint64_t p10 = x1 * y0;
    const uint64_t p11 = x1 * y1;

    const uint64_t mid = (p00 >> 32) + (uint32_t)p01 + (uint32_t)p10;
    *lo = (p00 & UINT64_C(0xffffffff)) | (mid << 32);
    *hi = p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32);
}

static inline uint64_t
sqisign_add_u64(uint64_t *x, uint64_t y)
{
    const uint64_t old = *x;
    *x = old + y;
    return *x < old;
}

static inline uint64_t
sqisign_subborrow_u64(uint64_t a, uint64_t b, uint64_t *borrow)
{
    const uint64_t bb = b + *borrow;
    const uint64_t bcarry = bb < b;
    const uint64_t r = a - bb;
    *borrow = bcarry | (a < bb);
    return r;
}

static inline uint64_t
sqisign_muladd_u64(uint64_t acc, uint64_t x, uint64_t y, uint64_t carry, uint64_t *out)
{
    uint64_t hi, lo;
    sqisign_mul64wide(x, y, &hi, &lo);
    hi += sqisign_add_u64(&lo, acc);
    hi += sqisign_add_u64(&lo, carry);
    *out = lo;
    return hi;
}

static inline void
sqisign_montgomery_mul_portable(uint64_t *out,
                                const uint64_t *a,
                                const uint64_t *b,
                                const uint64_t *modulus,
                                size_t nwords)
{
    uint64_t t[SQISIGN_MAX_FIELD_WORDS + 2];
    uint64_t sub[SQISIGN_MAX_FIELD_WORDS];

    memset(t, 0, sizeof(t));

    for (size_t i = 0; i < nwords; i++) {
        uint64_t carry = 0;
        for (size_t j = 0; j < nwords; j++) {
            carry = sqisign_muladd_u64(t[j], a[i], b[j], carry, &t[j]);
        }
        t[nwords + 1] += sqisign_add_u64(&t[nwords], carry);

        const uint64_t q = t[0];
        carry = 0;
        for (size_t j = 0; j < nwords; j++) {
            uint64_t lo;
            carry = sqisign_muladd_u64(t[j], q, modulus[j], carry, &lo);
            if (j != 0) {
                t[j - 1] = lo;
            }
        }

        uint64_t top = t[nwords];
        const uint64_t top_carry = sqisign_add_u64(&top, carry);
        t[nwords - 1] = top;
        t[nwords] = t[nwords + 1] + top_carry;
        t[nwords + 1] = 0;
    }

    uint64_t borrow = 0;
    for (size_t i = 0; i < nwords; i++) {
        sub[i] = sqisign_subborrow_u64(t[i], modulus[i], &borrow);
    }

    const uint64_t use_sub = (t[nwords] != 0) | (borrow == 0);
    const uint64_t mask = (uint64_t)0 - use_sub;
    for (size_t i = 0; i < nwords; i++) {
        out[i] = (sub[i] & mask) | (t[i] & ~mask);
    }
}

#endif
