/*
 * Shared allocation hooks for the Reed-Solomon encoder.
 * The reference implementation keeps the API shape used by optimized builds,
 * but only needs portable aligned zeroed allocation.
 *
 * Do not add CPU-specific behavior here in the reference tree unless the public
 * RS helper API remains source-compatible with the optimized implementation.
 */

#ifndef RSENCODE_COMMON_H
#define RSENCODE_COMMON_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RS_ALIGNMENT_BYTES 16u

/* Allocate a zeroed buffer rounded up to the reference alignment boundary. */
uint8_t *SIMDSafeAllocate(size_t size);
/* Free buffers returned by SIMDSafeAllocate(). */
void SIMDSafeFree(void *ptr);
/* Retained compatibility hook; no CPU dispatch is needed in reference code. */
void InitializeCPUArch(void);

#ifdef __cplusplus
}
#endif

#endif
