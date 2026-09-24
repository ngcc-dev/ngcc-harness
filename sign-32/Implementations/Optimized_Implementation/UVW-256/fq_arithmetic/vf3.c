/******************************************************************************
WAVE -- Code-Based Digital Signature Scheme
Copyright (c) 2023 The Wave Team
contact: wave-contact@inria.fr

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

#include "vf3.h"
#include "../macros.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_API_PKC
    #include "../drng.h"
    extern DRNG_ctx *uvw_drng;

    static inline void vf3_csprng_read(void *out, size_t length) {
        get_random_number(uvw_drng, out, (unsigned long long)length * 8);
    }
    #define csprng_read(buf, len) vf3_csprng_read(buf, len)
#else
    #include "../csprng.h"
#endif

void vf3_init(vf3_e *e, const size_t length) {
  e->size = length;
  e->alloc = 1 + ((length - 1) / WORD_LENGTH);
  e->r0 = (wave_word *)calloc(e->alloc, sizeof(wave_word));
  e->r1 = (wave_word *)calloc(e->alloc, sizeof(wave_word));
}

vf3_e *vf3_array_alloc(const size_t length, const size_t array_length) {
  vf3_e *e = malloc(sizeof(vf3_e) * array_length);
  for (size_t i = 0; i < array_length; ++i) {
    vf3_init(e + i, length);
  }
  return e;
}

vf3_e *vf3_alloc(const size_t length) {
  vf3_e *e = malloc(sizeof(vf3_e));
  vf3_init(e, length);
  return e;
}

void vf3_array_free(vf3_e *e, const size_t array_length) {
  for (size_t i = 0; i < array_length; ++i) {
    free(e[i].r0);
    free(e[i].r1);
  }
  free(e);
}

void vf3_free(vf3_e *e) {
  free(e->r0);
  free(e->r1);
  free(e);
}

void vf3_copy(vf3_e *dest, const vf3_e *src) {
  memcpy(dest->r0, src->r0, src->alloc * sizeof(wave_word));
  memcpy(dest->r1, src->r1, src->alloc * sizeof(wave_word));
}

// remove stray bits in the last allocated word
void vf3_trim(vf3_e *e) {
  if (e->size % WORD_LENGTH) {
    e->r0[e->alloc - 1] &= ((wave_word)1 << (e->size % WORD_LENGTH)) - 1;
    e->r1[e->alloc - 1] &= ((wave_word)1 << (e->size % WORD_LENGTH)) - 1;
  }
}

bool vf3_equal(vf3_e *x, vf3_e *y) {
  vf3_trim(x);
  vf3_trim(y);
  return (memcmp(x->r0, y->r0, x->alloc * sizeof(wave_word)) == 0) &&
         (memcmp(x->r1, y->r1, x->alloc * sizeof(wave_word)) == 0);
}

void vf3_vector_constant(vf3_e *e, const uint8_t a) {
  memset(e->r0, (a > 0) * 0xFF, e->alloc * sizeof(wave_word));
  memset(e->r1, (a > 1) * 0xFF, e->alloc * sizeof(wave_word));
}

void vf3_set_to_zero(vf3_e *e) {
  memset(e->r0, 0, e->alloc * sizeof(wave_word));
  memset(e->r1, 0, e->alloc * sizeof(wave_word));
}

uint8_t vf3_get_element(const size_t j, const vf3_e *x) {
  size_t i, t;
  i = j / WORD_LENGTH;
  t = j % WORD_LENGTH;
  // add the high and low bits
  return ((x->r0[i] >> t) & 1) + ((x->r1[i] >> t) & 1);
}

void vf3_set_coeff(size_t j, vf3_e *x, const uint8_t a) {
  // assume a in {0, 1, 2}
  size_t i;
  i = j / WORD_LENGTH;
  j %= WORD_LENGTH;
  wave_word z = ((wave_word)1) << j;
  wave_word a0 = ((wave_word)(a > 0)) << j;
  wave_word a1 = ((wave_word)(a > 1)) << j;
  x->r0[i] &= ~z;
  x->r1[i] &= ~z;
  x->r0[i] ^= a0;
  x->r1[i] ^= a1;
}

// x_j <- x_j + a mod 3
void vf3_add_coeff(size_t j, vf3_e *x, const uint8_t a) {
  // assume a in {0, 1, 2}
  size_t i = j / WORD_LENGTH;
  j %= WORD_LENGTH;
  x->r0[i] ^= ((wave_word)(a > 0)) << j;
  x->r1[i] ^= ((wave_word)(a > 1)) << j;
}

#ifdef NO_MUL_128
// low + 2^64 * high = x * 81
void mul81(uint64_t *low, uint64_t *high, uint64_t x) {
  static const uint64_t m56 = 0x00FFFFFFFFFFFFFF;
  *low = (x & m56) * 81;
  *high = (x >> 56) * 81 + (*low >> 56);
  *low = (*low & m56) | *high << 56;
  *high >>= 8;
}
#endif

// 转换为32个00 01 10
// 大概就是把x % (3 ** 32)当成三进制整数？
// (2 ** 64) / (3 ** 32) = 9954.96，差距不大
uint64_t convert64(uint64_t x) {
  static const uint64_t F2 = 0x0404040404040404;
  static const uint64_t F3 = 0x0808080808080808;
  static const uint64_t F4 = 0x1010101010101010;
  static const uint64_t F5 = 0x2020202020202020;
  static const uint64_t F6 = 0x4040404040404040;
  static const uint64_t F7 = 0x8080808080808080;
  static const uint64_t Y2 = 0x0101010101010101;
  static const uint64_t Y3 = 0x0707070707070707;
  static const uint64_t Y4 = 0x2525252525252525;
  size_t i;
  uint64_t u, y, z, r = 0;
  for (i = 0; i < 8; ++i) {
#ifdef NO_MUL_128
    uint64_t v;
    mul81(&v, &u, x);  // v + u * 2^64 <- x * 81
#else
    unsigned __int128 v;
    v = x;
    v *= 81;
    u = (v >> 64);
#endif
    x = v;
    r = u ^ (r << 8);
  }
  y = r + Y4;
  z = r + (2 * Y4);
  r += (((y & z & F6) ^ (z & F7)) >> 6) * 37;
  y = r + Y3;
  z = r + (2 * Y3);
  r += (((y & z & F4) ^ (z & F5)) >> 4) * 7;
  y = r + Y2;
  z = r + (2 * Y2);
  r += ((y & z & F2) ^ (z & F3)) >> 2;

  return r;
}

typedef uint64_t convert_word_t;
#define EVEN 0x5555555555555555UL
#define ODD 0xAAAAAAAAAAAAAAAAUL

void vf3_trits_from_bits(vf3_e *y, uint64_t *rnd) {
  // assume rnd is loaded with 2 * len random words of 64 bits
  size_t len =
      1 + (y->alloc * sizeof(wave_word) - 1) / sizeof(uint64_t);  // rounded up

  convert_word_t r0, r1, *y0, *y1;
  y0 = rnd;
  y1 = rnd + len;
  for (size_t l = 0; l * sizeof(convert_word_t) < len * sizeof(uint64_t); ++l) {
    r0 = convert64(y0[l]);
    r1 = convert64(y1[l]);
    // r0和r1一共输出了32+32=64个00 01 10
    // y0[l]是这些的高位，y1[l]是低位
    // ^ y0[l]的作用是将10转换为F3内部使用的11
    y0[l] = ((r0 >> 1) & EVEN) ^ (r1 & ODD);
    y1[l] = (r0 & EVEN) ^ ((r1 << 1) & ODD) ^ y0[l];
  }
  memcpy(y->r0, y1, y->alloc * sizeof(wave_word));
  memcpy(y->r1, y0, y->alloc * sizeof(wave_word));
}

void vf3_random(vf3_e *e) {
  // rounded up in case sizeof(wave_word) < sizeof(uint64_t)
  size_t len = 1 + (e->alloc * sizeof(wave_word) - 1) / sizeof(uint64_t);
  uint64_t *rnd = (uint64_t *)malloc(2 * len * sizeof(uint64_t));
  csprng_read(rnd, 2 * len * sizeof(uint64_t));
  vf3_trits_from_bits(e, rnd);
  free(rnd);
}

void vf3_random_non_zero(vf3_e *e) {
  memset(e->r0, 0xFF, e->alloc * sizeof(wave_word));
  csprng_read(e->r1, e->alloc * sizeof(wave_word));
}

void vf3_vector_cat_zero(vf3_e *res, vf3_e *const a) {
  // assume res->alloc >= a->alloc
  memset(res->r0, 0, sizeof(wave_word) * res->alloc);
  memset(res->r1, 0, sizeof(wave_word) * res->alloc);

  memcpy(res->r0, a->r0, sizeof(wave_word) * a->alloc);
  memcpy(res->r1, a->r1, sizeof(wave_word) * a->alloc);

  wave_word shift = a->size % WORD_LENGTH;
  if (shift > 0) {
    res->r0[a->alloc - 1] &= ((wave_word)1 << shift) - 1;
    res->r1[a->alloc - 1] &= ((wave_word)1 << shift) - 1;
  }
}

void vf3_vector_cat(vf3_e *res, vf3_e *const a, vf3_e *const b) {
  if (b == NULL) return vf3_vector_cat_zero(res, a);
  // assume res->size = a->size + b->size
  memset(res->r0, 0, sizeof(wave_word) * res->alloc);
  memset(res->r1, 0, sizeof(wave_word) * res->alloc);

  memcpy(res->r0, a->r0, sizeof(wave_word) * a->alloc);
  memcpy(res->r1, a->r1, sizeof(wave_word) * a->alloc);

  wave_word shift = a->size % WORD_LENGTH;
  if (shift == 0) {
    memcpy(res->r0 + a->alloc, b->r0, sizeof(wave_word) * b->alloc);
    memcpy(res->r1 + a->alloc, b->r1, sizeof(wave_word) * b->alloc);
  } else {
    size_t i, j;
    res->r0[a->alloc - 1] &= ((wave_word)1 << shift) - 1;
    res->r1[a->alloc - 1] &= ((wave_word)1 << shift) - 1;

    for (i = 0, j = a->alloc - 1; j + 1 < res->alloc; ++i, ++j) {
      res->r0[j] ^= b->r0[i] << shift;
      res->r0[j + 1] ^= b->r0[i] >> (WORD_LENGTH - shift);
      res->r1[j] ^= b->r1[i] << shift;
      res->r1[j + 1] ^= b->r1[i] >> (WORD_LENGTH - shift);
    }
    // j = res->alloc - 1
    // i could be b->alloc or b->alloc - 1
    // if i = b->alloc - 1, we are not done yet
    if (i < b->alloc) {
      res->r0[j] ^= b->r0[i] << shift;
      res->r1[j] ^= b->r1[i] << shift;
    }
  }
}

void vf3_vector_split_zero(vf3_e *a, vf3_e *const x) {
  // assume a->size + b->size = x->size
  memset(a->r0, 0, sizeof(wave_word) * a->alloc);
  memset(a->r1, 0, sizeof(wave_word) * a->alloc);

  memcpy(a->r0, x->r0, sizeof(wave_word) * a->alloc);
  memcpy(a->r1, x->r1, sizeof(wave_word) * a->alloc);
  vf3_trim(a);
}

void vf3_vector_split(vf3_e *a, vf3_e *b, vf3_e *const x) {
  if (b == NULL) return vf3_vector_split_zero(a, x);
  // assume a->size + b->size = x->size
  memset(a->r0, 0, sizeof(wave_word) * a->alloc);
  memset(a->r1, 0, sizeof(wave_word) * a->alloc);
  memset(b->r0, 0, sizeof(wave_word) * b->alloc);
  memset(b->r1, 0, sizeof(wave_word) * b->alloc);

  memcpy(a->r0, x->r0, sizeof(wave_word) * a->alloc);
  memcpy(a->r1, x->r1, sizeof(wave_word) * a->alloc);
  vf3_trim(a);

  wave_word shift = a->size % WORD_LENGTH;
  if (shift == 0) {
    memcpy(b->r0, x->r0 + a->alloc, sizeof(wave_word) * b->alloc);
    memcpy(b->r1, x->r1 + a->alloc, sizeof(wave_word) * b->alloc);
  } else {
    size_t i, j;

    for (i = 0, j = a->alloc - 1; j + 1 < x->alloc; ++i, ++j) {
      b->r0[i] ^= x->r0[j] >> shift;
      b->r0[i] ^= x->r0[j + 1] << (WORD_LENGTH - shift);
      b->r1[i] ^= x->r1[j] >> shift;
      b->r1[i] ^= x->r1[j + 1] << (WORD_LENGTH - shift);
    }
    // j = x->alloc - 1
    // i could be b->alloc or b->alloc - 1
    // if i = b->alloc - 1, we are not done yet
    if (i < b->alloc) {
      b->r0[i] = x->r0[j] >> shift;
      b->r1[i] = x->r1[j] >> shift;
    }
  }
}

size_t vf3_read(vf3_e *e, FILE *f) {
  if (fread(e->r0, sizeof(wave_word), e->alloc, f) != e->alloc) return 0;
  if (fread(e->r1, sizeof(wave_word), e->alloc, f) != e->alloc) return 0;
  return e->size;
}

size_t vf3_write(vf3_e *e, FILE *f) {
  if (fwrite(e->r0, sizeof(wave_word), e->alloc, f) != e->alloc) return 0;
  if (fwrite(e->r1, sizeof(wave_word), e->alloc, f) != e->alloc) return 0;
  return e->size;
}

void f3_compact_print(const wave_word r0, const wave_word r1, size_t len) {
  wave_word one = 1;
  for (size_t i = 0; i < len; i++) {
    wave_word pos = one << i;
    if ((r0 & pos) && (r1 & pos)) {
      printf("2,");
    } else if (r0 & pos) {
      printf("1,");
    } else if (!(r0 & pos) && !(r1 & pos)) {
      printf("0,");
    } else {
      printf("X,");
    }
  }
  // putchar('\n');
}

void vf3_print(const vf3_e *a) {
  for (size_t i = 0; i < a->alloc; i++) {
    size_t len = a->size - i * WORD_LENGTH;
    f3_compact_print(a->r0[i], a->r1[i],
                     (len < WORD_LENGTH) ? len : WORD_LENGTH);
  }
  putchar('\n');
}
