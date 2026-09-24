#ifndef VERIFY_H
#define VERIFY_H    

#include <stddef.h>
#include <stdint.h>
#include "params.h"

/// @brief Compare two byte arrays for equality in constant time.
/// @param a First byte array.
/// @param b Second byte array.
/// @param len_bytes Length of the byte arrays.
/// @return 0 if the arrays are equal, non-zero otherwise.
int verify(
    const uint8_t *a, 
    const uint8_t *b, 
    size_t len_bytes);

/// @brief Conditionally move bytes from one array to another in constant time.
/// @param r Destination array.
/// @param x Source array.
/// @param len_bytes Number of bytes to move.
/// @param b Condition flag (0 or 1).
void cmov(
    uint8_t *r, 
    const uint8_t *x, 
    size_t len_bytes, 
    uint8_t b);

#endif