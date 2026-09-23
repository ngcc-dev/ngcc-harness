#include "rbc_qre.h"

static uint32_t rbc_qre_modulus_degree = 0;
static rbc_poly_sparse rbc_qre_modulus = NULL;

void rbc_qre_init_modulus(uint32_t coeffs_nb, const uint32_t* coeffs) {
  rbc_qre_clear_modulus();
  if(coeffs_nb == 0 || coeffs == NULL) {
    return;
  }

  rbc_qre_modulus_degree = 0;
  for(uint32_t i = 0; i < coeffs_nb; ++i) {
    if(coeffs[i] > rbc_qre_modulus_degree) {
      rbc_qre_modulus_degree = coeffs[i];
    }
  }
  rbc_poly_sparse_init(&rbc_qre_modulus, coeffs_nb, coeffs);
}

void rbc_qre_clear_modulus(void) {
  if(rbc_qre_modulus != NULL) {
    rbc_poly_sparse_clear(rbc_qre_modulus);
    rbc_qre_modulus = NULL;
  }
  rbc_qre_modulus_degree = 0;
}

uint32_t rbc_qre_get_modulus_degree(void) {
  return rbc_qre_modulus_degree;
}

void rbc_qre_init(rbc_qre* p) {
  if(rbc_qre_modulus_degree == 0) {
    *p = NULL;
    return;
  }
  rbc_poly_init(p, (int32_t) rbc_qre_modulus_degree - 1);
}

void rbc_qre_clear(rbc_qre p) {
  rbc_poly_clear(p);
}

void rbc_qre_set_zero(rbc_qre o) {
  rbc_poly_set_zero(o);
}

uint8_t rbc_qre_is_equal_to(const rbc_qre p1, const rbc_qre p2) {
  return rbc_poly_is_equal_to(p1, p2);
}

void rbc_qre_add(rbc_qre o, const rbc_qre p1, const rbc_qre p2) {
  rbc_poly_add(o, p1, p2);
}

void rbc_qre_mul(rbc_qre o, const rbc_qre p1, const rbc_qre p2) {
  rbc_poly_mulmod_sparse(o, p1, p2, rbc_qre_modulus);
}

void rbc_qre_print(const rbc_qre p) {
  rbc_poly_print(p);
}
