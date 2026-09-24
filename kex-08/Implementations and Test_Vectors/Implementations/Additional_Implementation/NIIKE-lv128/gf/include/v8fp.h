// SPDX-FileCopyrightText: 2026 The Project OSIDH-LD Authors
// SPDX-License-Identifier: Apache-2.0

/*
 * The implementation in this file refers to the algorithm
 * proposed by Cheng Hao et al. in the article
 * "Batching CSIDH Group Actions using AVX-512"
 * (DOI: 10.46586/tches.v2021.i4.618-649)
 */

#ifndef V8FP_H
#define V8FP_H

#include "fp.h"
#include <immintrin.h>

typedef __m512i v8fp_t[NWORDS_FIELD];

#define HT_BRADIX 52                    // limb size 
#define HT_BMASK  0xFFFFFFFFFFFFFULL    // 2^52 - 1
#define HT_MONTW  0x3a641f9a28e39       // the constant for Montgomery Multiplication

void v8fp_pack8(v8fp_t* x, const fp_t* a1, const fp_t* a2, const fp_t* a3, const fp_t* a4, const fp_t* a5, const fp_t* a6, const fp_t* a7, const fp_t* a8);
void v8fp_unpack8(fp_t* a1, fp_t* a2, fp_t* a3, fp_t* a4, fp_t* a5, fp_t* a6, fp_t* a7, fp_t* a8, const v8fp_t* x);
void v8fp_pack7(v8fp_t* x, const fp_t* a1, const fp_t* a2, const fp_t* a3, const fp_t* a4, const fp_t* a5, const fp_t* a6, const fp_t* a7);
void v8fp_unpack7(fp_t* a1, fp_t* a2, fp_t* a3, fp_t* a4, fp_t* a5, fp_t* a6, fp_t* a7, const v8fp_t* x);
void v8fp_pack6(v8fp_t* x, const fp_t* a1, const fp_t* a2, const fp_t* a3, const fp_t* a4, const fp_t* a5, const fp_t* a6);
void v8fp_unpack6(fp_t* a1, fp_t* a2, fp_t* a3, fp_t* a4, fp_t* a5, fp_t* a6, const v8fp_t* x);
void v8fp_pack5(v8fp_t* x, const fp_t* a1, const fp_t* a2, const fp_t* a3, const fp_t* a4, const fp_t* a5);
void v8fp_unpack5(fp_t* a1, fp_t* a2, fp_t* a3, fp_t* a4, fp_t* a5, const v8fp_t* x);
void v8fp_pack4(v8fp_t* x, const fp_t* a1, const fp_t* a2, const fp_t* a3, const fp_t* a4);
void v8fp_unpack4(fp_t* a1, fp_t* a2, fp_t* a3, fp_t* a4, const v8fp_t* x);
void v8fp_pack3(v8fp_t* x, const fp_t* a1, const fp_t* a2, const fp_t* a3);
void v8fp_unpack3(fp_t* a1, fp_t* a2, fp_t* a3, const v8fp_t* x);
void v8fp_pack2(v8fp_t* x, const fp_t* a1, const fp_t* a2);
void v8fp_unpack2(fp_t* a1, fp_t* a2, const v8fp_t* x);


// the prime p of the field
static const uint64_t ht_p[NWORDS_FIELD] = {0xc4d86fc6efff7, 0x42a9f26f65995, 0x8d21a078008f6, 0xc1ba3edd76014, 0x53d1c0debdc0};

// p * 2
static const uint64_t ht_pmul2[NWORDS_FIELD] = {0x89b0df8ddffee, 0x8553e4decb32b, 0x1a4340f0011ec, 0x83747dbaec029, 0xa7a381bd7b81};
 
// R mod p = 2^260 mod p
static const uint64_t ht_montR[NWORDS_FIELD] = {0x176b0ab3001b0, 0x80228b1cf33eb, 0x89b1e97fe51d3, 0xad143679dfc25, 0x48abd63c6bdb};  

// R^2 mod p = (2^260)^2 mod p 
static const uint64_t ht_montR2[NWORDS_FIELD] = {0x3ae328536b3a0, 0xfea1f191ea2e7, 0x30ea6fb384997, 0xef5144546ca82, 0x4bf76b11f801};

void v8fp_add(v8fp_t r, const v8fp_t a, const v8fp_t b);
void v8fp_sub(v8fp_t r, const v8fp_t a, const v8fp_t b);
void v8fp_mul(v8fp_t r, const v8fp_t a, const v8fp_t b);
void v8fp_neg(v8fp_t r, const v8fp_t a);
void v8fp_set(v8fp_t r, const digit_t a);
void v8fp_copy(v8fp_t r, const v8fp_t a);
void v8fp_mont_setone(v8fp_t r);
void v8fp_swap(v8fp_t P, v8fp_t Q, const __m512i option);

#endif