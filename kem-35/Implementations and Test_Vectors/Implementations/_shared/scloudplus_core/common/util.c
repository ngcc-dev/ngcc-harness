/**
 * @file util.c
 * @brief Small constant-time and byte utility helpers shared by Scloud+ modules.
 */

#include "util.h"
#include <string.h>

/**
 * @brief Compare two byte strings without data-dependent early exit.
 */
uint8_t scloudplus_verify(const uint8_t *a, const uint8_t *b, size_t len)
{
	uint8_t diff = 0U;

	for (size_t i = 0; i < len; i++)
	{
		diff |= (uint8_t)(a[i] ^ b[i]);
	}

	return (uint8_t)(0U - (uint8_t)(((uint32_t)diff | (0U - (uint32_t)diff)) >> 31));
}

/**
 * @brief Select between byte strings with a byte mask and no secret branch.
 */
void scloudplus_cmov(uint8_t *r, const uint8_t *a, const uint8_t *b, size_t len,
					 uint8_t mask)
{
	const uint8_t keep = (uint8_t)~mask;

	for (size_t i = 0; i < len; i++)
	{
		r[i] = (uint8_t)((keep & a[i]) | (mask & b[i]));
	}
}

/**
 * @brief Wipe memory without relying on libc-specific explicit_bzero APIs.
 */
void scloudplus_secure_zeroize(void *ptr, size_t len)
{
#if defined(__GNUC__) || defined(__clang__)
	if (len == 0U)
	{
		return;
	}
	memset(ptr, 0, len);
	__asm__ __volatile__("" : : "r"(ptr) : "memory");
#else
	volatile uint8_t *p = (volatile uint8_t *)ptr;

	while (len-- != 0U)
	{
		*p++ = 0U;
	}
#endif
}
