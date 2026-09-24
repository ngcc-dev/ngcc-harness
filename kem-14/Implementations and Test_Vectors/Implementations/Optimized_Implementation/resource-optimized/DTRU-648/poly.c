#include <stdint.h>
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
  cbd2(a, buf);
}

void poly_sample_keygen_g(poly *a, const unsigned char *buf)
{
  cbd9(a, buf);
}

void poly_sample_enc_r(poly *a, const unsigned char *buf)
{
  cbd2(a, buf);
}

void poly_sample_enc_e(poly *a, const unsigned char *buf)
{
  cbd2(a, buf);
}

void poly_ntt(poly *b)
{
  ntt(b->coeffs);
}

void poly_invntt(poly *b)
{
  invntt(b->coeffs);
}

void poly_basemul(poly *c, const poly *a, const poly *b)
{
  for (int i = 0; i < DTRU_N / 27; ++i)
  {
    basemul(c->coeffs + 27 * i,
            a->coeffs + 27 * i,
            b->coeffs + 27 * i,
            zetas[24 + 2 * i]);
    basemul(c->coeffs + 27 * i + 9,
            a->coeffs + 27 * i + 9,
            b->coeffs + 27 * i + 9,
            zetas_base[i]);
    basemul(c->coeffs + 27 * i + 18,
            a->coeffs + 27 * i + 18,
            b->coeffs + 27 * i + 18,
            -zetas[24 + 2 * i]-zetas_base[i]);
  }
}

int poly_baseinv(poly *b, const poly *a)
{
  int r = 0;
  for (int i = 0; i < DTRU_N / 27; ++i)
  {
    if(r != 0)   return r;
    r += rq_inverse(b->coeffs + 27 * i,
                        a->coeffs + 27 * i,
                        zetas[24 + 2 * i]);
    if (r != 0)   return r;
    r += rq_inverse(b->coeffs + 27 * i + 9,
                        a->coeffs + 27 * i + 9,
                        zetas_base[i]);
    if (r != 0)   return r;
    r += rq_inverse(b->coeffs + 27 * i + 18,
                        a->coeffs + 27 * i + 18,
                        -zetas[24 + 2 * i] - zetas_base[i]);
  }
  return r;
}

void poly_encode_compress(poly *c,
                          const poly *sigma,
                          const unsigned char *msg)
{
  unsigned int i, j;
  int16_t mask;
  uint8_t tmp;
  int16_t s;
  int32_t t;

  for (i = 0; i < DTRU_N / 8; i++)
  {
    if (i < DTRU_MSGBYTES * 4)
    {
      unsigned int msg_index = i;
      if (msg_index >= DTRU_MSGBYTES * 2)
        msg_index -= DTRU_MSGBYTES * 2;

      tmp = msg[msg_index >> 1];
      tmp = (tmp >> ((msg_index & 1) << 2)) & 0xF;
      tmp = encode_e8(tmp);
    }
    else
    {
      tmp = 0;
    }

    for (j = 0; j < 8; j++)
    {
      mask = -(int16_t)((tmp >> j) & 1);
      s = sigma->coeffs[8 * i + j] + (mask & ((DTRU_Q + 1) >> 1));
      t = ((int32_t)(s << DTRU_LOGQ2) + (DTRU_Q >> 1)) / DTRU_Q;
      c->coeffs[8 * i + j] = t & (DTRU_Q2 - 1);
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

  for (i = 0; i < 40; i++)
  {
    for (j = 0; j < 8; j++)
    {
      tmp_mp[j] = cf->coeffs[8 * i + j];
      tmp_mp[j + 8] = cf->coeffs[8 * i + j + 320];
    }
    msg[i >> 1] |= decode_e8(tmp_mp) << ((i & 1) << 2);
  }
}
