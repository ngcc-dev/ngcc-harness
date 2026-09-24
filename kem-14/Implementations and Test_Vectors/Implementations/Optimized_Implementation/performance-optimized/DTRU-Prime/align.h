#ifndef ALIGN_H
#define ALIGN_H

#include <stdint.h>
#include <immintrin.h>

#define ALIGNED_INT32(N)        \
    union {                     \
        int32_t coeffs[N];      \
        __m256i vec[(N+7)/8];   \
    }


#endif