#ifndef GF_OPS_H
#define GF_OPS_H

#include "vdoo_config.h"
#include "gf16.h"
#include "blas_comm.h"

#if VDOO_Q == 16
#define gfv_get_ele gf16v_get_ele
#define gfv_set_ele gf16v_set_ele
#define gfv_mul gf16_mul
#define gfv_inv gf16_inv
#elif VDOO_Q == 256
#define gfv_get_ele gf256v_get_ele
#define gfv_set_ele gf256v_set_ele
#define gfv_mul gf256_mul
#define gfv_inv gf256_inv
#endif

#endif