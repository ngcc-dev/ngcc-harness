#include <stdint.h>
#include "params.h"
#include "reduce.h"
#include "rounding.h"

int32_t power2round(int32_t *a0, int32_t a)  {
  int32_t a1;

  a1 = (a + (1 << (D-1)) - 1) >> D;
  *a0 = a - (a1 << D);
  return a1;
}

int32_t decompose(int32_t *a0, int32_t a) {
  int32_t a1;
  
#if GAMMA2 == (Q-1)/32
  a1 = ((((int64_t)a + 2097023) >> 8) * 524321) >> 33;
  //a1 = (a + 2097023)/4194048;
  a1 &= 15;
#elif GAMMA2 == (Q-1)/48
  a1 = ((((int64_t)a + 1398015) >> 9) * 393241) >> 31;
  //a1 = (a + 1398015)/2796032;
  a1 ^= ((23 - a1) >> 31) & a1;
#elif GAMMA2 == (Q-1)/96
  a1 = ((((int64_t)a + 699007) >> 8) * 786481) >> 32;
  //a1 = (a + 699007)/1398016;
  a1 ^= ((47 - a1) >> 31) & a1;
#endif

  *a0  = a - a1*2*GAMMA2;
  *a0 -= (((Q-1)/2 - *a0) >> 31) & Q;
  return a1;
}

unsigned int make_hint(int32_t a0, int32_t a1) { 
  if(a0 > GAMMA2 || a0 < -GAMMA2 || (a0 == -GAMMA2 && a1 != 0))
    return 1;

  return 0;
}

int32_t use_hint(int32_t a, unsigned int hint) {
  int32_t a0, a1;

  a1 = decompose(&a0, a);
  if(hint == 0)
    return a1;

#if GAMMA2 == (Q-1)/32
  if(a0 > 0)
    return (a1 + 1) & 15;
  else
    return (a1 - 1) & 15;
#elif GAMMA2 == (Q-1)/48
  if(a0 > 0)
    return (a1 == 23) ?  0 : a1 + 1;
  else
    return (a1 ==  0) ? 23 : a1 - 1;
#elif GAMMA2 == (Q-1)/96
  if(a0 > 0)
    return (a1 == 47) ?  0 : a1 + 1;
  else
    return (a1 ==  0) ? 47 : a1 - 1;
#endif

}
