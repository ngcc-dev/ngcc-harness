#ifndef FFI_VEC_H
#define FFI_VEC_H

#include "drng.h"
#include "ffi.h"

ffi_elt ffi_vec_get_coeff(const ffi_vec *v, unsigned int position);
void ffi_vec_set_coeff(ffi_vec *o, const ffi_elt *e,
                       unsigned int position);
void ffi_vec_set_length(ffi_vec *o, unsigned int size);
void ffi_vec_set(ffi_vec *o, const ffi_vec *v, unsigned int size);
void ffi_vec_set_zero(ffi_vec *o, unsigned int size);
void ffi_vec_set_random(ffi_vec *o, unsigned int size, DRNG_ctx *ctx);
void ffi_vec_set_random2(ffi_vec *o, unsigned int size);
void ffi_vec_set_random_full_rank(ffi_vec *o, unsigned int size,
                                  DRNG_ctx *ctx);
void ffi_vec_set_random_full_rank_with_one(ffi_vec *o, unsigned int size,
                                           DRNG_ctx *ctx);
int uchar_array_to_ffi_vec(ffi_vec *o, const unsigned char *arr,
                           unsigned int size);
int uchar_array_to_ffi_vec_2(const unsigned char *arr, ffi_vec *o,
                             unsigned int size);
void ffi_vec_set_random_using_shake(ffi_vec *o, unsigned int size,
                                    const unsigned char *output);
int ffi_vec_set_random_full_rank_using_shake(ffi_vec *o,
                                              unsigned int size,
                                              const unsigned char *output);
int ffi_vec_set_g_using_shake(ffi_vec *o, unsigned int size,
                              unsigned int rank,
                              const unsigned char *output);
int ffi_vec_set_random_from_support_using_shake(
  ffi_vec *o, unsigned int size, const ffi_vec *support,
  unsigned int rank, const unsigned char *output);
void ffi_vec_set_random_from_support(ffi_vec *o, unsigned int size,
                                     const ffi_vec *support,
                                     unsigned int rank, DRNG_ctx *ctx);
int ffi_vec_set_error_from_support_using_shake(
  ffi_vec *o, unsigned int size, const ffi_vec *support,
  unsigned int rank, const unsigned char *output);
int ffi_vec_is_equal_to(const ffi_vec *v1, const ffi_vec *v2,
                        unsigned int size);
void ffi_vec_add(ffi_vec *o, const ffi_vec *v1, const ffi_vec *v2,
                 unsigned int size);
void ffi_vec_mul(ffi_vec *o, const ffi_vec *v1, const ffi_vec *v2,
                 unsigned int size);
void ffi_vec_to_string(unsigned char *str, const ffi_vec *v,
                       unsigned int size);
void ffi_vec_to_string_compact(unsigned char *str, const ffi_vec *v,
                               unsigned int size);
void ffi_vec_from_string(ffi_vec *o, unsigned int size,
                         const unsigned char *str);
void ffi_vec_from_string_compact(ffi_vec *o, unsigned int size,
                                 const unsigned char *str);
unsigned int ffi_vec_get_rank(const ffi_vec *v, unsigned int size);
void ffi_vec_print(const ffi_vec *v, unsigned int size);
void message_random2(unsigned char *o, unsigned int size);
int ffi_vec_recover_part_error_support(ffi_vec *o, unsigned int rank,
                                       unsigned int size,
                                       const ffi_vec *e);

#endif
