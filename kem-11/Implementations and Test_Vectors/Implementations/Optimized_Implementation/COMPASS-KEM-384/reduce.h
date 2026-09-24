#ifndef REDUCE_H
#define REDUCE_H

#include <stdint.h>
#include "params.h"
#include <immintrin.h>

#if (COMPASS_KEM_Q == 3329)
    #define QINV -3327  // Equivalent to -3327 mod 2^16
    #define MONT -1044 // 2^16 mod q
#elif (COMPASS_KEM_Q == 7681)
    #define QINV -7679  // 7681^(-1) mod 65536
    #define MONT 3593 // 2^16 mod q
#else
    #error "Unsupported COMPASS_KEM_Q for reduction!"
#endif

#define montgomery_reduce COMPASS_KEM_NAMESPACE(montgomery_reduce)
int16_t montgomery_reduce(int32_t a);

#define barrett_reduce COMPASS_KEM_NAMESPACE(barrett_reduce)
int16_t barrett_reduce(int16_t a);
#if (COMPASS_KEM_Q == 7681)
#define fqmul_avx COMPASS_KEM_NAMESPACE(fqmul_avx)
__m256i fqmul_avx(__m256i a, __m256i b, __m256i q_vec, __m256i qinv_vec);
#define barrett_reduce_avx COMPASS_KEM_NAMESPACE(barrett_reduce_avx)
__m256i barrett_reduce_avx(__m256i a, __m256i v_vec, __m256i q_vec);
#endif
#endif
