#ifndef OPSSIG_REDUCE_H
#define OPSSIG_REDUCE_H

#include <stdint.h>

int32_t montgomery_reduce(int64_t a);
int32_t reduce32(int32_t a);
int32_t caddq(int32_t a);
int32_t freeze(int32_t a);

#endif
