#ifndef RBC_VEC_H
#define RBC_VEC_H

#include "rbc_elt.h"

unsigned long long rbc_vec_get_encoded_size(unsigned long long size);

void rbc_vec_set_zero(rbc_elt *v, unsigned int size);
void rbc_vec_set(rbc_elt *o, const rbc_elt *v, unsigned int size);
void rbc_vec_add(rbc_elt *o, const rbc_elt *a, const rbc_elt *b,
                 unsigned int size);
int rbc_vec_equal(const rbc_elt *a, const rbc_elt *b, unsigned int size);
int rbc_vec_get_coefficient(rbc_elt *o, const rbc_elt *v, unsigned int position,
                            unsigned int size);
int rbc_vec_set_coefficient(rbc_elt *v, const rbc_elt *e,
                            unsigned int position, unsigned int size);

int rbc_vec_get_rank(unsigned int *rank, const rbc_elt *v, unsigned int size);
int rbc_vec_is_full_rank(const rbc_elt *v, unsigned int size,
                         unsigned int expected_rank);

int rbc_vec_encode(unsigned char *out, unsigned long long out_len_bytes,
                   const rbc_elt *v, unsigned int size);
int rbc_vec_decode(rbc_elt *v, unsigned int size, const unsigned char *in,
                   unsigned long long in_len_bytes);

#endif
