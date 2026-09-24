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
  return baseinv_avx2(b->coeffs, a->coeffs);
}

void poly_ntt_simd_v2(poly_simd *out, const poly *in)
{
  ntt_forward_81x8_v2(out->vec, in->coeffs);
}

void poly_basemul_simd(poly_simd *c, const poly_simd *a, const poly_simd *b)
{
  basemul_81x8(c->vec, a->vec, b->vec);
}

void poly_invntt_simd(poly *out, const poly_simd *in)
{
  invntt_full_81x8(out->coeffs, in->vec);
}

static void poly_from_simd(poly *out, const poly_simd *in)
{
  int32_t lane[8];

  for (int i = 0; i < 81; ++i)
  {
    _mm256_storeu_si256((__m256i *)lane, in->vec[i]);
    for (int j = 0; j < 8; ++j)
      out->coeffs[81 * j + i] = (int16_t)lane[j];
  }
}

static void poly_to_simd(poly_simd *out, const poly *in)
{
  for (int i = 0; i < 81; ++i)
    out->vec[i] = _mm256_setr_epi32(
        (int32_t)in->coeffs[i],
        (int32_t)in->coeffs[81 + i],
        (int32_t)in->coeffs[162 + i],
        (int32_t)in->coeffs[243 + i],
        (int32_t)in->coeffs[324 + i],
        (int32_t)in->coeffs[405 + i],
        (int32_t)in->coeffs[486 + i],
        (int32_t)in->coeffs[567 + i]);
}

int poly_baseinv_simd(poly_simd *b, const poly_simd *a)
{
  poly a_scalar, b_scalar;
  int r;

  poly_from_simd(&a_scalar, a);
  r = baseinv_avx2(b_scalar.coeffs, a_scalar.coeffs);
  poly_to_simd(b, &b_scalar);
  return r;
}

void poly_encode_compress(poly *c,
                          const poly *sigma,
                          const unsigned char *msg)
{
  unsigned int i, j;
  int16_t mask;
  uint8_t mh[81] = {0};
  uint8_t tmp;
  int16_t s;
  int32_t t;
  for (i = 0; i < DTRU_MSGBYTES; i++)
  {
    tmp = msg[i] & 0xF;
    mh[2 * i] = encode_e8(tmp);
    mh[2 * i + 40] = mh[2 * i];

    tmp = (msg[i] >> 4) & 0xF;
    mh[2 * i + 1] = encode_e8(tmp);
    mh[2 * i + 1 + 40] = mh[2 * i + 1];
  }

  for (i = 0; i < 81; i++)
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

#include "poly_avx2.h"
void poly_freeze_avx2(poly *a)
{
  poly_freeze_avx2_impl(a);
}

void poly_add_avx2(poly *c, const poly *a, const poly *b)
{
  poly_add_avx2_impl(c, a, b);
}

void poly_multi_p_avx2(poly *b, const poly *a)
{
  poly_multi_p_avx2_impl(b, a);
}
