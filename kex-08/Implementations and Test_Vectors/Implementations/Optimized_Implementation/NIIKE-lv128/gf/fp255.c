// SPDX-FileCopyrightText: 2026 The Project OSIDH-LD Authors
// SPDX-License-Identifier: Apache-2.0

#include "fp.h"

extern const fp_t r_squared_mod_p;
void fp_add3(digit_t* out, const digit_t* a, const digit_t* b);
void fp_sub3(digit_t* out, const digit_t* a, const digit_t* b);
void fp_mul3(digit_t* out, const digit_t* a, const digit_t* b);
void fp_sq2(digit_t* out, const digit_t* a);

void fp_add(uint64_t* out, const uint64_t* a, const uint64_t* b) {
    fp_add3(out, a, b);
}

void fp_sub(uint64_t* out, const uint64_t* a, const uint64_t* b) {
    fp_sub3(out, a, b);
}

void fp_sqr(uint64_t* out, const uint64_t* a) {
    fp_sq2(out, a);
}

void fp_mul(uint64_t* out, const uint64_t* a, const uint64_t* b) {
    fp_mul3(out, a, b);
}

void fp_tomont(uint64_t* out, const uint64_t* a) {
    fp_mul3(out, a, &r_squared_mod_p);
}

void fp_frommont(uint64_t* out, const uint64_t* a) {
    fp_t one = {1, 0, 0, 0};
    fp_mul3(out, a, &one);
}

void fp_mont_setone(uint64_t* out) {
    out[0]=0x33eb176b0ab3001bUL;
    out[1]=0x7fe51d380228b1cfUL;
    out[2]=0x43679dfc2589b1e9UL;
    out[3]=0x48abd63c6bdbad1UL;
}