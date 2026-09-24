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
  for (int i = 0; i < DTRU_N / 2; ++i)
  {
    b->coeffs[i] = a->coeffs[i + DTRU_N / 2] + a->coeffs[i];
    b->coeffs[i + DTRU_N / 2] = a->coeffs[i + DTRU_N / 2] - a->coeffs[i];
  }
}

void poly_sample_keygen_f(poly *a, const unsigned char *buf)
{
  cbd1(a, buf);
}

void poly_sample_keygen_g(poly *a, const unsigned char *buf)
{
  cbd2(a, buf);
}

void poly_sample_enc_r(poly *a, const unsigned char *buf)
{
  cbd1(a, buf);
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
  for (int i = 0; i < DTRU_N / 8; ++i)
  {
    basemul(c->coeffs + 8 * i,
            a->coeffs + 8 * i,
            b->coeffs + 8 * i,
            zetas[64 + i]);
    basemul(c->coeffs + 8 * i + 4,
            a->coeffs + 8 * i + 4,
            b->coeffs + 8 * i + 4,
            -zetas[64 + i]);
  }
}

int poly_baseinv(poly *b, const poly *a)
{
  int r = 0;

  for (int i = 0; i < DTRU_N / 8; ++i)
  {
    if (r != 0)
      return r;
    r += rq_inverse_recursive(b->coeffs + 8 * i,
                              a->coeffs + 8 * i,
                              zetas[64 + i]);
    if (r != 0)
      return r;
    r += rq_inverse_recursive(b->coeffs + 8 * i + 4,
                              a->coeffs + 8 * i + 4,
                              -zetas[64 + i]);
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
    unsigned int msg_index = i & (DTRU_N / 16 - 1);
    tmp = msg[msg_index >> 1];
    tmp = (tmp >> ((msg_index & 1) << 2)) & 0xF;
    tmp = encode_e8(tmp);

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
    msg[i] = 0;

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

void poly_ntt_avx(poly *a)
{
  poly_ntt(a);
}

void poly_invntt_avx(poly *a)
{
  poly_invntt(a);
}

void poly_basemul_avx(poly *c, const poly *a, const poly *b)
{
  poly_basemul(c, a, b);
}

int poly_baseinv_avx(poly *b, const poly *a)
{
  return poly_baseinv(b, a);
}

void poly_reduce_avx(poly *a)
{
  poly_reduce(a);
}

void fqcsubq_avx(poly *a)
{
  for (int i = 0; i < DTRU_N; ++i)
    a->coeffs[i] = fqcsubq(a->coeffs[i]);
}

void poly_freeze_avx(poly *a)
{
  poly_freeze(a);
}

void poly_add_avx(poly *c, const poly *a, const poly *b)
{
  poly_add(c, a, b);
}

void poly_double_avx(poly *b, const poly *a)
{
  for (int i = 0; i < DTRU_N; ++i)
    b->coeffs[i] = a->coeffs[i] + a->coeffs[i];
}

void poly_freeze_avx2(poly *a)
{
  poly_freeze(a);
}

void poly_decode_avx(unsigned char *msg, const poly *cf)
{
  poly_decode(msg, cf);
}

void poly_ntt_avx2(poly *b)
{
  poly_ntt(b);
}

void poly_invntt_avx2(poly *b)
{
  poly_invntt(b);
}

void poly_basemul_avx2(poly *c, const poly *a, const poly *b)
{
  poly_basemul(c, a, b);
}

int poly_baseinv_avx2(poly *b, const poly *a)
{
  return poly_baseinv(b, a);
}
