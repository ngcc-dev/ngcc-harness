#ifndef TRANSPOSE_H
#define TRANSPOSE_H

#include "config.h"
#include "block.h"
#include "util.h"

#include "transpose_impl.h"

#define TRANSPOSE_BITS_ROWS (1 << TRANSPOSE_BITS_ROWS_SHIFT)



ALWAYS_INLINE void transpose4x4_32(block128* output, const block128* input);

ALWAYS_INLINE void transpose4x2_32(block128* output, block128 input0, block128 input1);

ALWAYS_INLINE block256 transpose2x2_64(block256 input);

ALWAYS_INLINE void transpose2x2_128(block256* output, block256 input0, block256 input1);

void transpose_secpar(const void* input, void* output, size_t stride, size_t rows);

ALWAYS_INLINE block128 transpose8x2_8(block128 x);

ALWAYS_INLINE block128 transpose2x8_8(block128 x);

ALWAYS_INLINE block192 transpose3x8_8(block192 x);

ALWAYS_INLINE block256 transpose4x8_8(block256 x);

ALWAYS_INLINE block256 transpose2x8_16(block256 x);

ALWAYS_INLINE uint64_t transpose8x8_1(uint64_t x);

ALWAYS_INLINE block128 transpose16x8_1(block128 x);

ALWAYS_INLINE block192 transpose24x8_1(block192 x);

ALWAYS_INLINE block256 transpose32x8_1(block256 x);

#endif
