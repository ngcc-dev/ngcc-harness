#ifndef RBC_H
#define RBC_H

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RBC_FIELD_Q 2
#define RBC_FIELD_M 47
#define RBC_ELT_UINT64 1
#define RBC_ELT_UR_UINT64 2
#define RBC_FIELD_BYTES ((RBC_FIELD_M + 7) / 8)

typedef uint64_t rbc_elt[RBC_ELT_UINT64];
typedef uint64_t rbc_elt_ur[RBC_ELT_UR_UINT64];

typedef rbc_elt* rbc_vec;
typedef rbc_vec rbc_vspace;


typedef struct {
  rbc_vec v;
  int32_t degree;
  int32_t max_degree;
} rbc_poly_struct;

typedef struct {
  uint32_t coeffs_nb;
  uint32_t* coeffs;
} rbc_poly_sparse_struct;

typedef rbc_poly_struct* rbc_poly;
typedef rbc_poly_sparse_struct* rbc_poly_sparse;
typedef rbc_poly rbc_qre;

static const rbc_elt RBC_ELT_MODULUS = {0x0000800000000021ULL};


#endif
