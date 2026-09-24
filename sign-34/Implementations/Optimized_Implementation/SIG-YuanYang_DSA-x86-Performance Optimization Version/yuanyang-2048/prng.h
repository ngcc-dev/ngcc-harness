#ifndef YUANYANG_2048_PRNG_H
#define YUANYANG_2048_PRNG_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "yuanyang_inner.h"

#ifndef YUANYANG_PRNG_BUFFER_BYTES
#define YUANYANG_PRNG_BUFFER_BYTES 1024u
#endif

/*
 * Buffered wrapper around the external RNG callback.
 *
 * This is intentionally much simpler than Falcon's ChaCha20 PRNG: the
 * underlying submission RNG already provides the deterministic byte stream,
 * and we only want to amortize callback overhead on hot paths such as
 * samplerz.
 */
struct prng {
	union {
		unsigned char d[YUANYANG_PRNG_BUFFER_BYTES];
		uint64_t align_u64;
	} buf;
	size_t ptr;
	yuanyang_randombytes randombytes;
	void *random_ctx;
	int status;
};

/* Initialize a buffered wrapper around the provided randombytes callback. */
void prng_init(prng *p, yuanyang_randombytes randombytes, void *random_ctx);

/* Refill the internal byte buffer from the callback. */
int prng_refill(prng *p);

/* Copy len pseudorandom bytes into dst, refilling as needed. */
int prng_get_bytes(prng *p, void *dst, size_t len);

/* Return the first error observed by the buffered PRNG, or YUANYANG_SUCCESS. */
int prng_status(const prng *p);

/* Return nonzero when this build selects the operating-system RNG backend. */
int prng_uses_system_randomness(void);

static inline uint64_t
prng_get_u64(prng *p)
{
	size_t u;

	u = p->ptr;
	if (u >= (sizeof p->buf.d) - 9u) {
		if (prng_refill(p) != YUANYANG_SUCCESS) {
			return 0;
		}
		u = 0;
	}
	p->ptr = u + 8u;
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	{
		uint64_t v;

		memcpy(&v, p->buf.d + u, sizeof v);
		return v;
	}
#else
	return (uint64_t)p->buf.d[u + 0]
		| ((uint64_t)p->buf.d[u + 1] << 8)
		| ((uint64_t)p->buf.d[u + 2] << 16)
		| ((uint64_t)p->buf.d[u + 3] << 24)
		| ((uint64_t)p->buf.d[u + 4] << 32)
		| ((uint64_t)p->buf.d[u + 5] << 40)
		| ((uint64_t)p->buf.d[u + 6] << 48)
		| ((uint64_t)p->buf.d[u + 7] << 56);
#endif
}

static inline unsigned
prng_get_u8(prng *p)
{
	unsigned v;

	if (p->ptr == sizeof p->buf.d) {
		if (prng_refill(p) != YUANYANG_SUCCESS) {
			return 0;
		}
	}
	v = p->buf.d[p->ptr];
	p->ptr++;
	return v;
}

#endif
