#ifndef RBC_ELT_H
#define RBC_ELT_H

#include "rbc.h"

typedef struct {
	uint64_t v[RBC_ELT_WORDS];
} rbc_elt;

void rbc_elt_set_zero(rbc_elt *o);
void rbc_elt_set_one(rbc_elt *o);
void rbc_elt_set(rbc_elt *o, const rbc_elt *e);
void rbc_elt_normalize(rbc_elt *o);

int rbc_elt_is_zero(const rbc_elt *e);
int rbc_elt_equal(const rbc_elt *a, const rbc_elt *b);
int rbc_elt_get_degree(const rbc_elt *e);
int rbc_elt_get_coefficient(const rbc_elt *e, unsigned int position);
int rbc_elt_set_coefficient(rbc_elt *e, unsigned int position, unsigned int bit);

void rbc_elt_add(rbc_elt *o, const rbc_elt *a, const rbc_elt *b);
void rbc_elt_mul(rbc_elt *o, const rbc_elt *a, const rbc_elt *b);
void rbc_elt_sqr(rbc_elt *o, const rbc_elt *a);
void rbc_elt_qpow_2exp(rbc_elt *o, const rbc_elt *a, unsigned int exponent);
void rbc_elt_nth_root(rbc_elt *o, const rbc_elt *a, unsigned int n);
int rbc_elt_inv(rbc_elt *o, const rbc_elt *a);

#endif
