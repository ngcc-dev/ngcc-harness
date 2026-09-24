// Comes from https://github.com/PQClean/PQClean/blob/master/crypto_kem/mlkem-768/clean/reduce.h

#ifndef REDUCE_H
#define REDUCE_H
#include "p512_params.h"
#include <immintrin.h>

/// @brief Performs Barrett reduction mod q
/// @param[in, out] a mod q in {0,...,q-1}
#define reduce_avx DKE_NAMESPACE(reduce_avx)
void reduce_avx(__m256i *r, const __m256i *qdata);

/// @brief Performs centered Barrett reduction mod q
/// @param[in, out] a mod q in {-(q-1)/2,...,(q-1)/2}
#define center_reduce_avx DKE_NAMESPACE(center_reduce_avx)
void center_reduce_avx(__m256i *r, const __m256i *qdata);

/// @brief Transform to Montgomery domain mod q
/// @param[in] a an integer
/// @return    a mod q in {-(q-1)/2,...,(q-1)/2}
#define tomont_avx DKE_NAMESPACE(tomont_avx)
void tomont_avx(__m256i *r, const __m256i *qdata);

#endif
