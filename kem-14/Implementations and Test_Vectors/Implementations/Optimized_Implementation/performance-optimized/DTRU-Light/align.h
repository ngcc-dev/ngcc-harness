#ifndef ALIGN_H
#define ALIGN_H
#include <stdio.h>
#include <stdint.h>
#include <immintrin.h>

#define ALIGNED_INT16(N)        \
    union {                     \
        int16_t coeffs[N];      \
        __m256i vec[(N+15)/16];   \
    }
#endif