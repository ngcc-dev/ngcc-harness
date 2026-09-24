#ifndef SYMMETRIC_H
#define SYMMETRIC_H

#include "drng.h"
#include "auxfunc.h"

// DRNG_ctx for generating pseudorandom numbers
DRNG_ctx drng_algorithm;

#define H(OUT, OUTLEN, IN, INLEN) pseudoXOF((OUTLEN) * 8, IN, (INLEN) * 8, OUT)
#define G(OUT, OUTLEN, IN, INLEN) pseudoXOF((OUTLEN) * 8, IN, (INLEN) * 8, OUT)

#define H_Init   init_random_number
#define G_Init   init_random_number
#define H_Squeeze get_random_number
#define G_Squeeze get_random_number

#endif
