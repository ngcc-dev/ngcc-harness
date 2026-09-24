/**
 * @file hash_sm3_avx2.c
 * @brief Optimized SM3-family F/G/H/K adapter for x86_64 AVX2 builds.
 *
 * This file connects the KEM hash interface in `hash.h` to the AVX2 SM3
 * bounded SM3 expansion helpers implemented in `sm3_avx2.c`.
 */

#include "sm3_backend.h"
#include "hash.h"
#include "scloudplus_param_common.h"
#include <stdlib.h>
#include <string.h>

int pseudoXOF8_equal_inputlen_avx2(unsigned long long output_len_bits,
								   const unsigned char *msg[8],
								   unsigned long long msg_len_bits,
								   unsigned char *output[8]);
int pseudoXOF_from_counter_avx2(unsigned long long output_len_bits,
								const unsigned char *msg,
								unsigned long long msg_len_bits,
								unsigned int start_ct,
								unsigned char *output);
int pseudoXOF_precompute_byte_aligned(const unsigned char *msg,
									  unsigned long long msg_len_bits,
									  unsigned int base_digest[8],
									  unsigned char tail[64],
									  size_t *tail_len,
									  unsigned long long *total_bitlen);
int pseudoXOF_from_precomputed_counter(unsigned long long output_len_bits,
									   const unsigned int base_digest[8],
									   const unsigned char tail[64],
									   size_t tail_len,
									   unsigned long long total_bitlen,
									   unsigned int start_ct,
									   unsigned char *output);

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
	state->cache_pos = SM3_BOUNDED_XOF_CACHE_BYTES;
	state->cache_len = SM3_BOUNDED_XOF_CACHE_BYTES;
	state->status = 0;
}

static int sm3_pseudoXOF_labeled_avx2(unsigned long long output_len_bits,
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
		rc = pseudoXOF_from_counter_avx2(output_len_bits, labeled,
										 (unsigned long long)labeled_len * 8ULL,
										 1U, output);
		sm3_secure_zeroize(labeled, labeled_len);
		if (labeled != stack_labeled)
		{
			free(labeled);
		}
		return rc;
	}
}

static int sm3_pseudohash_labeled_avx2(int digest_len_bits, uint8_t label,
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
	state->tail_len = 0U;
	state->total_bitlen = 0ULL;
	state->precomputed_ready = 0U;
	state->cache_pos = SM3_BOUNDED_XOF_CACHE_BYTES;
	state->cache_len = SM3_BOUNDED_XOF_CACHE_BYTES;
	state->status = 0;
	return 0;
}

static int scloudplus_sm3_bounded_xof_state_prepare(sm3_bounded_xof_state *state)
{
	if (state->precomputed_ready != 0U)
	{
		return state->status;
	}
	if (pseudoXOF_precompute_byte_aligned(
			state->msg, (unsigned long long)state->msg_len * 8ULL,
			state->base_digest, state->tail, &state->tail_len,
			&state->total_bitlen) != 0)
	{
		sm3_bounded_xof_state_reset(state);
		state->status = -1;
		return -1;
	}
	state->precomputed_ready = 1U;
	state->status = 0;
	return 0;
}

static int scloudplus_sm3_bounded_xof_state_refill(sm3_bounded_xof_state *state)
{
	const uint32_t ct = (uint32_t)(state->generated / 32U) + 1U;

	if (scloudplus_sm3_bounded_xof_state_prepare(state) != 0)
	{
		memset(state->cache, 0, sizeof(state->cache));
		state->cache_pos = 0U;
		state->cache_len = SM3_BOUNDED_XOF_CACHE_BYTES;
		return -1;
	}
	if (pseudoXOF_from_precomputed_counter(
			(unsigned long long)SM3_BOUNDED_XOF_CACHE_BYTES * 8ULL,
			state->base_digest, state->tail, state->tail_len,
			state->total_bitlen, ct, state->cache) != 0)
	{
		memset(state->cache, 0, sizeof(state->cache));
		state->status = -1;
	}
	state->generated += SM3_BOUNDED_XOF_CACHE_BYTES;
	state->cache_pos = 0U;
	state->cache_len = SM3_BOUNDED_XOF_CACHE_BYTES;
	return state->status;
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
	state->precomputed_ready = 0U;
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

static int sm3_bounded_xof8_equal_inputlen(uint8_t *out[8], size_t outlen,
										   const uint8_t *in[8], size_t inlen)
{
	if (pseudoXOF8_equal_inputlen_avx2((unsigned long long)outlen * 8ULL,
									   (const unsigned char **)in,
									   (unsigned long long)inlen * 8ULL,
									   (unsigned char **)out) != 0)
	{
		for (size_t i = 0U; i < 8U; i++)
		{
			sm3_zero_output(out[i], outlen);
		}
		return -1;
	}
	return 0;
}

int sm3_bounded_xof8_a_rows(uint8_t *out[8], size_t outlen, const uint8_t *seed,
							uint32_t row_start)
{
	uint8_t in[8][scloudplus_seedA_bytes + 4U];
	const uint8_t *in_ptrs[8];
	const size_t row_off = scloudplus_seedA_bytes;

	for (size_t lane = 0U; lane < 8U; lane++)
	{
		const uint32_t row_index = row_start + (uint32_t)lane;
		memcpy(in[lane], seed, scloudplus_seedA_bytes);
		in[lane][row_off + 0U] = (uint8_t)(row_index >> 0U);
		in[lane][row_off + 1U] = (uint8_t)(row_index >> 8U);
		in[lane][row_off + 2U] = (uint8_t)(row_index >> 16U);
		in[lane][row_off + 3U] = (uint8_t)(row_index >> 24U);
		in_ptrs[lane] = in[lane];
	}
	if (sm3_bounded_xof8_equal_inputlen(out, outlen, in_ptrs, sizeof(in[0])) != 0)
	{
		sm3_secure_zeroize(in, sizeof(in));
		return -1;
	}
	sm3_secure_zeroize(in, sizeof(in));
	return 0;
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
	if (sm3_pseudoXOF_labeled_avx2(outlen * 8ULL, SCLOUDPLUS_HASH_LABEL_F,
								   input, inlen, output) != 0)
	{
		sm3_zero_output(output, (size_t)outlen);
	}
}

void scloudplus_K(unsigned char *output, unsigned long long outlen,
				  const unsigned char *input, unsigned long long inlen)
{
	if (sm3_pseudoXOF_labeled_avx2(outlen * 8ULL, SCLOUDPLUS_HASH_LABEL_K,
								   input, inlen, output) != 0)
	{
		sm3_zero_output(output, (size_t)outlen);
	}
}

void scloudplus_H(unsigned char *output, const unsigned char *input,
				  unsigned long long inlen)
{
	if (sm3_pseudohash_labeled_avx2((int)scloudplus_hash_bytes * 8,
									SCLOUDPLUS_HASH_LABEL_H, input, inlen,
									output) != 0)
	{
		sm3_zero_output(output, scloudplus_hash_bytes);
	}
}

void scloudplus_G(unsigned char *output, const unsigned char *input,
				  unsigned long long inlen)
{
	if (sm3_pseudoXOF_labeled_avx2((unsigned long long)scloudplus_G_bytes * 8ULL,
								   SCLOUDPLUS_HASH_LABEL_G, input, inlen,
								   output) != 0)
	{
		sm3_zero_output(output, scloudplus_G_bytes);
	}
}
