// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from fp.c in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modified to adapt to OSIDH-LD
 */

#include "include/fp.h"

const uint64_t p[NWORDS_FIELD] =  { 0x47c166dc811b8e3f, 0x4581c51d2a089960, 0x939ce1b31b9aebfe, 0x44ca4184f9080721, 0x9be1b2613d6d03b7, 0x7efbb63a67acbd8f, 0xa1c444dc3495ee85, 0x62af377936addf63 };

void fp_set(digit_t* x, const digit_t val)
{ // Set field element x = val, where val has wordsize

    x[0] = val;
    for (unsigned int i = 1; i < NWORDS_FIELD; i++) {
        x[i] = 0;
    }
}

bool fp_is_equal(const digit_t* a, const digit_t* b)
{ // Compare two field elements in constant time
  // Returns 1 (true) if a=b, 0 (false) otherwise
    digit_t r = 0;

    for (unsigned int i = 0; i < NWORDS_FIELD; i++)
        r |= a[i] ^ b[i];

    return (bool)is_digit_zero_ct(r);
}

bool fp_is_zero(const digit_t* a)
{ // Is a field element zero?
  // Returns 1 (true) if a=0, 0 (false) otherwise
    digit_t r = 0;

    for (unsigned int i = 0; i < NWORDS_FIELD; i++)
        r |= a[i] ^ 0;

    return (bool)is_digit_zero_ct(r);
}

uint32_t
fp_is_zero_mask(const fp_t *a)
{
    return -(uint32_t)fp_is_zero(*a);
}

void fp_copy(digit_t* out, const digit_t* a)
{
    memcpy(out, a, NWORDS_FIELD*RADIX/8);
}

void fp_neg(digit_t* out, const digit_t* a)
{ // Modular negation, out = -a mod p
  // Input: a in [0, p-1] 
  // Output: out in [0, p-1] 
    unsigned int i, borrow = 0;

    for (i = 0; i < NWORDS_FIELD; i++) {
        SUBC(out[i], borrow, ((digit_t*)p)[i], a[i], borrow);
    }
    fp_sub(out, out, (digit_t*)p);
}

void MUL(digit_t* out, const digit_t a, const digit_t b)
{ // Digit multiplication, digit*digit -> 2-digit result 
  // Inputs: a, b in [0, 2^w-1], where w is the computer wordsize 
  // Output: 0 < out < 2^(2w)-1    
    register digit_t al, ah, bl, bh, temp;
    digit_t albl, albh, ahbl, ahbh, res1, res2, res3, carry;
    digit_t mask_low = (digit_t)(-1) >> (sizeof(digit_t)*4), mask_high = (digit_t)(-1) << (sizeof(digit_t)*4);

    al = a & mask_low;                        // Low part
    ah = a >> (sizeof(digit_t)*4);            // High part
    bl = b & mask_low;
    bh = b >> (sizeof(digit_t)*4);

    albl = al * bl;
    albh = al * bh;
    ahbl = ah * bl;
    ahbh = ah * bh;
    out[0] = albl & mask_low;                 // out00

    res1 = albl >> (sizeof(digit_t)*4);
    res2 = ahbl & mask_low;
    res3 = albh & mask_low;
    temp = res1 + res2 + res3;
    carry = temp >> (sizeof(digit_t)*4);
    out[0] ^= temp << (sizeof(digit_t)*4);    // out01   

    res1 = ahbl >> (sizeof(digit_t)*4);
    res2 = albh >> (sizeof(digit_t)*4);
    res3 = ahbh & mask_low;
    temp = res1 + res2 + res3 + carry;
    out[1] = temp & mask_low;                 // out10 
    carry = temp & mask_high;
    out[1] ^= (ahbh & mask_high) + carry;     // out11
}

digit_t mp_shiftr(digit_t* x, const unsigned int shift, const unsigned int nwords)
{ // Multiprecision right shift
    digit_t bit_out = x[0] & 1;

    for (unsigned int i = 0; i < nwords-1; i++) {
        SHIFTR(x[i+1], x[i], shift, x[i], RADIX);
    }
    x[nwords-1] >>= shift;
    return bit_out;
}

void mp_shiftl(digit_t* x, const unsigned int shift, const unsigned int nwords)
{ // Multiprecision left shift

    for (int i = nwords-1; i > 0; i--) {
        SHIFTL(x[i], x[i-1], shift, x[i], RADIX);
    }
    x[0] <<= shift;
}

void fp_exp3div4(digit_t* out, const digit_t* a)
{ // Fixed exponentiation out = a^((p-3)/4) mod p
  // Input: a in [0, p-1] 
  // Output: out in [0, p-1] 
  // Requirement: p = 3(mod 4)
    fp_t p_t, acc;
    digit_t bit;

    memcpy((digit_t*)p_t, (digit_t*)p, NWORDS_FIELD*RADIX/8);
    memcpy((digit_t*)acc, (digit_t*)a, NWORDS_FIELD*RADIX/8);
    mp_shiftr(p_t, 1, NWORDS_FIELD);
    mp_shiftr(p_t, 1, NWORDS_FIELD);
    fp_set(out, 1);
    fp_tomont(out, out);

    for (int i = 0; i < NWORDS_FIELD*RADIX-2; i++) {
        bit = p_t[0] & 1;
        mp_shiftr(p_t, 1, NWORDS_FIELD);
        if (bit == 1) {
            fp_mul(out, out, acc);
        }
        fp_sqr(acc, acc);
    }
}

void fp_inv(digit_t* a)
{ // Modular inversion, out = x^-1*R mod p, where R = 2^(w*nwords), w is the computer wordsize and nwords is the number of words to represent p
  // Input: a=xR in [0, p-1] 
  // Output: out in [0, p-1]. It outputs 0 if the input does not have an inverse  
  // Requirement: Ceiling(Log(p)) < w*nwords
    fp_t t;

    fp_exp3div4(t, a);
    fp_sqr(t, t);
    fp_sqr(t, t);
    fp_mul(a, t, a);    // a^(p-2)
}

bool fp_is_square(const digit_t* a)
{ // Is field element a square?
  // Output: out = 0 (false), 1 (true)
    fp_t t, one;

    fp_exp3div4(t, a);
    fp_sqr(t, t);
    fp_mul(t, t, a);    // a^((p-1)/2)
    fp_frommont(t, t);
    fp_set(one, 1);

    return fp_is_equal(t, one);
}

void fp_sqrt(digit_t* a)
{ // Square root computation, out = a^((p+1)/4) mod p
    fp_t t;

    fp_exp3div4(t, a);
    fp_mul(a, t, a);    // a^((p+1)/4)
}

void fp_swap(digit_t* P, digit_t* Q, const digit_t option)
{ // If option = 0 then P <- P and Q <- Q, else if option = 0xFF...FF then P <- Q and Q <- P
    digit_t temp;

    for (int i = 0; i < NWORDS_FIELD; i++) {
        temp = option & (P[i] ^ Q[i]);
        P[i] = temp ^ P[i];
        Q[i] = temp ^ Q[i];
    }
}

/*
 * If ctl == 0x00000000, then *d is set to a0
 * If ctl == 0xFFFFFFFF, then *d is set to a1
 * ctl MUST be either 0x00000000 or 0xFFFFFFFF.
 */
void fp_select(fp_t *d, const fp_t *a0, const fp_t *a1, uint32_t ctl)
{
    digit_t cw = (int32_t)ctl;
    for (unsigned int i = 0; i < NWORDS_FIELD; i++) {
        (*d)[i] = (*a0)[i] ^ (cw & ((*a0)[i] ^ (*a1)[i]));
    }
}

void fp_encode(void *dst, const fp_t *a)
{
    fp_frommont(dst, a);
}

void fp_decode(fp_t *d, const void *src)
{
    fp_tomont(d, src);
}