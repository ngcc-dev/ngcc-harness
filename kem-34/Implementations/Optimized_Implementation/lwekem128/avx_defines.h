#ifndef ALIGNED_H
#define ALIGNED_H

#include <immintrin.h>

#define ALIGN(x) __attribute__ ((aligned(x)))

#endif // ALIGNED_H