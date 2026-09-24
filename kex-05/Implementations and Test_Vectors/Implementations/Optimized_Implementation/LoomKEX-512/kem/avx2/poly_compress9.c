#include <stdint.h>
#include "params.h"
#include "poly.h"
#include "poly_compress9.h"

uint16_t poly_compress9_coeff_scalar(int16_t a)
{
  int16_t u = a;
  uint32_t w;

  u += ((int16_t)u >> 15) & WEAVER_Q;
  w = (uint16_t)u;
  return (uint16_t)((((w << 9) + WEAVER_Q / 2) / WEAVER_Q) & 0x1ff);
}

void poly_compress9_pack8(uint8_t r[9], const uint16_t t[8])
{
  r[0] = (uint8_t)(t[0] >> 0);
  r[1] = (uint8_t)((t[0] >> 8) | (t[1] << 1));
  r[2] = (uint8_t)((t[1] >> 7) | (t[2] << 2));
  r[3] = (uint8_t)((t[2] >> 6) | (t[3] << 3));
  r[4] = (uint8_t)((t[3] >> 5) | (t[4] << 4));
  r[5] = (uint8_t)((t[4] >> 4) | (t[5] << 5));
  r[6] = (uint8_t)((t[5] >> 3) | (t[6] << 6));
  r[7] = (uint8_t)((t[6] >> 2) | (t[7] << 7));
  r[8] = (uint8_t)(t[7] >> 1);
}

void poly_compress9_scalar(uint8_t r[(WEAVER_N * 9) / 8], const poly *a)
{
  unsigned int j, k;
  uint16_t t[8];

  for(j = 0; j < WEAVER_N / 8; j++) {
    for(k = 0; k < 8; k++)
      t[k] = poly_compress9_coeff_scalar(a->coeffs[8 * j + k]);
    poly_compress9_pack8(r + 9 * j, t);
  }
}
