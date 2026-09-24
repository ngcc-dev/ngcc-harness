#ifndef RBC_VEC_H
#define RBC_VEC_H

#include "rbc_elt.h"

#ifdef __cplusplus
extern "C" {
#endif

void rbc_vec_init(rbc_vec* v, uint32_t size);
void rbc_vec_clear(rbc_vec v);
void rbc_vec_set_zero(rbc_vec v, uint32_t size);
void rbc_vec_set(rbc_vec o, const rbc_vec v, uint32_t size);
void rbc_vec_set_random(rbc_vec o, uint32_t size, DRNG_ctx* ctx);
void rbc_vec_set_random2(rbc_vec o, uint32_t size);
void rbc_vec_set_random_full_rank(rbc_vec o, uint32_t size, DRNG_ctx* ctx);
void rbc_vec_set_random_full_rank2(rbc_vec o, uint32_t size);
uint32_t rbc_vec_get_rank(const rbc_vec v, uint32_t size);
uint8_t rbc_vec_is_equal_to(const rbc_vec v1, const rbc_vec v2, uint32_t size);
void rbc_vec_add(rbc_vec o, const rbc_vec v1, const rbc_vec v2, uint32_t size);
void rbc_vec_scalar_mul(rbc_vec o, const rbc_vec v, const rbc_elt e, uint32_t size);
void rbc_vec_from_string(rbc_vec o, uint32_t size, const uint8_t* str);
void rbc_vec_to_string(uint8_t* str, const rbc_vec v, uint32_t size);
void rbc_vec_print(const rbc_vec v, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif
