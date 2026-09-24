#include "small_vole.h"
#include <string.h>
#include <stdbool.h>

#include "util.h"

#define COL_LEN VOLE_COL_BLOCKS



static ALWAYS_INLINE void xor_reduce(vole_block* in_out)
{
	#ifdef __GNUC__
	#pragma GCC unroll (5)
	#endif
	for (size_t i = 0; i < VOLE_WIDTH_SHIFT; i++)
	{
		size_t stride = 1 << i;
		#ifdef __GNUC__
		#pragma GCC unroll (32)
		#endif
		for (size_t j = 0; j < VOLE_WIDTH; j += 2 * stride)
		{
			#ifdef __GNUC__
			#pragma GCC unroll (5)
			#endif
			for (size_t d = 0; d <= i; ++d)
				in_out[j + d] = vole_block_xor(in_out[j + d], in_out[j + d + stride]);
			in_out[j + i + 1] = in_out[j + stride];
		}
	}
}

static ALWAYS_INLINE void convert_prg_output(
	vole_block* prg_output, const prg_vole_block* raw_prg_output, size_t blocks)
{
#if PRG_VOLE_BLOCKS == 2
	#ifdef __GNUC__
	#pragma GCC unroll (32)
	#endif
	for (size_t i = 0; i < blocks; ++i)
		prg_output[i] = block256_from_2_block128(raw_prg_output[PRG_VOLE_BLOCKS * i], raw_prg_output[PRG_VOLE_BLOCKS * i + 1]);
#else

	memcpy(prg_output, raw_prg_output, blocks * sizeof(vole_block));
#endif
}

static ALWAYS_INLINE void process_prg_output(
	bool ignore_0, size_t j, unsigned int output_col, vole_block* accum, vole_block* vq,
	const prg_vole_block* raw_prg_output)
{
	vole_block prg_output[VOLE_WIDTH];
	if (!ignore_0)
		convert_prg_output(prg_output, raw_prg_output, VOLE_WIDTH);
	else
	{
		prg_output[0] = vole_block_set_zero();
		convert_prg_output(&prg_output[1], raw_prg_output, VOLE_WIDTH - 1);
	}

	xor_reduce(prg_output);

	if (!ignore_0)
		accum[j] = vole_block_xor(accum[j], prg_output[0]);

	for (size_t col = 0; col < VOLE_WIDTH_SHIFT; ++col)
		vq[COL_LEN * col + j] = vole_block_xor(vq[COL_LEN * col + j], prg_output[col + 1]);

	if (!ignore_0)
		vq[COL_LEN * output_col + j] = vole_block_xor(vq[COL_LEN * output_col + j], accum[j]);
}

static ALWAYS_INLINE void vole(
	bool receiver, unsigned int k,
	const block_secpar* restrict keys, block128 iv, const prg_vole_fixed_key* restrict fixed_key,
	const vole_block* restrict u_or_c_in, vole_block* restrict vq, vole_block* restrict c_out,
	const uint8_t* restrict delta)
{
	vole_block accum[COL_LEN];
	memset(&accum[0], 0, COL_LEN * sizeof(vole_block));

	if (receiver && u_or_c_in)
	{
		vole_block* q_ptr = vq;
		for (unsigned int col = 0; col < k; ++col)
			for (size_t j = 0; j < COL_LEN; ++j)
				*(q_ptr++) = vole_block_and(u_or_c_in[j], vole_block_set_all_8(delta[col]));
	}
	else
		memset(&vq[0], 0, COL_LEN * k * sizeof(vole_block));

	size_t i = 0;
	if (receiver)
	{
		prg_vole_key prgs[VOLE_WIDTH - 1];
		prg_vole_iv ivs[VOLE_WIDTH - 1];
		prg_vole_block raw_prg_output[(VOLE_WIDTH - 1) * PRG_VOLE_BLOCKS];

		#ifdef __GNUC__
		_Pragma(STRINGIZE(GCC unroll (VOLE_WIDTH)))
		#endif
		for (size_t i = 0; i < VOLE_WIDTH - 1; ++i)
			memcpy(&ivs[i], &iv, sizeof(ivs[i]));

		prg_vole_init(prgs, fixed_key, &keys[1], ivs, VOLE_WIDTH - 1, PRG_VOLE_BLOCKS, 0, raw_prg_output);
		process_prg_output(true, 0, 0, accum, vq, raw_prg_output);

		for (size_t j = 1; j < COL_LEN; ++j)
		{
			prg_vole_gen(prgs, fixed_key, VOLE_WIDTH - 1, PRG_VOLE_BLOCKS, j * PRG_VOLE_BLOCKS, raw_prg_output);
			process_prg_output(true, j, 0, accum, vq, raw_prg_output);
		}

		i = VOLE_WIDTH;
	}

	for (; i < (size_t) 1 << k; i += VOLE_WIDTH)
	{
		prg_vole_key prgs[VOLE_WIDTH];
		prg_vole_iv ivs[VOLE_WIDTH];
		prg_vole_block raw_prg_output[VOLE_WIDTH * PRG_VOLE_BLOCKS];

		#ifdef __GNUC__
		_Pragma(STRINGIZE(GCC unroll (VOLE_WIDTH)))
		#endif
		for (size_t i = 0; i < VOLE_WIDTH; ++i)
			memcpy(&ivs[i], &iv, sizeof(ivs[i]));

		unsigned int output_col = count_trailing_zeros((i + VOLE_WIDTH) | (1 << (k - 1)));

		prg_vole_init(prgs, fixed_key, &keys[i], ivs, VOLE_WIDTH, PRG_VOLE_BLOCKS, 0, raw_prg_output);
		process_prg_output(false, 0, output_col, accum, vq, raw_prg_output);

		for (size_t j = 1; j < COL_LEN; ++j)
		{
			prg_vole_gen(prgs, fixed_key, VOLE_WIDTH, PRG_VOLE_BLOCKS, j * PRG_VOLE_BLOCKS, raw_prg_output);
			process_prg_output(false, j, output_col, accum, vq, raw_prg_output);
		}
	}

	if (!receiver)
	{
		if (u_or_c_in)
			for (size_t j = 0; j < COL_LEN; ++j)
				c_out[j] = vole_block_xor(u_or_c_in[j], accum[j]);
		else
			memcpy(c_out, accum, sizeof(accum));
	}
}

void vole_sender(
	unsigned int k, const block_secpar* restrict keys,
	block128 iv, const prg_vole_fixed_key* restrict fixed_key,
	const vole_block* restrict u, vole_block* restrict v, vole_block* restrict c)
{
	vole(false, k, keys, iv, fixed_key, u, v, c, NULL);
}

void vole_receiver(
	unsigned int k, const block_secpar* restrict keys,
	block128 iv, const prg_vole_fixed_key* restrict fixed_key,
	const vole_block* restrict c, vole_block* restrict q,
	const uint8_t* restrict delta)
{
	vole(true, k, keys, iv, fixed_key, c, q, NULL, delta);
}

void vole_receiver_apply_correction(
	size_t row_blocks, size_t cols,
	const vole_block* restrict c, vole_block* restrict q, const uint8_t* restrict delta)
{
	for (unsigned int col = 0; col < cols; ++col)
		for (size_t j = 0; j < row_blocks; ++j)
			q[col * COL_LEN + j] = vole_block_xor(q[col * COL_LEN + j],
				vole_block_and(c[j], vole_block_set_all_8(delta[col])));
}

extern inline size_t vole_permute_key_index(size_t i);
extern inline size_t vole_permute_key_index_inv(size_t i);
extern inline size_t vole_permute_inv_increment(size_t i, size_t offset);
extern inline void vole_fixed_key_init(prg_vole_fixed_key* fixed_key, block_secpar iv);
