#ifndef NTT_H
#define NTT_H

#include "params.h"
#include <stdint.h>

#define ntt KEM_NAMESPACE (ntt)
void ntt (uint16_t poly[KEM_N]);

#define invntt KEM_NAMESPACE (invntt)
void invntt (uint16_t poly[KEM_N]);

#endif
