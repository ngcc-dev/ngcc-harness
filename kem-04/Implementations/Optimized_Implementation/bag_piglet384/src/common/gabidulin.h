#ifndef GABIDULIN_H
#define GABIDULIN_H

#include "ffi_vec.h"
#include "q_polynomial.h"

typedef struct gabidulin_code {
  ffi_vec g;
  unsigned int k;
  unsigned int n;
} gabidulin_code;

int gabidulin_code_init(gabidulin_code *code, const ffi_vec *g,
                        unsigned int k, unsigned int n);
void gabidulin_code_clear(gabidulin_code *code);
void gabidulin_code_encode(ffi_vec *c, const gabidulin_code *code,
                           const ffi_vec *message);
void gabidulin_code_decode(ffi_vec *message, const gabidulin_code *code,
                           const ffi_vec *word);
void gabidulin_code_decode_2(ffi_vec *message,
                             const gabidulin_code *code,
                             const ffi_vec *word,
                             q_polynomial *v2);
void gabidulin_code_decode_3(ffi_vec *message,
                             const gabidulin_code *code,
                             const ffi_vec *word,
                             q_polynomial *v2);

#endif
