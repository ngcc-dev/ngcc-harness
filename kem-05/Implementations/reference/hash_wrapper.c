/******************************************************************************
 * BIKE_MLThre
 ******************************************************************************/

#include <limits.h>

#include "auxfunc.h"
#include "hash_wrapper.h"

status_t bike_hash(
    unsigned char *output,
    uint64_t output_size,
    const unsigned char *input,
    uint64_t input_size)
{
    int result;

    if (output == NULL || input == NULL || input_size > ULLONG_MAX / 8ULL)
    {
        return E_HASH_FAIL;
    }

    if (output_size == 32ULL)
    {
        result = sm3hash(256, input, input_size * 8ULL, output);
    }
    else if (output_size == 64ULL)
    {
        result = pseudohash(512, input, input_size * 8ULL, output);
    }
    else
    {
        return E_HASH_FAIL;
    }

    return result == 0 ? SUCCESS : E_HASH_FAIL;
}
