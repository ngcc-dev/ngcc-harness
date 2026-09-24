#ifndef ROUNDING_H
#define ROUNDING_H

#include "conversion.h"

int CheckNormPoly(const poly* a, int bnd);

void CheckParityPoly(poly* r, const poly a);

int CheckNormVector(const poly* a, int dim, int bnd);

void Power2RoundVector(poly* hi, poly* low, const poly* a);

void HighBitsVector(poly* hi, const poly* a);

void HighBitsZ2Vector(poly* r, const poly* w, const poly* z2);

void hModpVector(poly* r, const poly* w1, const poly* w0, int sgn);

void ComposeVector(poly* a, const poly* low, const int32_t* hi, int dim, int dc);

void DecomposePoly(int32_t* hi, poly* low, const poly a, int dc);

void DecomposeVector(int32_t* hi, poly* low, const poly* a, int dim, int dc);

#endif
