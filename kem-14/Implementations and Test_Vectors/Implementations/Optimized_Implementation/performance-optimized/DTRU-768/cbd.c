#include <stdint.h>
#include "params.h"
#include "cbd.h"

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

void cbd3(poly *r, const uint8_t buf[DTRU_CBD3_BYTES])
{
  int i;
  uint8_t t[3];
  for (i = 0; i < DTRU_N / 4; i++)
  {
    t[0] = buf[3 * i + 0];
    t[1] = buf[3 * i + 1];
    t[2] = buf[3 * i + 2];
    r->coeffs[4 * i + 0] = ((t[0] >> 0) & 1) + ((t[1] >> 0) & 1) + ((t[2] >> 0) & 1) - 
                           ((t[0] >> 1) & 1) - ((t[1] >> 1) & 1) - ((t[2] >> 1) & 1);
    r->coeffs[4 * i + 1] = ((t[0] >> 2) & 1) + ((t[1] >> 2) & 1) + ((t[2] >> 2) & 1) - 
                           ((t[0] >> 3) & 1) - ((t[1] >> 3) & 1) - ((t[2] >> 3) & 1);
    r->coeffs[4 * i + 2] = ((t[0] >> 4) & 1) + ((t[1] >> 4) & 1) + ((t[2] >> 4) & 1) - 
                           ((t[0] >> 5) & 1) - ((t[1] >> 5) & 1) - ((t[2] >> 5) & 1);
    r->coeffs[4 * i + 3] = ((t[0] >> 6) & 1) + ((t[1] >> 6) & 1) + ((t[2] >> 6) & 1) - 
                           ((t[0] >> 7) & 1) - ((t[1] >> 7) & 1) - ((t[2] >> 7) & 1);
  }
}

void cbd4(poly *r, const uint8_t buf[DTRU_CBD4_BYTES])
{
  int i;
  uint8_t t;
  for (i = 0; i < DTRU_N; i++)
  {
    t = buf[i];
    r->coeffs[i] = ((t >> 0) & 1) + ((t >> 1) & 1) + ((t >> 2) & 1) + ((t >> 3) & 1) - 
                   ((t >> 4) & 1) - ((t >> 5) & 1) - ((t >> 6) & 1) - ((t >> 7) & 1);
  }
}
