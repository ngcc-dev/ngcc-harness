#include <stdint.h>
#include <stdio.h>
#include "params.h"
#include "reduce.h"
#include "poly.h"
#include "coding.h"
#include "cbd.h"
#include "inverse.h"
#include "ntt.h"

void poly_reduce(poly *a)
{
  for (int i = 0; i < DTRU_N; ++i)
    a->coeffs[i] = barrett_reduce(a->coeffs[i]);
}

void poly_freeze(poly *a)
{
  poly_reduce(a);
  for (int i = 0; i < DTRU_N; ++i)
    a->coeffs[i] = fqcsubq(a->coeffs[i]);
}

void poly_add(poly *c, const poly *a, const poly *b)
{
  for (int i = 0; i < DTRU_N; ++i)
    c->coeffs[i] = a->coeffs[i] + b->coeffs[i];
}

void poly_multi_p(poly *b, const poly *a)
{
  for (int i = 0; i < DTRU_N; ++i)
  {
    b->coeffs[i] = 2 * a->coeffs[i];
  }
}


void poly_sample_keygen_f(poly *a, const unsigned char *buf)
{
  cbd3(a, buf);
}

void poly_sample_keygen_g(poly *a, const unsigned char *buf)
{
  cbd4(a, buf);
}

void poly_sample_enc_r(poly *a, const unsigned char *buf)
{
  cbd4(a, buf);
}

void poly_sample_enc_e(poly *a, const unsigned char *buf)
{
  cbd2(a, buf);
}

void poly_ntt(poly *b)
{
  ntt_asm3457(b->coeffs);
}

void poly_invntt(poly *b)
{
  invntt_asm(b->coeffs);
}

void poly_basemul(poly *c, const poly *a, const poly *b)
{
  for (int i = 0; i < DTRU_N / 6; ++i)
  {
    basemul(c->coeffs + 6 * i,
            a->coeffs + 6 * i,
            b->coeffs + 6 * i,
            zetas[128 + 2 * i]);
    basemul(c->coeffs + 6 * i + 2,
            a->coeffs + 6 * i + 2,
            b->coeffs + 6 * i + 2,
            zetas_base[i]);
    basemul(c->coeffs + 6 * i + 4,
            a->coeffs + 6 * i + 4,
            b->coeffs + 6 * i + 4,
            -zetas[128 + 2 * i] - zetas_base[i]);
  }
}
int poly_baseinv(poly *b, const poly *a)
{
  int r = 0;
  for (int i = 0; i < DTRU_N / 4; ++i)
  {
    if(r != 0)   return r;
    r += rq_inverse_det_asm(b->coeffs + 4 * i,
                        a->coeffs + 4 * i,
                        zetas_invseq + 2 * i);
  }
  return r;
}

static inline int16_t compress_coeff(int16_t sigma_c, int16_t mask)
{
  int16_t s = sigma_c + (mask & ((DTRU_Q + 1) >> 1));
  int32_t t = ((int32_t)(s << DTRU_LOGQ2) + (DTRU_Q >> 1)) / DTRU_Q;
  return (int16_t)(t & (DTRU_Q2 - 1));
}

void poly_encode_compress(poly *c,
                          const poly *sigma,
                          const unsigned char *msg)
{
  unsigned int i, j;
  int16_t mask;
  uint8_t code;
  uint8_t tmp;

  for (i = 0; i < 2u * DTRU_MSGBYTES; i++)
  {
    tmp = (i & 1) ? ((msg[i >> 1] >> 4) & 0xF) : (msg[i >> 1] & 0xF);
    code = encode_e8(tmp);
    for (j = 0; j < 8; j++)
    {
      mask = -(int16_t)((code >> j) & 1);
      c->coeffs[8 * i + j] = compress_coeff(sigma->coeffs[8 * i + j], mask);
      c->coeffs[8 * (i + DTRU_N / 16) + j] =
          compress_coeff(sigma->coeffs[8 * (i + DTRU_N / 16) + j], mask);
    }
  }
}

void poly_decode(unsigned char *msg,
                 const poly *cf)
{
  unsigned int i, j;
  int16_t tmp_mp[16];

  for (i = 0; i < DTRU_MSGBYTES; i++)
  {
    msg[i] = 0;
  }

  for (i = 0; i < DTRU_N / 16; i++)
  {
    for (j = 0; j < 8; j++)
    {
      tmp_mp[j] = cf->coeffs[8 * i + j];
      tmp_mp[j + 8] = cf->coeffs[8 * i + j + DTRU_N / 2];
    }
    msg[i >> 1] |= decode_e8(tmp_mp) << ((i & 1) << 2);
  }
}
