#include <stdint.h>
#include "params.h"
#include "matacc.h"
#include "poly.h"
#include "polyvec.h"
#include "symmetric.h"

#if(XOF_BLOCKBYTES % 3)
#error "matacc assumes that XOF_BLOCKBYTES is a multiple of 3"
#endif

#define GEN_MATRIX_NBLOCKS ((12*KYBER_N/8*(1 << 12)/KYBER_Q + XOF_BLOCKBYTES)/XOF_BLOCKBYTES)

#define GEN_MATRIX_MAXBLOCKS (GEN_MATRIX_NBLOCKS + 1)

static unsigned int rej_uniform(int16_t *r,
                                unsigned int len,
                                const uint8_t *buf,
                                unsigned int buflen)
{
  unsigned int ctr, pos;
  uint16_t val0, val1;

  ctr = pos = 0;
  while(ctr < len && pos + 3 <= buflen) {
    val0 = ((buf[pos+0] >> 0) | ((uint16_t)buf[pos+1] << 8)) & 0xFFF;
    val1 = ((buf[pos+1] >> 4) | ((uint16_t)buf[pos+2] << 4)) & 0xFFF;
    pos += 3;

    if(val0 < KYBER_Q)
      r[ctr++] = val0;
    if(ctr < len && val1 < KYBER_Q)
      r[ctr++] = val1;
  }

  return ctr;
}

static void genpoly(poly *a, const uint8_t seed[KYBER_SYMBYTES],
                    unsigned int i, unsigned int j, int transposed)
{
  uint8_t buf[GEN_MATRIX_MAXBLOCKS*XOF_BLOCKBYTES];
  xof_state state;
  unsigned int ctr;
  size_t total = GEN_MATRIX_NBLOCKS*XOF_BLOCKBYTES;

  if(transposed)
    xof_absorb(&state, seed, (uint8_t)i, (uint8_t)j);
  else
    xof_absorb(&state, seed, (uint8_t)j, (uint8_t)i);

  xof_squeeze_full(buf, total, &state);
  ctr = rej_uniform(a->coeffs, KYBER_N, buf, total);

  while(ctr < KYBER_N && total + XOF_BLOCKBYTES <= sizeof(buf)) {
    size_t prev = total;
    total += XOF_BLOCKBYTES;
    xof_squeeze_full(buf, total, &state);
    ctr += rej_uniform(a->coeffs + ctr, KYBER_N - ctr, buf + prev, XOF_BLOCKBYTES);
  }
}

void matacc(poly *r, const polyvec *b,
            unsigned int i, const uint8_t seed[KYBER_SYMBYTES], int transposed)
{
  poly t;

  genpoly(&t, seed, i, 0, transposed);
  poly_basemul_opt_16_16_noprime(r, &b->vec[0], &t);

  for(unsigned int j = 1; j < KYBER_K; j++) {
    genpoly(&t, seed, i, j, transposed);
    poly_basemul_acc_opt_16_16_noprime(r, &b->vec[j], &t);
  }
  poly_reduce_centered(r);
}
