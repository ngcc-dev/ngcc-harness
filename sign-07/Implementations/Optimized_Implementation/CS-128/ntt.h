#ifndef NTT_H
#define NTT_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "params.h"

/*
 * Elements of R_q = Z_q[X]/(X^n + 1). Represents polynomial
 * coeffs[0] + X*coeffs[1] + X^2*xoeffs[2] + ... + X^{n-1}*coeffs[n-1]
 */
typedef struct {
	int32_t coeffs[n];
} poly;

poly NTT(poly* r);
void INTT(poly* r);

void AddNTT(poly* r, const poly* a, const poly* b);
void AddPoly(poly* r, const poly* a, const poly* b);
void SubPoly(poly* r, const poly* a, const poly* b);
void ShiftLeftVector(poly* r, int value);
void HalfCenterModVector(poly* r);
void AddorSubPoly(poly* r, const poly* a, const poly* b, int32_t cp);
void MultiplyNTT(poly* r, const poly* a, const poly* b);
void MultiplyPoly(poly* r, const poly* a, const int32_t* coordinates, int32_t num, const poly* b);
void TruncPoly(poly* r, const poly* a, const int32_t* coordinates, int32_t num);

void MatrixVectorNTT(poly* w,
                     const poly M[k][l],
                     const poly* v);

void AddVector(poly* u, const poly* v, const poly* w);
void SubVector(poly* u, const poly* v, const poly* w);
void ScalarVector(poly* w, const int scalar);
void ScalarVectorNTT(poly* w, const poly c, const poly* v);
void ScalarMulVector(poly* w, const poly c, const int32_t* coordinates, const poly* v);

void LazyReductionVector(poly* r);

void ExchangeModulus(poly* b, const poly y);

#endif
