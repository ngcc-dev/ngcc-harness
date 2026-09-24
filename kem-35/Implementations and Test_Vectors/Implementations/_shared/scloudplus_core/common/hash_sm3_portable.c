/**
 * @file hash_sm3_portable.c
 * @brief Portable SM3-family F/G/H/K adapter for Scloud+.
 *
 * This file connects the common KEM hash interface to the bounded SM3 expansion
 * backend declared in sm3_backend.h.  Optimized x86_64 builds use their own
 * hash_sm3_avx2.c adapter; this portable adapter is used by Reference SM3 and
 * AArch64 SM3 builds.
 */

#include "sm3_backend.h"
#include "hash.h"
#include "scloudplus_param_common.h"
#include <stdlib.h>
#include <string.h>

enum
{
	SCLOUDPLUS_HASH_LABEL_F = 0x46U,
	SCLOUDPLUS_HASH_LABEL_G = 0x47U,
	SCLOUDPLUS_HASH_LABEL_H = 0x48U,
	SCLOUDPLUS_HASH_LABEL_K = 0x4BU,
	SCLOUDPLUS_SM3_LABEL_STACK_BYTES = 512U
};

static void sm3_secure_zeroize(void *ptr, size_t len)
{
	volatile uint8_t *p = (volatile uint8_t *)ptr;

	while (len-- != 0U)
	{
		*p++ = 0U;
	}
}

static void sm3_zero_output(uint8_t *out, size_t outlen)
{
	if (outlen != 0U)
	{
		memset(out, 0, outlen);
	}
}

static void sm3_bounded_xof_state_reset(sm3_bounded_xof_state *state)
{
	memset(state, 0, sizeof(*state));
	state->status = 0;
}

static int sm3_pseudoXOF_labeled(unsigned long long output_len_bits,
								 uint8_t label, const unsigned char *msg,
								 unsigned long long msg_len_bytes,
								 unsigned char *output)
{
	if (msg_len_bytes > (unsigned long long)(SIZE_MAX - 1U))
	{
		return -1;
	}
	{
		const size_t msg_len = (size_t)msg_len_bytes;
		const size_t labeled_len = msg_len + 1U;
		uint8_t stack_labeled[SCLOUDPLUS_SM3_LABEL_STACK_BYTES];
		uint8_t *labeled = stack_labeled;
		int rc;

		if (labeled_len > sizeof(stack_labeled))
		{
			labeled = (uint8_t *)malloc(labeled_len);
			if (labeled == NULL)
			{
				return -1;
			}
		}
		labeled[0] = label;
		if (msg_len != 0U)
		{
			memcpy(labeled + 1U, msg, msg_len);
		}
		rc = pseudoXOF(output_len_bits, labeled,
					   (unsigned long long)labeled_len * 8ULL, output);
		sm3_secure_zeroize(labeled, labeled_len);
		if (labeled != stack_labeled)
		{
			free(labeled);
		}
		return rc;
	}
}

static int sm3_pseudohash_labeled(int digest_len_bits, uint8_t label,
								  const unsigned char *msg,
								  unsigned long long msg_len_bytes,
								  unsigned char *output)
{
	if (msg_len_bytes > (unsigned long long)(SIZE_MAX - 1U))
	{
		return -1;
	}
	{
		const size_t msg_len = (size_t)msg_len_bytes;
		const size_t labeled_len = msg_len + 1U;
		uint8_t stack_labeled[SCLOUDPLUS_SM3_LABEL_STACK_BYTES];
		uint8_t *labeled = stack_labeled;
		int rc;

		if (labeled_len > sizeof(stack_labeled))
		{
			labeled = (uint8_t *)malloc(labeled_len);
			if (labeled == NULL)
			{
				return -1;
			}
		}
		labeled[0] = label;
		if (msg_len != 0U)
		{
			memcpy(labeled + 1U, msg, msg_len);
		}
		rc = pseudohash(digest_len_bits, labeled,
						(unsigned long long)labeled_len * 8ULL, output);
		sm3_secure_zeroize(labeled, labeled_len);
		if (labeled != stack_labeled)
		{
			free(labeled);
		}
		return rc;
	}
}

static int scloudplus_sm3_bounded_xof_state_init(sm3_bounded_xof_state *state,
											  const uint8_t *in, size_t inlen)
{
	if (inlen > sizeof(state->msg))
	{
		sm3_bounded_xof_state_reset(state);
		state->status = -1;
		return -1;
	}
	if (inlen != 0U)
	{
		memcpy(state->msg, in, inlen);
	}
	state->msg_len = inlen;
	state->generated = 0U;
	state->cache_pos = SM3_BOUNDED_XOF_CACHE_BYTES;
	state->cache_len = SM3_BOUNDED_XOF_CACHE_BYTES;
	state->status = 0;
	return 0;
}

static int scloudplus_sm3_bounded_xof_state_refill(sm3_bounded_xof_state *state)
{
	const unsigned int ct =
		(unsigned int)(state->generated / SM3_BOUNDED_XOF_RATE) + 1U;

	if (pseudoXOF_from_counter(
			(unsigned long long)SM3_BOUNDED_XOF_CACHE_BYTES * 8ULL,
			state->msg, (unsigned long long)state->msg_len * 8ULL,
			ct, state->cache) != 0)
	{
		sm3_zero_output(state->cache, sizeof(state->cache));
		state->cache_pos = 0U;
		state->cache_len = SM3_BOUNDED_XOF_CACHE_BYTES;
		state->status = -1;
		return -1;
	}
	state->generated += SM3_BOUNDED_XOF_CACHE_BYTES;
	state->cache_pos = 0U;
	state->cache_len = SM3_BOUNDED_XOF_CACHE_BYTES;
	return 0;
}

void sm3_bounded_xof_init(sm3_bounded_xof_state *state)
{
	sm3_bounded_xof_state_reset(state);
}

int sm3_bounded_xof_absorb(sm3_bounded_xof_state *state, const uint8_t *in, size_t inlen)
{
	if (state->msg_len + inlen > sizeof(state->msg))
	{
		sm3_bounded_xof_state_reset(state);
		state->status = -1;
		return -1;
	}
	if (inlen != 0U)
	{
		memcpy(state->msg + state->msg_len, in, inlen);
	}
	state->msg_len += inlen;
	state->generated = 0U;
	state->cache_pos = SM3_BOUNDED_XOF_CACHE_BYTES;
	state->cache_len = SM3_BOUNDED_XOF_CACHE_BYTES;
	state->status = 0;
	return 0;
}

void sm3_bounded_xof_finalize(sm3_bounded_xof_state *state)
{
	(void)state;
}

int sm3_bounded_xof_squeeze(uint8_t *out, size_t outlen, sm3_bounded_xof_state *state)
{
	size_t copied = 0U;

	if (outlen == 0U)
	{
		return state->status;
	}
	if (state->status != 0)
	{
		sm3_zero_output(out, outlen);
		return -1;
	}
	while (copied < outlen)
	{
		size_t take;

		if (state->cache_pos == state->cache_len)
		{
			if (scloudplus_sm3_bounded_xof_state_refill(state) != 0)
			{
				sm3_zero_output(out + copied, outlen - copied);
				return -1;
			}
		}
		take = state->cache_len - state->cache_pos;
		if (take > outlen - copied)
		{
			take = outlen - copied;
		}
		memcpy(out + copied, state->cache + state->cache_pos, take);
		state->cache_pos += take;
		copied += take;
	}
	return state->status;
}

int sm3_bounded_xof_absorb_once(sm3_bounded_xof_state *state, const uint8_t *in, size_t inlen)
{
	return scloudplus_sm3_bounded_xof_state_init(state, in, inlen);
}

int sm3_bounded_xof_squeezeblocks(uint8_t *out, size_t nblocks, sm3_bounded_xof_state *state)
{
	for (size_t i = 0U; i < nblocks; i++)
	{
		if (sm3_bounded_xof_squeeze(out + i * SM3_BOUNDED_XOF_RATE,
									SM3_BOUNDED_XOF_RATE, state) != 0)
		{
			return -1;
		}
	}
	return 0;
}

int sm3_bounded_xof(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen)
{
	if (pseudoXOF((unsigned long long)outlen * 8ULL, in,
				  (unsigned long long)inlen * 8ULL, out) != 0)
	{
		sm3_zero_output(out, outlen);
		return -1;
	}
	return 0;
}

int sm3_bounded_xof_seed_row(uint8_t *out, size_t outlen, const uint8_t *seed,
						  uint32_t row)
{
	uint8_t input[scloudplus_seedA_bytes + 4U];
	memcpy(input, seed, scloudplus_seedA_bytes);
	input[scloudplus_seedA_bytes + 0U] = (uint8_t)(row >> 0U);
	input[scloudplus_seedA_bytes + 1U] = (uint8_t)(row >> 8U);
	input[scloudplus_seedA_bytes + 2U] = (uint8_t)(row >> 16U);
	input[scloudplus_seedA_bytes + 3U] = (uint8_t)(row >> 24U);
	if (pseudoXOF((unsigned long long)outlen * 8ULL, input,
				  (unsigned long long)sizeof(input) * 8ULL, out) != 0)
	{
		sm3_zero_output(out, outlen);
		sm3_secure_zeroize(input, sizeof(input));
		return -1;
	}
	sm3_secure_zeroize(input, sizeof(input));
	return 0;
}

int sm3_bounded_xof8_a_rows(uint8_t *out[8], size_t outlen, const uint8_t *seed,
							uint32_t row_start)
{
	uint8_t input[8][scloudplus_seedA_bytes + 4U];
	const size_t row_off = scloudplus_seedA_bytes;
	int status = 0;

	for (size_t lane = 0U; lane < 8U; lane++)
	{
		const uint32_t row = row_start + (uint32_t)lane;
		memcpy(input[lane], seed, scloudplus_seedA_bytes);
		input[lane][row_off + 0U] = (uint8_t)(row >> 0U);
		input[lane][row_off + 1U] = (uint8_t)(row >> 8U);
		input[lane][row_off + 2U] = (uint8_t)(row >> 16U);
		input[lane][row_off + 3U] = (uint8_t)(row >> 24U);
	}
	for (size_t lane = 0U; lane < 8U; lane++)
	{
		if (pseudoXOF_from_counter((unsigned long long)outlen * 8ULL,
								   input[lane],
								   (unsigned long long)sizeof(input[0]) * 8ULL,
								   1U, out[lane]) != 0)
		{
			sm3_zero_output(out[lane], outlen);
			status = -1;
		}
	}
	sm3_secure_zeroize(input, sizeof(input));
	return status;
}

void sm3_256(uint8_t out[32], const uint8_t *in, size_t inlen)
{
	if (sm3hash(256, in, (unsigned long long)inlen * 8ULL, out) != 0)
	{
		sm3_zero_output(out, 32U);
	}
}

void scloudplus_F(unsigned char *output, unsigned long long outlen,
				  const unsigned char *input, unsigned long long inlen)
{
	if (sm3_pseudoXOF_labeled(outlen * 8ULL, SCLOUDPLUS_HASH_LABEL_F, input,
							  inlen, output) != 0)
	{
		sm3_zero_output(output, (size_t)outlen);
	}
}

void scloudplus_K(unsigned char *output, unsigned long long outlen,
				  const unsigned char *input, unsigned long long inlen)
{
	if (sm3_pseudoXOF_labeled(outlen * 8ULL, SCLOUDPLUS_HASH_LABEL_K, input,
							  inlen, output) != 0)
	{
		sm3_zero_output(output, (size_t)outlen);
	}
}

void scloudplus_H(unsigned char *output, const unsigned char *input,
				  unsigned long long inlen)
{
	if (sm3_pseudohash_labeled((int)scloudplus_hash_bytes * 8,
							   SCLOUDPLUS_HASH_LABEL_H, input, inlen,
							   output) != 0)
	{
		sm3_zero_output(output, scloudplus_hash_bytes);
	}
}

void scloudplus_G(unsigned char *output, const unsigned char *input,
				  unsigned long long inlen)
{
	if (sm3_pseudoXOF_labeled((unsigned long long)scloudplus_G_bytes * 8ULL,
							  SCLOUDPLUS_HASH_LABEL_G, input, inlen,
							  output) != 0)
	{
		sm3_zero_output(output, scloudplus_G_bytes);
	}
}
