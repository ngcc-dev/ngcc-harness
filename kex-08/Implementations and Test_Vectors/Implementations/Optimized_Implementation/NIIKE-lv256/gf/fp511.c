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
    fp_t one = {1, 0, 0, 0, 0, 0, 0, 0};
    fp_mul3(out, a, &one);
}

void fp_mont_setone(uint64_t* out) {
    out[0]=0x707d3246fdc8e382;
    out[1]=0x74fc75c5abeecd3f;
    out[2]=0xd8c63c99c8ca2803;
    out[3]=0x766b7cf60deff1bc;
    out[4]=0xc83c9b3d8525f891;
    out[5]=0x0208938b30a684e0;
    out[6]=0xbc77764796d422f5;
    out[7]=0x3aa1910d92a44138;
}