#ifndef SAMPLE_H
#define SAMPLE_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "api.h"
#include "params.h"
#include "sign.h"
#include "poly.h"
#include "polyvec.h"
#include "packing.h"
#include "randombytes.h"
#include "hashkdf.h"
#include "api.h"
#include "pspm.h"
#include <string.h>

#define REJ_UNIFORM_BYTES 1440 // fail with prob. less than 2^-14
#define REJ_UNIFORM_NBLOCKS  (REJ_UNIFORM_BYTES + KDF128RATE - 1) / KDF128RATE

void expand_mat(polyvecl mat[PARAM_K], const unsigned char rho[SEEDBYTES]);

void challenge(uint8_t *seed, const unsigned char mu[CRHBYTES],
	const polyveck *w1);

void unpack_c(poly *c, const uint8_t seed[SEEDBYTES]);


void expand_matr(polyvecl mat[PARAM_K], const unsigned char rho[SEEDBYTES]);
#endif