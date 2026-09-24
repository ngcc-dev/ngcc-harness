#ifndef NTT_H
#define NTT_H

#include <stdint.h>
#include "avx_defines.h"
#include "params.h"

#include "consts.h"

#define ntt KEM_NAMESPACE(ntt)
void ntt(uint16_t poly[64]);

#define invntt KEM_NAMESPACE(invntt)
void invntt(uint16_t poly[64]);

void ntt_avx(uint16_t poly[64], const uint16_t qdata[QDATA_N]);

void invntt_avx(uint16_t poly[64], const uint16_t qdata[QDATA_N]);

#endif
