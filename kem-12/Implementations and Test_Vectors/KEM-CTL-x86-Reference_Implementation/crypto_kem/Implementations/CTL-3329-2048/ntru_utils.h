#ifndef NTRU_UTILS_H__
#define NTRU_UTILS_H__

#include <stdint.h>

int ctl_ntrugen_solve_FG(int8_t *F, int8_t *G,
	const int8_t *f, const int8_t *g,
	uint32_t q, unsigned logn, uint32_t *tmp);

#endif
