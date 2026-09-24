#include <stdint.h>
#include "params.h"
#include "poly.h"
#include "polyvec.h"

#include <string.h>

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

static int32_t rej_eta_1(int32_t *a, int32_t *cur, int32_t n, const uint8_t *buf, int32_t blen)
{
	int32_t ctr, pos;
	int32_t t[8];

	ctr = *cur;
	pos = 0;

	while (pos + 2 <= blen && ctr + 8 <= n)
	{
		t[0] = buf[pos] & 0x03;
		t[1] = (buf[pos] >> 2) & 0x03;
		t[2] = (buf[pos] >> 4) & 0x03;
		t[3] = (buf[pos++] >> 6) & 0x03;
		t[4] = buf[pos] & 0x03;
		t[5] = (buf[pos] >> 2) & 0x03;
		t[6] = (buf[pos] >> 4) & 0x03;
		t[7] = (buf[pos++] >> 6) & 0x03;

		if (t[0] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[0];
		if (t[1] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[1];
		if (t[2] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[2];
		if (t[3] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[3];
		if (t[4] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[4];
		if (t[5] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[5];
		if (t[6] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[6];
		if (t[7] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[7];
	}

	while (pos < blen && ctr < n)
	{
		t[0] = buf[pos] & 0x03;
		t[1] = (buf[pos] >> 2) & 0x03;
		t[2] = (buf[pos] >> 4) & 0x03;
		t[3] = (buf[pos++] >> 6) & 0x03;

		if (t[0] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[0];
		if (t[1] <= 2 * ETA1 && ctr < n)
			a[ctr++] = ETA1 - t[1];
		if (t[2] <= 2 * ETA1 && ctr < n)
			a[ctr++] = ETA1 - t[2];
		if (t[3] <= 2 * ETA1 && ctr < n)
			a[ctr++] = ETA1 - t[3];
	}
	*cur = ctr;
	return pos;
}

static int32_t rej_eta_2(int32_t *a, int32_t *cur, int32_t n, const uint8_t *buf, int32_t blen)
{
	const int32_t eta = 2;
	int32_t ctr, pos;
	int32_t t[8];

	ctr = *cur;
	pos = 0;

	while (pos + 3 < blen && ctr + 8 <= n)
	{
		t[0] = buf[pos] & 0x07;
		t[1] = (buf[pos] >> 3) & 0x07;
		t[2] = (buf[pos] >> 6) | ((buf[pos + 1] & 0x1) << 2);
		t[3] = (buf[++pos] >> 1) & 0x07;
		t[4] = (buf[pos] >> 4) & 0x07;
		t[5] = (buf[pos] >> 7) | ((buf[pos + 1] & 0x3) << 1);
		t[6] = (buf[++pos] >> 2) & 0x07;
		t[7] = buf[pos++] >> 5;

		if (t[0] <= 2 * eta)
			a[ctr++] = eta - t[0];
		if (t[1] <= 2 * eta)
			a[ctr++] = eta - t[1];
		if (t[2] <= 2 * eta)
			a[ctr++] = eta - t[2];
		if (t[3] <= 2 * eta)
			a[ctr++] = eta - t[3];
		if (t[4] <= 2 * eta)
			a[ctr++] = eta - t[4];
		if (t[5] <= 2 * eta)
			a[ctr++] = eta - t[5];
		if (t[6] <= 2 * eta)
			a[ctr++] = eta - t[6];
		if (t[7] <= 2 * eta)
			a[ctr++] = eta - t[7];

	}

	while (ctr < n && pos + 1 < blen)
	{
		t[0] = buf[pos] & 0x07;
		t[1] = (buf[pos] >> 3) & 0x07;
		if (t[0] <= 2 * eta)
			a[ctr++] = eta - t[0];
		if (ctr < n  &&  t[1] <= 2 * eta)
			a[ctr++] = eta - t[1];
		if (ctr >= n || pos + 2 > blen)
		{
			*cur = ctr;
			return pos + 1;
		}

		t[0] = (buf[pos] >> 6) | ((buf[pos + 1] & 0x1) << 2);
		t[1] = (buf[++pos] >> 1) & 0x07;
		t[2] = (buf[pos] >> 4) & 0x07;

		if (t[0] <= 2 * eta)
			a[ctr++] = eta - t[0];
		if (ctr < n && t[1] <= 2 * eta)
			a[ctr++] = eta - t[1];
		if (ctr < n && t[2] <= 2 * eta)
			a[ctr++] = eta - t[2];
		if (ctr >= n || pos + 2 > blen)
		{
			*cur = ctr;
			return pos + 1;
		}

		t[0] = (buf[pos] >> 7) | ((buf[pos + 1] & 0x3) << 1);
		t[1] = (buf[++pos] >> 2) & 0x07;
		t[2] = buf[pos++] >> 5;

		if (t[0] <= 2 * eta)
			a[ctr++] = eta - t[0];
		if (ctr < n && t[1] <= 2 * eta)
			a[ctr++] = eta - t[1];
		if (ctr < n && t[2] <= 2 * eta)
			a[ctr++] = eta - t[2];
	}

	* cur = ctr;
	return pos;
}

#ifdef USE_ICCS

void poly_uniform_eta_1(poly *a, const uint8_t seed[SEEDBYTES], uint8_t nonce)
{
	uint8_t i;
	int32_t cur = 0, pos, step;
	uint8_t inbuf[SEEDBYTES + 1];
	int32_t nblock = (REJ_ETA1_BYTES + KDF_RATE - 1) / KDF_RATE;
	uint8_t outbuf[nblock * KDF_RATE];
	int32_t len;

	memcpy(inbuf,seed,SEEDBYTES);
	inbuf[SEEDBYTES] = nonce;

	kdfstate ctx;
	kdf_init(&ctx, inbuf, SEEDBYTES + 1);
	kdf_squeezeblocks(outbuf, nblock, &ctx);

	len = nblock * KDF_RATE;

#if ETA1 == 1
	rej_eta_1(a->coeffs, &cur, PARAM_N, outbuf, len);
#elif ETA1 == 2
	rej_eta_2(a->coeffs, &cur, PARAM_N, outbuf, len);
#endif

	while (cur < PARAM_N)
	{
		kdf_squeezeblocks(outbuf, 1, &ctx);
#if ETA1 == 1
		rej_eta_1(a->coeffs, &cur, PARAM_N, outbuf, KDF_RATE);
#elif ETA1 == 2
		rej_eta_2(a->coeffs, &cur, PARAM_N, outbuf, KDF_RATE);
#endif
	}
}
#else
void poly_uniform_eta_1(poly *a, const uint8_t seed[SEEDBYTES], uint8_t nonce)
{
	uint8_t i;
	int32_t cur = 0, pos, step;
	uint8_t inbuf[SEEDBYTES + 1];
	int32_t nblock = (REJ_ETA1_BYTES + KDF_RATE - 1) / KDF_RATE;
	uint8_t outbuf[nblock * KDF_RATE];
	int32_t len;

	memcpy(inbuf,seed,SEEDBYTES);
	inbuf[SEEDBYTES] = nonce;

	kdfstate ctx;
	KDF_ABSORB(&ctx, inbuf, SEEDBYTES + 1);
	KDF_SQUEEZEBLOCK(outbuf, nblock, &ctx);
	len = nblock * KDF_RATE;

#if ETA1 == 1
	rej_eta_1(a->coeffs, &cur, PARAM_N, outbuf, len);
#elif ETA1 == 2
	rej_eta_2(a->coeffs, &cur, PARAM_N, outbuf, len);
#endif
	while (cur < PARAM_N)
	{
		KDF_SQUEEZEBLOCK(outbuf, 1, &ctx);
#if ETA1 == 1
		rej_eta_1(a->coeffs, &cur, PARAM_N, outbuf, KDF_RATE);
#elif ETA1 == 2
		rej_eta_2(a->coeffs, &cur, PARAM_N, outbuf, KDF_RATE);
#endif
	}
}
#endif


#define REJ_ETA_2_BYTES 307

#ifdef USE_ICCS

void poly_uniform_eta_2(poly *a,
	const uint8_t seed[SEEDBYTES],
	uint8_t nonce)
{

	int i;
	int cur = 0;
	uint8_t inbuf[SEEDBYTES + 1];
	int nblock = (REJ_ETA_2_BYTES + KDF_RATE - 1) / KDF_RATE;
	uint8_t outbuf[nblock * KDF_RATE];
	int len;

	memcpy(inbuf,seed,SEEDBYTES);
	inbuf[SEEDBYTES] = nonce;

	kdfstate  ctx;
	kdf_init(&ctx, inbuf, SEEDBYTES + 1);
	kdf_squeezeblocks(outbuf, nblock, &ctx);
	len = nblock * KDF_RATE;

	rej_eta_2(a->coeffs, &cur, PARAM_N, outbuf, len);

	while (cur < PARAM_N)
	{
		kdf_squeezeblocks(outbuf, 1, &ctx);
		rej_eta_2(a->coeffs, &cur, PARAM_N, outbuf, KDF_RATE);
	}
}

#else

void poly_uniform_eta_2(poly *a,
					   const uint8_t seed[SEEDBYTES],
					   uint8_t nonce)
{
	int cur = 0;
	uint8_t inbuf[SEEDBYTES + 1];
	int nblock = (REJ_ETA_2_BYTES + KDF_RATE - 1) / KDF_RATE;
	uint8_t outbuf[nblock * KDF_RATE];
	int len;

	memcpy(inbuf,seed,SEEDBYTES);
	inbuf[SEEDBYTES] = nonce;

	kdfstate  ctx;
	KDF_ABSORB(&ctx, inbuf, SEEDBYTES + 1);
	KDF_SQUEEZEBLOCK(outbuf, nblock, &ctx);
	len = nblock * KDF_RATE;

	rej_eta_2(a->coeffs, &cur, PARAM_N, outbuf, len);

	while (cur < PARAM_N)
	{
		KDF_SQUEEZEBLOCK(outbuf, 1, &ctx);
		rej_eta_2(a->coeffs, &cur, PARAM_N, outbuf, KDF_RATE);
	}
}

#endif

void polyvecl_uniform_gamma1(polyvecl* v, unsigned char* seed, unsigned int nonce)
{
#ifdef USE_SHA3
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
#ifdef USE_SHA3
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
	for (i = 0; i < PARAM_L; i++)
	{
		poly_uniform_eta_1(v->vec + i, seed, nonce++);
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

#ifdef USE_ICCS

#define REJ_ETA_7_BYTES 288 // fail with prob. less than 2^-17
#define REJ_ETA_5_BYTES 384

static int32_t rej_eta_5(int32_t* a, int32_t* cur, int32_t n, const uint8_t* buf, int32_t blen)
{

	int32_t ctr, pos;
	int32_t t[8];

	ctr = *cur;
	pos = 0;

	while (pos < blen && ctr + 2 <= n)
	{

		t[0] = buf[pos] & 0x0F;
		t[1] = (buf[pos++] >> 4) & 0x0F;

		if (t[0] <= 2 * ETA2)
			a[ctr++] = ETA2 - t[0];
		if (t[1] <= 2 * ETA2)
			a[ctr++] = ETA2 - t[1];
	}
	while (pos < blen && ctr < n)
	{
		t[0] = buf[pos] & 0x0F;
		t[1] = (buf[pos++] >> 4) & 0x0F;
		if (t[0] <= 2 * ETA2)
			a[ctr++] = ETA2 - t[0];
		if (ctr < n && t[1] <= 2 * ETA2)
			a[ctr++] = ETA2 - t[1];
	}

	* cur = ctr;
	return pos;
}

void poly_uniform_eta_5(poly *a,
	const uint8_t seed[SEEDBYTES],
	uint8_t nonce)
{

	int i;
	int cur = 0;
	uint8_t inbuf[SEEDBYTES + 1];
	int nblock = (REJ_ETA_5_BYTES + KDF_RATE - 1) / KDF_RATE;
	uint8_t outbuf[nblock * KDF_RATE];
	int len;

	memcpy(inbuf,seed,SEEDBYTES);
	inbuf[SEEDBYTES] = nonce;

	kdfstate  ctx;
	kdf_init(&ctx, inbuf, SEEDBYTES + 1);
	kdf_squeezeblocks(outbuf, nblock, &ctx);
	len = nblock * KDF_RATE;

	rej_eta_5(a->coeffs, &cur, PARAM_N, outbuf, len);

	while (cur < PARAM_N)
	{
		kdf_squeezeblocks(outbuf, 1, &ctx);
		rej_eta_5(a->coeffs, &cur, PARAM_N, outbuf, KDF_RATE);
	}
}

#else



#endif

void polyveck_uniform_eta2(polyveck* v, unsigned char* seed, unsigned int nonce)
{
#ifdef USE_SHA3
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
	for (i = 0; i < PARAM_K; i++)
	{
#if ETA2 == 5
		poly_uniform_eta_5(v->vec + i, seed, nonce++);
#elif ETA2 == 2
		poly_uniform_eta_2(v->vec + i, seed, nonce++);
#elif ETA2 == 1
		poly_uniform_eta_1(v->vec + i, seed, nonce++);
#endif
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

