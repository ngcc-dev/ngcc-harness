#ifndef FFI_ELT_H
#define FFI_ELT_H

#include "drng.h"
#include "ffi.h"

ffi_elt ffi_elt_get_zero(void);
ffi_elt ffi_elt_get_one(void);
void ffi_elt_set(ffi_elt *o, const ffi_elt *e);
void ffi_elt_set_zero(ffi_elt *o);
void ffi_elt_set_one(ffi_elt *o);
void ffi_elt_set_random(ffi_elt *o, DRNG_ctx *ctx);
void ffi_elt_set_random2(ffi_elt *o);
int ffi_elt_is_zero(const ffi_elt *e);
void ffi_elt_inv(ffi_elt *o, const ffi_elt *e);
void ffi_elt_sqr(ffi_elt *o, const ffi_elt *e);
void ffi_elt_nth_root(ffi_elt *o, const ffi_elt *e, int n);
void ffi_elt_add(ffi_elt *o, const ffi_elt *e1, const ffi_elt *e2);
void ffi_elt_mul(ffi_elt *o, const ffi_elt *e1, const ffi_elt *e2);
ffi_elt uchar_array_to_ffi_elt(const unsigned char *arr);
void ffi_elt_from_string(ffi_elt *o, const unsigned char *str);
void ffi_elt_to_string(unsigned char *str, const ffi_elt *e);
void ffi_elt_print(const ffi_elt *e);

#endif
