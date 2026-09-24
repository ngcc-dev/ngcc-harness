#ifndef RHYME_KG_SOLVER_H
#define RHYME_KG_SOLVER_H
#include <stdint.h>
/* cols layout: cols[(j*d + i)*n + t] = coefficient t of entry (row i, sampled column j).
 * On success writes F[(i)*n + t] (int32, small) = last-column entries with det = +1.
 * Returns 0 ok, -1 gcd != 1 (resample), -2 reduction stalled (resample), -3 internal. */
int kg_solve_last_column(int32_t *F, const int32_t *cols, unsigned n, unsigned d);
/* When nonzero, kg_solve_last_column returns right after the coprimality test
 * (0 = coprime/solvable, -1 = not), skipping F assembly and Babai reduction.
 * Default 0 (full solve).  Used by cut-F keygen, where F is never stored. */
extern int kg_solve_check_only;
/* check det(B) == 1 over a few CRT primes (probabilistic exact). 0 = ok */
int kg_det_check(const int32_t *cols, const int32_t *F, unsigned n, unsigned d);

/* Coprimality decision via field-norm descent (Ewha Algorithm 2, descent half).
 * Decides solvability (exists last row with det=1) WITHOUT assembling F.
 * Same integer decision as kg_solve_last_column+check_only, but obtains each
 * minor's resultant by halving the polynomial degree via the field norm, so
 * the per-prime cost drops and only one scalar (not n NTT-point values) is
 * CRT-rebuilt per minor.  Returns 0 = coprime/solvable, -1 = not, -3 internal. */
int kg_solve_coprime_descent(const int32_t *cols, unsigned n, unsigned d);
#endif
int kg_check_dp1_coprime(const int32_t *block, unsigned n, unsigned D, unsigned d);
