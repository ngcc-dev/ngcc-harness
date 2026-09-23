#ifndef RBC_POLY_H
#define RBC_POLY_H

#include "rbc_vec.h"

#ifdef __cplusplus
extern "C" {
#endif

void rbc_poly_init(rbc_poly* p, int32_t max_degree);
void rbc_poly_clear(rbc_poly p);
void rbc_poly_sparse_init(rbc_poly_sparse* p, uint32_t coeffs_nb, const uint32_t* coeffs);
void rbc_poly_sparse_clear(rbc_poly_sparse p);
void rbc_poly_update_degree(rbc_poly p, int32_t position);
void rbc_poly_set_zero(rbc_poly p);
void rbc_poly_set(rbc_poly o, const rbc_poly p);
void rbc_poly_set_coefficient(rbc_poly p, uint32_t position, const rbc_elt e);
void rbc_poly_get_coefficient(rbc_elt o, const rbc_poly p, uint32_t position);
uint8_t rbc_poly_is_equal_to(const rbc_poly p1, const rbc_poly p2);
void rbc_poly_add(rbc_poly o, const rbc_poly p1, const rbc_poly p2);
void rbc_poly_scalar_mul(rbc_poly o, const rbc_poly p, const rbc_elt e);
void rbc_poly_mul(rbc_poly o, const rbc_poly p1, const rbc_poly p2);
void rbc_poly_mulmod_sparse(rbc_poly o, const rbc_poly p1, const rbc_poly p2, const rbc_poly_sparse modulus);
void rbc_poly_print(const rbc_poly p);

#ifdef __cplusplus
}
#endif

#endif
