#include "rbc_vspace.h"

void rbc_vspace_init(rbc_vspace* vs, uint32_t size) {
  rbc_vec_init(vs, size);
}

void rbc_vspace_clear(rbc_vspace vs) {
  rbc_vec_clear(vs);
}

void rbc_vspace_set_zero(rbc_vspace vs, uint32_t size) {
  rbc_vec_set_zero(vs, size);
}

void rbc_vspace_set(rbc_vspace o, const rbc_vspace vs, uint32_t size) {
  rbc_vec_set(o, vs, size);
}

void rbc_vspace_set_random_full_rank(rbc_vspace vs, uint32_t size, DRNG_ctx* ctx) {
  rbc_vec_set_random_full_rank(vs, size, ctx);
}

void rbc_vspace_set_random_full_rank2(rbc_vspace vs, uint32_t size) {
  rbc_vec_set_random_full_rank2(vs, size);
}

uint32_t rbc_vspace_get_rank(const rbc_vspace vs, uint32_t size) {
  return rbc_vec_get_rank(vs, size);
}

uint8_t rbc_vspace_is_equal_to(const rbc_vspace vs1, const rbc_vspace vs2, uint32_t size) {
  return rbc_vec_is_equal_to(vs1, vs2, size);
}

void rbc_vspace_print(const rbc_vspace vs, uint32_t size) {
  rbc_vec_print(vs, size);
}
