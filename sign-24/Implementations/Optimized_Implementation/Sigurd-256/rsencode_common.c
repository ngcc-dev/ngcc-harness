#include "rsencode_common.h"

#include <stdlib.h>
#include <string.h>

static size_t round_up(size_t size, size_t alignment)
{
    return ((size + alignment - 1u) / alignment) * alignment;
}

uint8_t *SIMDSafeAllocate(size_t size)
{
    void *memory;

    if (size == 0u)
    {
        return NULL;
    }

    memory = malloc(round_up(size, RS_ALIGNMENT_BYTES));
    if (memory == NULL)
    {
        return NULL;
    }
    memset(memory, 0, round_up(size, RS_ALIGNMENT_BYTES));
    return (uint8_t *)memory;
}

void SIMDSafeFree(void *ptr)
{
    if (ptr != NULL)
    {
        free(ptr);
    }
}

void InitializeCPUArch(void)
{
}
