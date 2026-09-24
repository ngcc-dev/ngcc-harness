#ifndef POLYVEC_H
#define POLYVEC_H

#include <stdint.h>
#include "params.h"
#include "poly.h"

typedef struct {
    poly vec[LORE_K];
} polyvec;

#define polyvec_ntt LORE_NAMESPACE(polyvec_ntt)
void polyvec_ntt(polyvec *v);
#define polyvec_invntt_tomont LORE_NAMESPACE(polyvec_invntt_tomont)
void polyvec_invntt_tomont(polyvec *v);
#define polyvec_pointwise_acc_montgomery LORE_NAMESPACE(polyvec_pointwise_acc_montgomery)
void polyvec_pointwise_acc_montgomery(poly *r, const polyvec *a, const polyvec *b);
#define polyvec_add LORE_NAMESPACE(polyvec_add)
void polyvec_add(polyvec *r, const polyvec *a, const polyvec *b);

// CRT vector functions
#define poly_crt_vec_add LORE_NAMESPACE(poly_crt_vec_add)
void poly_crt_vec_add(poly_crt_vec *r, const poly_crt_vec *a, const poly_crt_vec *b);
#define poly_crt_vec_ntt LORE_NAMESPACE(poly_crt_vec_ntt)
void poly_crt_vec_ntt(poly_crt_vec *v);
#define poly_crt_vec_invntt_tomont LORE_NAMESPACE(poly_crt_vec_invntt_tomont)
void poly_crt_vec_invntt_tomont(poly_crt_vec *v);

#define poly_crt_vec_pointwise_acc_montgomery LORE_NAMESPACE(poly_crt_vec_pointwise_acc_montgomery)
void poly_crt_vec_pointwise_acc_montgomery(poly_crt *r, const poly_crt_vec *a_row_dense, const poly_crt_vec *b_dense);
#define poly_crt_vec_pointwise_acc_montgomery_sparse LORE_NAMESPACE(poly_crt_vec_pointwise_acc_montgomery_sparse)
void poly_crt_vec_pointwise_acc_montgomery_sparse(poly_crt *r, const poly_crt_vec *a_row_dense, const poly_crt_vec *b_crt_with_ntt_q, const poly_sparse *b_t_poly_sparse_vec);

#define gen_matrix_std LORE_NAMESPACE(gen_matrix_std)
void gen_matrix_std(poly_crt_vec a[LORE_K], const unsigned char *seed);
#define gen_matrix_ntt LORE_NAMESPACE(gen_matrix_ntt)
void gen_matrix_ntt(poly_crt_vec a[LORE_K], const unsigned char *seed, int transposed);

#endif