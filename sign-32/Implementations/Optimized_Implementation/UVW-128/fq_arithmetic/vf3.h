#ifndef WAVE2_VF3_H
#define WAVE2_VF3_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __AVX2__
    #include <immintrin.h>
#endif

#include "types_f3.h"
#include "popcount.h"

void vf3_init(vf3_e *e, const size_t length);
vf3_e *vf3_alloc(const size_t length);
vf3_e *vf3_array_alloc(const size_t length, const size_t array_length);
void vf3_array_free(vf3_e *e, const size_t array_length);
void vf3_free(vf3_e *e);
void vf3_copy(vf3_e *dest, const vf3_e *src);
void vf3_trim(vf3_e *e);
bool vf3_equal(vf3_e *x, vf3_e *y);
void vf3_vector_constant(vf3_e *e, const uint8_t a);
void vf3_set_to_zero(vf3_e *e);
uint8_t vf3_get_element(const size_t pos, const vf3_e *a);
void vf3_set_coeff(const size_t pos, vf3_e *a, const uint8_t coeff);
void vf3_add_coeff(const size_t j, vf3_e *x, const uint8_t a);
uint64_t convert64(uint64_t x);
void vf3_trits_from_bits(vf3_e *y, uint64_t *rnd);
void vf3_random(vf3_e *e);
void vf3_random_non_zero(vf3_e *e);
void vf3_vector_cat_zero(vf3_e *res, vf3_e *const a);
void vf3_vector_cat(vf3_e *res, vf3_e *const a, vf3_e *const b);
void vf3_vector_split_zero(vf3_e *a, vf3_e *const x);
void vf3_vector_split(vf3_e *a, vf3_e *b, vf3_e *const x);
void vf3_print(const vf3_e *a);
size_t vf3_read(vf3_e *e, FILE *f);
size_t vf3_write(vf3_e *e, FILE *f);

static inline size_t vf3_hamming_weight(vf3_e *e) {
    size_t weight = 0;
    size_t length = e->size;
    size_t i;

#ifdef __AVX2__
    size_t full_words = length / WORD_LENGTH;
    for (i = 0; i < full_words; i++) {
        weight += popcount(e->r0[i] | e->r1[i]);
    }
    length -= full_words * WORD_LENGTH;
#else
    for (i = 0; length >= WORD_LENGTH; i++, length -= WORD_LENGTH) {
        weight += popcount(e->r0[i] | e->r1[i]);
    }
#endif

    if (length) {
        wave_word mask = ((wave_word)1 << length) - 1;
        weight += popcount((e->r0[i] | e->r1[i]) & mask);
    }
    return weight;
}

static inline size_t vf3_number_of_coordinates_equal_to_two(vf3_e *e) {
    size_t weight = 0;
    size_t length = e->size;
    size_t i;

#ifdef __AVX2__
    size_t full_words = length / WORD_LENGTH;
    for (i = 0; i < full_words; i++) {
        weight += popcount(e->r1[i]);
    }
    length -= full_words * WORD_LENGTH;
#else
    for (i = 0; length >= WORD_LENGTH; i++, length -= WORD_LENGTH) {
        weight += popcount(e->r1[i]);
    }
#endif

    if (length) {
        wave_word mask = ((wave_word)1 << length) - 1;
        weight += popcount(e->r1[i] & mask);
    }
    return weight;
}

#ifdef __AVX2__

static inline void vf3_vector_add_inplace(vf3_e *a, const vf3_e *b) {
    size_t i = 0;
    size_t limit4 = (a->alloc / 4) * 4;
    for (; i < limit4; i += 4) {
        __m256i a0 = _mm256_loadu_si256((__m256i*)(a->r0 + i));
        __m256i a1 = _mm256_loadu_si256((__m256i*)(a->r1 + i));
        __m256i b0 = _mm256_loadu_si256((__m256i*)(b->r0 + i));
        __m256i b1 = _mm256_loadu_si256((__m256i*)(b->r1 + i));
        __m256i tmp = _mm256_xor_si256(a1, b0);
        _mm256_storeu_si256((__m256i*)(a->r1 + i), _mm256_and_si256(_mm256_xor_si256(a0, b1), tmp));
        _mm256_storeu_si256((__m256i*)(a->r0 + i), _mm256_or_si256(_mm256_xor_si256(a0, b0), _mm256_xor_si256(tmp, b1)));
    }
    for (; i < a->alloc; i++) {
        wave_word tmp = a->r1[i] ^ b->r0[i];
        a->r1[i] = (a->r0[i] ^ b->r1[i]) & tmp;
        a->r0[i] = (a->r0[i] ^ b->r0[i]) | (tmp ^ b->r1[i]);
    }
}

static inline void vf3_vector_add(vf3_e *c, const vf3_e *a, const vf3_e *b) {
    size_t i = 0;
    size_t limit4 = (a->alloc / 4) * 4;
    for (; i < limit4; i += 4) {
        __m256i a0 = _mm256_loadu_si256((__m256i*)(a->r0 + i));
        __m256i a1 = _mm256_loadu_si256((__m256i*)(a->r1 + i));
        __m256i b0 = _mm256_loadu_si256((__m256i*)(b->r0 + i));
        __m256i b1 = _mm256_loadu_si256((__m256i*)(b->r1 + i));
        __m256i tmp = _mm256_xor_si256(a1, b0);
        _mm256_storeu_si256((__m256i*)(c->r1 + i), _mm256_and_si256(_mm256_xor_si256(a0, b1), tmp));
        _mm256_storeu_si256((__m256i*)(c->r0 + i), _mm256_or_si256(_mm256_xor_si256(a0, b0), _mm256_xor_si256(tmp, b1)));
    }
    for (; i < a->alloc; i++) {
        wave_word tmp = a->r1[i] ^ b->r0[i];
        c->r1[i] = (a->r0[i] ^ b->r1[i]) & tmp;
        c->r0[i] = (a->r0[i] ^ b->r0[i]) | (tmp ^ b->r1[i]);
    }
}

static inline void vf3_vector_neg(vf3_e *a, const vf3_e *b) {
    memcpy(a->r0, b->r0, a->alloc * sizeof(wave_word));
    size_t i = 0;
    size_t limit4 = (b->alloc / 4) * 4;
    for (; i < limit4; i += 4) {
        __m256i r0 = _mm256_loadu_si256((__m256i*)(a->r0 + i));
        __m256i r1 = _mm256_loadu_si256((__m256i*)(b->r1 + i));
        _mm256_storeu_si256((__m256i*)(a->r1 + i), _mm256_xor_si256(r1, r0));
    }
    for (; i < b->alloc; i++) a->r1[i] = b->r1[i] ^ b->r0[i];
}

static inline void vf3_vector_neg_inplace(vf3_e *b) {
    size_t i = 0;
    size_t limit4 = (b->alloc / 4) * 4;
    for (; i < limit4; i += 4) {
        __m256i r0 = _mm256_loadu_si256((__m256i*)(b->r0 + i));
        __m256i r1 = _mm256_loadu_si256((__m256i*)(b->r1 + i));
        _mm256_storeu_si256((__m256i*)(b->r1 + i), _mm256_xor_si256(r1, r0));
    }
    for (; i < b->alloc; i++) b->r1[i] ^= b->r0[i];
}

static inline void vf3_vector_sub(vf3_e *c, const vf3_e *a, const vf3_e *b) {
    size_t i = 0;
    size_t limit4 = (a->alloc / 4) * 4;
    for (; i < limit4; i += 4) {
        __m256i a0 = _mm256_loadu_si256((__m256i*)(a->r0 + i));
        __m256i a1 = _mm256_loadu_si256((__m256i*)(a->r1 + i));
        __m256i b0 = _mm256_loadu_si256((__m256i*)(b->r0 + i));
        __m256i b1 = _mm256_loadu_si256((__m256i*)(b->r1 + i));
        __m256i tmp = _mm256_xor_si256(a0, b0);
        _mm256_storeu_si256((__m256i*)(c->r0 + i), _mm256_or_si256(tmp, _mm256_xor_si256(a1, b1)));
        _mm256_storeu_si256((__m256i*)(c->r1 + i), _mm256_and_si256(_mm256_xor_si256(tmp, b1), _mm256_xor_si256(a1, b0)));
    }
    for (; i < a->alloc; i++) {
        wave_word tmp = a->r0[i] ^ b->r0[i];
        c->r0[i] = tmp | (a->r1[i] ^ b->r1[i]);
        c->r1[i] = (tmp ^ b->r1[i]) & (a->r1[i] ^ b->r0[i]);
    }
}

static inline void vf3_vector_sub_inplace(vf3_e *a, const vf3_e *b) {
    size_t i = 0;
    size_t limit4 = (a->alloc / 4) * 4;
    for (; i < limit4; i += 4) {
        __m256i a0 = _mm256_loadu_si256((__m256i*)(a->r0 + i));
        __m256i a1 = _mm256_loadu_si256((__m256i*)(a->r1 + i));
        __m256i b0 = _mm256_loadu_si256((__m256i*)(b->r0 + i));
        __m256i b1 = _mm256_loadu_si256((__m256i*)(b->r1 + i));
        __m256i tmp = _mm256_xor_si256(a0, b0);
        _mm256_storeu_si256((__m256i*)(a->r0 + i), _mm256_or_si256(tmp, _mm256_xor_si256(a1, b1)));
        _mm256_storeu_si256((__m256i*)(a->r1 + i), _mm256_and_si256(_mm256_xor_si256(tmp, b1), _mm256_xor_si256(a1, b0)));
    }
    for (; i < a->alloc; i++) {
        wave_word tmp = a->r0[i] ^ b->r0[i];
        a->r0[i] = tmp | (a->r1[i] ^ b->r1[i]);
        a->r1[i] = (tmp ^ b->r1[i]) & (a->r1[i] ^ b->r0[i]);
    }
}

static inline void vf3_vector_mul(vf3_e *res, const vf3_e *x, const vf3_e *y) {
    size_t i = 0;
    size_t limit4 = (x->alloc / 4) * 4;
    for (; i < limit4; i += 4) {
        __m256i x0 = _mm256_loadu_si256((__m256i*)(x->r0 + i));
        __m256i x1 = _mm256_loadu_si256((__m256i*)(x->r1 + i));
        __m256i y0 = _mm256_loadu_si256((__m256i*)(y->r0 + i));
        __m256i y1 = _mm256_loadu_si256((__m256i*)(y->r1 + i));
        __m256i r0 = _mm256_and_si256(x0, y0);
        _mm256_storeu_si256((__m256i*)(res->r0 + i), r0);
        _mm256_storeu_si256((__m256i*)(res->r1 + i), _mm256_and_si256(_mm256_xor_si256(x1, y1), r0));
    }
    for (; i < x->alloc; i++) {
        res->r0[i] = x->r0[i] & y->r0[i];
        res->r1[i] = (x->r1[i] ^ y->r1[i]) & res->r0[i];
    }
}

static inline void vf3_vector_mul_inplace(vf3_e *x, const vf3_e *y) {
    size_t i = 0;
    size_t limit4 = (x->alloc / 4) * 4;
    for (; i < limit4; i += 4) {
        __m256i x0 = _mm256_loadu_si256((__m256i*)(x->r0 + i));
        __m256i x1 = _mm256_loadu_si256((__m256i*)(x->r1 + i));
        __m256i y0 = _mm256_loadu_si256((__m256i*)(y->r0 + i));
        __m256i y1 = _mm256_loadu_si256((__m256i*)(y->r1 + i));
        __m256i r0 = _mm256_and_si256(x0, y0);
        _mm256_storeu_si256((__m256i*)(x->r0 + i), r0);
        _mm256_storeu_si256((__m256i*)(x->r1 + i), _mm256_and_si256(_mm256_xor_si256(x1, y1), r0));
    }
    for (; i < x->alloc; i++) {
        x->r0[i] = x->r0[i] & y->r0[i];
        x->r1[i] = (x->r1[i] ^ y->r1[i]) & x->r0[i];
    }
}

static inline void vf3_vector_scalarmul(vf3_e *res, const uint8_t coeff,
                                        const vf3_e *x) {
    wave_word mask0 = -(coeff > 0);
    wave_word mask1 = -(coeff > 1);
    __m256i vmask0 = _mm256_set1_epi64x((int64_t)mask0);
    __m256i vmask1 = _mm256_set1_epi64x((int64_t)mask1);
    size_t i = 0;
    size_t limit4 = (x->alloc / 4) * 4;
    for (; i < limit4; i += 4) {
        __m256i x0 = _mm256_loadu_si256((__m256i*)(x->r0 + i));
        __m256i x1 = _mm256_loadu_si256((__m256i*)(x->r1 + i));
        __m256i r0 = _mm256_and_si256(x0, vmask0);
        _mm256_storeu_si256((__m256i*)(res->r0 + i), r0);
        _mm256_storeu_si256((__m256i*)(res->r1 + i), _mm256_and_si256(_mm256_xor_si256(x1, vmask1), r0));
    }
    for (; i < x->alloc; i++) {
        res->r0[i] = x->r0[i] & mask0;
        res->r1[i] = (x->r1[i] ^ mask1) & res->r0[i];
    }
}

static inline void vf3_vector_scalarmul_inplace(vf3_e *x, const uint8_t coeff) {
    wave_word mask0 = -(coeff > 0);
    wave_word mask1 = -(coeff > 1);
    __m256i vmask0 = _mm256_set1_epi64x((int64_t)mask0);
    __m256i vmask1 = _mm256_set1_epi64x((int64_t)mask1);
    size_t i = 0;
    size_t limit4 = (x->alloc / 4) * 4;
    for (; i < limit4; i += 4) {
        __m256i x0 = _mm256_loadu_si256((__m256i*)(x->r0 + i));
        __m256i x1 = _mm256_loadu_si256((__m256i*)(x->r1 + i));
        __m256i r0 = _mm256_and_si256(x0, vmask0);
        _mm256_storeu_si256((__m256i*)(x->r0 + i), r0);
        _mm256_storeu_si256((__m256i*)(x->r1 + i), _mm256_and_si256(_mm256_xor_si256(x1, vmask1), r0));
    }
    for (; i < x->alloc; i++) {
        x->r0[i] = x->r0[i] & mask0;
        x->r1[i] = (x->r1[i] ^ mask1) & x->r0[i];
    }
}

static inline void vf3_vector_scalarmul_nonzero_inplace(vf3_e *x,
                                                        const uint8_t coeff) {
    wave_word mask1 = -(coeff > 1);
    __m256i vmask1 = _mm256_set1_epi64x((int64_t)mask1);
    size_t i = 0;
    size_t limit4 = (x->alloc / 4) * 4;
    for (; i < limit4; i += 4) {
        __m256i x0 = _mm256_loadu_si256((__m256i*)(x->r0 + i));
        __m256i x1 = _mm256_loadu_si256((__m256i*)(x->r1 + i));
        _mm256_storeu_si256((__m256i*)(x->r1 + i), _mm256_and_si256(_mm256_xor_si256(x1, vmask1), x0));
    }
    for (; i < x->alloc; i++) x->r1[i] = (x->r1[i] ^ mask1) & x->r0[i];
}

static inline void vf3_vector_add_multiple_inplace(vf3_e *x,
                                                   const uint8_t coeff,
                                                   const vf3_e *y) {
    wave_word mask0 = -(coeff > 0);
    wave_word mask1 = -(coeff > 1);
    __m256i vmask0 = _mm256_set1_epi64x((int64_t)mask0);
    __m256i vmask1 = _mm256_set1_epi64x((int64_t)mask1);
    size_t i = 0;
    size_t limit4 = (x->alloc / 4) * 4;
    for (; i < limit4; i += 4) {
        __m256i x0 = _mm256_loadu_si256((__m256i*)(x->r0 + i));
        __m256i x1 = _mm256_loadu_si256((__m256i*)(x->r1 + i));
        __m256i y0 = _mm256_and_si256(_mm256_loadu_si256((__m256i*)(y->r0 + i)), vmask0);
        __m256i y1 = _mm256_and_si256(_mm256_xor_si256(_mm256_loadu_si256((__m256i*)(y->r1 + i)), vmask1), y0);
        __m256i tmp = _mm256_xor_si256(x1, y0);
        _mm256_storeu_si256((__m256i*)(x->r1 + i), _mm256_and_si256(_mm256_xor_si256(x0, y1), tmp));
        _mm256_storeu_si256((__m256i*)(x->r0 + i), _mm256_or_si256(_mm256_xor_si256(x0, y0), _mm256_xor_si256(tmp, y1)));
    }
    for (; i < x->alloc; i++) {
        wave_word y0 = y->r0[i] & mask0;
        wave_word y1 = (y->r1[i] ^ mask1) & y0;
        wave_word tmp = x->r1[i] ^ y0;
        x->r1[i] = (x->r0[i] ^ y1) & tmp;
        x->r0[i] = (x->r0[i] ^ y0) | (tmp ^ y1);
    }
}

static inline void vf3_vector_slice_add_multiple_inplace(vf3_e *x,
                                                         const uint8_t coeff,
                                                         const vf3_e *y,
                                                         size_t start) {
    wave_word mask0 = -(coeff > 0);
    wave_word mask1 = -(coeff > 1);
    __m256i vmask0 = _mm256_set1_epi64x((int64_t)mask0);
    __m256i vmask1 = _mm256_set1_epi64x((int64_t)mask1);
    size_t i = start / WORD_LENGTH;
    size_t limit4 = (x->alloc / 4) * 4;
    for (; i < limit4; i += 4) {
        __m256i x0 = _mm256_loadu_si256((__m256i*)(x->r0 + i));
        __m256i x1 = _mm256_loadu_si256((__m256i*)(x->r1 + i));
        __m256i y0 = _mm256_and_si256(_mm256_loadu_si256((__m256i*)(y->r0 + i)), vmask0);
        __m256i y1 = _mm256_and_si256(_mm256_xor_si256(_mm256_loadu_si256((__m256i*)(y->r1 + i)), vmask1), y0);
        __m256i tmp = _mm256_xor_si256(x1, y0);
        _mm256_storeu_si256((__m256i*)(x->r1 + i), _mm256_and_si256(_mm256_xor_si256(x0, y1), tmp));
        _mm256_storeu_si256((__m256i*)(x->r0 + i), _mm256_or_si256(_mm256_xor_si256(x0, y0), _mm256_xor_si256(tmp, y1)));
    }
    for (; i < x->alloc; i++) {
        wave_word y0 = y->r0[i] & mask0;
        wave_word y1 = (y->r1[i] ^ mask1) & y0;
        wave_word tmp = x->r1[i] ^ y0;
        x->r1[i] = (x->r0[i] ^ y1) & tmp;
        x->r0[i] = (x->r0[i] ^ y0) | (tmp ^ y1);
    }
}

static inline void vf3_vector_add_mul_inplace(vf3_e *x, const vf3_e *a,
                                              const vf3_e *y) {
    size_t i = 0;
    size_t limit4 = (x->alloc / 4) * 4;
    for (; i < limit4; i += 4) {
        __m256i x0 = _mm256_loadu_si256((__m256i*)(x->r0 + i));
        __m256i x1 = _mm256_loadu_si256((__m256i*)(x->r1 + i));
        __m256i a0 = _mm256_loadu_si256((__m256i*)(a->r0 + i));
        __m256i a1 = _mm256_loadu_si256((__m256i*)(a->r1 + i));
        __m256i y0 = _mm256_and_si256(_mm256_loadu_si256((__m256i*)(y->r0 + i)), a0);
        __m256i y1 = _mm256_and_si256(_mm256_xor_si256(_mm256_loadu_si256((__m256i*)(y->r1 + i)), a1), y0);
        __m256i tmp = _mm256_xor_si256(x1, y0);
        _mm256_storeu_si256((__m256i*)(x->r1 + i), _mm256_and_si256(_mm256_xor_si256(x0, y1), tmp));
        _mm256_storeu_si256((__m256i*)(x->r0 + i), _mm256_or_si256(_mm256_xor_si256(x0, y0), _mm256_xor_si256(tmp, y1)));
    }
    for (; i < x->alloc; i++) {
        wave_word y0 = y->r0[i] & a->r0[i];
        wave_word y1 = (y->r1[i] ^ a->r1[i]) & y0;
        wave_word tmp = x->r1[i] ^ y0;
        x->r1[i] = (x->r0[i] ^ y1) & tmp;
        x->r0[i] = (x->r0[i] ^ y0) | (tmp ^ y1);
    }
}

static inline void vf3_vector_sub_mul_inplace(vf3_e *x, const vf3_e *a,
                                              const vf3_e *y) {
    size_t i = 0;
    size_t limit4 = (x->alloc / 4) * 4;
    for (; i < limit4; i += 4) {
        __m256i x0 = _mm256_loadu_si256((__m256i*)(x->r0 + i));
        __m256i x1 = _mm256_loadu_si256((__m256i*)(x->r1 + i));
        __m256i a0 = _mm256_loadu_si256((__m256i*)(a->r0 + i));
        __m256i a1 = _mm256_loadu_si256((__m256i*)(a->r1 + i));
        __m256i y0 = _mm256_and_si256(_mm256_loadu_si256((__m256i*)(y->r0 + i)), a0);
        __m256i y1 = _mm256_and_si256(_mm256_xor_si256(_mm256_loadu_si256((__m256i*)(y->r1 + i)), a1), y0);
        __m256i tmp = _mm256_xor_si256(x0, y0);
        _mm256_storeu_si256((__m256i*)(x->r0 + i), _mm256_or_si256(tmp, _mm256_xor_si256(x1, y1)));
        _mm256_storeu_si256((__m256i*)(x->r1 + i), _mm256_and_si256(_mm256_xor_si256(tmp, y1), _mm256_xor_si256(x1, y0)));
    }
    for (; i < x->alloc; i++) {
        wave_word y0 = y->r0[i] & a->r0[i];
        wave_word y1 = (y->r1[i] ^ a->r1[i]) & y0;
        wave_word tmp = x->r0[i] ^ y0;
        x->r0[i] = tmp | (x->r1[i] ^ y1);
        x->r1[i] = (tmp ^ y1) & (x->r1[i] ^ y0);
    }
}

#else

static inline void vf3_vector_add_inplace(vf3_e *a, const vf3_e *b) {
  for (size_t i = 0; i < a->alloc; i++) {
    wave_word tmp = a->r1[i] ^ b->r0[i];
    a->r1[i] = (a->r0[i] ^ b->r1[i]) & tmp;
    a->r0[i] = (a->r0[i] ^ b->r0[i]) | (tmp ^ b->r1[i]);
  }
}

static inline void vf3_vector_add(vf3_e *c, const vf3_e *a, const vf3_e *b) {
  for (size_t i = 0; i < a->alloc; i++) {
    wave_word tmp = a->r1[i] ^ b->r0[i];
    c->r1[i] = (a->r0[i] ^ b->r1[i]) & tmp;
    c->r0[i] = (a->r0[i] ^ b->r0[i]) | (tmp ^ b->r1[i]);
  }
}

static inline void vf3_vector_neg(vf3_e *a, const vf3_e *b) {
  memcpy(a->r0, b->r0, a->alloc * sizeof(wave_word));
  for (size_t i = 0; i < b->alloc; i++) {
    a->r1[i] = b->r1[i] ^ b->r0[i];
  }
}

static inline void vf3_vector_neg_inplace(vf3_e *b) {
  for (size_t i = 0; i < b->alloc; i++) {
    b->r1[i] = b->r1[i] ^ b->r0[i];
  }
}

static inline void vf3_vector_sub(vf3_e *c, const vf3_e *a, const vf3_e *b) {
  for (size_t i = 0; i < a->alloc; i++) {
    wave_word tmp = a->r0[i] ^ b->r0[i];
    c->r0[i] = tmp | ((a->r1[i] ^ b->r1[i]));
    c->r1[i] = (tmp ^ b->r1[i]) & (a->r1[i] ^ b->r0[i]);
  }
}

static inline void vf3_vector_sub_inplace(vf3_e *a, const vf3_e *b) {
  for (size_t i = 0; i < a->alloc; i++) {
    wave_word tmp = a->r0[i] ^ b->r0[i];
    a->r0[i] = tmp | (a->r1[i] ^ b->r1[i]);
    a->r1[i] = (tmp ^ b->r1[i]) & (a->r1[i] ^ b->r0[i]);
  }
}

static inline void vf3_vector_mul(vf3_e *res, const vf3_e *x, const vf3_e *y) {
  for (size_t i = 0; i < x->alloc; i++) {
    res->r0[i] = x->r0[i] & y->r0[i];
    res->r1[i] = (x->r1[i] ^ y->r1[i]) & res->r0[i];
  }
}

static inline void vf3_vector_mul_inplace(vf3_e *x, const vf3_e *y) {
  for (size_t i = 0; i < x->alloc; i++) {
    x->r0[i] = x->r0[i] & y->r0[i];
    x->r1[i] = (x->r1[i] ^ y->r1[i]) & x->r0[i];
  }
}

static inline void vf3_vector_scalarmul(vf3_e *res, const uint8_t coeff,
                                        const vf3_e *x) {
  wave_word mask0 = -(coeff > 0);
  wave_word mask1 = -(coeff > 1);
  for (size_t i = 0; i < x->alloc; i++) {
    res->r0[i] = x->r0[i] & mask0;
    res->r1[i] = (x->r1[i] ^ mask1) & res->r0[i];
  }
}

static inline void vf3_vector_scalarmul_inplace(vf3_e *x, const uint8_t coeff) {
  wave_word mask0 = -(coeff > 0);
  wave_word mask1 = -(coeff > 1);
  for (size_t i = 0; i < x->alloc; i++) {
    x->r0[i] = x->r0[i] & mask0;
    x->r1[i] = (x->r1[i] ^ mask1) & x->r0[i];
  }
}

static inline void vf3_vector_scalarmul_nonzero_inplace(vf3_e *x,
                                                        const uint8_t coeff) {
  wave_word mask1 = -(coeff > 1);
  for (size_t i = 0; i < x->alloc; i++) {
    x->r1[i] = (x->r1[i] ^ mask1) & x->r0[i];
  }
}

static inline void vf3_vector_add_multiple_inplace(vf3_e *x,
                                                   const uint8_t coeff,
                                                   const vf3_e *y) {
  wave_word mask0 = -(coeff > 0);
  wave_word mask1 = -(coeff > 1);
  for (size_t i = 0; i < x->alloc; i++) {
    wave_word y0 = y->r0[i] & mask0;
    wave_word y1 = (y->r1[i] ^ mask1) & y0;
    wave_word tmp = x->r1[i] ^ y0;
    x->r1[i] = (x->r0[i] ^ y1) & tmp;
    x->r0[i] = (x->r0[i] ^ y0) | (tmp ^ y1);
  }
}

static inline void vf3_vector_slice_add_multiple_inplace(vf3_e *x,
                                                         const uint8_t coeff,
                                                         const vf3_e *y,
                                                         size_t start) {
  wave_word mask0 = -(coeff > 0);
  wave_word mask1 = -(coeff > 1);
  for (size_t i = start / WORD_LENGTH; i < x->alloc; i++) {
    wave_word y0 = y->r0[i] & mask0;
    wave_word y1 = (y->r1[i] ^ mask1) & y0;
    wave_word tmp = x->r1[i] ^ y0;
    x->r1[i] = (x->r0[i] ^ y1) & tmp;
    x->r0[i] = (x->r0[i] ^ y0) | (tmp ^ y1);
  }
}

static inline void vf3_vector_add_mul_inplace(vf3_e *x, const vf3_e *a,
                                              const vf3_e *y) {
  for (size_t i = 0; i < x->alloc; i++) {
    wave_word y0 = y->r0[i] & a->r0[i];
    wave_word y1 = (y->r1[i] ^ a->r1[i]) & y0;
    wave_word tmp = x->r1[i] ^ y0;
    x->r1[i] = (x->r0[i] ^ y1) & tmp;
    x->r0[i] = (x->r0[i] ^ y0) | (tmp ^ y1);
  }
}

static inline void vf3_vector_sub_mul_inplace(vf3_e *x, const vf3_e *a,
                                              const vf3_e *y) {
  for (size_t i = 0; i < x->alloc; i++) {
    wave_word y0 = y->r0[i] & a->r0[i];
    wave_word y1 = (y->r1[i] ^ a->r1[i]) & y0;
    wave_word tmp = x->r0[i] ^ y0;
    x->r0[i] = tmp | (x->r1[i] ^ y1);
    x->r1[i] = (tmp ^ y1) & (x->r1[i] ^ y0);
  }
}

#endif

static inline void vf3_set_slice(vf3_e *x, const uint8_t *data) {
  for (size_t i = 0; i < x->size; i++) {
    vf3_set_coeff(i, x, data[i]);
  }
}

static inline void vf3_swap(vf3_e *x, vf3_e *y) {
  vf3_e *z = vf3_alloc(x->size);
  vf3_copy(z, x);
  vf3_copy(x, y);
  vf3_copy(y, z);
  vf3_free(z);
}

#endif  // WAVE2_VF3_H
