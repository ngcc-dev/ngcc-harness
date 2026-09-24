#include <fp.h>
#include "encoded_sizes.h"

extern uint64_t p[NWORDS_FIELD];

/*
 * If ctl == 0x00000000, then *d is set to a0
 * If ctl == 0xFFFFFFFF, then *d is set to a1
 * ctl MUST be either 0x00000000 or 0xFFFFFFFF.
 */
void
fp_select(fp_t *d, const fp_t *a0, const fp_t *a1, uint32_t ctl)
{
    uint64_t cw = (uint64_t) * (int32_t *)&ctl;
    for (unsigned int i = 0; i < NWORDS_FIELD; i++) {
        (*d)[i] = (*a0)[i] ^ (cw & ((*a0)[i] ^ (*a1)[i]));
    }
}

/*
 * If ctl == 0x00000000, then *a and *b are unchanged.
 * If ctl == 0xFFFFFFFF, then the contents of *a and *b are swapped.
 * ctl MUST be either 0x00000000 or 0xFFFFFFFF.
 */
void
fp_cswap(fp_t *a, fp_t *b, uint32_t ctl)
{
    uint64_t cw = (uint64_t) * (int32_t *)&ctl;
    uint64_t t;

    for (unsigned int i = 0; i < NWORDS_FIELD; i++) {
        t = cw & ((*a)[i] ^ (*b)[i]);
        (*a)[i] ^= t;
        (*b)[i] ^= t;
    }
}

void
fp_set_zero(fp_t *a)
{
    for (unsigned int i = 0; i < NWORDS_FIELD; i++) {
        (*a)[i] = 0;
    }
}

void
fp_set_one(fp_t *a)
{
    fp_mont_setone(a);
}

void
fp_set_small(fp_t *x, const digit_t val)
{ // Set field element x = val, where val has wordsize
    // automatically converted to Montgomery form
    (*x)[0] = val;
    for (unsigned int i = 1; i < NWORDS_FIELD; i++) {
        (*x)[i] = 0;
    }
    fp_tomont(x, x);
}

uint32_t
fp_is_equal(const fp_t *a, const fp_t *b)
{ // Compare two field elements in constant time
  // Returns 0xFF...FF (true) if a=b, 0 (false) otherwise
    digit_t r = 0;

    for (unsigned int i = 0; i < NWORDS_FIELD; i++)
        r |= (*a)[i] ^ (*b)[i];

    return -(uint32_t)is_digit_zero_ct(r);
}

uint32_t
fp_is_zero(const fp_t *a)
{ // Is a field element zero?
  // Returns 0xFF...FF (true) if a=0, 0 (false) otherwise
    digit_t r = 0;

    for (unsigned int i = 0; i < NWORDS_FIELD; i++)
        r |= (*a)[i] ^ 0;

    return -(uint32_t)is_digit_zero_ct(r);
}

void
fp_copy(fp_t *out, const fp_t *a)
{
    memcpy(*out, *a, sizeof(fp_t));
}

void
fp_neg(fp_t *out, const fp_t *a)
{ // Modular negation, out = -a mod p
  // Input: a in [0, p-1]
  // Output: out in [0, p-1]
    unsigned int i, borrow = 0;

    for (i = 0; i < NWORDS_FIELD; i++) {
        SUBC((*out)[i], borrow, p[i], (*a)[i], borrow);
    }
    fp_sub(out, out, (fp_t *)&p);
}

void
fp_half(fp_t *out, const fp_t *a)
{
    fp_t t;
    uint64_t carry = 0;
    const uint64_t mask = (uint64_t)0 - ((*a)[0] & 1);

    for (unsigned int i = 0; i < NWORDS_FIELD; i++) {
        const uint64_t addend = p[i] & mask;
        const uint64_t sum = (*a)[i] + addend;
        const uint64_t c1 = (sum < (*a)[i]);
        const uint64_t sum2 = sum + carry;
        const uint64_t c2 = (sum2 < sum);
        t[i] = sum2;
        carry = c1 | c2;
    }

    for (unsigned int i = 0; i < NWORDS_FIELD - 1; i++) {
        (*out)[i] = (t[i] >> 1) | (t[i + 1] << 63);
    }
    (*out)[NWORDS_FIELD - 1] = (t[NWORDS_FIELD - 1] >> 1) | (carry << 63);
}

// Little-endian encoding of a 64-bit integer.
static void
enc64le(void *dst, uint64_t x)
{
    uint8_t *buf = dst;
    buf[0] = (uint8_t)x;
    buf[1] = (uint8_t)(x >> 8);
    buf[2] = (uint8_t)(x >> 16);
    buf[3] = (uint8_t)(x >> 24);
    buf[4] = (uint8_t)(x >> 32);
    buf[5] = (uint8_t)(x >> 40);
    buf[6] = (uint8_t)(x >> 48);
    buf[7] = (uint8_t)(x >> 56);
}

// Little-endian decoding of a 64-bit integer.
static uint64_t
dec64le(const void *src)
{
    const uint8_t *buf = src;
    return (uint64_t)buf[0] | ((uint64_t)buf[1] << 8) | ((uint64_t)buf[2] << 16) |
           ((uint64_t)buf[3] << 24) | ((uint64_t)buf[4] << 32) | ((uint64_t)buf[5] << 40) |
           ((uint64_t)buf[6] << 48) | ((uint64_t)buf[7] << 56);
}
// Encode elements to bytes
void
fp_encode(void *dst, const fp_t *a)
{
	uint8_t buf[NWORDS_FIELD * 8];
	fp_t x;

	fp_frommont(&x, a);
	for (int i = 0; i < NWORDS_FIELD; i++) {
		enc64le(buf + i * 8, x[i]);
	}
	memcpy(dst, buf, FP_ENCODED_BYTES);
}


// TODO: handle lengths
void
fp_decode_reduce(fp_t *d, const void *src, size_t len)
{
    (void)len;
    const uint8_t *buf = src;

    for (int i = 0; i < NWORDS_FIELD; i++) {
        (*d)[i] = dec64le(buf + i * 8);
    }

    fp_tomont(d, d);
}

void
fp_decode(fp_t *d, const void *src)
{
	uint8_t buf[NWORDS_FIELD * 8] = { 0 };

	memcpy(buf, src, FP_ENCODED_BYTES);

	int j = 0;
	for (int i = 0; i < NWORDS_FIELD; i++) {
		(*d)[i] = dec64le(buf + j);
		j += 8;
	}

	fp_tomont(d, d);
}


static void
fp_exp3div4(fp_t *out, const fp_t *a)
{ // Fixed exponentiation out = a^((p-3)/4) mod p
  // Input: a in [0, p-1]
  // Output: out in [0, p-1]
  // Requirement: p = 3(mod 4)
    fp_t p_t, acc;
    digit_t bit;

    memcpy((digit_t *)p_t, (digit_t *)p, NWORDS_FIELD * RADIX / 8);
    memcpy((digit_t *)acc, (digit_t *)*a, NWORDS_FIELD * RADIX / 8);
    mp_shiftr(p_t, 1, NWORDS_FIELD);
    mp_shiftr(p_t, 1, NWORDS_FIELD);
    fp_set_one(out);

    for (int i = 0; i < NWORDS_FIELD * RADIX - 2; i++) {
        bit = p_t[0] & 1;
        mp_shiftr(p_t, 1, NWORDS_FIELD);
        if (bit == 1) {
            fp_mul(out, out, &acc);
        }
        fp_sqr(&acc, &acc);
    }
}

void
fp_inv(fp_t *a)
{ // Modular inversion, out = x^-1*R mod p, where R = 2^(w*nwords), w is the computer wordsize and
  // nwords is the number of words to represent p Input: a=xR in [0, p-1] Output: out in [0, p-1].
  // It outputs 0 if the input does not have an inverse Requirement: Ceiling(Log(p)) < w*nwords
    fp_t t;

    fp_exp3div4(&t, a);
    fp_sqr(&t, &t);
    fp_sqr(&t, &t);
    fp_mul(a, &t, a); // a^(p-2)
}

uint32_t
fp_is_square(const fp_t *a)
{ // Is the field element a square?
  // Returns 0xFF...FF (true) if a is a square, 0 (false) otherwise
    fp_t t, one;

    fp_exp3div4(&t, a);
    fp_sqr(&t, &t);
    fp_mul(&t, &t, a); // a^((p-1)/2)
    fp_set_one(&one);

    return fp_is_equal(&t, &one);
}

void
fp_sqrt(fp_t *a)
{ // Square root computation, out = a^((p+1)/4) mod p
    fp_t t;

    fp_exp3div4(&t, a);
    fp_mul(a, &t, a); // a^((p+1)/4)

    // Sign management, compute canonical representation
    // and negate if the first limb is odd
    fp_frommont(&t, a);
    uint32_t ctl = -((uint32_t)t[0] & 1);
    fp_neg(&t, a);
    fp_select(a, a, &t, ctl);
}
