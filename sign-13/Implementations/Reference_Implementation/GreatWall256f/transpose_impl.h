#ifndef TRANSPOSE_IMPL_H
#define TRANSPOSE_IMPL_H

#define TRANSPOSE_BITS_ROWS_SHIFT 8

ALWAYS_INLINE void transpose4x4_32(block128* output, const block128* input)
{
	uint32_t in[4][4];
	uint32_t out[4][4];
	memcpy(in, input, sizeof(in));
	for (size_t r = 0; r < 4; ++r)
		for (size_t c = 0; c < 4; ++c)
			out[c][r] = in[r][c];
	memcpy(output, out, sizeof(out));
}

ALWAYS_INLINE void transpose4x2_32(block128* output, block128 input0, block128 input1)
{
	uint32_t in[4][2];
	uint32_t out[2][4];
	memcpy(&in[0], &input0, sizeof(input0));
	memcpy(&in[2], &input1, sizeof(input1));
	for (size_t r = 0; r < 4; ++r)
		for (size_t c = 0; c < 2; ++c)
			out[c][r] = in[r][c];
	memcpy(output, out, sizeof(out));
}

ALWAYS_INLINE block256 transpose2x2_64(block256 input)
{
	block256 out = {{input.data[0], input.data[2], input.data[1], input.data[3]}};
	return out;
}

ALWAYS_INLINE void transpose2x2_128(block256* output, block256 input0, block256 input1)
{
	output[0].data[0] = input0.data[0];
	output[0].data[1] = input0.data[1];
	output[0].data[2] = input1.data[0];
	output[0].data[3] = input1.data[1];
	output[1].data[0] = input0.data[2];
	output[1].data[1] = input0.data[3];
	output[1].data[2] = input1.data[2];
	output[1].data[3] = input1.data[3];
}

ALWAYS_INLINE block128 transpose8x2_8(block128 x)
{
	const uint8_t* in = (const uint8_t*)&x;
	block128 out = block128_set_zero();
	uint8_t* dst = (uint8_t*)&out;
	const unsigned int order[16] = {0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15};
	for (size_t i = 0; i < 16; ++i)
		dst[i] = in[order[i]];
	return out;
}

ALWAYS_INLINE block128 transpose2x8_8(block128 x)
{
	const uint8_t* in = (const uint8_t*)&x;
	block128 out = block128_set_zero();
	uint8_t* dst = (uint8_t*)&out;
	const unsigned int order[16] = {0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15};
	for (size_t i = 0; i < 16; ++i)
		dst[i] = in[order[i]];
	return out;
}

ALWAYS_INLINE block256 transpose2x8_16(block256 x)
{
	uint16_t in[2][8];
	uint16_t out[8][2];
	memcpy(in, &x, sizeof(in));
	for (size_t r = 0; r < 2; ++r)
		for (size_t c = 0; c < 8; ++c)
			out[c][r] = in[r][c];
	block256 y;
	memcpy(&y, out, sizeof(y));
	return y;
}

ALWAYS_INLINE block256 transpose4x8_8(block256 x)
{
	uint8_t in[4][8];
	uint8_t out[8][4];
	memcpy(in, &x, sizeof(in));
	for (size_t r = 0; r < 4; ++r)
		for (size_t c = 0; c < 8; ++c)
			out[c][r] = in[r][c];
	block256 y;
	memcpy(&y, out, sizeof(y));
	return y;
}

ALWAYS_INLINE uint64_t transpose8x8_1(uint64_t x)
{
	uint64_t out = 0;
	for (size_t r = 0; r < 8; ++r)
		for (size_t c = 0; c < 8; ++c)
			out |= ((x >> (8 * r + c)) & 1u) << (8 * c + r);
	return out;
}

ALWAYS_INLINE block128 transpose16x8_1(block128 x)
{
	block128 out;
	out.data[0] = transpose8x8_1(x.data[0]);
	out.data[1] = transpose8x8_1(x.data[1]);
	return transpose2x8_8(out);
}

ALWAYS_INLINE block256 transpose32x8_1(block256 x)
{
	block256 out;
	for (size_t i = 0; i < 4; ++i)
		out.data[i] = transpose8x8_1(x.data[i]);
	return transpose4x8_8(out);
}

ALWAYS_INLINE block192 transpose3x8_8(block192 x)
{
	uint8_t in[3][8];
	uint8_t out_bytes[24] = {0};
	memcpy(in, &x, sizeof(in));
	for (size_t r = 0; r < 3; ++r)
		for (size_t c = 0; c < 8; ++c)
			out_bytes[c * 3 + r] = in[r][c];
	block192 out;
	memcpy(&out, out_bytes, sizeof(out));
	return out;
}

ALWAYS_INLINE block192 transpose24x8_1(block192 x)
{
	for (size_t i = 0; i < 3; ++i)
		x.data[i] = transpose8x8_1(x.data[i]);
	return transpose3x8_8(x);
}

#endif
