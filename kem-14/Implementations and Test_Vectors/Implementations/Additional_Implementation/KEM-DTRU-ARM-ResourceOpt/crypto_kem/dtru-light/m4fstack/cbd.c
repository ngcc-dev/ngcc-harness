#include <stdint.h>
#include "params.h"
#include "cbd.h"

void cbd1(poly *r, const uint8_t buf[DTRU_CBD1_BYTES])
{
  int i;
  uint8_t t;
  for (i = 0; i < DTRU_N / 4; i++)
  {
    t = buf[i];
    r->coeffs[4 * i + 0] = ((t >> 0) & 1) - ((t >> 1) & 1);
    r->coeffs[4 * i + 1] = ((t >> 2) & 1) - ((t >> 3) & 1);
    r->coeffs[4 * i + 2] = ((t >> 4) & 1) - ((t >> 5) & 1);
    r->coeffs[4 * i + 3] = ((t >> 6) & 1) - ((t >> 7) & 1);
  }
}

void cbd2(poly *r, const uint8_t buf[DTRU_CBD2_BYTES])
{
  int i;
  uint8_t t;
  for (i = 0; i < DTRU_N / 2; i++)
  {
    t = buf[i];
    r->coeffs[2 * i + 0] = ((t >> 0) & 1) + ((t >> 1) & 1) - ((t >> 2) & 1) - ((t >> 3) & 1);
    r->coeffs[2 * i + 1] = ((t >> 4) & 1) + ((t >> 5) & 1) - ((t >> 6) & 1) - ((t >> 7) & 1);
  }
}
