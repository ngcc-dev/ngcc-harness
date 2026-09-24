#ifndef NTT_H
#define NTT_H

#include "params.h"
#include "consts.h"
#include "avx_defines.h"
#include <stdint.h>

#define ntt KEM_NAMESPACE (ntt)
void ntt (uint16_t poly[KEM_N]);

#define invntt KEM_NAMESPACE (invntt)
void invntt (uint16_t poly[KEM_N]);

void ntt_avx(uint16_t poly[KEM_N], const uint16_t qdata[QDATA_N]);

void invntt_avx(uint16_t poly[KEM_N], const uint16_t qdata[QDATA_N]);

#endif
