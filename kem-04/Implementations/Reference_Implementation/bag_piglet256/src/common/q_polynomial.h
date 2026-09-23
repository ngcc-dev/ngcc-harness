#ifndef Q_POLYNOMIAL_H
#define Q_POLYNOMIAL_H

#include "ffi_elt.h"
#include "ffi_vec.h"

typedef struct q_polynomial {
  ffi_vec values;
  int max_degree;
  int degree;
} q_polynomial;

int q_polynomial_init(q_polynomial *p, unsigned int max_degree);
void q_polynomial_clear(q_polynomial *p);
void q_polynomial_update_degree(q_polynomial *p, unsigned int position);
int q_polynomial_set(q_polynomial *o, const q_polynomial *p);
int q_polynomial_set_zero(q_polynomial *o);
int q_polynomial_set_one(q_polynomial *o);
int q_polynomial_set_interpolate_vect_and_zero(q_polynomial *o1,
                                                q_polynomial *o2,
                                                const ffi_vec *v1,
                                                const ffi_vec *v2,
                                                int size);
int q_polynomial_set_interpolate(q_polynomial *o, const ffi_vec *v,
                                 int size);
int q_polynomial_is_zero(const q_polynomial *p);
void q_polynomial_evaluate(ffi_elt *o, const q_polynomial *p,
                           const ffi_elt *e);
int q_polynomial_scalar_mul(q_polynomial *o, const q_polynomial *p,
                            const ffi_elt *e);
int q_polynomial_qexp(q_polynomial *o, const q_polynomial *p);
int q_polynomial_add(q_polynomial *o, const q_polynomial *p,
                     const q_polynomial *q);
int q_polynomial_mul(q_polynomial *o, const q_polynomial *p,
                     const q_polynomial *q);
int q_polynomial_left_div(q_polynomial *q, q_polynomial *r,
                          const q_polynomial *a,
                          const q_polynomial *b);
void q_polynomial_print(const q_polynomial *p);

#endif
