/// @file blas.h
/// @brief Defining the implementations for linear algebra functions depending on the machine architecture.
///

#ifndef _BLAS_H_
#define _BLAS_H_


#include "config.h"

//
//  These are light-weighted inlined functions.
//  We use macros to link them to corresponding inlined functions.
//

#define _BLAS_UNIT_LEN_ 32

#include "blas_avx2.h"

#define gf16v_mul_scalar    gf16v_mul_scalar_avx2
#define gf16v_madd          gf16v_madd_avx2

#define gf256v_add          gf256v_add_avx2
#define gf256v_mul_scalar   gf256v_mul_scalar_avx2
#define gf256v_madd         gf256v_madd_avx2

#define gf16v_madd_multab   gf16v_madd_multab_avx2
#define gf256v_madd_multab  gf256v_madd_multab_avx2
#define gf16v_generate_multabs  gf16v_generate_multabs_avx2
#define gf256v_generate_multabs  gf256v_generate_multabs_avx2

#include "blas_u32.h"
#define gf256v_conditional_add      _gf256v_conditional_add_u32

#include "blas_comm.h"
#include "blas_matrix.h"
#include "blas_matrix_sse.h"
#include "blas_matrix_avx2.h"






#endif // _BLAS_H_

