/*
 * Portable allocation helpers retained for the shared Reed-Solomon API.
 * Reference builds do not use architecture-specific dispatch, but optimized
 * and reference directories keep the same helper names for compatibility.
 */

#include "rsencode_common.h"

#include <stdlib.h>
#include <string.h>

static size_t round_up(size_t size, size_t alignment)
{
    return ((size + alignment - 1u) / alignment) * alignment;
}

/*
 * Allocate zeroed storage with the alignment expected by the RS encoder.
 * The reference path currently uses malloc, but rounds the requested size up
 * so callers that assume RS_ALIGNMENT_BYTES-sized chunks keep valid padding.
 */
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

/* Free RS helper storage; NULL is accepted to mirror free(). */
void SIMDSafeFree(void *ptr)
{
    if (ptr != NULL)
    {
        free(ptr);
    }
}

/*
 * Reference implementation placeholder for optimized CPU feature setup.
 * Kept as a no-op so shared RS code can be built in both reference and
 * optimized directories without conditional API differences.
 */
void InitializeCPUArch(void)
{
}
