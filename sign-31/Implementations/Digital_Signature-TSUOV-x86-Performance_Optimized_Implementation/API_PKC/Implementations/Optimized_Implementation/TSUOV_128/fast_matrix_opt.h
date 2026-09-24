#ifndef FAST_MATRIX_OPT_H
#define FAST_MATRIX_OPT_H

#include <stdint.h>

void MATRIX_mul_MATRIX(const uint8_t *A, const uint8_t *B, uint8_t *C, int A_rows, int A_cols, int B_cols, int C_rows);
void VECTOR_mul_MATRIX(const uint8_t *V, const uint8_t *M, uint8_t *C, int V_cols, int M_cols, int C_cols);
void vector_add(uint8_t *C, const uint8_t *D, int total_len);
uint8_t vector_dot(const uint8_t *a, const uint8_t *b, size_t length);

#endif
