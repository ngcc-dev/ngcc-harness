#include "ntt.h"
#include "params.h"
#include "reduce.h"
#include <stdint.h>

const uint16_t omegas_inv_bitrev_montgomery[KEM_N / 2]
		= OMEGAS_INV_BITREV_MONTGOMERY;
const uint16_t psis_inv_montgomery[KEM_N] = PSIS_INV_MONTGOMERY;
const uint16_t zetas[KEM_N] = ZETAS;

/*************************************************
 * Name:        ntt
 *
 * Description: Computes negacyclic number-theoretic transform (NTT) of
 *              a polynomial (vector of 64 coefficients) in place;
 *              inputs assumed to be in normal order, output in bitreversed
 * order
 *
 * Arguments:   - uint16_t *p: pointer to in/output polynomial
 **************************************************/
void
ntt (uint16_t p[KEM_N])
{
	int level, start, j, k;
	uint16_t zeta, t;

	k = 1;
	for (level = 6; level >= 0; level--)
		{
			for (start = 0; start < KEM_N; start = start + 2 * (1 << level))
				{
					zeta = zetas[k++];
					for (j = start; j < start + (1 << level); ++j)
						{
							t = montgomery_reduce ((uint32_t)zeta * p[j + (1 << level)]);

							// Explanations:
							// - (KEM_Q << 1): 2 * KEM_Q to avoid underflow, 0 < t < 2 *
							// KEM_Q
							p[j + (1 << level)] = barrett_reduce (p[j] + (KEM_Q << 1) - t);

							p[j] = barrett_reduce (p[j] + t);
						}
				}
		}

	for (j = 0; j < KEM_N; j++)
		{
			p[j] = freeze (p[j]);
		}
}

/*************************************************
 * Name:        invntt
 *
 * Description: Computes inverse of negacyclic number-theoretic transform (NTT)
 * of a polynomial (vector of 64 coefficients) in place; inputs assumed to be
 * in bitreversed order, output in normal order
 *
 * Arguments:   - uint16_t *a: pointer to in/output polynomial
 **************************************************/
void
invntt (uint16_t a[KEM_N])
{
	int start, j, jTwiddle, level;
	uint16_t temp, W;
	uint32_t t;

	for (level = 0; level < 7; level++)
		{
			for (start = 0; start < (1 << level); start++)
				{
					jTwiddle = 0;
					for (j = start; j < KEM_N - 1; j += 2 * (1 << level))
						{
							W = omegas_inv_bitrev_montgomery[jTwiddle++];
							temp = a[j];

							a[j] = barrett_reduce ((temp + a[j + (1 << level)]));
							// Explanations:
							// - (KEM_Q << 1): 2 * KEM_Q to avoid underflow, 0 < a[...] < 2 *
							// KEM_Q
							t = (W * ((uint32_t)temp + (KEM_Q << 1) - a[j + (1 << level)]));
							a[j + (1 << level)] = montgomery_reduce (t);
						}
				}
		}

	for (j = 0; j < KEM_N; j++)
		{
			a[j] = montgomery_reduce ((a[j] * psis_inv_montgomery[j]));
			a[j] = freeze (a[j]);
		}
}
