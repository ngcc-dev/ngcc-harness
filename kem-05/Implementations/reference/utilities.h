/******************************************************************************
 * BIKE_MLThre
 ******************************************************************************/

#ifndef _UTILITIES_H_
#define _UTILITIES_H_

#include "types.h"

//Printing values in Little Endian
void print_LE(IN const uint64_t *in, IN const uint32_t bits_num);

//Printing values in Big Endian
void print_BE(IN const uint64_t *in, IN const uint32_t bits_num);

//Printing number is required only in verbose level 2 or above.
#if VERBOSE==2
#ifdef PRINT_IN_BE
//Print in Big Endian
#define print(in, bits_num) print_BE(in, bits_num)
#else
//Print in Little Endian
#define print(in, bits_num) print_LE(in, bits_num)
#endif
#else
//No prints at all
#define print(in, bits_num)
#endif

//Comparing value in a constant time manner.
_INLINE_ uint32_t safe_cmp(IN const uint8_t* a,
        IN const uint8_t* b,
        IN const uint32_t size)
{
    volatile uint8_t res = 0;

    for(uint32_t i=0; i < size; ++i)
    {
        res |= (a[i] ^ b[i]);
    }

    return (res == 0);
}

//BSR returns ceil(log2(val))
_INLINE_ uint8_t bit_scan_reverse(uint64_t val)
{
    //index is always smaller than 64.
    uint8_t index = 0;

    while(val != 0)
    {
        val >>= 1;
        index++;
    }

    return index;
}

#endif //_UTILITIES_H_
