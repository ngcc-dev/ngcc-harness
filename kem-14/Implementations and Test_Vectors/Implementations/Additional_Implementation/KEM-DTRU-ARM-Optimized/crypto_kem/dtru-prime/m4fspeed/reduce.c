#include <stdint.h>
#include "params.h"
#include "reduce.h"


int16_t montgomery_reduce(int32_t a)
{
  int32_t t;
  int16_t u;

  u = a * QINV;
  t = (int32_t)u * DTRU_Q;
  t = a - t;
  t >>= 16;
  return t;
}

int16_t barrett_reduce(int32_t a)
{
  int64_t t;

  t = (int64_t)BARRETT_V * a;
  t >>= 31;
  t *= DTRU_Q;
  return a - t;
}

int16_t fqcsubq(int16_t a)
{
  a += (a >> 15) & DTRU_Q;
  a -= DTRU_Q;
  a += (a >> 15) & DTRU_Q;
  return a;
}

int16_t fqinv(int16_t a)
{
  int16_t exp, t = 1;

  a = fqmul(a, 1785); // aR = fqmul(a, R^2), (R^2 = 2^32 = 867 mod q)
  for (exp = DTRU_Q - 2; exp > 0; exp >>= 1)
  {
    if (exp & 1)
      t = fqmul(t, a);
    a = fqmul(a, a);
  }
  return t;
}

int16_t fqmul(int16_t a, int16_t b)
{
  return montgomery_reduce((int32_t)a * b);
}
