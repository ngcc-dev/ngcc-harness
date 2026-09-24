/*
 * Common yuanyang-2048 primitives used by keygen, signing and verification.
 *
 * The optimized ring product itself lives in ntt.c; this file keeps the public
 * internal entry point so callers do not depend on the chosen multiplication
 * backend.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "auxfunc.h"
#include "yuanyang_inner.h"

/*
 * Big-prime NTT backend implemented in ntt.c.  It is intentionally kept out
 * of yuanyang_inner.h so callers only see the definitive multiplication entry
 * point, plus the quadratic reference used by tests.
 */
void yuanyang_mul_mod_xn_plus_1_ntt_big(
	uint16_t out[YUANYANG_D],
	const uint16_t h[YUANYANG_D],
	const int16_t s1[YUANYANG_D]);

int16_t
yuanyang_center_lift_q(uint16_t x)
{
	return x > (YUANYANG_Q / 2u) ? (int16_t)(x - YUANYANG_Q) : (int16_t)x;
}

int
yuanyang_hash_message_with_public_key(
	unsigned char seed[32],
	const unsigned char *m, unsigned long long m_len_bytes,
	const uint16_t h[YUANYANG_D])
{
	unsigned char *buf;
	unsigned long long total_len;
	int rc;

	if (seed == 0 || m == 0 || h == 0) {
		return YUANYANG_BADARG;
	}
	total_len = m_len_bytes + YUANYANG_D * sizeof h[0];
	buf = (unsigned char *)malloc((size_t)total_len);
	if (buf == 0) {
		return YUANYANG_MEMORY_ERROR;
	}
	memcpy(buf, m, (size_t)m_len_bytes);
	memcpy(buf + m_len_bytes, h, YUANYANG_D * sizeof h[0]);
	rc = sm3hash(256, buf, 8u * total_len, seed);
	free(buf);
	return rc == 0 ? YUANYANG_SUCCESS : YUANYANG_BADARG;
}

int
yuanyang_hash_to_challenge(
	uint16_t c[YUANYANG_D],
	const unsigned char salt[YUANYANG_SALT_BYTES],
	const unsigned char seed[32])
{
	enum {
		YUANYANG_HASH_ACCEPTANCE_BOUND =
			(65536u / YUANYANG_Q) * YUANYANG_Q,
		YUANYANG_HASH_MIN_SAMPLES =
			(YUANYANG_D * 65536u + YUANYANG_HASH_ACCEPTANCE_BOUND - 1u)
			/ YUANYANG_HASH_ACCEPTANCE_BOUND,
		YUANYANG_HASH_MARGIN_SAMPLES = YUANYANG_D >> 4,
		YUANYANG_HASH_STREAM_BYTES =
			2u * (YUANYANG_HASH_MIN_SAMPLES
			+ YUANYANG_HASH_MARGIN_SAMPLES)
	};
	unsigned char input[YUANYANG_SALT_BYTES + 32u];
	unsigned char stack_stream[YUANYANG_HASH_STREAM_BYTES];
	unsigned char *stream;
	unsigned char *heap_stream;
	unsigned stream_len, off, idx;

	if (c == 0 || salt == 0 || seed == 0) {
		return YUANYANG_BADARG;
	}
	memcpy(input, salt, YUANYANG_SALT_BYTES);
	memcpy(input + YUANYANG_SALT_BYTES, seed, 32u);

	/*
	 * HashToChallenge samples 16-bit XOF words with rejection modulo q.  The
	 * KDF-SM3 XOF is prefix-stable, so asking for a larger initial prefix keeps
	 * the exact same byte stream while avoiding almost all retry-and-regenerate
	 * cases for the larger parameter sets.
	 */
	stream = stack_stream;
	heap_stream = 0;
	stream_len = (unsigned)sizeof stack_stream;
	if (pseudoXOF(8u * stream_len, input, 8u * sizeof input, stream) != 0) {
		return YUANYANG_BADARG;
	}
	off = 0;
	idx = 0;
	while (idx < YUANYANG_D) {
		if (off + 2u > stream_len) {
			unsigned char *next;

			if (stream_len > (1u << 23)) {
				free(heap_stream);
				return YUANYANG_BADARG;
			}
			stream_len <<= 1;
			next = (unsigned char *)malloc(stream_len);
			if (next == 0) {
				free(heap_stream);
				return YUANYANG_MEMORY_ERROR;
			}
			if (pseudoXOF(8u * stream_len, input, 8u * sizeof input, next) != 0) {
				free(heap_stream);
				free(next);
				return YUANYANG_BADARG;
			}
			free(heap_stream);
			heap_stream = next;
			stream = heap_stream;
		}
		{
			unsigned value;

			value = ((unsigned)stream[off] << 8) | (unsigned)stream[off + 1u];
			off += 2u;
			if (value < YUANYANG_HASH_ACCEPTANCE_BOUND) {
				c[idx++] = (uint16_t)(value % YUANYANG_Q);
			}
		}
	}

	free(heap_stream);
	return YUANYANG_SUCCESS;
}

void
yuanyang_mul_mod_xn_plus_1(
	uint16_t out[YUANYANG_D],
	const uint16_t h[YUANYANG_D],
	const int16_t s1[YUANYANG_D])
{
	yuanyang_mul_mod_xn_plus_1_ntt_big(out, h, s1);
}
