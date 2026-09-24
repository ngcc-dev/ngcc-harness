#ifndef SIGN_SAMPLER_H
#define SIGN_SAMPLER_H

#include "fixpoint.h"
#include "params.h"
#include "reduce_neon.h"
#include <stdint.h>

#define rej_uniform DARTS_NAMESPACE(rej_uniform)
unsigned int rej_uniform(int32_t *a, unsigned int len, const uint8_t *buf, unsigned int buflen);

#define rej_p DARTS_NAMESPACE(rej_p)
unsigned int rej_p(int32_t *a, unsigned int len, const uint8_t *buf, unsigned int buflen);

#define sample_gauss_N DARTS_NAMESPACE(sample_gauss_N)
void sample_gauss_N(uint64_t *r, uint8_t *signs, fp96_76 *sqsum, const uint8_t seed[CRHBYTES], const uint16_t nonce, const size_t len);

#endif