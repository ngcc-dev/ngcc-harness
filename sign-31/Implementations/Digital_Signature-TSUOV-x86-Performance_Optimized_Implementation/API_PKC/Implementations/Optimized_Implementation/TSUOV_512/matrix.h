// matrix.h
#pragma once

#include "tsuov_params.h"
#include "Fql.h"
#include <string.h>

#define SHIFT 5
#define BYTES2BLOCKS(BYTES)          (((BYTES)>>(SHIFT))+(((BYTES)&((1<<SHIFT)-1))?1:0))
#define BLOCK_ALIGNED_BYTES(BYTES)   (BYTES2BLOCKS(BYTES)<<(SHIFT))

typedef Fq         vector_V        [BLOCK_ALIGNED_BYTES(TSUOV_V)] ALIGN_256BIT ;
typedef Fq         vector_O        [BLOCK_ALIGNED_BYTES(TSUOV_O)] ALIGN_256BIT ;
typedef Fq         vector_m1       [BLOCK_ALIGNED_BYTES(TSUOV_m1)] ALIGN_256BIT;
typedef Fq         vector_N        [BLOCK_ALIGNED_BYTES(TSUOV_N)] ALIGN_256BIT;

typedef vector_V   VECTOR_V        [TSUOV_L]                             ;
typedef vector_O   VECTOR_O        [TSUOV_L]                             ;
typedef vector_N   VECTOR_N        [TSUOV_L]                             ;

typedef VECTOR_V   MATRIX_VxV      [TSUOV_V]                             ;
typedef VECTOR_V   MATRIX_OxV      [TSUOV_O]                             ;
typedef VECTOR_N   MATRIX_NxN      [TSUOV_N]                             ;
typedef VECTOR_N   MATRIX_OxN      [TSUOV_O]          ;

typedef Fq         TSUOV_P3        [TSUOV_m1][TSUOV_O][TSUOV_L][TSUOV_O] ;

typedef VECTOR_O  MATRIX_kxO       [TSUOV_k];
typedef VECTOR_N  MATRIX_kxN       [TSUOV_k];
typedef VECTOR_V   MATRIX_kxV      [TSUOV_k]                             ;

extern uint64_t vector_v_dot_vector_v(const VECTOR_V A, const VECTOR_V B);
extern void vector_mul_scalar_avx2_fp31(const uint8_t *vec, uint8_t scalar, size_t len, uint8_t *res);
extern void MATRIX_VxO_dot_VECTOR_O_whipk(const MATRIX_OxV A, const MATRIX_kxO a, MATRIX_kxV res);
extern void VECTOR_V_sub_VECTOR_V_whipk_save_in_sign(const MATRIX_kxV a, const MATRIX_kxV b, MATRIX_kxN sign);
extern void MATRIX_TRANSPOSE_m2xLO(const Fq A[TSUOV_m1][TSUOV_k][TSUOV_o], Fq  C[TSUOV_k*TSUOV_o][TSUOV_m2]);

static inline void VECTOR_V_CLEAR_TAIL(VECTOR_V C){
  for(int k=0;k<TSUOV_L;k++) memset(C[k]+TSUOV_V, 0, BLOCK_ALIGNED_BYTES(TSUOV_V)-TSUOV_V) ;
}
