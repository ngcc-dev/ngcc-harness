#include <stdint.h>
#include <stdio.h>
#include <immintrin.h>
#include "params.h"
#include "reduce.h"
#include "poly.h"
#include "coding.h"
#include "cbd.h"
#include "inverse.h"
#include "ntt.h"
#include "ntt_avx2.h"
#include "inverse_avx2.h"
void poly_reduce(poly *a)
{
  for (int i = 0; i < DTRU_N; ++i)
    a->coeffs[i] = barrett_reduce(a->coeffs[i]);
}

void poly_reduce_avx2(poly *a)
{
  for (int i = 0; i < DTRU_N / 8 / 16; ++i) {
    __m256i barrett_v_vec = _mm256_set1_epi16(BARRETT_V);
    //__m256i q_vec = _mm256_set1_epi16(DTRU_Q);
    __m256i a_vec[8], a_vec_reduced[8];
    a_vec[0] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +   0));
    a_vec[1] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  16));
    a_vec[2] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  32));
    a_vec[3] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  48));
    a_vec[4] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  64));
    a_vec[5] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  80));
    a_vec[6] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  96));
    a_vec[7] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i + 112));
    a_vec_reduced[0] = _mm256_barrett_epi16(a_vec[0], barrett_v_vec, q_vec);
    a_vec_reduced[1] = _mm256_barrett_epi16(a_vec[1], barrett_v_vec, q_vec);
    a_vec_reduced[2] = _mm256_barrett_epi16(a_vec[2], barrett_v_vec, q_vec);
    a_vec_reduced[3] = _mm256_barrett_epi16(a_vec[3], barrett_v_vec, q_vec);
    a_vec_reduced[4] = _mm256_barrett_epi16(a_vec[4], barrett_v_vec, q_vec);
    a_vec_reduced[5] = _mm256_barrett_epi16(a_vec[5], barrett_v_vec, q_vec);
    a_vec_reduced[6] = _mm256_barrett_epi16(a_vec[6], barrett_v_vec, q_vec);
    a_vec_reduced[7] = _mm256_barrett_epi16(a_vec[7], barrett_v_vec, q_vec);

    _mm256_storeu_si256((__m256i *)(a->coeffs + 128 * i +   0), a_vec_reduced[0]);
    _mm256_storeu_si256((__m256i *)(a->coeffs + 128 * i +  16), a_vec_reduced[1]);
    _mm256_storeu_si256((__m256i *)(a->coeffs + 128 * i +  32), a_vec_reduced[2]);
    _mm256_storeu_si256((__m256i *)(a->coeffs + 128 * i +  48), a_vec_reduced[3]);
    _mm256_storeu_si256((__m256i *)(a->coeffs + 128 * i +  64), a_vec_reduced[4]);
    _mm256_storeu_si256((__m256i *)(a->coeffs + 128 * i +  80), a_vec_reduced[5]);
    _mm256_storeu_si256((__m256i *)(a->coeffs + 128 * i +  96), a_vec_reduced[6]);
    _mm256_storeu_si256((__m256i *)(a->coeffs + 128 * i + 112), a_vec_reduced[7]);
  }
}

void poly_freeze(poly *a)
{
  poly_reduce(a);
  for (int i = 0; i < DTRU_N; ++i)
    a->coeffs[i] = fqcsubq(a->coeffs[i]);
}

void poly_freeze_avx2(poly *a)
{
  poly_reduce_avx2(a);
  for (int i = 0; i < DTRU_N / 8 / 16; ++i) {
    //__m256i q_vec = _mm256_set1_epi16(DTRU_Q);
    __m256i a_vec[8];
    a_vec[0] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +   0));
    a_vec[1] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  16));
    a_vec[2] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  32));
    a_vec[3] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  48));
    a_vec[4] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  64));
    a_vec[5] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  80));
    a_vec[6] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  96));
    a_vec[7] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i + 112));

    a_vec[0] = fqcsubq_avx2(a_vec[0], q_vec);
    a_vec[1] = fqcsubq_avx2(a_vec[1], q_vec);
    a_vec[2] = fqcsubq_avx2(a_vec[2], q_vec);
    a_vec[3] = fqcsubq_avx2(a_vec[3], q_vec);
    a_vec[4] = fqcsubq_avx2(a_vec[4], q_vec);
    a_vec[5] = fqcsubq_avx2(a_vec[5], q_vec);
    a_vec[6] = fqcsubq_avx2(a_vec[6], q_vec);
    a_vec[7] = fqcsubq_avx2(a_vec[7], q_vec);

    _mm256_storeu_si256((__m256i *)(a->coeffs + 128 * i +   0), a_vec[0]);
    _mm256_storeu_si256((__m256i *)(a->coeffs + 128 * i +  16), a_vec[1]);
    _mm256_storeu_si256((__m256i *)(a->coeffs + 128 * i +  32), a_vec[2]);
    _mm256_storeu_si256((__m256i *)(a->coeffs + 128 * i +  48), a_vec[3]);
    _mm256_storeu_si256((__m256i *)(a->coeffs + 128 * i +  64), a_vec[4]);
    _mm256_storeu_si256((__m256i *)(a->coeffs + 128 * i +  80), a_vec[5]);
    _mm256_storeu_si256((__m256i *)(a->coeffs + 128 * i +  96), a_vec[6]);
    _mm256_storeu_si256((__m256i *)(a->coeffs + 128 * i + 112), a_vec[7]);
  }
}

void poly_add(poly *c, const poly *a, const poly *b)
{
  for (int i = 0; i < DTRU_N; ++i)
    c->coeffs[i] = a->coeffs[i] + b->coeffs[i];
}

void poly_add_avx2(poly *c, const poly *a, const poly *b)
{
  for (int i = 0; i < DTRU_N / 8 / 8; ++i) {
    __m256i a_vec[8], b_vec[8], c_vec[8];

    a_vec[0] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +   0));
    a_vec[1] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  16));
    a_vec[2] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  32));
    a_vec[3] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  48));
    a_vec[4] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  64));
    a_vec[5] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  80));
    a_vec[6] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  96));
    a_vec[7] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i + 112));

    b_vec[0] = _mm256_loadu_si256((__m256i *)(b->coeffs + 128 * i +   0));
    b_vec[1] = _mm256_loadu_si256((__m256i *)(b->coeffs + 128 * i +  16));
    b_vec[2] = _mm256_loadu_si256((__m256i *)(b->coeffs + 128 * i +  32));
    b_vec[3] = _mm256_loadu_si256((__m256i *)(b->coeffs + 128 * i +  48));
    b_vec[4] = _mm256_loadu_si256((__m256i *)(b->coeffs + 128 * i +  64));
    b_vec[5] = _mm256_loadu_si256((__m256i *)(b->coeffs + 128 * i +  80));
    b_vec[6] = _mm256_loadu_si256((__m256i *)(b->coeffs + 128 * i +  96));
    b_vec[7] = _mm256_loadu_si256((__m256i *)(b->coeffs + 128 * i + 112));

    c_vec[0] = _mm256_add_epi16(a_vec[0], b_vec[0]);
    c_vec[1] = _mm256_add_epi16(a_vec[1], b_vec[1]);
    c_vec[2] = _mm256_add_epi16(a_vec[2], b_vec[2]);
    c_vec[3] = _mm256_add_epi16(a_vec[3], b_vec[3]);
    c_vec[4] = _mm256_add_epi16(a_vec[4], b_vec[4]);
    c_vec[5] = _mm256_add_epi16(a_vec[5], b_vec[5]);
    c_vec[6] = _mm256_add_epi16(a_vec[6], b_vec[6]);
    c_vec[7] = _mm256_add_epi16(a_vec[7], b_vec[7]);

    _mm256_storeu_si256((__m256i *)(c->coeffs + 128 * i +   0), c_vec[0]);
    _mm256_storeu_si256((__m256i *)(c->coeffs + 128 * i +  16), c_vec[1]);
    _mm256_storeu_si256((__m256i *)(c->coeffs + 128 * i +  32), c_vec[2]);
    _mm256_storeu_si256((__m256i *)(c->coeffs + 128 * i +  48), c_vec[3]);
    _mm256_storeu_si256((__m256i *)(c->coeffs + 128 * i +  64), c_vec[4]);
    _mm256_storeu_si256((__m256i *)(c->coeffs + 128 * i +  80), c_vec[5]);
    _mm256_storeu_si256((__m256i *)(c->coeffs + 128 * i +  96), c_vec[6]);
    _mm256_storeu_si256((__m256i *)(c->coeffs + 128 * i + 112), c_vec[7]);
  }
}

void poly_multi_p(poly *b, const poly *a)
{
  for (int i = 0; i < DTRU_N; ++i)
  {
    b->coeffs[i] = 2 * a->coeffs[i];
  }
}

void poly_multi_p_avx2(poly *b, const poly *a)
{
  for (int i = 0; i < DTRU_N / 8 / 16; ++i) {
    __m256i a_vec[8], b_vec[8];

    a_vec[0] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +   0));
    a_vec[1] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  16));
    a_vec[2] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  32));
    a_vec[3] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  48));
    a_vec[4] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  64));
    a_vec[5] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  80));
    a_vec[6] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i +  96));
    a_vec[7] = _mm256_loadu_si256((__m256i *)(a->coeffs + 128 * i + 112));

    /* ×2 == 左移 1 位 */
    b_vec[0] = _mm256_slli_epi16(a_vec[0], 1);
    b_vec[1] = _mm256_slli_epi16(a_vec[1], 1);
    b_vec[2] = _mm256_slli_epi16(a_vec[2], 1);
    b_vec[3] = _mm256_slli_epi16(a_vec[3], 1);
    b_vec[4] = _mm256_slli_epi16(a_vec[4], 1);
    b_vec[5] = _mm256_slli_epi16(a_vec[5], 1);
    b_vec[6] = _mm256_slli_epi16(a_vec[6], 1);
    b_vec[7] = _mm256_slli_epi16(a_vec[7], 1);

    _mm256_storeu_si256((__m256i *)(b->coeffs + 128 * i +   0), b_vec[0]);
    _mm256_storeu_si256((__m256i *)(b->coeffs + 128 * i +  16), b_vec[1]);
    _mm256_storeu_si256((__m256i *)(b->coeffs + 128 * i +  32), b_vec[2]);
    _mm256_storeu_si256((__m256i *)(b->coeffs + 128 * i +  48), b_vec[3]);
    _mm256_storeu_si256((__m256i *)(b->coeffs + 128 * i +  64), b_vec[4]);
    _mm256_storeu_si256((__m256i *)(b->coeffs + 128 * i +  80), b_vec[5]);
    _mm256_storeu_si256((__m256i *)(b->coeffs + 128 * i +  96), b_vec[6]);
    _mm256_storeu_si256((__m256i *)(b->coeffs + 128 * i + 112), b_vec[7]);
  }
}

void poly_sample_keygen_f(poly *a, const unsigned char *buf)
{
  cbd1(a, buf);
}

void poly_sample_keygen_g(poly *a, const unsigned char *buf)
{
  cbd5(a, buf);
}

void poly_sample_enc_r(poly *a, const unsigned char *buf)
{
  cbd2(a, buf);
}

void poly_sample_enc_e(poly *a, const unsigned char *buf)
{
  cbd1(a, buf);
}

// AVX2 versions of poly_sample functions
void poly_sample_keygen_f_avx2(poly *a, const unsigned char *buf)
{
  cbd1_avx2(a, buf);
}

void poly_sample_keygen_g_avx2(poly *a, const unsigned char *buf)
{
  cbd5_avx2(a, buf);
}

void poly_sample_enc_r_avx2(poly *a, const unsigned char *buf)
{
  cbd2_avx2(a, buf);
}

void poly_sample_enc_e_avx2(poly *a, const unsigned char *buf)
{
  cbd1_avx2(a, buf);
}

void poly_ntt(poly *b)
{
  ntt(b->coeffs);
}

void poly_ntt_avx2(poly *b)
{
  ntt_avx2(b->coeffs, b->coeffs);
}

void poly_invntt(poly *b)
{
  invntt(b->coeffs);
}

void poly_invntt_avx2(poly *b)
{
  invntt_avx2(b->coeffs, b->coeffs);
}

void poly_basemul(poly *c, const poly *a, const poly *b)
{
  for (int i = 0; i < DTRU_N / 32; ++i)
  {
    basemul(c->coeffs + 32 * i,
            a->coeffs + 32 * i,
            b->coeffs + 32 * i,
            zetas[64 + i]);
    basemul(c->coeffs + 32 * i + 16,
            a->coeffs + 32 * i + 16,
            b->coeffs + 32 * i + 16,
            -zetas[64 + i]);
  }
}

void poly_basemul_avx2(poly *c, const poly *a, const poly *b)
{
  for (int i = 0; i < DTRU_N / 32 / 8; ++i)
  {
    basemul_avx2(c->coeffs + 256 * i,
                      a->coeffs + 256 * i,
                      b->coeffs + 256 * i,
                      zetas + 64 + 8 * i);
  }
}

int poly_baseinv(poly *b, const poly *a)
{
  // int r = 0;
  // for (int i = 0; i < DTRU_N / 32; ++i)
  // {
  //   if(r != 0)   return r;
  //   r += rq_inverse_recursive16(b->coeffs + 32 * i,
  //                   a->coeffs + 32 * i,
  //                   zetas[64 + i]);
  //   if(r != 0)   return r;
  //   r += rq_inverse_recursive16(b->coeffs + 32 * i + 16,
  //                   a->coeffs + 32 * i + 16,
  //                   -zetas[64 + i]);
  // }
  // return r;
  return baseinv_avx2(b->coeffs, a->coeffs);
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
