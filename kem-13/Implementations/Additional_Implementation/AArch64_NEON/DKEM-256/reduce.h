#ifndef DKE_REDUCE_H
#define DKE_REDUCE_H

#include <stdint.h>

int16_t DKE_montgomery_reduce(int32_t a);
int16_t DKE_barrett_reduce(int16_t a);

#endif
