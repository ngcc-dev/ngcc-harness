#include <stdint.h>
#include "params.h"
#include "reduce.h"
#include "poly.h"
#include "coding.h"
#include "cbd.h"
#include "inverse.h"

void poly_fqcsubq(poly *a)
{
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
    b->coeffs[i] = 2 * a->coeffs[i];
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
  cbd2(a, buf);
}

void poly_sample_enc_e(poly *a, const unsigned char *buf)
{
  cbd1(a, buf);
}

void poly_inverse(poly *b, const poly *a)
{
  rq_inverse(b->coeffs, a->coeffs);
}

void poly_encode_compress(poly *c,
                          const poly *sigma,
                          const unsigned char *msg)
{
  unsigned int i, j;
  int16_t mask;
  uint8_t mh[4 * DTRU_MSGBYTES];
  uint8_t tmp;
  int16_t s;
  int32_t t;
  for (i = 0; i < DTRU_MSGBYTES; i++)
  {
    tmp = msg[i] & 0xF;
    mh[2 * i] = encode_e8(tmp);
    mh[2 * i + 2 * DTRU_MSGBYTES] = mh[2 * i];

    tmp = (msg[i] >> 4) & 0xF;
    mh[2 * i + 1] = encode_e8(tmp);
    mh[2 * i + 1 + 2 * DTRU_MSGBYTES] = mh[2 * i + 1];
  }

  for (i = 0; i < 4 * DTRU_MSGBYTES; i++)
  {
    for (j = 0; j < 8; j++)
    {
      mask = -(int16_t)((mh[i] >> j) & 1);
      s = sigma->coeffs[8 * i + j] + (mask & ((DTRU_Q + 1) >> 1));
      t = ((int32_t)(s << DTRU_LOGQ2) + (DTRU_Q >> 1)) / DTRU_Q;
      c->coeffs[8 * i + j] = t & (DTRU_Q2 - 1);
    }
  }
  for (j = 8 * i; j < DTRU_N; j++)
  {
    t = ((int32_t)(sigma->coeffs[j] << DTRU_LOGQ2) + (DTRU_Q >> 1)) / DTRU_Q;
    c->coeffs[j] = t & (DTRU_Q2 - 1);
  }
}

void poly_decode(unsigned char *msg,
                 const poly *cf)
{
  unsigned int i, j;
  int16_t tmp_mp[16];

  for (i = 0; i < DTRU_MSGBYTES; i++)
    msg[i] = 0;

  for (i = 0; i < 2 * DTRU_MSGBYTES; i++)
  {
    for (j = 0; j < 8; j++)
    {
      tmp_mp[j] = cf->coeffs[8 * i + j];
      tmp_mp[j + 8] = cf->coeffs[8 * i + j + 16 * DTRU_MSGBYTES];
    }
    msg[i >> 1] |= decode_e8(tmp_mp) << ((i & 1) << 2);
  }
}
