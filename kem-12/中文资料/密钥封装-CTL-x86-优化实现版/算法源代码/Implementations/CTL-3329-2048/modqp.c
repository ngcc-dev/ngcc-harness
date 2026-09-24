#include "inner.h"
#ifdef CTL_ENABLE_PERF
#include "perf.h"
#endif

#define Q   64513
#include "modgen.c"

/* see inner.h */
void
ctl_polyqp_mulneg(int16_t *d, const int16_t *a, const int32_t *b,
	unsigned logn, uint32_t *tmp)
{
	size_t u, n;
	uint16_t *t1, *t2;

	n = (size_t)1 << logn;

	/*
	 * In order to save memory, we use the destination array for
	 * intermediate computations as well. Since d may partially
	 * overlap with a, we first do a memmove().
	 */
	if (d != a) {
		memmove(d, a, n * sizeof *a);
	}
	t1 = (uint16_t *)d;
	t2 = (uint16_t *)tmp;
	for (u = 0; u < n; u ++) {
		t1[u] = mq_set(*(int16_t *)&t1[u]);
		t2[u] = mq_set(-b[u]);
	}
	NTT(t1, t1, logn);
	NTT(t2, t2, logn);
	mq_poly_mul_ntt(t1, t1, t2, logn);
	iNTT(t1, t1, logn);
	for (u = 0; u < n; u ++) {
		*(int16_t *)&t1[u] = mq_snorm(t1[u]);
	}
}
