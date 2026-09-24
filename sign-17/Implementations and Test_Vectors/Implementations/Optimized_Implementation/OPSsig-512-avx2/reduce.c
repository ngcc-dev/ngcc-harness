#include <stdint.h>
#include "params.h"
#include "reduce.h"

/* Q^{-1} mod 2^32 for Q = 67104769. */
static const uint32_t QINV = 0xfd001001u;

int32_t montgomery_reduce(int64_t a) {
  uint32_t t;

  t = (uint32_t)a * QINV;
  return (int32_t)((a - (int64_t)t * Q) >> 32);
}

int32_t reduce32(int32_t a) {
  int32_t t;

  t = (a + (1 << 25)) >> 26;
  t = a - t*Q;
  return t;
}

int32_t caddq(int32_t a) {
  if(a < 0) a += Q;
  return a;
}

int32_t freeze(int32_t a) {
  int32_t r = reduce32(a);
  r = caddq(r);
  return r;
}
