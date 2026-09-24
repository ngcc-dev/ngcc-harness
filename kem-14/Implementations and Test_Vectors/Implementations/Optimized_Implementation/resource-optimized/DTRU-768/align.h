#ifndef ALIGN_H
#define ALIGN_H

#include <stdint.h>

#define ALIGNED_UINT8(N)        \
    struct {                    \
        uint8_t coeffs[N];      \
    } __attribute__((aligned(32)))

#define ALIGNED_INT16(N)        \
    struct {                    \
        int16_t coeffs[N];      \
    } __attribute__((aligned(32)))

#endif
