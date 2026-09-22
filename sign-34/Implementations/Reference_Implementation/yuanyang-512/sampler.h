#ifndef YUANYANG_512_SAMPLER_H
#define YUANYANG_512_SAMPLER_H

#include "yuanyang_inner.h"
#include "prng.h"

/*
 * Signature sampler interface used by Algorithm 6.
 *
 * The "large" sampler uses the sigma = 4*eta RCDT table used by the
 * presampler. The "small" sampler uses the eta table for the online ring
 * samples. Both centered samplers implement the Falcon-style
 * half-Gaussian-plus-BerExp correction in sampler.c.
 */


/* Draw from the large discrete Gaussian centered at mu. */
int yuanyang_samplerz_large(
	int *out, fpr mu, prng *rng);

/* Draw from the large discrete Gaussian centered at zero. */
int yuanyang_samplerz_large_zero(
	int *out, prng *rng);

/* Draw from the small discrete Gaussian centered at mu. */
int yuanyang_samplerz_small(
	int *out, fpr mu, prng *rng);

/* Accept with a Q62 probability, used by the Delta rejection steps. */
int yuanyang_accept_probability_q62(fpr_q62 probability, prng *rng);


#endif
