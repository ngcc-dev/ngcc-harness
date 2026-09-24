/******************************************************************************
 * BIKE_MLThre
 ******************************************************************************/

#include "types.h"

//////////////////////////////////////////
//      Conversion functions.
/////////////////////////////////////////

void convert2compact(OUT uint32_t out[DV], IN const uint8_t in[R_SIZE])
{
    uint32_t idx=0;

    for (uint32_t i = 0; i < R_SIZE; i++)
    {
        for (uint32_t j = 0; j < 8ULL; j++)
        {
            if ((i*8 + j) == R_BITS)
            {
                break;
            }

            if ((in[i] >> j) & 1)
            {
                out[idx++] = i*8+j;
            }
        }
    }
}

// convert a sequence of uint8_t elements which fully uses all 8-bits of an uint8_t element to
// a sequence of uint8_t which uses just a single bit per byte (either 0 or 1).
int convertByteToBinary(uint8_t* out, uint8_t * in, uint32_t length)
{
    uint32_t paddingLen = length % 8;
    uint32_t numBytes = (paddingLen == 0) ? (length / 8) : (1 + (length/8));

    for (uint32_t i = 0; i < numBytes; i++)
    {
        for (uint32_t j = 0; j < 8ULL; j++)
        {
            if ((i*8 + j) == length)
            {
                break;
            }

            if ((in[i] >> j) & 1)
            {
                out[i*8+j] = 1;
            }
        }
    }
    return 0;
}

// convert a sequence of uint8_t elements which uses just a single bit per byte (either 0 or 1) to
// a sequence of uint8_t which fully uses all 8-bits of an uint8_t element.
int convertBinaryToByte(uint8_t * out, const uint8_t* in, uint32_t length)
{
    uint32_t paddingLen = length % 8;
    uint32_t numBytes = (paddingLen == 0) ? (length / 8) : (1 + (length/8));

    for (uint32_t i = 0; i < numBytes; i++)
    {
        for (uint32_t j = 0; j < 8; j++)
        {
            if ((i*8 + j) == length)
            {
                break;
            }

            if (in[i*8 + j])
            {
                out[i] |= (uint8_t)(1 << j);
            }
        }
    }
    return 0;
}
