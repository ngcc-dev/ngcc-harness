#include <stdint.h>
#include "params.h"
#include "poly.h"
#include "polyvec.h"
#include "ntt.h"
#include "hashkdf.h"
#include "api.h"

void polyvecl_reduce(polyvecl *v) {
	int i;
	for (i = 0; i < PARAM_L; ++i)
		poly_reduce(v->vec + i);
}
void polyvecl_add(polyvecl *w, const polyvecl *u, const polyvecl *v) {
  unsigned int i;

  for(i = 0; i < PARAM_L; ++i)
    poly_add(w->vec+i, u->vec+i, v->vec+i);
}

void polyvecl_ntt(polyvecl *v) {
  unsigned int i;

  for(i = 0; i < PARAM_L; ++i)
    poly_ntt(v->vec+i);
}
void polyvecl_uniform_gamma1(polyvecl* v, unsigned char* seed, unsigned int nonce)
{
#if defined(KDF_AVX) && !defined(USE_NICCS_API)
	int i;
	unsigned char inbuf[4][SEEDBYTES + CRHBYTES + 2];
	unsigned char outbuf[4][SZBITS * PARAM_N / 8];
	for (i = 0; i < SEEDBYTES + CRHBYTES; ++i)
	{
		inbuf[0][i] = seed[i];
		inbuf[1][i] = seed[i];
#if PARAM_L > 2
		inbuf[2][i] = seed[i];
		inbuf[3][i] = seed[i];
#endif
	}
	inbuf[0][SEEDBYTES + CRHBYTES] = nonce & 0xFF;
	inbuf[0][SEEDBYTES + CRHBYTES + 1] = nonce >> 8;
	nonce++;
	inbuf[1][SEEDBYTES + CRHBYTES] = nonce & 0xFF;
	inbuf[1][SEEDBYTES + CRHBYTES + 1] = nonce >> 8;
	nonce++;
#if PARAM_L>2
	inbuf[2][SEEDBYTES + CRHBYTES] = nonce & 0xFF;
	inbuf[2][SEEDBYTES + CRHBYTES + 1] = nonce >> 8;
	nonce++;
	
	inbuf[3][SEEDBYTES + CRHBYTES] = nonce & 0xFF;
	inbuf[3][SEEDBYTES + CRHBYTES + 1] = nonce >> 8;
	nonce++;
#endif


	KDFX4(outbuf[0], outbuf[1], outbuf[2], outbuf[3], sizeof(outbuf[0]),
		inbuf[0], inbuf[1], inbuf[2], inbuf[3], SEEDBYTES + CRHBYTES + 2);

	polyz_unpack(v->vec, outbuf[0]);
	polyz_unpack(v->vec + 1, outbuf[1]);
#if PARAM_L > 2
	polyz_unpack(v->vec + 2, outbuf[2]);
	polyz_unpack(v->vec + 3, outbuf[3]);
#endif

#if PARAM_L == 7
	inbuf[0][SEEDBYTES + CRHBYTES] = nonce & 0xFF;
	inbuf[0][SEEDBYTES + CRHBYTES + 1] = nonce >> 8;
	nonce++;
	inbuf[1][SEEDBYTES + CRHBYTES] = nonce & 0xFF;
	inbuf[1][SEEDBYTES + CRHBYTES + 1] = nonce >> 8;
	nonce++;
	inbuf[2][SEEDBYTES + CRHBYTES] = nonce & 0xFF;
	inbuf[2][SEEDBYTES + CRHBYTES + 1] = nonce >> 8;
	nonce++;

	KDFX4(outbuf[0], outbuf[1], outbuf[2], outbuf[3], sizeof(outbuf[0]),
		inbuf[0], inbuf[1], inbuf[2], inbuf[3], SEEDBYTES + CRHBYTES + 2);

	polyz_unpack(v->vec + 4, outbuf[0]);
	polyz_unpack(v->vec + 5, outbuf[1]);
	polyz_unpack(v->vec + 6, outbuf[2]);
#endif
#else
	int i;
	unsigned char inbuf[SEEDBYTES + CRHBYTES + 2];
	unsigned char outbuf[SZBITS * PARAM_N / 8];
	for (i = 0; i < SEEDBYTES + CRHBYTES; ++i)
		inbuf[i] = seed[i];
	for (i = 0; i < PARAM_L; i++)
	{
		inbuf[SEEDBYTES + CRHBYTES] = nonce & 0xFF;
		inbuf[SEEDBYTES + CRHBYTES + 1] = nonce >> 8;
		KDF(outbuf, sizeof(outbuf), inbuf, SEEDBYTES + CRHBYTES + 2);
		polyz_unpack(v->vec + i, outbuf);
		nonce++;
	}
#endif
}

void polyvecl_uniform_eta1(polyvecl* v, unsigned char* seed, unsigned int nonce)
{
#if defined(KDF_AVX) && !defined(USE_NICCS_API)
	int i;
	ALIGN(32) unsigned char inbuf[4][SEEDBYTES + 1];
	ALIGN(32) unsigned char outbuf[4][REJ_ETA1_BYTES + KDF_RATE];
	int nblock = (REJ_ETA1_BYTES + KDF_RATE - 1) / KDF_RATE;
	int len = nblock * KDF_RATE;

	for (i = 0; i < SEEDBYTES; ++i)
	{
		inbuf[0][i] = seed[i];
		inbuf[1][i] = seed[i];
#if PARAM_L>2
		inbuf[2][i] = seed[i];
		inbuf[3][i] = seed[i];
#endif
	}
	inbuf[0][SEEDBYTES] = nonce;
	nonce++;
	inbuf[1][SEEDBYTES] = nonce;
	nonce++;
#if PARAM_L>2
	inbuf[2][SEEDBYTES] = nonce;
	nonce++;
	inbuf[3][SEEDBYTES] = nonce;
	nonce++;
#endif

	KDFX4(outbuf[0], outbuf[1], outbuf[2], outbuf[3], len,
		inbuf[0], inbuf[1], inbuf[2], inbuf[3], SEEDBYTES + 1);

	if (poly_uniform_eta1(v->vec, outbuf[0], len) != PARAM_N)
		poly_uniform_eta1_seed(v->vec, inbuf[0], SEEDBYTES + 1);
	if (poly_uniform_eta1(v->vec + 1, outbuf[1], len) != PARAM_N)
		poly_uniform_eta1_seed(v->vec + 1, inbuf[1], SEEDBYTES + 1);
#if PARAM_L>2
	if (poly_uniform_eta1(v->vec + 2, outbuf[2], len) != PARAM_N)
		poly_uniform_eta1_seed(v->vec + 2, inbuf[2], SEEDBYTES + 1);
	if (poly_uniform_eta1(v->vec + 3, outbuf[3], len) != PARAM_N)
		poly_uniform_eta1_seed(v->vec + 3, inbuf[3], SEEDBYTES + 1);
#endif

#if PARAM_L == 7
	inbuf[0][SEEDBYTES] = nonce;
	nonce++;
	inbuf[1][SEEDBYTES] = nonce;
	nonce++;
	inbuf[2][SEEDBYTES] = nonce;

	KDFX4(outbuf[0], outbuf[1], outbuf[2], outbuf[3], len,
		inbuf[0], inbuf[1], inbuf[2], inbuf[3], SEEDBYTES + 1);

	if (poly_uniform_eta1(v->vec + 4, outbuf[0], len) != PARAM_N)
		poly_uniform_eta1_seed(v->vec + 4, inbuf[0], SEEDBYTES + 1);
	if (poly_uniform_eta1(v->vec + 5, outbuf[1], len) != PARAM_N)
		poly_uniform_eta1_seed(v->vec + 5, inbuf[1], SEEDBYTES + 1);
	if (poly_uniform_eta1(v->vec + 6, outbuf[2], len) != PARAM_N)
		poly_uniform_eta1_seed(v->vec + 6, inbuf[2], SEEDBYTES + 1);
#endif
#else
	int i;
	ALIGN(32) unsigned char inbuf[SEEDBYTES + 1];
	for (i = 0; i < SEEDBYTES; ++i)
		inbuf[i] = seed[i];
	for (i = 0; i < PARAM_L; i++)
	{
		inbuf[SEEDBYTES] = nonce;
		poly_uniform_eta1_seed(v->vec + i, inbuf, SEEDBYTES + 1);
		nonce++;
	}
#endif
}
void polyvecl_pointwise_acc_montgomery(poly *w,
                                          const polyvecl *u,
                                          const polyvecl *v) 
{
  unsigned int i;
  poly t;

  poly_pointwise_montgomery(w, u->vec+0, v->vec+0);

  for(i = 1; i < PARAM_L; ++i) {
    poly_pointwise_montgomery(&t, u->vec+i, v->vec+i);
    poly_add(w, w, &t);
  }
  poly_reduce(w);
}

int polyvecl_chknorm(const polyvecl *v, uint32_t bound)  {
  unsigned int i;
  int ret = 0;

  for(i = 0; i < PARAM_L; ++i)
    ret |= poly_chknorm(v->vec+i, bound);

  return ret;
}
void polyveck_amodq(polyveck *v) {
	int i;
	for (i = 0; i < PARAM_K; ++i)
		poly_amodq(v->vec + i);
}
void polyveck_cmodq(polyveck *v) {
	int i;
	for (i = 0; i < PARAM_K; ++i)
		poly_cmodq(v->vec + i);
}
void polyveck_reduce(polyveck *v) {
	int i;
	for (i = 0; i < PARAM_K; ++i)
		poly_reduce(v->vec + i);
}
void polyveck_g_reduce(polyveck *v) {
	int i;
	for (i = 0; i < PARAM_K; ++i)
		poly_g_reduce_avx(v->vec + i);
}
void polyveck_add(polyveck *w, const polyveck *u, const polyveck *v) {
  unsigned int i;

  for(i = 0; i < PARAM_K; ++i)
    poly_add(w->vec+i, u->vec+i, v->vec+i);
}

void polyveck_sub(polyveck *w, const polyveck *u, const polyveck *v) {
  unsigned int i;

  for(i = 0; i < PARAM_K; ++i)
    poly_sub(w->vec+i, u->vec+i, v->vec+i);
}

void polyveck_subw(polyveck *v, const polyveck *u, const polyveck *w)
{
	unsigned int i;

	for (i = 0; i < PARAM_K; ++i)
		poly_subw(v->vec + i, u->vec + i, w->vec + i);
}

void polyveck_shiftl(polyveck *v, unsigned int k) { 
  unsigned int i;

  for(i = 0; i < PARAM_K; ++i)
    poly_shiftl(v->vec+i, k);
}
void polyveck_uniform_eta2(polyveck* v, unsigned char* seed, unsigned int nonce)
{
#if defined(KDF_AVX) && !defined(USE_NICCS_API)
	int i;
	ALIGN(32) unsigned char inbuf[4][SEEDBYTES + 1];
	ALIGN(32) unsigned char outbuf[4][REJ_ETA2_BYTES + KDF_RATE];
	int nblock = (REJ_ETA2_BYTES + KDF_RATE - 1) / KDF_RATE;
	int len = nblock * KDF_RATE;

	for (i = 0; i < SEEDBYTES; ++i)
	{
		inbuf[0][i] = seed[i];
		inbuf[1][i] = seed[i];
#if PARAM_K>2
		inbuf[2][i] = seed[i];
		inbuf[3][i] = seed[i];
#endif
	}
	inbuf[0][SEEDBYTES] = nonce;
	nonce++;
	inbuf[1][SEEDBYTES] = nonce;
	nonce++;
#if PARAM_K>2
	inbuf[2][SEEDBYTES] = nonce;
	nonce++;
	inbuf[3][SEEDBYTES] = nonce;
	nonce++;
#endif

	KDFX4(outbuf[0], outbuf[1], outbuf[2], outbuf[3], len,
		inbuf[0], inbuf[1], inbuf[2], inbuf[3], SEEDBYTES + 1);

	if (poly_uniform_eta2(v->vec, outbuf[0], len) != PARAM_N)
		poly_uniform_eta2_seed(v->vec, inbuf[0], SEEDBYTES + 1);
	if (poly_uniform_eta2(v->vec + 1, outbuf[1], len) != PARAM_N)
		poly_uniform_eta2_seed(v->vec + 1, inbuf[1], SEEDBYTES + 1);
#if PARAM_K > 2
	if (poly_uniform_eta2(v->vec + 2, outbuf[2], len) != PARAM_N)
		poly_uniform_eta2_seed(v->vec + 2, inbuf[2], SEEDBYTES + 1);
	if (poly_uniform_eta2(v->vec + 3, outbuf[3], len) != PARAM_N)
		poly_uniform_eta2_seed(v->vec + 3, inbuf[3], SEEDBYTES + 1);
#endif

#if PARAM_K == 8
	inbuf[0][SEEDBYTES] = nonce;
	nonce++;
	inbuf[1][SEEDBYTES] = nonce;
	nonce++;
	inbuf[2][SEEDBYTES] = nonce;
	nonce++;
	inbuf[3][SEEDBYTES] = nonce;

	KDFX4(outbuf[0], outbuf[1], outbuf[2], outbuf[3], len,
		inbuf[0], inbuf[1], inbuf[2], inbuf[3], SEEDBYTES + 1);

	if (poly_uniform_eta2(v->vec + 4, outbuf[0], len) != PARAM_N)
		poly_uniform_eta2_seed(v->vec + 4, inbuf[0], SEEDBYTES + 1);
	if (poly_uniform_eta2(v->vec + 5, outbuf[1], len) != PARAM_N)
		poly_uniform_eta2_seed(v->vec + 5, inbuf[1], SEEDBYTES + 1);
	if (poly_uniform_eta2(v->vec + 6, outbuf[2], len) != PARAM_N)
		poly_uniform_eta2_seed(v->vec + 6, inbuf[2], SEEDBYTES + 1);
	if (poly_uniform_eta2(v->vec + 7, outbuf[3], len) != PARAM_N)
		poly_uniform_eta2_seed(v->vec + 7, inbuf[3], SEEDBYTES + 1);
#endif
#else
	int i;
	ALIGN(32) unsigned char inbuf[SEEDBYTES + 1];
	for (i = 0; i < SEEDBYTES; ++i)
		inbuf[i] = seed[i];
	for (i = 0; i < PARAM_K; i++)
	{
		inbuf[SEEDBYTES] = nonce;
		poly_uniform_eta2_seed(v->vec + i, inbuf, SEEDBYTES + 1);
		nonce++;
	}
#endif
}
void polyveck_ntt(polyveck *v) {
  unsigned int i;

  for(i = 0; i < PARAM_K; ++i)
    poly_ntt(v->vec+i);
}

void polyveck_invntt_montgomery(polyveck *v) {
  unsigned int i;

  for(i = 0; i < PARAM_K; ++i)
    poly_invntt_montgomery(v->vec+i);
}

int polyveck_chknorm(const polyveck *v, uint32_t bound) {
  unsigned int i;
  int ret = 0;

  for(i = 0; i < PARAM_K; ++i)
    ret |= poly_chknorm(v->vec+i, bound);

  return ret;
}

void polyveck_power2round(polyveck *v1, polyveck *v0, const polyveck *v) {
  unsigned int i;
  for(i = 0; i < PARAM_K; ++i)
  	poly_power2round(&v1->vec[i],&v0->vec[i], &v->vec[i]);
}

void polyveck_decompose(polyveck *v1, polyveck *v0, const polyveck *v) {
  unsigned int i;
  for(i = 0; i < PARAM_K; ++i)
    poly_decompose(&v1->vec[i],&v0->vec[i],&v->vec[i]);
}
int32_t sec_make_hint(poly *h, const poly *a, const poly *b)
{
	int i, s = 0;
	__m256i t, r, r0, r1;
	__m256i *ph = (__m256i *)h->coeffs;
	__m256i *pa = (__m256i *)a->coeffs;
	__m256i *pb = (__m256i *)b->coeffs;
	__m256i gamma2 = _mm256_set1_epi32(GAMMA2 + 1);
	__m256i qmgamma2 = _mm256_set1_epi32(PARAM_Q - GAMMA2);
	__m256i zero = _mm256_setzero_si256();
	__m256i one = _mm256_set1_epi32(1);

	for (i = 0; i < SEC / 8; ++i)
	{
		t = _mm256_load_si256(&pa[i]);
		r0 = _mm256_cmpgt_epi32(gamma2, t);

		r1 = _mm256_cmpgt_epi32(t, qmgamma2);
		r = _mm256_or_si256(r0, r1);

		r0 = _mm256_cmpeq_epi32(t, qmgamma2);
		r1 = _mm256_cmpeq_epi32(pb[i], zero);
		r0 = _mm256_and_si256(r0, r1);

		r = _mm256_or_si256(r, r0);

		s += _mm_popcnt_u32(_mm256_movemask_ps(_mm256_castsi256_ps(r)));
		r = _mm256_add_epi32(r, one);
		_mm256_store_si256(&ph[i], r);
	}
	return SEC - s;
}

unsigned int polyveck_make_hint(polyveck *h,
								const polyveck *u,
								const polyveck *v)
{
	unsigned int i, j, k, s = 0, t = 0;

	for (i = 0; i < PARAM_K; ++i)
	{
		for (j = 0; j < PARAM_N / SEC; ++j)
		{
			s = sec_make_hint(&h->vec[i].coeffs[SEC * j],&u->vec[i].coeffs[SEC * j],&v->vec[i].coeffs[SEC * j]);
			if (s > NHW)
				return -1;
			t += s;
		}
	}
	return t;
}

void polyveck_use_hint(polyveck *w, const polyveck *u, const polyveck *h) {
	unsigned int i, j;
	poly v1, v0;
	for (i = 0; i < PARAM_K; ++i)
	{
		poly_decompose(&v1, &v0, &u->vec[i]);
		for (j = 0; j < PARAM_N; ++j)
		{
			if (h->vec[i].coeffs[j] == 0)
				w->vec[i].coeffs[j] = v1.coeffs[j];
			else if (v0.coeffs[j] > 0)
				w->vec[i].coeffs[j] = (v1.coeffs[j] == (PARAM_Q - 1) / ALPHA - 1) ? 0 : v1.coeffs[j] + 1;
			else
				w->vec[i].coeffs[j] = (v1.coeffs[j] == 0) ? (PARAM_Q - 1) / ALPHA - 1 : v1.coeffs[j] - 1;
		}
	}
}



