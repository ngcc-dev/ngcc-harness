/******************************************************************************
 * BIKE_MLThre
 ******************************************************************************/

#ifndef HASH_WRAPPER_H
#define HASH_WRAPPER_H

#include "types.h"

status_t bike_hash(
    OUT unsigned char *output,
    IN uint64_t output_size,
    IN const unsigned char *input,
    IN uint64_t input_size);

#endif
