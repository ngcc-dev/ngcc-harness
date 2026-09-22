#include <string.h>

#include "prng.h"

/*
 * The reference implementation uses the provided software DRNG callback by
 * default, as required by the submission guidelines. System randomness remains
 * available for benchmarks and optimized implementations through the explicit
 * YUANYANG_FORCE_SYSTEM_RANDOMNESS build flag. NGCC_KATS takes precedence over
 * that flag so that KAT generation remains reproducible.
 */
#if defined(YUANYANG_FORCE_SYSTEM_RANDOMNESS) \
	&& !defined(NGCC_KATS) \
	&& (defined(__linux__) || defined(__unix__) || defined(__APPLE__) \
		|| defined(SYSTEM_RANDOMNESS) || defined(YUANYANG_SYSTEM_RANDOMNESS))
#define YUANYANG_USE_SYSTEM_RANDOMNESS   1
#else
#define YUANYANG_USE_SYSTEM_RANDOMNESS   0
#endif

#if YUANYANG_USE_SYSTEM_RANDOMNESS
#include <errno.h>
#if defined(__linux__)
#include <sys/random.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

static int
get_random_system(void *ctx, unsigned char *buf, unsigned long long len_bytes)
{
	(void)ctx;
	while (len_bytes > 0) {
#if defined(__linux__)
		ssize_t r = getrandom(buf, (size_t)len_bytes, 0);
#else
		int fd;
		ssize_t r;

		fd = open("/dev/urandom", O_RDONLY);
		if (fd < 0) {
			return YUANYANG_BADARG;
		}
		r = read(fd, buf, (size_t)len_bytes);
		close(fd);
#endif
		if (r < 0) {
			if (errno == EINTR) {
				continue;
			}
			return YUANYANG_BADARG;
		}
		if (r == 0) {
			return YUANYANG_BADARG;
		}
		buf += (size_t)r;
		len_bytes -= (unsigned long long)r;
	}
	return YUANYANG_SUCCESS;
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
	p->randombytes = get_random_system;
	p->random_ctx = NULL;
	p->fallback_randombytes = randombytes;
	p->fallback_random_ctx = random_ctx;
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
#if YUANYANG_USE_SYSTEM_RANDOMNESS
		if (p->fallback_randombytes != NULL) {
			p->randombytes = p->fallback_randombytes;
			p->random_ctx = p->fallback_random_ctx;
			p->fallback_randombytes = NULL;
			p->fallback_random_ctx = NULL;
			if (p->randombytes(p->random_ctx, p->buf.d,
				(unsigned long long)sizeof p->buf.d) == 0)
			{
				p->ptr = 0;
				return YUANYANG_SUCCESS;
			}
		}
#endif
		memset(p->buf.d, 0, sizeof p->buf.d);
		p->ptr = sizeof p->buf.d;
		p->status = YUANYANG_BADARG;
		return p->status;
	}
	p->ptr = 0;
	return YUANYANG_SUCCESS;
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
