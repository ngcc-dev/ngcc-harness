#ifndef FFI_H
#define FFI_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "rbc.h"
#include "rbc_elt.h"

typedef struct ffi_elt {
  rbc_elt value;
} ffi_elt;

typedef struct ffi_vec {
  ffi_elt *data;
  size_t size;
} ffi_vec;

static inline void ffi_vec_init(ffi_vec *v) {
  v->data = NULL;
  v->size = 0;
}

static inline void ffi_vec_clear(ffi_vec *v) {
  free(v->data);
  v->data = NULL;
  v->size = 0;
}

static inline int ffi_vec_resize(ffi_vec *v, size_t new_size) {
  if(new_size == v->size) {
    return 0;
  }
  ffi_elt *replacement = new_size == 0
    ? NULL : (ffi_elt *) calloc(new_size, sizeof(ffi_elt));
  if(new_size != 0 && replacement == NULL) {
    return -1;
  }
  const size_t copy_size = v->size < new_size ? v->size : new_size;
  if(copy_size != 0) {
    memcpy(replacement, v->data, copy_size * sizeof(ffi_elt));
  }
  free(v->data);
  v->data = replacement;
  v->size = new_size;
  return 0;
}

static inline int ffi_vec_copy(ffi_vec *out, const ffi_vec *in) {
  if(out == in) {
    return 0;
  }
  if(ffi_vec_resize(out, in->size) != 0) {
    return -1;
  }
  if(in->size != 0) {
    memcpy(out->data, in->data, in->size * sizeof(ffi_elt));
  }
  return 0;
}

#define FIELD_Q 2
#define FIELD_M 47
#define FIELD_ELT_BYTES 6
#define FFI_VEC_G_LENGTH 86
#define MODULO_NUMBER 5
#define MODULO_VALUES {0, 3, 4, 6, 43}

extern ffi_vec ideal_modulo;
extern unsigned int ideal_modulo_degrees[MODULO_NUMBER];
extern unsigned int ideal_modulo_degree;

#endif
