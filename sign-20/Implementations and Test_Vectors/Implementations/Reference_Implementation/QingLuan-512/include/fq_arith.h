/*
 * QingLuan Digital Signature Scheme
 * fq_arith.h - Arithmetic operations over F_q
 */

#ifndef QINGLUAN_FQ_ARITH_H
#define QINGLUAN_FQ_ARITH_H

#include "params.h"
#include <stddef.h>

/* Basic F_q arithmetic (constant-time) */
fq_t fq_add(fq_t a, fq_t b);
fq_t fq_sub(fq_t a, fq_t b);
fq_t fq_mul(fq_t a, fq_t b);
fq_t fq_neg(fq_t a);

/* Vector operations over F_q */
void fq_vec_add(fq_t *out, const fq_t *a, const fq_t *b, size_t len);
void fq_vec_sub(fq_t *out, const fq_t *a, const fq_t *b, size_t len);
fq_t fq_vec_inner(const fq_t *a, const fq_t *b, size_t len);
void fq_mat_vec_mul(fq_t *out, const fq_t *M, const fq_t *v,
                    size_t rows, size_t cols);
void fq_vec_zero(fq_t *v, size_t len);
void fq_vec_copy(fq_t *dst, const fq_t *src, size_t len);

#endif /* QINGLUAN_FQ_ARITH_H */
