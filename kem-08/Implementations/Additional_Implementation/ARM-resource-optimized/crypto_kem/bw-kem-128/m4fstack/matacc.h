#ifndef MATACC_H
#define MATACC_H

#include <stdint.h>
#include "params.h"
#include "poly.h"
#include "polyvec.h"

/*************************************************
* Name:        matacc
*
* Description: On-the-fly inner product of one row of the public matrix A
*              (or A^T) with a fixed vector b, accumulating into r.
*
*              r = sum_{j=0}^{K-1} A[i][j] * b[j]   (NTT domain)
*
*              Each A[i][j] is regenerated from (seed,i,j) via a SINGLE XOF
*              squeeze + rejection sampling and consumed immediately, so the
*              full K*K matrix never lives on the stack. The basemul reads the
*              zeta twiddles inline (noprime variant), so no per-vector Plant
*              cache is materialized either — only the fixed operand b stays.
*
*              IMPORTANT: unlike the SHAKE-based ML-KEM m4fstack, the squeeze
*              is NOT interleaved with the basemul. BW-KEM's XOF is the SM3
*              pseudoXOF, which recomputes the whole prefix on every squeeze;
*              interleaving would make generation O(n^2). One squeeze per
*              polynomial keeps the hash cost identical to the speed variant.
*
* Arguments:   - poly *r:               output accumulator (NTT domain, tight
*                                       Plant range, ready for invntt)
*              - const polyvec *b:      fixed vector operand (NTT domain)
*              - unsigned int i:        row index of A / A^T
*              - const uint8_t *seed:   public seed rho
*              - int transposed:        0 -> A, 1 -> A^T
*
* Note: accumulates the inner product in 16-bit in place (each term is
* Plant-reduced then uadd16'd into r), so no int32 scratch buffer is needed.
**************************************************/
void matacc(poly *r, const polyvec *b,
            unsigned int i, const uint8_t seed[KYBER_SYMBYTES], int transposed);

#endif /* MATACC_H */
