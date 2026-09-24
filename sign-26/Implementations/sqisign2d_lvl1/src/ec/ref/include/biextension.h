#ifndef _BIEXT_H_
#define _BIEXT_H_

#include <fp2.h>
#include "ec.h"

void A24_from_AC(ec_point_t *A24, ec_point_t const *AC);

void weil(fp2_t *r, int e, ec_point_t *P, ec_point_t *Q, ec_point_t *PQ, ec_point_t *A24);

void ec_dlog_2_weil(digit_t *scalarP1,
                    digit_t *scalarQ1,
                    digit_t *scalarP2,
                    digit_t *scalarQ2,
                    ec_basis_t *PQ,
                    ec_basis_t *basis,
                    ec_curve_t *curve,
                    int e);
#endif
