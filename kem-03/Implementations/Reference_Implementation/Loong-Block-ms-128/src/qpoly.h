#ifndef LOONG_QPOLY_H
#define LOONG_QPOLY_H

#include "rbc_elt.h"

typedef struct {
	rbc_elt *coeffs;
	unsigned int max_degree;
	int degree;
} qpoly;

int qpoly_init(qpoly *p, unsigned int max_degree);
void qpoly_clear(qpoly *p);
int qpoly_set_zero(qpoly *p);
int qpoly_set_one(qpoly *p);
int qpoly_set(qpoly *out, const qpoly *in);
int qpoly_is_zero(const qpoly *p);
int qpoly_set_coefficient(qpoly *p, unsigned int position, const rbc_elt *e);
int qpoly_get_coefficient(rbc_elt *e, const qpoly *p, unsigned int position);
void qpoly_update_degree(qpoly *p, unsigned int position);

int qpoly_evaluate(rbc_elt *out, const qpoly *p, const rbc_elt *x);
int qpoly_scalar_mul(qpoly *out, const qpoly *p, const rbc_elt *e);
int qpoly_qexp(qpoly *out, const qpoly *p);
int qpoly_add(qpoly *out, const qpoly *a, const qpoly *b);
int qpoly_mul(qpoly *out, const qpoly *a, const qpoly *b);
int qpoly_left_div(qpoly *quotient, qpoly *remainder, const qpoly *a,
                   const qpoly *b);

int qpoly_set_interpolate_zero(qpoly *annihilator, const rbc_elt *points,
                               unsigned int size);
int qpoly_set_interpolate_vect_and_zero(qpoly *annihilator, qpoly *interpolant,
                                        const rbc_elt *points,
                                        const rbc_elt *values,
                                        unsigned int size);

#endif
