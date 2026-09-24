#include <stdint.h>
#include <stdio.h>
#include "params.h"
#include "reduce.h"
#include "poly.h"
#include "coding.h"
#include "cbd.h"
#include "inverse.h"
#include "inverse_avx2.h"
#include "ntt.h"
#include "consts.h"

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
  ntt(b->coeffs);
}

void poly_invntt(poly *b)
{
  invntt(b->coeffs);
}

void poly_basemul(poly *c, const poly *a, const poly *b)
{
  for (int i = 0; i < DTRU_N / 6; ++i)
  {
    basemul(c->coeffs + 6 * i,
            a->coeffs + 6 * i,
            b->coeffs + 6 * i,
            zetas_ref[128 + 2 * i]);
    basemul(c->coeffs + 6 * i + 2,
            a->coeffs + 6 * i + 2,
            b->coeffs + 6 * i + 2,
            zetas_base_ref[i]);
    basemul(c->coeffs + 6 * i + 4,
            a->coeffs + 6 * i + 4,
            b->coeffs + 6 * i + 4,
            -zetas_ref[128 + 2 * i]-zetas_base_ref[i]);
  }
}

int poly_baseinv(poly *b, const poly *a)
{
  int r = 0;
  for (int i = 0; i < DTRU_N / 6; ++i)
  {
    if(r != 0)   return r;
    r += rq_inverse_det(b->coeffs + 6 * i,
                        a->coeffs + 6 * i,
                        zetas_ref[128 + 2 * i]);
    if(r != 0)   return r;
    r += rq_inverse_det(b->coeffs + 6 * i + 2,
                        a->coeffs + 6 * i + 2,
                        zetas_base_ref[i]);
    if(r != 0)   return r;
    r += rq_inverse_det(b->coeffs + 6 * i + 4,
                        a->coeffs + 6 * i + 4,
                        -zetas_ref[128 + 2 * i]-zetas_base_ref[i]);
  }
  return r;
}


void poly_encode_compress(poly *c,
                          const poly *sigma,
                          const unsigned char *msg)
{
  unsigned int i, j;
  int16_t mask;
  uint8_t mh[DTRU_N / 8];
  uint8_t tmp;
  int16_t s;
  int32_t t;
  for (i = 0; i < DTRU_MSGBYTES; i++)
  {
    tmp = msg[i] & 0xF;
    mh[2 * i] = encode_e8(tmp);
    mh[2 * i + DTRU_N / 16] = mh[2 * i];

    tmp = (msg[i] >> 4) & 0xF;
    mh[2 * i + 1] = encode_e8(tmp);
    mh[2 * i + 1 + DTRU_N / 16] = mh[2 * i + 1];
  }

  for (i = 0; i < DTRU_N / 8; i++)
  {
    for (j = 0; j < 8; j++)
    {
      mask = -(int16_t)((mh[i] >> j) & 1);
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

// AVX2 implementations
void poly_ntt_avx(poly *b, const poly *a)
{
  ntt_avx(b, a);
}

void poly_invntt_avx(poly *b, const poly *a)
{
   invntt_avx(b->coeffs, a->coeffs);
}

void poly_basemul_avx(poly *c, const poly *a, const poly *b)
{
  basemul_avx(c, a, b);
}

int poly_baseinv_avx(poly *b, const poly *a)
{
  return baseinv_avx2(b->coeffs, a->coeffs);
}

void poly_add_avx(poly *c, const poly *a, const poly *b)
{
  polyadd_avx(c->coeffs,a->coeffs,b->coeffs);
}

void poly_freeze_avx(poly *a)
{
  freeze_avx(a->coeffs);
}

void poly_multi_p_avx(poly *b, const poly *a)
{
  polydouble_avx(b->coeffs, a->coeffs);
}
