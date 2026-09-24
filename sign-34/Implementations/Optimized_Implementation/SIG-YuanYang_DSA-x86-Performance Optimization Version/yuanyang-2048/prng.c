#include <string.h>

#include "prng.h"
#include "sysrng.h"

/*
 * Optimized production builds use operating-system randomness. NGCC_KATS and
 * the explicit software override retain the submission callback so KAT streams
 * remain reproducible. An OS RNG failure is fatal; production code must not
 * silently fall back to a deterministic callback.
 */
#if !defined(NGCC_KATS) \
	&& !defined(YUANYANG_FORCE_SOFTWARE_RANDOMNESS) \
	&& (defined(_WIN32) || defined(_WIN64) || defined(__linux__) \
		|| defined(__unix__) || defined(__APPLE__))
#define YUANYANG_USE_SYSTEM_RANDOMNESS   1
#else
#define YUANYANG_USE_SYSTEM_RANDOMNESS   0
#endif

#if YUANYANG_USE_SYSTEM_RANDOMNESS
static int
get_random_system(void *ctx, unsigned char *buf, unsigned long long len_bytes)
{
	(void)ctx;
	if (len_bytes > (unsigned long long)SIZE_MAX) {
		return YUANYANG_BADARG;
	}
	return yuanyang_sysrng(buf, (size_t)len_bytes) == 0
		? YUANYANG_SUCCESS : YUANYANG_BADARG;
}
#endif

void
prng_init(prng *p, yuanyang_randombytes randombytes, void *random_ctx)
{
	if (p == NULL) {
		return;
	}
	memset(p, 0, sizeof *p);
	p->ptr = sizeof p->buf.d;
#if YUANYANG_USE_SYSTEM_RANDOMNESS
	(void)randombytes;
	(void)random_ctx;
	p->randombytes = get_random_system;
	p->random_ctx = NULL;
#else
	p->randombytes = randombytes;
	p->random_ctx = random_ctx;
#endif
	p->status = YUANYANG_SUCCESS;
}

int
prng_refill(prng *p)
{
	if (p == NULL || p->randombytes == NULL) {
		if (p != NULL) {
			p->status = YUANYANG_BADARG;
		}
		return YUANYANG_BADARG;
	}
	if (p->status != YUANYANG_SUCCESS) {
		return p->status;
	}
	if (p->randombytes(p->random_ctx, p->buf.d,
		(unsigned long long)sizeof p->buf.d) != 0)
	{
		memset(p->buf.d, 0, sizeof p->buf.d);
		p->ptr = sizeof p->buf.d;
		p->status = YUANYANG_BADARG;
		return p->status;
	}
	p->ptr = 0;
	return YUANYANG_SUCCESS;
}

int
prng_uses_system_randomness(void)
{
	return YUANYANG_USE_SYSTEM_RANDOMNESS;
}

int
prng_get_bytes(prng *p, void *dst, size_t len)
{
	unsigned char *buf;

	if (p == NULL || dst == NULL) {
		return YUANYANG_BADARG;
	}
	if (p->status != YUANYANG_SUCCESS) {
		return p->status;
	}
	buf = (unsigned char *)dst;
	while (len > 0) {
		size_t avail;
		size_t clen;

		if (p->ptr == sizeof p->buf.d) {
			int rc;

			rc = prng_refill(p);
			if (rc != YUANYANG_SUCCESS) {
				return rc;
			}
		}
		avail = sizeof p->buf.d - p->ptr;
		clen = avail < len ? avail : len;
		memcpy(buf, p->buf.d + p->ptr, clen);
		buf += clen;
		len -= clen;
		p->ptr += clen;
	}
	return YUANYANG_SUCCESS;
}

int
prng_status(const prng *p)
{
	return p == NULL ? YUANYANG_BADARG : p->status;
}
