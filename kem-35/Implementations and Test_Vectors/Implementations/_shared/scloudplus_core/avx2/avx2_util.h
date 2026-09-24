/**
 * @file avx2_util.h
 * @brief AVX2 helper intrinsics shared by optimized matrix arithmetic.
 */

#ifndef _SCLOUDPLUS_AVX2_UTIL_H_
#define _SCLOUDPLUS_AVX2_UTIL_H_

#include "scloudplus_param_common.h"
#include <stddef.h>
#include <stdint.h>

#if defined(SCLOUDPLUS_BACKEND_AVX2)
#include <immintrin.h>

#define SCLOUDPLUS_AVX2_LANES_U16 16U

/**
 * @brief Broadcast one 16-bit value to all AVX2 lanes.
 */
static inline __m256i scloudplus_avx2_set1_u16(uint16_t value)
{
	return _mm256_set1_epi16((short)value);
}

/**
 * @brief Reduce every AVX2 16-bit lane modulo `q = 2^10`.
 */
static inline __m256i scloudplus_avx2_mask_q(__m256i value)
{
	return _mm256_and_si256(value,
							_mm256_set1_epi16((short)scloudplus_q_mask));
}

/**
 * @brief Horizontally sum 16 unsigned 16-bit lanes modulo `2^16`.
 *
 * The matrix kernels accumulate in q-ary arithmetic and reduce later, so this
 * helper intentionally returns the natural 16-bit wraparound sum.
 */
static inline uint16_t scloudplus_avx2_hsum_u16_mod_2p16(__m256i value)
{
	uint16_t lanes[SCLOUDPLUS_AVX2_LANES_U16];
	uint32_t sum = 0;

	_mm256_storeu_si256((__m256i *)lanes, value);
	for (size_t i = 0; i < SCLOUDPLUS_AVX2_LANES_U16; i++)
	{
		sum += lanes[i];
	}
	return (uint16_t)sum;
}
#endif

#endif
