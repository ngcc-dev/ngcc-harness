/**
 * \file ffi_vec.c
 * \brief C implementation of vectors over finite field elements using RBC
 */

#include "ffi.h"
#include "ffi_elt.h"
#include "ffi_vec.h"
#include "rbc_qre.h"
#include "rbc_vec.h"
#include "rbc_vspace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char get_bit(unsigned char c, unsigned int position);
static unsigned int compact_size(unsigned int size);
static unsigned int elt_bit(const ffi_elt* e, unsigned int position);
static void set_elt_bit(ffi_elt* e, unsigned int position, unsigned int bit);
static unsigned int vec_degree(const ffi_vec* v);
static const ffi_elt* ffi_vec_coeff_const(const ffi_vec* v, unsigned int position);
static ffi_elt* ffi_vec_coeff_mut(ffi_vec* v, unsigned int position);
static void ffi_vec_copy_to_rbc(rbc_vec o, const ffi_vec* v, unsigned int size);
static void ffi_vec_copy_from_rbc(ffi_vec* o, const rbc_vec v, unsigned int size);

static const ffi_elt* ffi_vec_coeff_const(const ffi_vec* v, unsigned int position) {
  static const ffi_elt zero = {0};
  return position < v->size ? &v->data[position] : &zero;
}

static ffi_elt* ffi_vec_coeff_mut(ffi_vec* v, unsigned int position) {
  if(position >= v->size && ffi_vec_resize(v, (size_t) position + 1) != 0) {
    abort();
  }
  return &v->data[position];
}

static void ffi_vec_copy_to_rbc(rbc_vec o, const ffi_vec* v, unsigned int size) {
  for(unsigned int i = 0; i < size; ++i) {
    rbc_elt_set(o[i], ffi_vec_coeff_const(v, i)->value);
  }
}

static void ffi_vec_copy_from_rbc(ffi_vec* o, const rbc_vec v, unsigned int size) {
  ffi_vec_set_length(o, size);
  for(unsigned int i = 0; i < size; ++i) {
    rbc_elt_set(o->data[i].value, v[i]);
  }
}

int uchar_array_to_ffi_vec(ffi_vec* o, const unsigned char* arr, unsigned int size) {
  ffi_vec_set_length(o, size);
  unsigned long bit_pos = 0;

  for(unsigned int i = 0; i < size; ++i) {
    ffi_elt_set_zero(&o->data[i]);
    for(unsigned int j = 0; j < FIELD_M; ++j) {
      const unsigned long bit_index = bit_pos + j;
      const unsigned long byte_index = bit_index / 8;
      const unsigned long bit_in_byte = bit_index % 8;
      if((arr[byte_index] >> bit_in_byte) & 1) {
        set_elt_bit(&o->data[i], j, 1);
      }
    }
    bit_pos += FIELD_M;
  }

  return 0;
}

int uchar_array_to_ffi_vec_2(const unsigned char* arr, ffi_vec* o, unsigned int size) {
  return uchar_array_to_ffi_vec(o, arr, size);
}

unsigned int ffi_vec_get_rank(const ffi_vec* v, unsigned int size) {
  rbc_vspace space;
  rbc_vspace_init(&space, size);
  ffi_vec_copy_to_rbc(space, v, size);
  const unsigned int rank = rbc_vspace_get_rank(space, size);
  rbc_vspace_clear(space);
  return rank;
}

int ffi_vec_set_error_from_support_using_shake(ffi_vec* o, unsigned int size, const ffi_vec* support, unsigned int rank, const unsigned char* output) {
  (void) output;
  ffi_vec_set_length(o, size);

  for(unsigned int i = 0; i < rank && i < size; ++i) {
    ffi_vec_set_coeff(o, ffi_vec_coeff_const(support, i), i);
  }
  for(unsigned int i = rank; i < size; ++i) {
    if(ffi_elt_is_zero(&o->data[i])) {
      ffi_vec_set_coeff(o, ffi_vec_coeff_const(support, 0), i);
    }
  }
  for(unsigned int i = 43; i < 71 && i < size; ++i) {
    ffi_vec_set_coeff(o, ffi_vec_coeff_const(support, 0), i);
  }
  for(unsigned int i = 72; i < 104 && i < size; ++i) {
    if(i - 72 < rank) {
      ffi_vec_set_coeff(o, ffi_vec_coeff_const(support, i - 72), i);
    }
  }
  return 0;
}

int ffi_vec_set_random_from_support_using_shake(ffi_vec* o, unsigned int size, const ffi_vec* support, unsigned int rank, const unsigned char* output) {
  ffi_vec_set_length(o, size);
  const unsigned int random_position_size = 2 * rank;
  unsigned char* random_position = (unsigned char*) malloc(random_position_size);
  if(random_position == NULL) {
    abort();
  }
  memcpy(random_position, output, random_position_size);
  unsigned int count = random_position_size;

  unsigned int i = 0;
  unsigned int j = 0;
  while(i != rank) {
    const unsigned int position = random_position[j];
    if(ffi_elt_is_zero(&o->data[position % size])) {
      ffi_vec_set_coeff(o, ffi_vec_coeff_const(support, i % rank), position % size);
      ++i;
    }

    ++j;
    if(j % random_position_size == 0 && i != rank) {
      memcpy(random_position, output + count, random_position_size);
      count += random_position_size;
      j = 0;
    }
  }

  const unsigned int random_lin_comb_size = rank * (size - rank);
  unsigned char* random_lin_comb = (unsigned char*) malloc(random_lin_comb_size);
  if(random_lin_comb == NULL) {
    free(random_position);
    abort();
  }
  memcpy(random_lin_comb, output, random_lin_comb_size);

  unsigned int k = 0;
  for(i = 0; i < size; ++i) {
    if(ffi_elt_is_zero(&o->data[i])) {
      ffi_vec_set_coeff(o, ffi_vec_coeff_const(support, i % rank), i);
      for(j = 0; j < rank; ++j) {
        if((random_lin_comb[k * rank + j] % FIELD_Q) == 1) {
          ffi_elt_add(&o->data[i], ffi_vec_coeff_const(support, j), &o->data[i]);
        }
      }
      ++k;
    }
  }
  for(i = 0; i < size; ++i) {
    if(ffi_elt_is_zero(&o->data[i])) {
      ffi_vec_set_coeff(o, ffi_vec_coeff_const(support, i % rank), i);
    }
  }

  free(random_position);
  free(random_lin_comb);
  return 0;
}

int ffi_vec_set_random_full_rank_using_shake(ffi_vec* o, unsigned int size, const unsigned char* output) {
  const unsigned int rank_max = FIELD_M < size ? FIELD_M : size;
  unsigned int count = 0;
  unsigned int rank = (unsigned int) -1;
  const unsigned int chunk_size = size * FIELD_M / 8 + 1;

  while(rank != rank_max) {
    if(uchar_array_to_ffi_vec(o, output + count * chunk_size, size) != 0) {
      return -1;
    }
    rank = ffi_vec_get_rank(o, size);
    ++count;
  }
  return (int) count;
}

int ffi_vec_set_g_using_shake(ffi_vec* o, unsigned int size, unsigned int rank, const unsigned char* output) {
  const unsigned int rank_max = FIELD_M < rank ? FIELD_M : rank;
  unsigned int count = 0;
  unsigned int current_rank = (unsigned int) -1;
  const unsigned int chunk_size = rank_max * FIELD_M / 8 + 1;

  while(current_rank != rank_max) {
    uchar_array_to_ffi_vec_2(output + count * chunk_size, o, rank_max);
    current_rank = ffi_vec_get_rank(o, rank);
    ++count;
  }

  ffi_vec zero;
  ffi_vec_init(&zero);
  ffi_vec_set_length(&zero, size - rank);
  for(unsigned int i = rank; i < size; ++i) {
    ffi_vec_set_coeff(o, ffi_vec_coeff_const(&zero, i - rank), i);
  }
  ffi_vec_clear(&zero);
  if(ffi_vec_resize(o, FFI_VEC_G_LENGTH) != 0) {
    abort();
  }
  return (int) count;
}

int ffi_vec_recover_part_error_support(ffi_vec* o, unsigned int rank, unsigned int size, const ffi_vec* e) {
  ffi_vec_set_length(o, rank);
  int current_rank = 0;
  int i = 0;
  int count = 0;
  while(current_rank != (int) rank && i < (int) size) {
    ffi_vec_set_coeff(o, ffi_vec_coeff_const(e, (unsigned int) i), (unsigned int) count);
    if(ffi_vec_get_rank(o, (unsigned int) count + 1) == (unsigned int) (current_rank + 1)) {
      ++current_rank;
      ++count;
    }
    ++i;
  }
  return 0;
}

ffi_elt ffi_vec_get_coeff(const ffi_vec* v, unsigned int position) {
  return *ffi_vec_coeff_const(v, position);
}

void ffi_vec_set_coeff(ffi_vec* o, const ffi_elt* e, unsigned int position) {
  ffi_elt_set(ffi_vec_coeff_mut(o, position), e);
}

void ffi_vec_set_length(ffi_vec* o, unsigned int size) {
  if(ffi_vec_resize(o, size) != 0) {
    abort();
  }
}

void ffi_vec_set(ffi_vec* o, const ffi_vec* v, unsigned int size) {
  (void) size;
  if(ffi_vec_copy(o, v) != 0) {
    abort();
  }
}

void ffi_vec_set_zero(ffi_vec* o, unsigned int size) {
  ffi_vec_set_length(o, size);
  for(unsigned int i = 0; i < size; ++i) {
    ffi_elt_set_zero(&o->data[i]);
  }
}

void ffi_vec_set_random(ffi_vec* o, unsigned int size, DRNG_ctx* ctx) {
  ffi_vec_set_length(o, size);
  for(unsigned int i = 0; i < size; ++i) {
    ffi_elt_set_random(&o->data[i], ctx);
  }
}

void ffi_vec_set_random_using_shake(ffi_vec* o, unsigned int size, const unsigned char* output) {
  ffi_vec_set_length(o, size);
  for(unsigned int i = 0; i < size; ++i) {
    o->data[i] = uchar_array_to_ffi_elt(output + i * FIELD_ELT_BYTES);
  }
}

void ffi_vec_set_random2(ffi_vec* o, unsigned int size) {
  ffi_vec_set_length(o, size);
  for(unsigned int i = 0; i < size; ++i) {
    ffi_elt_set_random2(&o->data[i]);
  }
}

void message_random2(unsigned char* o, unsigned int size) {
  ffi_vec m;
  ffi_vec_init(&m);
  ffi_vec_set_random2(&m, size);
  ffi_vec_to_string_compact(o, &m, size);
  o[size - 1] &= 0x07;
  ffi_vec_clear(&m);
}

void ffi_vec_set_random_full_rank(ffi_vec* o, unsigned int size, DRNG_ctx* ctx) {
  const unsigned int rank_max = FIELD_M < size ? FIELD_M : size;
  unsigned int rank = (unsigned int) -1;
  while(rank != rank_max) {
    ffi_vec_set_random(o, size, ctx);
    rank = ffi_vec_get_rank(o, size);
  }
}

void ffi_vec_set_random_full_rank_with_one(ffi_vec* o, unsigned int size, DRNG_ctx* ctx) {
  const unsigned int rank_max = FIELD_M < size ? FIELD_M : size;
  unsigned int rank = (unsigned int) -1;
  const ffi_elt one = ffi_elt_get_one();
  while(rank != rank_max) {
    ffi_vec_set_random(o, size - 1, ctx);
    ffi_vec_set_coeff(o, &one, size - 1);
    rank = ffi_vec_get_rank(o, size);
  }
}

void ffi_vec_set_random_from_support(ffi_vec* o, unsigned int size, const ffi_vec* support, unsigned int rank, DRNG_ctx* ctx) {
  ffi_vec_set_length(o, size);

  const unsigned int random_position_size = 2 * rank;
  unsigned char* random_position = (unsigned char*) malloc(random_position_size);
  if(random_position == NULL) {
    abort();
  }
  get_random_number(ctx, random_position, 8ULL * random_position_size);

  unsigned int i = 0;
  unsigned int j = 0;
  while(i != rank) {
    const unsigned int position = random_position[j];
    if(position < size * (256 / size) && ffi_elt_is_zero(&o->data[position % size])) {
      ffi_vec_set_coeff(o, ffi_vec_coeff_const(support, i), position % size);
      ++i;
    }

    ++j;
    if(j % random_position_size == 0 && i != rank) {
      get_random_number(ctx, random_position, 8ULL * random_position_size);
      j = 0;
    }
  }

  const unsigned int random_lin_comb_size = rank * (size - rank);
  unsigned char* random_lin_comb = (unsigned char*) malloc(random_lin_comb_size);
  if(random_lin_comb == NULL) {
    free(random_position);
    abort();
  }
  get_random_number(ctx, random_lin_comb, 8ULL * random_lin_comb_size);

  unsigned int k = 0;
  for(i = 0; i < size; ++i) {
    if(ffi_elt_is_zero(&o->data[i])) {
      for(j = 0; j < rank; ++j) {
        if((random_lin_comb[k * rank + j] % FIELD_Q) == 1) {
          ffi_elt_add(&o->data[i], ffi_vec_coeff_const(support, j), &o->data[i]);
        }
      }
      ++k;
    }
  }

  free(random_position);
  free(random_lin_comb);
}

int ffi_vec_is_equal_to(const ffi_vec* v1, const ffi_vec* v2, unsigned int size) {
  for(unsigned int i = 0; i < size; ++i) {
    if(!rbc_elt_is_equal_to(ffi_vec_coeff_const(v1, i)->value, ffi_vec_coeff_const(v2, i)->value)) {
      return 0;
    }
  }
  return 1;
}

void ffi_vec_add(ffi_vec* o, const ffi_vec* v1, const ffi_vec* v2, unsigned int size) {
  rbc_vec left;
  rbc_vec right;
  rbc_vec result;
  rbc_vec_init(&left, size);
  rbc_vec_init(&right, size);
  rbc_vec_init(&result, size);
  ffi_vec_copy_to_rbc(left, v1, size);
  ffi_vec_copy_to_rbc(right, v2, size);
  rbc_vec_add(result, left, right, size);
  ffi_vec_copy_from_rbc(o, result, size);
  rbc_vec_clear(left);
  rbc_vec_clear(right);
  rbc_vec_clear(result);
}

void ffi_vec_mul(ffi_vec* o, const ffi_vec* v1, const ffi_vec* v2, unsigned int size) {
  if(size == 0) {
    ffi_vec_set_length(o, 0);
    return;
  }

  if(rbc_qre_get_modulus_degree() != size) {
    const uint32_t values[MODULO_NUMBER] = MODULO_VALUES;
    rbc_qre_init_modulus(MODULO_NUMBER, values);
  }

  rbc_qre left;
  rbc_qre right;
  rbc_qre result;
  rbc_qre_init(&left);
  rbc_qre_init(&right);
  rbc_qre_init(&result);
  if(left == NULL || right == NULL || result == NULL) {
    rbc_qre_clear(left);
    rbc_qre_clear(right);
    rbc_qre_clear(result);
    ffi_vec_set_zero(o, size);
    return;
  }

  ffi_vec_copy_to_rbc(left->v, v1, size);
  ffi_vec_copy_to_rbc(right->v, v2, size);
  rbc_poly_update_degree(left, (int32_t) size - 1);
  rbc_poly_update_degree(right, (int32_t) size - 1);
  rbc_qre_mul(result, left, right);
  ffi_vec_copy_from_rbc(o, result->v, size);

  rbc_qre_clear(left);
  rbc_qre_clear(right);
  rbc_qre_clear(result);
}

static unsigned char get_bit(unsigned char c, unsigned int position) {
  return (c >> position) & 0x01;
}

void ffi_vec_to_string(unsigned char* str, const ffi_vec* v, unsigned int size) {
  memset(str, 0, (size_t) size * FIELD_ELT_BYTES);
  const unsigned int degree = vec_degree(v);
  if(degree + 1 < size) {
    size = degree + 1;
  }
  for(unsigned int i = 0; i < size; ++i) {
    ffi_elt_to_string(str + i * FIELD_ELT_BYTES, ffi_vec_coeff_const(v, i));
  }
}

void ffi_vec_from_string(ffi_vec* o, unsigned int size, const unsigned char* str) {
  ffi_vec_set_length(o, size);
  for(unsigned int i = 0; i < size; ++i) {
    ffi_elt_from_string(&o->data[i], str + i * FIELD_ELT_BYTES);
  }
}

void ffi_vec_print(const ffi_vec* v, unsigned int size) {
  size = (unsigned int) v->size;
  printf("size: %u\n", size);
  printf("[ ");
  for(unsigned int i = 0; i < size; ++i) {
    ffi_elt_print(&v->data[i]);
  }
  printf("]\n\n");
}

void ffi_vec_to_string_compact(unsigned char* str, const ffi_vec* v, unsigned int size) {
  memset(str, 0, compact_size(size));

  unsigned int position = 0;
  for(unsigned int i = 0; i < size; ++i) {
    for(unsigned int j = 0; j < FIELD_M; ++j) {
      const unsigned char bit = elt_bit(ffi_vec_coeff_const(v, i), j) ? 1 : 0;
      str[position / 8] &= (unsigned char) ~(1U << (position % 8));
      str[position / 8] |= (unsigned char) (bit << (position % 8));
      ++position;
    }
  }
}

void ffi_vec_from_string_compact(ffi_vec* o, unsigned int size, const unsigned char* str) {
  unsigned int position = 0;
  ffi_vec_set_length(o, size);

  for(unsigned int i = 0; i < size; ++i) {
    ffi_elt_set_zero(&o->data[i]);
    for(unsigned int j = 0; j < FIELD_M; ++j) {
      set_elt_bit(&o->data[i], j, get_bit(str[position / 8], position % 8));
      ++position;
    }
  }
}

static unsigned int compact_size(unsigned int size) {
  return (size * FIELD_M + 7) / 8;
}

static unsigned int elt_bit(const ffi_elt* e, unsigned int position) {
  return rbc_elt_get_coefficient(e->value, position);
}

static void set_elt_bit(ffi_elt* e, unsigned int position, unsigned int bit) {
  rbc_elt_set_coefficient(e->value, position, bit & 1);
}

static unsigned int vec_degree(const ffi_vec* v) {
  if(v->size == 0) {
    return 0;
  }

  for(int i = (int) v->size - 1; i >= 0; --i) {
    if(!ffi_elt_is_zero(&v->data[i])) {
      return (unsigned int) i;
    }
  }
  return 0;
}
