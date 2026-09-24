#include "transpose.h"

#include <assert.h>
#include <string.h>
#include "vole_params.h"

#if SECURITY_PARAM == 128
#define TRANSPOSE_BITS_COLS_SHIFT 7
#else
#define TRANSPOSE_BITS_COLS_SHIFT 8
#endif

#define TRANSPOSE_BITS_COLS (1 << TRANSPOSE_BITS_COLS_SHIFT)

#define TRANSPOSE_CHUNK_SHIFT 4
#define TRANSPOSE_CHUNK_SIZE (1 << TRANSPOSE_CHUNK_SHIFT)

static ALWAYS_INLINE void transposeVxN_8_chunk(
	block256* restrict matrix, unsigned char* restrict output_mat,
	unsigned int i_start, unsigned int i, size_t j_chunk)
{
	size_t bit = 1 << (i % TRANSPOSE_BITS_COLS_SHIFT);
	j_chunk = rotate_left(j_chunk, i_start, TRANSPOSE_BITS_COLS_SHIFT);

	#ifdef __GNUC__
	_Pragma(STRINGIZE(GCC unroll (TRANSPOSE_CHUNK_SIZE)))
	#endif
	for (size_t j_unrotated = 0; j_unrotated < TRANSPOSE_CHUNK_SIZE; ++j_unrotated)
	{
		size_t j_in_chunk = rotate_left(j_unrotated, i_start, TRANSPOSE_BITS_COLS_SHIFT);

		if (j_in_chunk & bit)
			continue;

		size_t j = j_chunk + j_in_chunk;

		block256 x = matrix[j];
		block256 y = matrix[j + bit];
		block256 out[2];

		switch (i)
		{
		case 3:
			out[0] = _mm256_unpacklo_epi8(x, y);
			out[1] = _mm256_unpackhi_epi8(x, y);
			break;
		case 4:
			out[0] = _mm256_unpacklo_epi16(x, y);
			out[1] = _mm256_unpackhi_epi16(x, y);
			break;
		case 5:
			out[0] = _mm256_unpacklo_epi32(x, y);
			out[1] = _mm256_unpackhi_epi32(x, y);
			break;
		case 6:
			out[0] = _mm256_unpacklo_epi64(x, y);
			out[1] = _mm256_unpackhi_epi64(x, y);
			break;
		case 7:
			transpose2x2_128(&out[0], x, y);
			break;
		}

		if (i < 7)
		{
			matrix[j] = out[0];
			matrix[j + bit] = out[1];
		}
		else
		{

			j = rotate_right(j, 8, TRANSPOSE_BITS_COLS_SHIFT);

			size_t reverse_mask = 0xf << (TRANSPOSE_BITS_COLS_SHIFT - 5);
			size_t shift2_mask = 0xc << (TRANSPOSE_BITS_COLS_SHIFT - 5);
			size_t shift1_mask = 0xa << (TRANSPOSE_BITS_COLS_SHIFT - 5);
			size_t reversed = j & reverse_mask;
			reversed = ((reversed & shift2_mask) >> 2) | ((reversed << 2) & shift2_mask);
			reversed = ((reversed & shift1_mask) >> 1) | ((reversed << 1) & shift1_mask);
			j = (j & ~reverse_mask) | reversed;


			size_t used_bytes = SECURITY_PARAM == 192 ? sizeof(block192) : sizeof(block256);
			memcpy(output_mat + j * used_bytes, &out[0], used_bytes);
			memcpy(output_mat + (j + TRANSPOSE_BITS_COLS / 2) * used_bytes, &out[1], used_bytes);
		}
	}
}

static ALWAYS_INLINE void transposeVxN_block8x8(block256* matrix)
{
	#ifdef __GNUC__
	#pragma GCC unroll (3)
	#endif
	for (int i = 0; i < 3; ++i)
	{
		size_t bit = 1 << i;

		#ifdef __GNUC__
		_Pragma(STRINGIZE(GCC unroll (TRANSPOSE_CHUNK_SIZE)))
		#endif
		for (size_t j = 0; j < TRANSPOSE_CHUNK_SIZE; ++j)
		{
			if (j & bit)
				continue;

			block256 x = matrix[j];
			block256 y = matrix[j + bit];

			unsigned char mask = 0xf0;
			for (int k = 1; k >= (int) i; --k)
				mask ^= mask >> (1 << k);

			block256 diff = _mm256_xor_si256(x, _mm256_slli_epi16(y, 1 << i));
			diff = _mm256_and_si256(diff, _mm256_set1_epi8(mask));
			matrix[j] = _mm256_xor_si256(x, diff);
			matrix[j + bit] = _mm256_xor_si256(y, _mm256_srli_epi16(diff, 1 << i));
		}
	}
}

static void transposeVxN_blocks8x8(const unsigned char* input, block256* output, size_t stride)
{
	static_assert(TRANSPOSE_CHUNK_SIZE >= 8, "");
	static_assert(SECURITY_PARAM % TRANSPOSE_CHUNK_SIZE == 0, "");

	for (size_t j = 0; j < SECURITY_PARAM; j += TRANSPOSE_CHUNK_SIZE)
	{
		#ifdef __GNUC__
		_Pragma(STRINGIZE(GCC unroll (TRANSPOSE_CHUNK_SIZE)))
		#endif
		for (size_t j_low = 0; j_low < TRANSPOSE_CHUNK_SIZE; ++j_low)
			memcpy(&output[j + j_low], &input[stride * (j + j_low)], sizeof(block256));

		transposeVxN_block8x8(&output[j]);
	}

	if (SECURITY_PARAM == 192)
		memset(&output[SECURITY_PARAM], 0, (TRANSPOSE_BITS_COLS - SECURITY_PARAM) * sizeof(block256));
}

static ALWAYS_INLINE void transposeVxN(
	const unsigned char* restrict input, unsigned char* restrict output, size_t stride)
{
	block256 tmp[TRANSPOSE_BITS_COLS];

	transposeVxN_blocks8x8(input, tmp, stride);


	#ifdef __GNUC__
	#pragma GCC unroll (8)
	#endif
	for (unsigned int i_start = 3; i_start < 8; i_start += TRANSPOSE_CHUNK_SHIFT)
	{
		unsigned int i_end =
			i_start + TRANSPOSE_CHUNK_SHIFT < 8 ? i_start + TRANSPOSE_CHUNK_SHIFT : 8;
		for (size_t j_chunk = 0; j_chunk < TRANSPOSE_BITS_COLS; j_chunk += TRANSPOSE_CHUNK_SIZE)
		{
			#ifdef __GNUC__
			_Pragma(STRINGIZE(GCC unroll (TRANSPOSE_CHUNK_SHIFT)))
			#endif
			for (unsigned int i = i_start; i < i_end; ++i)
				transposeVxN_8_chunk(tmp, output, i_start, i, j_chunk);
		}
	}
}

void transpose_secpar(const void* input, void* output, size_t stride, size_t rows)
{
	const uint8_t* in = input;
	uint8_t* out = output;
#if SECURITY_PARAM == 512
	memset(out, 0, rows * sizeof(block_secpar));
	for (size_t row = 0; row < rows; ++row)
		for (size_t col = 0; col < SECURITY_PARAM; ++col) {
			const uint8_t bit = (in[col * stride + row / 8] >> (row & 7)) & 1;
			out[row * sizeof(block_secpar) + col / 8] |= bit << (col & 7);
		}
#else
	for (size_t i = 0; i < rows / 8; i += TRANSPOSE_BITS_ROWS / 8)
		transposeVxN(in + i, out + i * SECURITY_PARAM, stride);
#endif
}
