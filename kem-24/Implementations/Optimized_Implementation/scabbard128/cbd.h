#ifndef CBD_H
#define CBD_H   

#include <stdint.h>
#include "params.h"
#include "poly.h"

/// @brief Computes polynomial with coefficients distributed according to a centered binomial distribution from a uniformly random byte array
/// @param[out] r Polynomial with coefficients distributed as CBD
/// @param[in] buf Byte array (SCABBARD_CBD_POLYBYTES)

void cbd(
    poly *r, 
    const uint8_t buf[SCABBARD_CBD_POLYBYTES]);
#endif