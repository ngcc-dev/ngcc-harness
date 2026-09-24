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


void cbd9(poly *r, const uint8_t buf[DTRU_CBD9_BYTES])
{
  int i, j;
  uint8_t t[9];
  for (i = 0; i < DTRU_N / 4; i++)
  {
    t[0] = buf[9*i+0];
    t[1] = buf[9*i+1];
    t[2] = buf[9*i+2];
    t[3] = buf[9*i+3];
    t[4] = buf[9*i+4];
    t[5] = buf[9*i+5];
    t[6] = buf[9*i+6];
    t[7] = buf[9*i+7];
    t[8] = buf[9*i+8];
    
    for (j = 0; j < 4; j++)
    {
      r->coeffs[4*i+j] = 0;
      r->coeffs[4*i+j] += (t[0] >> (2*j+0)) & 1;
      r->coeffs[4*i+j] += (t[1] >> (2*j+0)) & 1;
      r->coeffs[4*i+j] += (t[2] >> (2*j+0)) & 1;
      r->coeffs[4*i+j] += (t[3] >> (2*j+0)) & 1;
      r->coeffs[4*i+j] += (t[4] >> (2*j+0)) & 1;
      r->coeffs[4*i+j] += (t[5] >> (2*j+0)) & 1;
      r->coeffs[4*i+j] += (t[6] >> (2*j+0)) & 1;
      r->coeffs[4*i+j] += (t[7] >> (2*j+0)) & 1;
      r->coeffs[4*i+j] += (t[8] >> (2*j+0)) & 1;
      
      r->coeffs[4*i+j] -= (t[0] >> (2*j+1)) & 1;
      r->coeffs[4*i+j] -= (t[1] >> (2*j+1)) & 1;
      r->coeffs[4*i+j] -= (t[2] >> (2*j+1)) & 1;
      r->coeffs[4*i+j] -= (t[3] >> (2*j+1)) & 1;
      r->coeffs[4*i+j] -= (t[4] >> (2*j+1)) & 1;
      r->coeffs[4*i+j] -= (t[5] >> (2*j+1)) & 1;
      r->coeffs[4*i+j] -= (t[6] >> (2*j+1)) & 1;
      r->coeffs[4*i+j] -= (t[7] >> (2*j+1)) & 1;
      r->coeffs[4*i+j] -= (t[8] >> (2*j+1)) & 1;
    }
  }
}
