#ifndef MULTIPLY_E_H
#define MULTIPLY_E_H

#include <stdint.h>
#include <stdlib.h>
#include "Fql.h"


void init_mul_table_mod31();
void multiply_E_add_m1_m2_avx2(Fq *u, Fq * u_new, int ell);

void multiply_E_mat_m2_avx2(Fq *M, Fq *M_new, int ctr, int m_col);
void multiply_E_add_mat_m2_avx2(Fq *M, Fq *M_new, int ctr, int m_col);

#endif   // MULTIPLY_E_H