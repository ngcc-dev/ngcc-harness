#include <stdint.h>

#ifndef POPCOUNT_H
#define POPCOUNT_H

#ifdef _MSC_VER
    #include <intrin.h>
#endif

static inline int popcount(uint64_t x) {
#if defined(_MSC_VER)
    return (int)__popcnt64(x);
#elif defined(__GNUC__) || defined(__clang__)
    return __builtin_popcountll(x);
#else
    const uint64_t m1 = 0x5555555555555555ULL;
    const uint64_t m2 = 0x3333333333333333ULL;
    const uint64_t m4 = 0x0f0f0f0f0f0f0f0fULL;
    const uint64_t h01 = 0x0101010101010101ULL;
    x -= (x >> 1) & m1;
    x = (x & m2) + ((x >> 2) & m2);
    x = (x + (x >> 4)) & m4;
    return (int)((x * h01) >> 56);
#endif
}

#ifdef __AVX2__
#include <immintrin.h>

static inline size_t popcount_avx2(const uint64_t *data, size_t n) {
    size_t cnt = 0;
    for (size_t i = 0; i < n; i++) {
        cnt += popcount(data[i]);
    }
    return cnt;
}

#else

static inline size_t popcount_avx2(const uint64_t *data, size_t n) {
    size_t cnt = 0;
    for (size_t i = 0; i < n; i++) {
        cnt += popcount(data[i]);
    }
    return cnt;
}

#endif

#endif  // POPCOUNT_H
