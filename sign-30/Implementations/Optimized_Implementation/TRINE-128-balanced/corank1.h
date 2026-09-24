#ifndef CORANK1_H
#define CORANK1_H

#include "hashkdf.h"
#include "params.h"

/*
 * Repeatedly sample u uniformly from F_q^n until
 *
 *   rank(Phi_U(u)) == n - 1.
 *
 * Return:
 *    0  success
 *   -1  invalid input
 *
 * Preconditions:
 *   1 <= n && n <= TRINE_n
 *   M contains n consecutive row-major n x n slices
 *   out_u provides n writable field elements
 *   xof is initialized and finalized for squeezing
 *   out_u does not overlap M
 *
 * The function consumes and advances xof.
 * The function is variable-time.
 *
 * On invalid input, out_u is left unchanged.
 *
 * For valid input, this implements Algorithm 1 directly and has no
 * hidden attempt bound. If M has no corank-one point, it does not
 * terminate.
 */
int corank1_cal_vartime(
    Fq *out_u,
    const Fq *M,
    trine_xof_state *xof,
    int n);

#endif
