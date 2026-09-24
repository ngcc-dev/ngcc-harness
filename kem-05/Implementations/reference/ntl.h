/******************************************************************************
 * BIKE_MLThre
 ******************************************************************************/

#ifndef _NTL_H
#define _NTL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void ntl_add(OUT uint8_t res_bin[R_SIZE],
        IN const uint8_t a_bin[R_SIZE],
        IN const uint8_t b_bin[R_SIZE]);

void ntl_mod_inv(OUT uint8_t res_bin[R_SIZE],
        IN const uint8_t a_bin[R_SIZE]);

void ntl_mod_mul(OUT uint8_t res_bin[R_SIZE],
        IN const uint8_t a_bin[R_SIZE],
        IN const uint8_t b_bin[R_SIZE]);

void ntl_split_polynomial(OUT uint8_t e0[R_SIZE],
        OUT uint8_t e1[R_SIZE],
        IN const uint8_t e[N_SIZE]);

#ifdef __cplusplus
}
#endif

#endif
