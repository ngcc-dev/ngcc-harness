#ifndef RAIN_IMPL_H
#define RAIN_IMPL_H

#include <assert.h>
#include <string.h>
#include <inttypes.h>
#include <immintrin.h>
#include <wmmintrin.h>

#include "transpose.h"
#include "util.h"

inline void rain_round_function(uint64_t* block, const uint64_t* key, const uint64_t* rc, const uint64_t* mat, uint64_t* after_sbox) {
#if SECURITY_PARAM == 128
    block128 tmp;
    memcpy(&tmp, block, 16);
    tmp = block128_xor(*(block128*)block, *(block128*)key);
    tmp = block128_xor(tmp, *(block128*)rc);
    block128_inverse(&tmp);
    memcpy(after_sbox, &tmp, 16);
    block128_multiply_with_GF2_matrix(&tmp, mat);
    memcpy(block, &tmp, 16);
#elif SECURITY_PARAM == 192
    block192 tmp;
    memcpy(&tmp, block, 24);
    tmp = block192_xor(*(block192*)block, *(block192*)key);
    tmp = block192_xor(tmp, *(block192*)rc);
    block192_inverse(&tmp);
    memcpy(after_sbox, &tmp, 24);
    block192_multiply_with_GF2_matrix(&tmp, mat);
    memcpy(block, &tmp, 24);
#elif SECURITY_PARAM == 256
    block256 tmp;
    memcpy(&tmp, block, 32);
    tmp = block256_xor(*(block256*)block, *(block256*)key);
    tmp = block256_xor(tmp, *(block256*)rc);
    block256_inverse(&tmp);
    memcpy(after_sbox, &tmp, 32);
    block256_multiply_with_GF2_matrix(&tmp, mat);
    memcpy(block, &tmp, 32);
#endif
}

inline void rain_last_round_function(uint64_t* block, const uint64_t* key, const uint64_t* rc) {
#if SECURITY_PARAM == 128
    block128 tmp;
    memcpy(&tmp, block, 16);
    tmp = block128_xor(*(block128*)block, *(block128*)key);
    tmp = block128_xor(tmp, *(block128*)rc);
    block128_inverse(&tmp);
    tmp = block128_xor(tmp, *(block128*)key);
    memcpy(block, &tmp, 16);
#elif SECURITY_PARAM == 192
    block192 tmp;
    memcpy(&tmp, block, 24);
    tmp = block192_xor(*(block192*)block, *(block192*)key);
    tmp = block192_xor(tmp, *(block192*)rc);
    block192_inverse(&tmp);
    tmp = block192_xor(tmp, *(block192*)key);
    memcpy(block, &tmp, 24);
#elif SECURITY_PARAM == 256
    block256 tmp;
    memcpy(&tmp, block, 32);
    tmp = block256_xor(*(block256*)block, *(block256*)key);
    tmp = block256_xor(tmp, *(block256*)rc);
    block256_inverse(&tmp);
    tmp = block256_xor(tmp, *(block256*)key);
    memcpy(block, &tmp, 32);
#endif
}

#endif
