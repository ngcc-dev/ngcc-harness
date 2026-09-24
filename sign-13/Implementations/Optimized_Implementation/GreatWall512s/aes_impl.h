#ifndef AES_IMPL_H
#define AES_IMPL_H

#include <assert.h>
#include <string.h>
#include <inttypes.h>
#include <immintrin.h>
#include <wmmintrin.h>

#include "transpose.h"
#include "util.h"

#define AES_PREFERRED_WIDTH_SHIFT 3
#define RIJNDAEL256_PREFERRED_WIDTH_SHIFT 2

void rijndael192_encrypt_block(
	const rijndael192_round_keys* restrict fixed_key, block192* restrict block);
void aes_ctr_bad_iv(
	const aes_round_keys* restrict aeses,
	size_t num_keys, uint32_t num_blocks, uint32_t counter, block128* restrict output);

inline void aes_round_function(
	const aes_round_keys* restrict round_keys, block128* restrict block,
	block128* restrict after_sbox, int round)
{
	block128 state = *block;
	block128 state_after_sbox = _mm_aesenclast_si128(state, block128_set_zero());
	*after_sbox = state_after_sbox;

	if (round < AES_ROUNDS)
		state = _mm_aesenc_si128(state, round_keys->keys[round]);
	else
		state = block128_xor(state_after_sbox, round_keys->keys[round]);
	*block = state;
}

ALWAYS_INLINE void aes_round(
	const aes_round_keys* aeses, block128* state, size_t num_keys, size_t evals_per_key, int round)
{
	#ifdef __GNUC__
	_Pragma(STRINGIZE(GCC unroll (2*AES_PREFERRED_WIDTH)))
	#endif
	for (size_t i = 0; i < num_keys * evals_per_key; ++i)
		if (round == 0)
			state[i] = block128_xor(state[i], aeses[i / evals_per_key].keys[round]);
		else if (round < AES_ROUNDS)
			state[i] = _mm_aesenc_si128(state[i], aeses[i / evals_per_key].keys[round]);
		else
			state[i] = _mm_aesenclast_si128(state[i], aeses[i / evals_per_key].keys[round]);
}

ALWAYS_INLINE void rijndael256_rotate_rows_undo_128(block128* s)
{
	block128 mask = _mm_setr_epi8(
		0, -1, -1, -1,
		0,  0, -1, -1,
		0,  0, -1, -1,
		0,  0,  0, -1);
	block128 b0_blended = _mm_blendv_epi8(s[0], s[1], mask);
	block128 b1_blended = _mm_blendv_epi8(s[1], s[0], mask);

	block128 perm = _mm_setr_epi8(
		 0,  1,  6,  7,
		 4,  5, 10, 11,
		 8,  9, 14, 15,
		12, 13,  2,  3);
	s[0] = _mm_shuffle_epi8(b0_blended, perm);
	s[1] = _mm_shuffle_epi8(b1_blended, perm);
}

ALWAYS_INLINE void rijndael256_round(
	const rijndael256_round_keys* round_keys, block256* state,
	size_t num_keys, size_t evals_per_key, int round)
{
	#ifdef __GNUC__
	_Pragma(STRINGIZE(GCC unroll (2*RIJNDAEL256_PREFERRED_WIDTH)))
	#endif
	for (size_t i = 0; i < num_keys * evals_per_key; ++i)
	{
		block128 s[2], round_key[2];
		memcpy(&s[0], &state[i], sizeof(block256));
		memcpy(&round_key[0], &round_keys[i / evals_per_key].keys[round], sizeof(block256));

		if (round == 0)
		{
			s[0] = block128_xor(s[0], round_key[0]);
			s[1] = block128_xor(s[1], round_key[1]);
		}
		else if (round < AES_ROUNDS)
		{
			rijndael256_rotate_rows_undo_128(&s[0]);
			s[0] = _mm_aesenc_si128(s[0], round_key[0]);
			s[1] = _mm_aesenc_si128(s[1], round_key[1]);
		}
		else
		{
			rijndael256_rotate_rows_undo_128(&s[0]);
			s[0] = _mm_aesenclast_si128(s[0], round_key[0]);
			s[1] = _mm_aesenclast_si128(s[1], round_key[1]);
		}

		memcpy(&state[i], &s[0], sizeof(block256));
	}
}

ALWAYS_INLINE uint64_t get_iv_counter(const block128* iv)
{
	uint64_t counter;
	memcpy(&counter, ((uint8_t*) iv) + sizeof(*iv) - sizeof(counter), sizeof(counter));
	return _bswap64(counter);
}

ALWAYS_INLINE void set_iv_counter(block128* iv, uint64_t counter)
{
	counter = _bswap64(counter);
	memcpy(((uint8_t*) iv) + sizeof(*iv) - sizeof(counter), &counter, sizeof(counter));
}

ALWAYS_INLINE bool aes_is_iv_bad(block128 iv, uint64_t counter_max)
{
	return get_iv_counter(&iv) > UINT64_MAX - counter_max;
}

ALWAYS_INLINE block128 aes_add_counter_to_iv_good(block128 iv, uint32_t ctr)
{
	block128 sum = iv;
	set_iv_counter(&sum, get_iv_counter(&sum) + ctr);
	return sum;
}

ALWAYS_INLINE block128 aes_add_counter_to_iv_bad(block128 iv, uint32_t ctr)
{
	block128 iv_rev = block128_byte_reverse(iv);
	__uint128_t sum;
	memcpy(&sum, &iv_rev, sizeof(sum));
	sum += ctr;
	return block128_byte_reverse(_mm_set_epi64x((uint64_t) (sum >> 64), (uint64_t) sum));
}

ALWAYS_INLINE void aes_keygen_ctr(
	aes_round_keys* restrict aeses, const block_secpar* restrict keys, const block128* restrict ivs,
	size_t num_keys, uint32_t num_blocks, uint32_t counter, block128* restrict output)
{
	assert(num_keys <= AES_PREFERRED_WIDTH);
	assert(1 <= num_blocks && num_blocks <= 3);

	uint64_t counter64 = counter;
	uint64_t counter_max = counter64 + num_blocks;

	bool bad_iv = false;
	#ifdef __GNUC__
	_Pragma(STRINGIZE(GCC unroll (3*AES_PREFERRED_WIDTH)))
	#endif
	for (size_t i = 0; i < num_keys; ++i)
		if (aes_is_iv_bad(ivs[i], counter_max))
			bad_iv = true;

	if (bad_iv)
	{
		#ifdef __GNUC__
		_Pragma(STRINGIZE(GCC unroll (3*AES_PREFERRED_WIDTH)))
		#endif
		for (size_t l = 0; l < num_keys; ++l)
			for (uint32_t m = 0; m < num_blocks; ++m)
				output[l * num_blocks + m] = aes_add_counter_to_iv_bad(ivs[l], counter64 + m);
	}
	else
	{
		#ifdef __GNUC__
		_Pragma(STRINGIZE(GCC unroll (3*AES_PREFERRED_WIDTH)))
		#endif
		for (size_t l = 0; l < num_keys; ++l)
			for (uint32_t m = 0; m < num_blocks; ++m)
				output[l * num_blocks + m] = aes_add_counter_to_iv_good(ivs[l], counter64 + m);
	}

	static_assert(AES_PREFERRED_WIDTH <= 16, "AES_PREFERRED_WIDTH must be at most 16");
	switch(num_keys * 3 + num_blocks)
	{
#define AES_KEYGEN_SWITCH_CASE_KB(num_keys,num_blocks) \
	case (num_keys * 3 + num_blocks): \
	{ \
		void aes_keygen_impl_##num_keys##_##num_blocks( \
			aes_round_keys*, const block_secpar*, block128*); \
		aes_keygen_impl_##num_keys##_##num_blocks(aeses, keys, output); \
		break; \
	}
#define AES_KEYGEN_SWITCH_CASE_K(num_keys) \
		AES_KEYGEN_SWITCH_CASE_KB(num_keys, 1) \
		AES_KEYGEN_SWITCH_CASE_KB(num_keys, 2) \
		AES_KEYGEN_SWITCH_CASE_KB(num_keys, 3)

		AES_KEYGEN_SWITCH_CASE_K(1)
		AES_KEYGEN_SWITCH_CASE_K(2)
		AES_KEYGEN_SWITCH_CASE_K(3)
		AES_KEYGEN_SWITCH_CASE_K(4)
		AES_KEYGEN_SWITCH_CASE_K(5)
		AES_KEYGEN_SWITCH_CASE_K(6)
		AES_KEYGEN_SWITCH_CASE_K(7)
		AES_KEYGEN_SWITCH_CASE_K(8)
		AES_KEYGEN_SWITCH_CASE_K(9)
		AES_KEYGEN_SWITCH_CASE_K(10)
		AES_KEYGEN_SWITCH_CASE_K(11)
		AES_KEYGEN_SWITCH_CASE_K(12)
		AES_KEYGEN_SWITCH_CASE_K(13)
		AES_KEYGEN_SWITCH_CASE_K(14)
		AES_KEYGEN_SWITCH_CASE_K(15)
		AES_KEYGEN_SWITCH_CASE_K(16)
#undef AES_KEYGEN_SWITCH_CASE_K
#undef AES_KEYGEN_SWITCH_CASE_KB

	default:
		assert(0);
	}

	#ifdef __GNUC__
	_Pragma(STRINGIZE(GCC unroll (3 * AES_PREFERRED_WIDTH)))
	#endif
	for (size_t l = 0; l < num_keys; ++l)
		aeses[l].iv = ivs[l];
}

inline void aes_ctr(
	const aes_round_keys* restrict aeses,
	size_t num_keys, uint32_t num_blocks, uint32_t counter, block128* restrict output)
{
	assert(num_keys * num_blocks <= 3 * AES_PREFERRED_WIDTH);
	block128 state[3 * AES_PREFERRED_WIDTH];

	uint64_t counter64 = counter;
	uint64_t counter_max = counter64 + num_blocks;

	bool bad_iv = false;
	#ifdef __GNUC__
	_Pragma(STRINGIZE(GCC unroll (3*AES_PREFERRED_WIDTH)))
	#endif
	for (size_t i = 0; i < num_keys; ++i)
		if (aes_is_iv_bad(aeses[i].iv, counter_max))
			bad_iv = true;

	if (bad_iv)
	{
		#ifdef __GNUC__
		_Pragma(STRINGIZE(GCC unroll (3*AES_PREFERRED_WIDTH)))
		#endif
		for (size_t l = 0; l < num_keys; ++l)
			for (uint32_t m = 0; m < num_blocks; ++m)
				state[l * num_blocks + m] = aes_add_counter_to_iv_bad(aeses[l].iv, counter64 + m);
	}
	else
	{
		#ifdef __GNUC__
		_Pragma(STRINGIZE(GCC unroll (3*AES_PREFERRED_WIDTH)))
		#endif
		for (size_t l = 0; l < num_keys; ++l)
			for (uint32_t m = 0; m < num_blocks; ++m)
				state[l * num_blocks + m] = aes_add_counter_to_iv_good(aeses[l].iv, counter64 + m);
	}

	aes_round(aeses, state, num_keys, num_blocks, 0);
	for (int round = 1; round < AES_ROUNDS; ++round)
		aes_round(aeses, state, num_keys, num_blocks, round);
	aes_round(aeses, state, num_keys, num_blocks, AES_ROUNDS);

	memcpy(output, state, num_keys * num_blocks * sizeof(block128));
}

inline void aes_fixed_key_ctr(
	const aes_round_keys* restrict fixed_key, const block128* restrict keys,
	size_t num_keys, uint32_t num_blocks, uint32_t counter, block128* restrict output)
{
	assert(num_keys * num_blocks <= 3 * AES_PREFERRED_WIDTH);
	block128 state[3 * AES_PREFERRED_WIDTH];

	for (size_t l = 0; l < num_keys; ++l)
		for (uint32_t m = 0; m < num_blocks; ++m)
			state[l * num_blocks + m] = block128_xor(block128_set_low32(counter + m), keys[l]);

	aes_round(fixed_key, state, 1, num_keys * num_blocks, 0);
	for (int round = 1; round < AES_ROUNDS; ++round)
		aes_round(fixed_key, state, 1, num_keys * num_blocks, round);
	aes_round(fixed_key, state, 1, num_keys * num_blocks, AES_ROUNDS);

	for (size_t l = 0; l < num_keys; ++l)
		for (uint32_t m = 0; m < num_blocks; ++m)
			output[l * num_blocks + m] = block128_xor(state[l * num_blocks + m], keys[l]);
}

inline void rijndael256_fixed_key_ctr(
	const rijndael256_round_keys* restrict fixed_key, const block256* restrict keys,
	size_t num_keys, uint32_t num_blocks, uint32_t counter, block256* restrict output)
{
	assert(num_keys * num_blocks <= 8 * RIJNDAEL256_PREFERRED_WIDTH);
	block256 state[8 * RIJNDAEL256_PREFERRED_WIDTH];

	for (size_t l = 0; l < num_keys; ++l)
		for (uint32_t m = 0; m < num_blocks; ++m)
			state[l * num_blocks + m] = block256_xor(block256_set_low32(counter + m), keys[l]);

	rijndael256_round(fixed_key, state, 1, num_keys * num_blocks, 0);
	for (int round = 1; round < RIJNDAEL256_ROUNDS; ++round)
		rijndael256_round(fixed_key, state, 1, num_keys * num_blocks, round);
	rijndael256_round(fixed_key, state, 1, num_keys * num_blocks, RIJNDAEL256_ROUNDS);

	for (size_t l = 0; l < num_keys; ++l)
		for (uint32_t m = 0; m < num_blocks; ++m)
			output[l * num_blocks + m] = block256_xor(state[l * num_blocks + m], keys[l]);
}

inline void rijndael192_fixed_key_ctr(
	const rijndael192_round_keys* restrict fixed_key, const block192* restrict keys,
	size_t num_keys, uint32_t num_blocks, uint32_t counter, block192* restrict output)
{
	for (size_t l = 0; l < num_keys; ++l)
	{
		for (uint32_t m = 0; m < num_blocks; ++m)
		{
			block192 state = block192_xor(block192_set_low32(counter + m), keys[l]);
			rijndael192_encrypt_block(fixed_key, &state);
			output[l * num_blocks + m] = state;
		}
	}
}

#endif
