#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ntru_utils.h"

#define NTRUGEN_PREFIX ctlng
#include "ng_inner.h"

/*
 * CTL solve profiles copied from ntrugen's CTL integration. We keep only the
 * solving parameters we need here so that keygen can reuse the improved NTRU
 * solver without pulling the rest of ntrugen's CTL keygen logic.
 */
static const ntru_profile SOLVE_CTL_257_512 = {
	257,
	9, 9,
	{ 1, 1, 1, 2, 3, 5, 8, 15, 30, 59, 0 },
	{ 1, 1, 2, 4, 6, 12, 23, 44, 87, 0 },
	{ 1, 1, 1, 2, 3, 3, 3, 4, 6, 0 },
	13,
	{ 0, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31 },
	{ 0, 0, 1, 2, 2, 2, 2, 2, 2, 3, 3 }
};

static const ntru_profile SOLVE_CTL_769_1024 = {
	769,
	10, 10,
	{ 1, 1, 1, 2, 3, 5, 10, 18, 35, 69, 137 },
	{ 1, 1, 2, 4, 7, 14, 27, 52, 102, 204 },
	{ 1, 1, 1, 2, 3, 3, 3, 4, 5, 7 },
	11,
	{ 0, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31 },
	{ 0, 0, 1, 2, 2, 2, 2, 2, 2, 3, 3 }
};

int
ctl_ntrugen_solve_FG(int8_t *F, int8_t *G, const int8_t *f, const int8_t *g,
	uint32_t q, unsigned logn, uint32_t *tmp)
{
	const ntru_profile *prof;
	size_t n;
	int err;
	int8_t *tG, *tF;

	switch (q) {
	case 257:
		prof = &SOLVE_CTL_257_512;
		break;
	case 769:
		prof = &SOLVE_CTL_769_1024;
		break;
	default:
		return -4;
	}

	/*
	 * ntrugen uses the Falcon convention f*G - g*F = q, while CTL uses
	 * g*F - f*G = q, so we swap (f,g) on input. On success, ntrugen writes
	 * the solution in tmp[] as (G,F) for the CTL convention.
	 */
	err = ctlng_solve_NTRU(prof, logn, g, f, tmp);
	if (err != SOLVE_OK) {
		return err;
	}

	n = (size_t)1 << logn;
	tG = (int8_t *)tmp;
	tF = tG + n;
	memmove(F, tF, n);
	memmove(G, tG, n);
	return SOLVE_OK;
}
