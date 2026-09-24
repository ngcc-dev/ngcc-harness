#ifndef SIGN_ALIGN_NEON_H
#define SIGN_ALIGN_NEON_H

#include <stdint.h>
#include <arm_neon.h>

#define ALIGNED_UINT8(N)        \
    union {                     \
        uint8_t coeffs[N];      \
        uint8x16_t vec[(N+15)/16]; \
    }
#define ALIGNED_INT32(N)        \
    union {                     \
        int32_t coeffs[N];      \
        int32x4_t vec[(N+3)/4]; \
    }
#define ALIGNED_INT64(N)        \
    union {                     \
        int64_t coeffs[N];      \
        int64x2_t vec[(N+1)/2]; \
    }

#endif
