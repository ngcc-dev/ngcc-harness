/******************************************************************************
 * BIKE_MLThre
 ******************************************************************************/


#ifndef _R_DECAPS_H_
#define _R_DECAPS_H_

#include "types.h"
#include "conversions.h"

// transpose a row into a column:
_INLINE_ void transpose(uint8_t col[R_BITS], uint8_t row[R_BITS])
{
    col[0] = row[0];
    for (uint64_t i = 1; i < R_BITS ; ++i)
    {
        col[i] = row[(R_BITS) - i];
    }
}

// Count number of 1's in tmp:
uint32_t getHammingWeight(const uint8_t tmp[R_BITS], const uint32_t length);

int BGF_decoder(uint8_t e[R_BITS*2],
        uint8_t s[R_BITS],
        uint32_t h0_compact[DV],
        uint32_t h1_compact[DV]);

#endif //_R_DECAPS_H_
