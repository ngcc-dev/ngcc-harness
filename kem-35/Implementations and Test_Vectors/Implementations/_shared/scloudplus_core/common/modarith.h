/**
 * @file modarith.h
 * @brief Modular arithmetic constants and helpers for q = 2^10 arithmetic.
 */

#ifndef _SCLOUDPLUS_MODARITH_H_
#define _SCLOUDPLUS_MODARITH_H_

#include "scloudplus_param_common.h"
#include <stdint.h>

/**
 * @brief Reduce an integer modulo `q = 2^10`.
 *
 * Because q is a power of two, reduction is a public bit mask rather than a
 * division. This helper is used on public matrix and ciphertext coefficients.
 */
static inline uint16_t scloudplus_mod_q(uint32_t value)
{
	return (uint16_t)(value & scloudplus_q_mask);
}

/**
 * @brief Round a q-ary coefficient to a smaller unsigned bit width.
 *
 * This is used when converting decrypted q-ary coordinates into the fixed
 * point values consumed by the BW/RBW decoder.
 */
static inline int32_t scloudplus_q_to_bits_rounded(uint16_t value,
								   unsigned int bits)
{
	const unsigned int shift = (unsigned int)scloudplus_logq - bits;
	const uint32_t v = (uint32_t)(value & scloudplus_q_mask);

	return (int32_t)((v + (1U << (shift - 1U))) >> shift);
}

#endif
