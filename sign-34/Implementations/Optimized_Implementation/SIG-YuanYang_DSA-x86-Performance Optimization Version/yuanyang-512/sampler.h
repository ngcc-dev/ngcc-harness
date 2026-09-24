#ifndef YUANYANG_512_SAMPLER_H
#define YUANYANG_512_SAMPLER_H

#include "yuanyang_inner.h"
#include "prng.h"


/* Draw from the large discrete Gaussian centered at mu. */
int yuanyang_samplerz_large(
	int *out, fpr mu, prng *rng);

/* Draw from the large discrete Gaussian centered at zero. */
int yuanyang_samplerz_large_zero(
	int *out, prng *rng);

/* Draw from the small discrete Gaussian centered at mu. */
int yuanyang_samplerz_small(
	int *out, fpr mu, prng *rng);

/* Accept with a high-precision bounded probability using randomness from rng. */
int yuanyang_accept_probability_q62(fpr_q62 probability, prng *rng);


#endif
