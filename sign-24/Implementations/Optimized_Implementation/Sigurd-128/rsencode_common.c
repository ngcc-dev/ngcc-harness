#include "rsencode_common.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static size_t round_up(size_t size, size_t alignment)
{
    return ((size + alignment - 1u) / alignment) * alignment;
}

uint8_t *SIMDSafeAllocate(size_t size)
{
    size_t padded;
    size_t total;
    uintptr_t raw_addr;
    uintptr_t aligned_addr;
    uint8_t *raw;
    void **slot;

    if (size == 0u)
    {
        return NULL;
    }

    padded = round_up(size, RS_ALIGNMENT_BYTES);
    total = padded + RS_ALIGNMENT_BYTES + sizeof(void *);
    raw = (uint8_t *)malloc(total);
    if (raw == NULL)
    {
        return NULL;
    }
    raw_addr = (uintptr_t)(raw + sizeof(void *));
    aligned_addr = (raw_addr + (RS_ALIGNMENT_BYTES - 1u)) & ~(uintptr_t)(RS_ALIGNMENT_BYTES - 1u);
    slot = (void **)(aligned_addr - sizeof(void *));
    *slot = raw;
    memset((void *)aligned_addr, 0, padded);
    return (uint8_t *)aligned_addr;
}

void SIMDSafeFree(void *ptr)
{
    if (ptr != NULL)
    {
        void **slot = (void **)((uint8_t *)ptr - sizeof(void *));
        free(*slot);
    }
}

void InitializeCPUArch(void)
{
}
