#ifndef TRANSPOSE_IMPL_H
#define TRANSPOSE_IMPL_H

#define TRANSPOSE_BITS_ROWS_SHIFT 8

ALWAYS_INLINE void transpose4x4_32(block128* output, const block128* input)
{
	block128 a0b0a1b1 = _mm_unpacklo_epi32(input[0], input[1]);
	block128 a2b2a3b3 = _mm_unpackhi_epi32(input[0], input[1]);
	block128 c0d0c1d1 = _mm_unpacklo_epi32(input[2], input[3]);
	block128 c2d2c3d3 = _mm_unpackhi_epi32(input[2], input[3]);
	output[0] = _mm_unpacklo_epi64(a0b0a1b1, c0d0c1d1);
	output[1] = _mm_unpackhi_epi64(a0b0a1b1, c0d0c1d1);
	output[2] = _mm_unpacklo_epi64(a2b2a3b3, c2d2c3d3);
	output[3] = _mm_unpackhi_epi64(a2b2a3b3, c2d2c3d3);
}

ALWAYS_INLINE void transpose4x2_32(block128* output, block128 input0, block128 input1)
{
	output[0] = shuffle_2x4xepi32(input0, input1, 0x88);
	output[1] = shuffle_2x4xepi32(input0, input1, 0xdd);
}

ALWAYS_INLINE block256 transpose2x2_64(block256 input)
{
	return _mm256_permute4x64_epi64(input, 0xd8);
}

ALWAYS_INLINE void transpose2x2_128(block256* output, block256 input0, block256 input1)
{
	block256 a0b0 = _mm256_permute2x128_si256(input0, input1, 0x20);
	block256 a1b1 = _mm256_permute2x128_si256(input0, input1, 0x31);
	output[0] = a0b0;
	output[1] = a1b1;
}

ALWAYS_INLINE block128 transpose8x2_8(block128 x)
{
	block128 shuffle = _mm_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15);
	return _mm_shuffle_epi8(x, shuffle);
}

ALWAYS_INLINE block128 transpose2x8_8(block128 x)
{
	block128 shuffle = _mm_setr_epi8(0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15);
	return _mm_shuffle_epi8(x, shuffle);
}

ALWAYS_INLINE block256 transpose2x8_16(block256 x)
{
	block128 row0 = _mm256_extracti128_si256(x, 0);
	block128 row1 = _mm256_extracti128_si256(x, 1);
	block128 out_lo = _mm_unpacklo_epi16(row0, row1);
	block128 out_hi = _mm_unpackhi_epi16(row0, row1);
	return block256_from_2_block128(out_lo, out_hi);
}

ALWAYS_INLINE block256 transpose4x8_8(block256 x)
{
	block256 shuffle_in_lanes = _mm256_setr_epi8(
		0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15,
		0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15
	);
	x = _mm256_shuffle_epi8(x, shuffle_in_lanes);
	return transpose2x8_16(x);
}

ALWAYS_INLINE uint64_t transpose8x8_1(uint64_t x)
{
	#ifdef __GNUC__
	#pragma GCC unroll (3)
	#endif
	for (int i = 0; i < 3; ++i)
	{
		uint32_t mask = 0xf0f0f0f0;
		for (int k = 1; k >= i; --k)
			mask ^= mask >> (1 << k);

		int shift = 32 - (1 << i);
		uint64_t diff = x ^ (x >> shift);
		diff &= mask;
		x ^= diff;
		x ^= diff << shift;
	}

	return x;
}

ALWAYS_INLINE block128 transpose16x8_1(block128 x)
{
	uint64_t x_arr[] = {(uint64_t) _mm_extract_epi64(x, 0), (uint64_t) _mm_extract_epi64(x, 1)};
	#ifdef __GNUC__
	#pragma GCC unroll (2)
	#endif
	for (int i = 0; i < 2; ++i)
		x_arr[i] = transpose8x8_1(x_arr[i]);
	x = _mm_set_epi64x(x_arr[1], x_arr[0]);

	return transpose2x8_8(x);
}

ALWAYS_INLINE block256 transpose32x8_1(block256 x)
{
	uint64_t x_arr[] = {
		(uint64_t) _mm256_extract_epi64(x, 0), (uint64_t) _mm256_extract_epi64(x, 1),
		(uint64_t) _mm256_extract_epi64(x, 2), (uint64_t) _mm256_extract_epi64(x, 3)
	};
	#ifdef __GNUC__
	#pragma GCC unroll (4)
	#endif
	for (int i = 0; i < 4; ++i)
		x_arr[i] = transpose8x8_1(x_arr[i]);
	x = _mm256_set_epi64x(x_arr[3], x_arr[2], x_arr[1], x_arr[0]);

	return transpose4x8_8(x);
}

ALWAYS_INLINE block192 transpose3x8_8(block192 x)
{
	block128 x01 = _mm_set_epi64x(x.data[1], x.data[0]);
	block128 shuffle = _mm_setr_epi8(0, 8, -1, 1, 9, -1, 2, 10, -1, 3, 11, -1, 4, 12, -1, 5);
	block128 out0 = _mm_shuffle_epi8(x01, shuffle);

	shuffle = _mm_setr_epi8(-1, -1, 0, -1, -1, 1, -1, -1, 2, -1, -1, 3, -1, -1, 4, -1);
	out0 = _mm_xor_si128(out0, _mm_shuffle_epi8(block128_set_low64(x.data[2]), shuffle));

	block128 tmp = _mm_insert_epi32(x01, x.data[2] >> 32, 0);
	shuffle = _mm_setr_epi8(13, 1, 6, 14, 2, 7, 15, 3, 0, 0, 0, 0, 0, 0, 0, 0);
	block128 out1 = _mm_shuffle_epi8(tmp, shuffle);

	block192 out;
	out.data[0] = _mm_extract_epi64(out0, 0);
	out.data[1] = _mm_extract_epi64(out0, 1);
	out.data[2] = _mm_extract_epi64(out1, 0);
	return out;
}

ALWAYS_INLINE block192 transpose24x8_1(block192 x)
{
	#ifdef __GNUC__
	#pragma GCC unroll (4)
	#endif
	for (int i = 0; i < 3; ++i)
		x.data[i] = transpose8x8_1(x.data[i]);

	return transpose3x8_8(x);
}

#endif
