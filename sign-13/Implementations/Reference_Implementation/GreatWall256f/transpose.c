#include "transpose.h"

#include <stdint.h>
#include <string.h>

void transpose_secpar(const void* input, void* output, size_t stride, size_t rows)
{
	const uint8_t* in = input;
	uint8_t* out = output;
	memset(out, 0, rows * sizeof(block_secpar));
	for (size_t row = 0; row < rows; ++row) {
		for (size_t col = 0; col < SECURITY_PARAM; ++col) {
			const uint8_t bit = (in[col * stride + row / 8] >> (row & 7)) & 1u;
			out[row * sizeof(block_secpar) + col / 8] |= (uint8_t)(bit << (col & 7));
		}
	}
}
