#ifndef NTT_H
#define NTT_H

#include <stdint.h>
#include "params.h"

#define ntt KEM_NAMESPACE(ntt)
void ntt(uint16_t poly[64]);

#define invntt KEM_NAMESPACE(invntt)
void invntt(uint16_t poly[64]);


#endif
