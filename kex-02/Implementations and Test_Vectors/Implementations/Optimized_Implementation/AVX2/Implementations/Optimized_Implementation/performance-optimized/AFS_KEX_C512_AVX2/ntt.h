#ifndef NTT_H
#define NTT_H

#include <stdint.h>
#include "params.h"

#define ntt_avx KYBER_NAMESPACE(ntt_avx)
void ntt_avx(int16_t r[KYBER_N], const int16_t *qdata);

#define invntt_avx KYBER_NAMESPACE(invntt_avx)
void invntt_avx(int16_t r[KYBER_N], const int16_t *qdata);

#define basemul_avx KYBER_NAMESPACE(basemul_avx)
void basemul_avx(int16_t r[KYBER_N], const int16_t a[KYBER_N], const int16_t b[KYBER_N], const int16_t* qdata);

#endif
