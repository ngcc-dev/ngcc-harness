#include <inttypes.h>
#include <encoded_sizes.h>
#include <fp2.h>
#include <pike_profile.h>
#include <string.h>

#if defined(PIKE_NGCC1_OPTIMIZED_FIELD) && PIKE_NGCC1_OPTIMIZED_FIELD && NWORDS_FIELD == 8
#define FP2_NGCC1_DIRECT_FIELD 1
#else
#define FP2_NGCC1_DIRECT_FIELD 0
#endif

#if defined(PIKE_NGCC2_OPTIMIZED_FIELD) && PIKE_NGCC2_OPTIMIZED_FIELD && NWORDS_FIELD == 13
#define FP2_NGCC2_DIRECT_FIELD 1
#define FP2_DIRECT_LIMBS 12
#else
#define FP2_NGCC2_DIRECT_FIELD 0
#endif

#ifndef FP2_DIRECT_LIMBS
#define FP2_DIRECT_LIMBS NWORDS_FIELD
#endif

#if FP2_NGCC1_DIRECT_FIELD
extern void fp479_fp2_add_asm(fp2_t *x, const fp2_t *y, const fp2_t *z);
extern void fp479_fp2_neg_asm(fp2_t *x, const fp2_t *y);
extern void fp479_fp2_sub_asm(fp2_t *x, const fp2_t *y, const fp2_t *z);
extern void fp479_fp2_mul_asm(fp2_t *x, const fp2_t *y, const fp2_t *z);
extern void fp479_fp2_sqr_asm(fp2_t *x, const fp2_t *y);
#endif

#if FP2_NGCC2_DIRECT_FIELD
extern void fp765_12_add_asm(uint64_t c[NWORDS_FIELD], const uint64_t a[NWORDS_FIELD],
                             const uint64_t b[NWORDS_FIELD]);
extern void fp765_12_double_asm(uint64_t c[NWORDS_FIELD], const uint64_t a[NWORDS_FIELD]);
extern void fp765_12_sub_asm(uint64_t c[NWORDS_FIELD], const uint64_t a[NWORDS_FIELD],
                             const uint64_t b[NWORDS_FIELD]);
extern void fp765_12_neg_asm(uint64_t c[NWORDS_FIELD], const uint64_t a[NWORDS_FIELD]);
extern void fp765_12_half_asm(uint64_t c[NWORDS_FIELD], const uint64_t a[NWORDS_FIELD]);
extern void fp765_12_montmul_asm(uint64_t *c, const uint64_t *a, const uint64_t *b);
extern void fp765_12_montsqr_asm(uint64_t *c, const uint64_t *a);
extern void fp765_12_fp2_add_asm(fp2_t *x, const fp2_t *y, const fp2_t *z);
extern void fp765_12_fp2_sub_asm(fp2_t *x, const fp2_t *y, const fp2_t *z);
extern void fp765_12_fp2_neg_asm(fp2_t *x, const fp2_t *y);
extern void fp765_12_fp2_half_asm(fp2_t *x, const fp2_t *y);
#endif

static inline void
fp2_field_zero(fp_t *x)
{
#if FP2_NGCC1_DIRECT_FIELD || FP2_NGCC2_DIRECT_FIELD
    memset(*x, 0, sizeof(fp_t));
#else
    fp_set_zero(x);
#endif
}

static inline void
fp2_field_one(fp_t *x)
{
#if FP2_NGCC1_DIRECT_FIELD || FP2_NGCC2_DIRECT_FIELD
    memcpy(*x, ONE, sizeof(fp_t));
#else
    fp_set_one(x);
#endif
}

static inline void
fp2_field_copy(fp_t *out, const fp_t *a)
{
#if FP2_NGCC2_DIRECT_FIELD
    memcpy(*out, *a, sizeof(uint64_t) * FP2_DIRECT_LIMBS);
    (*out)[12] = 0;
#elif FP2_NGCC1_DIRECT_FIELD
    memcpy(*out, *a, sizeof(fp_t));
#else
    fp_copy(out, a);
#endif
}

static inline uint32_t
fp2_field_is_equal(const fp_t *a, const fp_t *b)
{
#if FP2_NGCC1_DIRECT_FIELD || FP2_NGCC2_DIRECT_FIELD
    uint64_t diff = 0;

    for (int i = 0; i < FP2_DIRECT_LIMBS; i++) {
        diff |= (*a)[i] ^ (*b)[i];
    }

    return -(uint32_t)(diff == 0);
#else
    return fp_is_equal(a, b);
#endif
}

static inline uint32_t
fp2_field_is_zero(const fp_t *a)
{
#if FP2_NGCC1_DIRECT_FIELD || FP2_NGCC2_DIRECT_FIELD
    uint64_t nonzero = 0;

    for (int i = 0; i < FP2_DIRECT_LIMBS; i++) {
        nonzero |= (*a)[i];
    }

    return -(uint32_t)(nonzero == 0);
#else
    return fp_is_zero(a);
#endif
}

static inline void
fp2_field_select(fp_t *d, const fp_t *a0, const fp_t *a1, uint32_t ctl)
{
#if FP2_NGCC1_DIRECT_FIELD || FP2_NGCC2_DIRECT_FIELD
    uint64_t mask = (uint64_t)(int32_t)ctl;

    for (int i = 0; i < FP2_DIRECT_LIMBS; i++) {
        (*d)[i] = (*a0)[i] ^ (mask & ((*a0)[i] ^ (*a1)[i]));
    }
#if FP2_NGCC2_DIRECT_FIELD
    (*d)[12] = 0;
#endif
#else
    fp_select(d, a0, a1, ctl);
#endif
}

static inline void
fp2_field_cswap(fp_t *a, fp_t *b, uint32_t ctl)
{
#if FP2_NGCC1_DIRECT_FIELD || FP2_NGCC2_DIRECT_FIELD
    uint64_t mask = (uint64_t)(int32_t)ctl;

    for (int i = 0; i < FP2_DIRECT_LIMBS; i++) {
        uint64_t swap = mask & ((*a)[i] ^ (*b)[i]);
        (*a)[i] ^= swap;
        (*b)[i] ^= swap;
    }
#else
    fp_cswap(a, b, ctl);
#endif
}

static inline void
fp2_field_half(fp_t *out, const fp_t *a)
{
#if FP2_NGCC2_DIRECT_FIELD
    fp765_12_half_asm(*out, *a);
#else
    fp_half(out, a);
#endif
}

static inline void
fp2_field_add(fp_t *out, const fp_t *a, const fp_t *b)
{
#if FP2_NGCC2_DIRECT_FIELD
    fp765_12_add_asm(*out, *a, *b);
#else
    fp_add(out, a, b);
#endif
}

static inline void
fp2_field_sub(fp_t *out, const fp_t *a, const fp_t *b)
{
#if FP2_NGCC2_DIRECT_FIELD
    fp765_12_sub_asm(*out, *a, *b);
#else
    fp_sub(out, a, b);
#endif
}

static inline void
fp2_field_neg(fp_t *out, const fp_t *a)
{
#if FP2_NGCC2_DIRECT_FIELD
    fp765_12_neg_asm(*out, *a);
#else
    fp_neg(out, a);
#endif
}

static inline void
fp2_field_sqr(fp_t *out, const fp_t *a)
{
#if FP2_NGCC2_DIRECT_FIELD
    fp765_12_montsqr_asm(*out, *a);
#else
    fp_sqr(out, a);
#endif
}

static inline void
fp2_field_mul(fp_t *out, const fp_t *a, const fp_t *b)
{
#if FP2_NGCC2_DIRECT_FIELD
    fp765_12_montmul_asm(*out, *a, *b);
#else
    fp_mul(out, a, b);
#endif
}

/* Arithmetic modulo X^2 + 1 */

void
fp2_set_small(fp2_t *x, const digit_t val)
{
    fp_set_small(&(x->re), val);
    fp2_field_zero(&(x->im));
}

void
fp2_set_one(fp2_t *x)
{
    fp2_field_one(&(x->re));
    fp2_field_zero(&(x->im));
}

void
fp2_set_zero(fp2_t *x)
{
#if FP2_NGCC1_DIRECT_FIELD || FP2_NGCC2_DIRECT_FIELD
    memset(x, 0, sizeof(fp2_t));
#else
    fp2_field_zero(&(x->re));
    fp2_field_zero(&(x->im));
#endif
}

uint32_t
fp2_is_zero(const fp2_t *a)
{ // Is a GF(p^2) element zero?
  // Returns 0xFF...FF (true) if a=0, 0 (false) otherwise

#if FP2_NGCC1_DIRECT_FIELD || FP2_NGCC2_DIRECT_FIELD
    uint64_t nonzero = 0;

    for (int i = 0; i < FP2_DIRECT_LIMBS; i++) {
        nonzero |= a->re[i] | a->im[i];
    }

    return -(uint32_t)(nonzero == 0);
#else
    return fp2_field_is_zero(&(a->re)) & fp2_field_is_zero(&(a->im));
#endif
}

uint32_t
fp2_is_equal(const fp2_t *a, const fp2_t *b)
{ // Compare two GF(p^2) elements in constant time
  // Returns 0xFF...FF (true) if a=b, 0 (false) otherwise

#if FP2_NGCC1_DIRECT_FIELD || FP2_NGCC2_DIRECT_FIELD
    uint64_t diff = 0;

    for (int i = 0; i < FP2_DIRECT_LIMBS; i++) {
        diff |= (a->re[i] ^ b->re[i]) | (a->im[i] ^ b->im[i]);
    }

    return -(uint32_t)(diff == 0);
#else
    return fp2_field_is_equal(&(a->re), &(b->re)) & fp2_field_is_equal(&(a->im), &(b->im));
#endif
}

uint32_t
fp2_is_one(const fp2_t *a)
{ // Is a GF(p^2) element one?
  // Returns 0xFF...FF (true) if a=1, 0 (false) otherwise

#if FP2_NGCC1_DIRECT_FIELD || FP2_NGCC2_DIRECT_FIELD
    uint64_t diff = 0;

    for (int i = 0; i < FP2_DIRECT_LIMBS; i++) {
        diff |= (a->re[i] ^ ONE[i]) | a->im[i];
    }

    return -(uint32_t)(diff == 0);
#else
    return fp2_field_is_equal(&(a->re), &ONE) & fp2_field_is_zero(&(a->im));
#endif
}

void
fp2_select(fp2_t *d, const fp2_t *a0, const fp2_t *a1, uint32_t ctl)
{
#if FP2_NGCC1_DIRECT_FIELD || FP2_NGCC2_DIRECT_FIELD
    uint64_t mask = (uint64_t)(int32_t)ctl;

    for (int i = 0; i < FP2_DIRECT_LIMBS; i++) {
        d->re[i] = a0->re[i] ^ (mask & (a0->re[i] ^ a1->re[i]));
        d->im[i] = a0->im[i] ^ (mask & (a0->im[i] ^ a1->im[i]));
    }
#if FP2_NGCC2_DIRECT_FIELD
    d->re[12] = 0;
    d->im[12] = 0;
#endif
#else
    fp2_field_select(&(d->re), &(a0->re), &(a1->re), ctl);
    fp2_field_select(&(d->im), &(a0->im), &(a1->im), ctl);
#endif
}

void
fp2_cswap(fp2_t *a, fp2_t *b, uint32_t ctl)
{
#if FP2_NGCC1_DIRECT_FIELD || FP2_NGCC2_DIRECT_FIELD
    uint64_t mask = (uint64_t)(int32_t)ctl;

    for (int i = 0; i < FP2_DIRECT_LIMBS; i++) {
        uint64_t re_swap = mask & (a->re[i] ^ b->re[i]);
        uint64_t im_swap = mask & (a->im[i] ^ b->im[i]);
        a->re[i] ^= re_swap;
        b->re[i] ^= re_swap;
        a->im[i] ^= im_swap;
        b->im[i] ^= im_swap;
    }
#else
    fp2_field_cswap(&(a->re), &(b->re), ctl);
    fp2_field_cswap(&(a->im), &(b->im), ctl);
#endif
}

void
fp2_copy(fp2_t *x, const fp2_t *y)
{
#if FP2_NGCC2_DIRECT_FIELD
    memcpy(x->re, y->re, sizeof(uint64_t) * FP2_DIRECT_LIMBS);
    memcpy(x->im, y->im, sizeof(uint64_t) * FP2_DIRECT_LIMBS);
    x->re[12] = 0;
    x->im[12] = 0;
#elif FP2_NGCC1_DIRECT_FIELD
    memcpy(x, y, sizeof(fp2_t));
#else
    fp2_field_copy(&(x->re), &(y->re));
    fp2_field_copy(&(x->im), &(y->im));
#endif
}

void
fp2_encode(void *dst, const fp2_t *a)
{
    PIKE_PROFILE_START(prof_start, PIKE_PROFILE_FP2_ENCODE);
    uint8_t *buf = dst;
    fp_encode(buf, &(a->re));
    fp_encode(buf + FP_ENCODED_BYTES, &(a->im));
    PIKE_PROFILE_STOP(prof_start, PIKE_PROFILE_FP2_ENCODE);
}

void
fp2_decode(fp2_t *d, const void *src)
{
    PIKE_PROFILE_START(prof_start, PIKE_PROFILE_FP2_DECODE);
    const uint8_t *buf = src;

    fp_decode(&(d->re), buf);
    fp_decode(&(d->im), buf + FP_ENCODED_BYTES);
    PIKE_PROFILE_STOP(prof_start, PIKE_PROFILE_FP2_DECODE);
}

void
fp2_half(fp2_t *x, const fp2_t *y)
{
    PIKE_PROFILE_COUNT(PIKE_PROFILE_FP2_HALF);
#if FP2_NGCC2_DIRECT_FIELD
    fp765_12_fp2_half_asm(x, y);
#else
    fp2_field_half(&(x->re), &(y->re));
    fp2_field_half(&(x->im), &(y->im));
#endif
}

void
fp2_add(fp2_t *x, const fp2_t *y, const fp2_t *z)
{
    PIKE_PROFILE_COUNT(PIKE_PROFILE_FP2_ADD);
#if FP2_NGCC1_DIRECT_FIELD
    fp479_fp2_add_asm(x, y, z);
#elif FP2_NGCC2_DIRECT_FIELD
    fp765_12_fp2_add_asm(x, y, z);
#else
    fp2_field_add(&(x->re), &(y->re), &(z->re));
    fp2_field_add(&(x->im), &(y->im), &(z->im));
#endif
}

void
fp2_sub(fp2_t *x, const fp2_t *y, const fp2_t *z)
{
    PIKE_PROFILE_COUNT(PIKE_PROFILE_FP2_SUB);
#if FP2_NGCC1_DIRECT_FIELD
    fp479_fp2_sub_asm(x, y, z);
#elif FP2_NGCC2_DIRECT_FIELD
    fp765_12_fp2_sub_asm(x, y, z);
#else
    fp2_field_sub(&(x->re), &(y->re), &(z->re));
    fp2_field_sub(&(x->im), &(y->im), &(z->im));
#endif
}

void
fp2_neg(fp2_t *x, const fp2_t *y)
{
#if FP2_NGCC1_DIRECT_FIELD
    fp479_fp2_neg_asm(x, y);
#elif FP2_NGCC2_DIRECT_FIELD
    fp765_12_fp2_neg_asm(x, y);
#else
    fp2_field_neg(&(x->re), &(y->re));
    fp2_field_neg(&(x->im), &(y->im));
#endif
}

void
fp2_mul(fp2_t *x, const fp2_t *y, const fp2_t *z)
{
    PIKE_PROFILE_COUNT(PIKE_PROFILE_FP2_MUL);
#if FP2_NGCC1_DIRECT_FIELD
    fp479_fp2_mul_asm(x, y, z);
#else
    fp_t t0, t1;

    fp2_field_add(&t0, &(y->re), &(y->im));
    fp2_field_add(&t1, &(z->re), &(z->im));
    fp2_field_mul(&t0, &t0, &t1);
    fp2_field_mul(&t1, &(y->im), &(z->im));
    fp2_field_mul(&(x->re), &(y->re), &(z->re));
    fp2_field_sub(&(x->im), &t0, &t1);
    fp2_field_sub(&(x->im), &(x->im), &(x->re));
    fp2_field_sub(&(x->re), &(x->re), &t1);
#endif
}

void
fp2_sqr(fp2_t *x, const fp2_t *y)
{
    PIKE_PROFILE_COUNT(PIKE_PROFILE_FP2_SQR);
#if FP2_NGCC1_DIRECT_FIELD
    fp479_fp2_sqr_asm(x, y);
#elif FP2_NGCC2_DIRECT_FIELD
    fp_t sum, diff;

    fp2_field_add(&sum, &(y->re), &(y->im));
    fp2_field_sub(&diff, &(y->re), &(y->im));
    fp2_field_mul(&(x->im), &(y->re), &(y->im));
    fp765_12_double_asm(x->im, x->im);
    fp2_field_mul(&(x->re), &sum, &diff);
#else
    fp_t sum, diff;

    fp2_field_add(&sum, &(y->re), &(y->im));
    fp2_field_sub(&diff, &(y->re), &(y->im));
    fp2_field_mul(&(x->im), &(y->re), &(y->im));
    fp2_field_add(&(x->im), &(x->im), &(x->im));
    fp2_field_mul(&(x->re), &sum, &diff);
#endif
}

void
fp2_inv(fp2_t *x)
{
    PIKE_PROFILE_START(prof_start, PIKE_PROFILE_FP2_INV);
    fp_t t0, t1;

    fp2_field_sqr(&t0, &(x->re));
    fp2_field_sqr(&t1, &(x->im));
    fp2_field_add(&t0, &t0, &t1);
    fp_inv(&t0);
    fp2_field_mul(&(x->re), &(x->re), &t0);
    fp2_field_mul(&(x->im), &(x->im), &t0);
    fp2_field_neg(&(x->im), &(x->im));
    PIKE_PROFILE_STOP(prof_start, PIKE_PROFILE_FP2_INV);
}

void
fp2_batched_inv(fp2_t *x, int len)
{
    if (len == 1) {
        fp2_inv(&x[0]);
        return;
    }

    if (len == 2) {
        fp2_t x0, inverse;

        fp2_copy(&x0, &x[0]);
        fp2_mul(&inverse, &x[0], &x[1]);
        fp2_inv(&inverse);
        fp2_mul(&x[0], &x[1], &inverse);
        fp2_mul(&x[1], &x0, &inverse);
        return;
    }

    fp2_t t1[len], t2[len];
    fp2_t inverse;

    // x = x0,...,xn
    // t1 = x0, x0*x1, ... ,x0 * x1 * ... * xn
    fp2_copy(&t1[0], &x[0]);
    for (int i = 1; i < len; i++) {
        fp2_mul(&t1[i], &t1[i - 1], &x[i]);
    }

    // inverse = 1/ (x0 * x1 * ... * xn)
    fp2_copy(&inverse, &t1[len - 1]);
    fp2_inv(&inverse);

    fp2_copy(&t2[0], &inverse);
    // t2 = 1/ (x0 * x1 * ... * xn), 1/ (x0 * x1 * ... * x(n-1)) , ... , 1/xO
    for (int i = 1; i < len; i++) {
        fp2_mul(&t2[i], &t2[i - 1], &x[len - i]);
    }

    fp2_copy(&x[0], &t2[len - 1]);

    for (int i = 1; i < len; i++) {
        fp2_mul(&x[i], &t1[i - 1], &t2[len - i - 1]);
    }
}

uint32_t
fp2_is_square(const fp2_t *x)
{
    fp_t t0, t1;

    fp2_field_sqr(&t0, &(x->re));
    fp2_field_sqr(&t1, &(x->im));
    fp2_field_add(&t0, &t0, &t1);

    return fp_is_square(&t0);
}

void
fp2_sqrt(fp2_t *x)
// x^p = (x0 + i*x1)^p = x0 - i*x1  (Frobenius automorphism)
// Thus: x^(p+1) = (x0 + i*x1)*(x0 - i*x1) = x0^2 + x1^2, which
// is an element of GF(p). All elements of GF(p) are squares in
// GF(p^2), but x0^2 + x1^2 is not necessarily a square in GF(p).
//
// Let conj(p) = x^p = x0 - i*x1. Note that conj() is analogous to
// the conjugate in complex numbers. In particular:
//    conj(a + b) = conj(a) + conj(b)
//    conj(a * b) = conj(a) * conj(b)
// This implies that conj(x) is a square if and only if x is a
// square, and conj(sqrt(x)) = sqrt(conj(x)). Thus, if x is a
// square, then:
//    (sqrt(x)*conj(sqrt(x)))^2 = x*conj(x) = x0^2 + x1^2
// But sqrt(x)*conj(sqrt(x)) is in GF(p); therefore, if x is a
// square, then x0^2 + x1^2 must be a square in GF(p).
{
    fp_t sqrt_delta, tmp;
    fp_t y0, y1;

    // sqrt_delta = sqrt(re^2 + im^2)
    fp2_field_sqr(&sqrt_delta, &(x->re));
    fp2_field_sqr(&tmp, &(x->im));
    fp2_field_add(&sqrt_delta, &sqrt_delta, &tmp);
    fp_sqrt(&sqrt_delta);

    // y0^2 = (x0 + sqrt(delta)) / 2
    fp2_field_add(&y0, &(x->re), &sqrt_delta);
    fp2_field_half(&y0, &y0);

    // Now if the imaginary part of x = 0
    // then we instead want to set y0^2 = x0
    uint32_t x1_is_zero = fp2_field_is_zero(&(x->im));
    fp2_field_select(&y0, &y0, &(x->re), x1_is_zero);

    // If y0^2 is a square, nqr = 0, otherwise
    // nqr = 0xFF...FF
    uint32_t nqr = ~fp_is_square(&y0);

    // Now we need to check if y02 is a square!
    // If y0 is not a square:
    //     If x1 = 0 then we want to set y0 = -x0
    //     If x1 != 0, then we want to set y0 = y0 - sqrt(delta)
    fp2_field_neg(&tmp, &y0);
    fp2_field_select(&y0, &y0, &tmp, nqr & x1_is_zero);
    fp2_field_sub(&tmp, &y0, &sqrt_delta);
    fp2_field_select(&y0, &y0, &tmp, nqr & ~x1_is_zero);

    // Now we take the square root
    fp_sqrt(&y0);
    fp2_field_add(&tmp, &y0, &y0);
    fp_inv(&tmp);
    fp2_field_mul(&y1, &(x->im), &tmp);

    // If x1 = 0 then the sqrt worked and y1 = 0, but if
    // x0 was not a square, we must swap y0 and y1
    fp2_field_cswap(&y0, &y1, nqr & x1_is_zero);

    //
    // Sign management
    //
    // To ensure deterministic square-root we conditionally negate the output

    // Check if y0 is zero
    uint32_t y0_is_zero = fp2_field_is_zero(&y0);

    // Check whether y0, y1 are odd
    // Note: we encode to ensure canonical representation
    uint8_t tmp_bytes[FP_ENCODED_BYTES];
    fp_encode(tmp_bytes, &y0);
    uint32_t y0_is_odd = -((uint32_t)tmp_bytes[0] & 1);
    fp_encode(tmp_bytes, &y1);
    uint32_t y1_is_odd = -((uint32_t)tmp_bytes[0] & 1);

    // We negate the output if:
    // y0 is odd, or
    // y0 is zero and y1 is odd
    uint32_t negate_output = y0_is_odd | (y0_is_zero & y1_is_odd);
    fp2_field_neg(&tmp, &y0);
    fp2_field_select(&(x->re), &y0, &tmp, negate_output);
    fp2_field_neg(&tmp, &y1);
    fp2_field_select(&(x->im), &y1, &tmp, negate_output);
}

// exponentiation using square and multiply
// Warning!! Not constant time when the scalar varies.
// But it is in constant time when the scalar is fixed.
void
fp2_exp(fp2_t *out, const fp2_t *x, const digit_t *exp, const int size)
{
    fp2_t acc;
    digit_t bit;

    fp2_copy(&acc, x);
    fp2_set_one(out);

    // Iterate over each word of exp
    for (int j = 0; j < size; j++) {
        // Iterate over each bit of the word
        for (int i = 0; i < RADIX; i++) {
            bit = (exp[j] >> i) & 1;
            if (bit == 1) {
                fp2_mul(out, out, &acc);
            }
            fp2_sqr(&acc, &acc);
        }
    }
}

// Lucas sequence
// Input: the trace of an \mu_{p+1}-element v,
// Output: the trace of the exp powers of v.
void
lucas_sequence(fp_t *out, const fp_t *x, const digit_t *exp, const int size)
{
    fp_t v0, v1, tmp, cross, two;
    digit_t bit;

    // v0 <- 2
    fp2_field_one(&two);
    fp2_field_add(&two, &two, &two);
    fp2_field_copy(&v0, &two);
    
    // v1 <- tr(gamma)
    fp2_field_copy(&v1, x);
    fp2_field_copy(&tmp, &v1);

    // Lucas序列必须从最高位到最低位 (MSB to LSB) 迭代
    for (int j = size - 1; j >= 0; j--) {
        // 从每个 word 的最高位 bit 开始
        for (int i = RADIX - 1; i >= 0; i--) {
            bit = (exp[j] >> i) & 1;

            // 预先计算交叉乘积 cross = v0 * v1 - tmp
            // 避免因 v0 或 v1 被提前覆盖导致计算错误
            fp2_field_mul(&cross, &v0, &v1);
            fp2_field_sub(&cross, &cross, &tmp);

            if (bit == 1) {
                // v0 <- v0 * v1 - tmp
                fp2_field_copy(&v0, &cross);      
                
                // v1 <- v1^2 - 2
                fp2_field_sqr(&v1, &v1);          
                fp2_field_sub(&v1, &v1, &two);    
            } else {
                // v1 <- v0 * v1 - tmp
                fp2_field_copy(&v1, &cross);      
                
                // v0 <- v0^2 - 2
                fp2_field_sqr(&v0, &v0);          
                fp2_field_sub(&v0, &v0, &two);    
            }
        }
    }
    fp2_field_copy(out, &v0);
}

void
fp2_print(char *name, const fp2_t *a)
{
    printf("%s0x", name);

    uint8_t buf[FP_ENCODED_BYTES];
    fp_encode(&buf, &a->re); // Encoding ensures canonical rep
    for (int i = 0; i < FP_ENCODED_BYTES; i++) {
        printf("%02x", buf[FP_ENCODED_BYTES - i - 1]);
    }

    printf(" + i*0x");

    fp_encode(&buf, &a->im);
    for (int i = 0; i < FP_ENCODED_BYTES; i++) {
        printf("%02x", buf[FP_ENCODED_BYTES - i - 1]);
    }
    printf("\n");
}
