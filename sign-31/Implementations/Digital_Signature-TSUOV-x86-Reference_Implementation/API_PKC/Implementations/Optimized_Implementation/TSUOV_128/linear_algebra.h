#pragma once

#include "Fql.h"
#include "tsuov.h"
#include "tsuov_params.h"

TYPEDEF_STRUCT(ROW,
  Fq *col;
  int original_row_id;
);

TYPEDEF_STRUCT(ECHELON_FORM,
  ROW_s row[TSUOV_m2];
  ROW_s *eqn[TSUOV_m2];
  int rank;
  int index[TSUOV_m2];
);

void tsuov_expand_sol(const TSUOV_SEED seed_sol, uint8_t dst[TSUOV_m2]);

void LU_decompose(Fq A[TSUOV_m2][TSUOV_m2], ECHELON_FORM echelon_form);

int consistent(ECHELON_FORM echelon_form, Fq b[TSUOV_m2], int *cacheR,
               Fq R[TSUOV_m2][TSUOV_m2]);

void sample_a_solution(const TSUOV_SEED seed_sol, ECHELON_FORM echelon_form,
                       Fq b[TSUOV_m2], Fq x[TSUOV_m2], Fq b2[TSUOV_m2]);
