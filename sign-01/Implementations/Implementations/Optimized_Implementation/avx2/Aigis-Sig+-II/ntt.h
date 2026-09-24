#ifndef NTT_H
#define NTT_H

#include <stdint.h>
#include "params.h"



void ntt(int32_t p[PARAM_N]);
void invntt(int32_t p[PARAM_N]);
#endif
