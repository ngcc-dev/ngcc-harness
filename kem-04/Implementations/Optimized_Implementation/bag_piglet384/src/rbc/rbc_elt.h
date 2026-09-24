#ifndef RBC_ELT_H
#define RBC_ELT_H

#include "rbc.h"
#include "drng.h"

#ifdef __cplusplus
extern "C" {
#endif

void rbc_field_init(void);
void rbc_elt_set_zero(rbc_elt o);
void rbc_elt_set_one(rbc_elt o);
void rbc_elt_set(rbc_elt o, const rbc_elt e);
void rbc_elt_set_random(rbc_elt o, DRNG_ctx* ctx);
void rbc_elt_set_random2(rbc_elt o);
uint8_t rbc_elt_is_zero(const rbc_elt e);
uint8_t rbc_elt_is_equal_to(const rbc_elt e1, const rbc_elt e2);
uint8_t rbc_elt_get_coefficient(const rbc_elt e, uint32_t index);
void rbc_elt_set_coefficient(rbc_elt o, uint32_t index, uint8_t bit);
void rbc_elt_add(rbc_elt o, const rbc_elt e1, const rbc_elt e2);
void rbc_elt_mul(rbc_elt o, const rbc_elt e1, const rbc_elt e2);
void rbc_elt_sqr(rbc_elt o, const rbc_elt e);
void rbc_elt_inv(rbc_elt o, const rbc_elt e);
void rbc_elt_reduce(rbc_elt o, const rbc_elt_ur e);
void rbc_elt_from_string(rbc_elt o, const uint8_t* str);
void rbc_elt_to_string(uint8_t* str, const rbc_elt e);
void rbc_elt_print(const rbc_elt e);

#ifdef __cplusplus
}
#endif

#endif
