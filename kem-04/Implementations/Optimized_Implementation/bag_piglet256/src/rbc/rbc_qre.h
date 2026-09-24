#ifndef RBC_QRE_H
#define RBC_QRE_H

#include "rbc_poly.h"

#ifdef __cplusplus
extern "C" {
#endif

void rbc_qre_init_modulus(uint32_t coeffs_nb, const uint32_t* coeffs);
void rbc_qre_clear_modulus(void);
uint32_t rbc_qre_get_modulus_degree(void);
void rbc_qre_init(rbc_qre* p);
void rbc_qre_clear(rbc_qre p);
void rbc_qre_set_zero(rbc_qre o);
uint8_t rbc_qre_is_equal_to(const rbc_qre p1, const rbc_qre p2);
void rbc_qre_add(rbc_qre o, const rbc_qre p1, const rbc_qre p2);
void rbc_qre_mul(rbc_qre o, const rbc_qre p1, const rbc_qre p2);
void rbc_qre_print(const rbc_qre p);

#ifdef __cplusplus
}
#endif

#endif
