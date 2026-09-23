/**
 * \file ffi_field.c
 * \brief Implementation of ffi_field.h using the RBC interface
 */

#include "ffi.h"
#include "ffi_elt.h"
#include "ffi_field.h"
#include "ffi_vec.h"
#include "rbc_qre.h"

int init = 0;
ffi_vec ideal_modulo;
unsigned int ideal_modulo_degrees[MODULO_NUMBER] = MODULO_VALUES;
unsigned int ideal_modulo_degree = 0;

void ffi_field_init(void) {
  if(!init) {
    init = 1;
    rbc_field_init();

    const uint32_t values[MODULO_NUMBER] = MODULO_VALUES;
    rbc_qre_init_modulus(MODULO_NUMBER, values);
    ffi_vec_set_length(&ideal_modulo, values[MODULO_NUMBER - 1] + 1);
    for(int i = 0; i < MODULO_NUMBER; ++i) {
      ffi_elt one = ffi_elt_get_one();
      ffi_vec_set_coeff(&ideal_modulo, &one, values[i]);
      if(values[i] > ideal_modulo_degree) {
        ideal_modulo_degree = values[i];
      }
    }
  }
}

int ffi_field_get_degree(void) {
  return FIELD_M;
}
