/**
 * @file crypto_memset.h
 * @brief Header file for crypto_memset.c
 */

#ifndef TRIQ_CRYPTO_MEMSET_H
#define TRIQ_CRYPTO_MEMSET_H

#include <stddef.h>

/**
 * safer call to memset
 */
extern void *(*volatile memset_volatile)(void *, int, size_t);

/**
 * @def memset_zero
 * @brief Securely zero a memory region.
 */
#define memset_zero(ptr, len) memset_volatile((ptr), 0, (len))

#endif  // TRIQ_CRYPTO_MEMSET_H
