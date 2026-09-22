#ifndef NTRUGEN_H
#define NTRUGEN_H

#include <stdint.h>

#define NTRUGEN_OK          0
#define NTRUGEN_ERR_GCD    -1
#define NTRUGEN_ERR_VERIFY -2

int ntrugen(const int8_t *f, const int8_t *g,
	int32_t *F, int32_t *G, int logn, int q, uint64_t *work);

#endif
