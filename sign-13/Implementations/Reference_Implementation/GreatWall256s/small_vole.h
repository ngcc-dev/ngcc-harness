#ifndef SMALL_VOLE_H
#define SMALL_VOLE_H

#include <assert.h>

#include "config.h"
#include "block.h"
#include "aes.h"
#include "vole_params.h"

void vole_sender(
	unsigned int k, const block_secpar* restrict keys,
	block128 iv, const prg_vole_fixed_key* restrict fixed_key,
	const vole_block* restrict u, vole_block* restrict v, vole_block* restrict c);

void vole_receiver(
	unsigned int k, const block_secpar* restrict keys,
	block128 iv, const prg_vole_fixed_key* restrict fixed_key,
	const vole_block* restrict c, vole_block* restrict q,
	const uint8_t* restrict delta);

void vole_receiver_apply_correction(
	size_t row_blocks, size_t cols,
	const vole_block* restrict c, vole_block* restrict q, const uint8_t* restrict delta);

inline size_t vole_permute_key_index(size_t i)
{
	return i ^ ((i >> 1) & -VOLE_WIDTH);
}

inline size_t vole_permute_key_index_inv(size_t i)
{
	size_t j = i;

	#ifdef __GNUC__
	#pragma GCC unroll (5)
	#endif
	for (unsigned int shift = 1; shift < VOLE_MAX_K - VOLE_WIDTH_SHIFT; shift <<= 1)
		j ^= (j >> shift);

	return (j & -VOLE_WIDTH) + (i % VOLE_WIDTH);
}

inline size_t vole_permute_inv_increment(size_t i, size_t offset)
{
	static_assert(VOLE_MAX_K < 32, "");

	size_t diff_in = i ^ (i + offset);
	size_t diff_out_even = diff_in & (0x55555555 | (VOLE_WIDTH - 1));
	size_t diff_out_odd = diff_in & (0xAAAAAAAA | (VOLE_WIDTH - 1));
	return diff_out_odd > diff_out_even ? diff_out_odd : diff_out_even;
}

inline void vole_fixed_key_init(prg_vole_fixed_key* fixed_key, block_secpar iv)
{
	(void) fixed_key, (void) iv;
#if defined(PRG_RIJNDAEL_EVEN_MANSOUR)
	rijndael_keygen(fixed_key, iv);
#endif
}

#endif
