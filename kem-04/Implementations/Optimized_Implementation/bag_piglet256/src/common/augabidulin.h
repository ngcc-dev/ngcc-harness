#ifndef AUGABIDULIN_H
#define AUGABIDULIN_H

#include "ffi_vec.h"

typedef struct augabidulin_code {
  ffi_vec g;
  unsigned int k;
  unsigned int n;
  unsigned int n_;
} augabidulin_code;

int augabidulin_code_init(augabidulin_code *code, const ffi_vec *g,
                          unsigned int k, unsigned int n,
                          unsigned int n_);
void augabidulin_code_clear(augabidulin_code *code);
void augabidulin_code_encode(ffi_vec *c, const augabidulin_code *code,
                             const ffi_vec *message);
void augabidulin_code_decode(ffi_vec *message,
                             const augabidulin_code *code,
                             const ffi_vec *word,
                             unsigned int epsilon);

#endif
