#ifndef RBC_VSPACE_H
#define RBC_VSPACE_H

#include "rbc_vec.h"

#ifdef __cplusplus
extern "C" {
#endif

void rbc_vspace_init(rbc_vspace* vs, uint32_t size);
void rbc_vspace_clear(rbc_vspace vs);
void rbc_vspace_set_zero(rbc_vspace vs, uint32_t size);
void rbc_vspace_set(rbc_vspace o, const rbc_vspace vs, uint32_t size);
void rbc_vspace_set_random_full_rank(rbc_vspace vs, uint32_t size, DRNG_ctx* ctx);
void rbc_vspace_set_random_full_rank2(rbc_vspace vs, uint32_t size);
uint32_t rbc_vspace_get_rank(const rbc_vspace vs, uint32_t size);
uint8_t rbc_vspace_is_equal_to(const rbc_vspace vs1, const rbc_vspace vs2, uint32_t size);
void rbc_vspace_print(const rbc_vspace vs, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif
