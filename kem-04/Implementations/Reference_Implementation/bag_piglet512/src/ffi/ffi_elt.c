/**
 * \file ffi_elt.c
 * \brief C implementation of finite field elements using RBC
 */

#include "ffi_elt.h"
#include <stdio.h>

ffi_elt ffi_elt_get_zero(void) {
  ffi_elt e = {0};
  rbc_elt_set_zero(e.value);
  return e;
}

ffi_elt ffi_elt_get_one(void) {
  ffi_elt e = {0};
  rbc_elt_set_one(e.value);
  return e;
}

ffi_elt uchar_array_to_ffi_elt(const unsigned char* arr) {
  ffi_elt result = {0};
  ffi_elt_from_string(&result, arr);
  return result;
}

void ffi_elt_set(ffi_elt* o, const ffi_elt* e) {
  rbc_elt_set(o->value, e->value);
}

void ffi_elt_set_zero(ffi_elt* o) {
  rbc_elt_set_zero(o->value);
}

void ffi_elt_set_one(ffi_elt* o) {
  rbc_elt_set_one(o->value);
}

void ffi_elt_set_random(ffi_elt* o, DRNG_ctx* ctx) {
  rbc_elt_set_random(o->value, ctx);
}

void ffi_elt_set_random2(ffi_elt* o) {
  rbc_elt_set_random2(o->value);
}

int ffi_elt_is_zero(const ffi_elt* e) {
  return rbc_elt_is_zero(e->value);
}

void ffi_elt_inv(ffi_elt* o, const ffi_elt* e) {
  rbc_elt_inv(o->value, e->value);
}

void ffi_elt_sqr(ffi_elt* o, const ffi_elt* e) {
  rbc_elt_sqr(o->value, e->value);
}

void ffi_elt_nth_root(ffi_elt* o, const ffi_elt* e, int n) {
  int exp = n * (FIELD_M - 1) % FIELD_M;

  rbc_elt_sqr(o->value, e->value);
  for(int i = 0; i < exp - 1; ++i) {
    rbc_elt_sqr(o->value, o->value);
  }
}

void ffi_elt_add(ffi_elt* o, const ffi_elt* e1, const ffi_elt* e2) {
  rbc_elt_add(o->value, e1->value, e2->value);
}

void ffi_elt_mul(ffi_elt* o, const ffi_elt* e1, const ffi_elt* e2) {
  rbc_elt_mul(o->value, e1->value, e2->value);
}

void ffi_elt_from_string(ffi_elt* o, const unsigned char* str) {
  rbc_elt_from_string(o->value, str);
}

void ffi_elt_to_string(unsigned char* str, const ffi_elt* e) {
  rbc_elt_to_string(str, e->value);
}

void ffi_elt_print(const ffi_elt* e) {
  rbc_elt_print(e->value);
  printf(" ");
}
