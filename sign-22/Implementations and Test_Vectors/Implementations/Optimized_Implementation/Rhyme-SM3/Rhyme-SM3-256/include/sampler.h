#ifndef RHYME_SAMPLER_H
#define RHYME_SAMPLER_H
#include <stdint.h>
#include "params.h"
#include "poly.h"
#include "symmetric.h"

#define SampleCBD RHYME_NAMESPACE(SampleCBD)
#define SampleY1 RHYME_NAMESPACE(SampleY1)
#define SampleGauss RHYME_NAMESPACE(SampleGauss)
#define SampleChallenge RHYME_NAMESPACE(SampleChallenge)
#define Sample_Z RHYME_NAMESPACE(Sample_Z)

/* centered binomial CBD(eta) polynomial (keygen, columns of B) */
void SampleCBD(poly *p, const uint8_t seed[CRHBYTES], uint16_t nonce);

/* y1: discrete Gaussian D_{sigma_y} conditioned |y| <= B0+R (CDT) */
void SampleY1(poly *p, const uint8_t seed[CRHBYTES], uint16_t nonce);

/* X' / e_bottom: discrete Gaussian D_{sigma_base} (CDT, tail GMAX) */
void SampleGauss(poly *p, const uint8_t seed[CRHBYTES], uint16_t nonce);

/* binary challenge with TAU ones */
void SampleChallenge(poly *c, const uint8_t seed[CTILDEBYTES]);

/* Algorithm 5 over all N coefficients.
 * outputs z1 (|z1| <= B0) and v (= x + c; v = c mod 2, |v| <= R).
 * returns 1 on acceptance of all coefficients, 0 on rejection. */
int Sample_Z(poly *z1, poly *v, const poly *c, const poly *y1, stream256_state *rng);

#endif
