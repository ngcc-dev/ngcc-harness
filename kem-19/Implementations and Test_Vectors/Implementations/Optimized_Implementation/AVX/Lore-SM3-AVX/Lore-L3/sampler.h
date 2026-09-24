#ifndef SAMPLER_H
#define SAMPLER_H

#include "poly.h"
#include "polyvec.h"

#define sample_fixed_weight LORE_NAMESPACE(sample_fixed_weight)
void sample_fixed_weight(poly_crt_vec *r_crt_vec, poly_sparse *r_sparse_vec, const unsigned char *seed, unsigned char nonce);

#endif