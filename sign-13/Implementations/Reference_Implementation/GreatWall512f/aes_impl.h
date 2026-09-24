#ifndef AES_IMPL_H
#define AES_IMPL_H

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "util.h"

#define AES_PREFERRED_WIDTH_SHIFT 3
#define RIJNDAEL256_PREFERRED_WIDTH_SHIFT 2

void aes_encrypt_block_portable(
	const aes_round_keys* restrict round_keys, block128 in, block128* restrict out);
void aes_round_function_portable(
	const aes_round_keys* restrict round_keys, block128* restrict block,
	block128* restrict after_sbox, int round);
block128 aes_add_counter_to_iv_portable(block128 iv, uint32_t ctr);

void aes_keygen(aes_round_keys* round_keys, block_secpar key);

void rijndael192_encrypt_block(
	const rijndael192_round_keys* restrict fixed_key, block192* restrict block);
void rijndael256_encrypt_block(
	const rijndael256_round_keys* restrict fixed_key, block256* restrict block);

inline void aes_round_function(
	const aes_round_keys* restrict round_keys, block128* restrict block,
	block128* restrict after_sbox, int round)
{
	aes_round_function_portable(round_keys, block, after_sbox, round);
}

ALWAYS_INLINE void aes_keygen_ctr(
	aes_round_keys* restrict aeses, const block_secpar* restrict keys, const block128* restrict ivs,
	size_t num_keys, uint32_t num_blocks, uint32_t counter, block128* restrict output)
{
	assert(1 <= num_blocks && num_blocks <= 3);

	for (size_t key_idx = 0; key_idx < num_keys; ++key_idx) {
		aes_keygen(&aeses[key_idx], keys[key_idx]);
		aeses[key_idx].iv = ivs[key_idx];
		for (uint32_t block_idx = 0; block_idx < num_blocks; ++block_idx) {
			const block128 ctr_block =
				aes_add_counter_to_iv_portable(ivs[key_idx], counter + block_idx);
			aes_encrypt_block_portable(
				&aeses[key_idx], ctr_block, &output[key_idx * num_blocks + block_idx]);
		}
	}
}

inline void aes_ctr(
	const aes_round_keys* restrict aeses,
	size_t num_keys, uint32_t num_blocks, uint32_t counter, block128* restrict output)
{
	for (size_t key_idx = 0; key_idx < num_keys; ++key_idx) {
		for (uint32_t block_idx = 0; block_idx < num_blocks; ++block_idx) {
			const block128 ctr_block =
				aes_add_counter_to_iv_portable(aeses[key_idx].iv, counter + block_idx);
			aes_encrypt_block_portable(
				&aeses[key_idx], ctr_block, &output[key_idx * num_blocks + block_idx]);
		}
	}
}

inline void aes_fixed_key_ctr(
	const aes_round_keys* restrict fixed_key, const block128* restrict keys,
	size_t num_keys, uint32_t num_blocks, uint32_t counter, block128* restrict output)
{
	for (size_t key_idx = 0; key_idx < num_keys; ++key_idx) {
		for (uint32_t block_idx = 0; block_idx < num_blocks; ++block_idx) {
			block128 in = block128_xor(block128_set_low32(counter + block_idx), keys[key_idx]);
			aes_encrypt_block_portable(fixed_key, in, &in);
			output[key_idx * num_blocks + block_idx] = block128_xor(in, keys[key_idx]);
		}
	}
}

inline void rijndael192_fixed_key_ctr(
	const rijndael192_round_keys* restrict fixed_key, const block192* restrict keys,
	size_t num_keys, uint32_t num_blocks, uint32_t counter, block192* restrict output)
{
	for (size_t key_idx = 0; key_idx < num_keys; ++key_idx) {
		for (uint32_t block_idx = 0; block_idx < num_blocks; ++block_idx) {
			block192 in = block192_xor(block192_set_low32(counter + block_idx), keys[key_idx]);
			rijndael192_encrypt_block(fixed_key, &in);
			output[key_idx * num_blocks + block_idx] = block192_xor(in, keys[key_idx]);
		}
	}
}

inline void rijndael256_fixed_key_ctr(
	const rijndael256_round_keys* restrict fixed_key, const block256* restrict keys,
	size_t num_keys, uint32_t num_blocks, uint32_t counter, block256* restrict output)
{
	for (size_t key_idx = 0; key_idx < num_keys; ++key_idx) {
		for (uint32_t block_idx = 0; block_idx < num_blocks; ++block_idx) {
			block256 in = block256_xor(block256_set_low32(counter + block_idx), keys[key_idx]);
			rijndael256_encrypt_block(fixed_key, &in);
			output[key_idx * num_blocks + block_idx] = block256_xor(in, keys[key_idx]);
		}
	}
}

#endif
