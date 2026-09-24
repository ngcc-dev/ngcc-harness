#ifndef RSENCODE_COMMON_H
#define RSENCODE_COMMON_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RS_ALIGNMENT_BYTES 32u

uint8_t *SIMDSafeAllocate(size_t size);
void SIMDSafeFree(void *ptr);
void InitializeCPUArch(void);

#ifdef __cplusplus
}
#endif

#endif
