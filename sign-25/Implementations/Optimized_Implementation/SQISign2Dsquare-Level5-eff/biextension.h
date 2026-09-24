#ifndef _BIEXT_H_
#define _BIEXT_H_

#include <stdint.h>
#include <fp2.h>
#include "ec.h"

void A24_from_AC(ec_point_t *A24, ec_point_t const *AC);

bool fp2_dlog_3e(digit_t* scal, const fp2_t* f, const fp2_t* g, int e);

void weil(fp2_t *r, uint64_t e, ec_point_t *P, ec_point_t *Q, ec_point_t *PQ, ec_point_t *A24);

void ec_dlog_2_weil(digit_t *scalarP1,
                    digit_t *scalarQ1,
                    digit_t *scalarP2,
                    digit_t *scalarQ2,
                    ec_basis_t *PQ,
                    ec_basis_t *basis,
                    ec_curve_t *curve,
                    int e);
void
reduced_tate_3(fp2_t* r,
        ec_point_t* P,
        ec_point_t* Q,
        ec_point_t* PQ,
        ec_point_t* A24);
void ec_dlog_3_tate(digit_t *scalarP,
		digit_t *scalarQ,
		ec_point_t *R,
		ec_basis_t *PQ,
		ec_curve_t *curve);
void ec_basis_dlog_3_tate(digit_t* scalarP1,
                    digit_t* scalarQ1,
                    digit_t* scalarP2,
                    digit_t* scalarQ2,
                    ec_basis_t* PQ,
                    ec_basis_t* basis,
                    ec_curve_t* curve);
#endif
