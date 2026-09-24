/*
 * yuanyang-512 key generation
 *
 * This file is the landing zone for porting the Python reference keygen:
 *   - PairGen, with Solmae/Antrag code as C refs.  The first C port is in
 *     pairgen.c and samples directly in the imported Falcon FFT layout.
 *   - public h = g/f in Z_q[X]/(X^d + 1), for q = 2689.
 *   - NTRUSolve, through the Falcon fixed-point ntrugen, adapted to fit our values
 *   - perturbation decomposition / weak-smoothness precomputations using the
 *     imported Falcon fpr/FFT layout.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ntt.h"
#include "ntttable.h"
#include "drng.h"
#include "codec.h"
#include "auxfunc.h"
#include "yy_kem_inner.h"
#include "prng.h"
#include "poly_inv_ntt.h"

#define YUANYANG_KEYGEN_MAX_ATTEMPTS  255u
#define YUANYANG_POLY_CAP             (2u * YUANYANG_D + 2u)
#define M_PI 3.14159265358979323846

const double SIGMAKEYTAB[]={1.7,1.6,1.5};

extern DRNG_ctx drng_algorithm;

static int
public_key_from_pair(uint16_t h[YUANYANG_D],
	const int8_t f[YUANYANG_D], const int8_t g[YUANYANG_D])
{
	uint16_t f_inv[YUANYANG_D];
	uint16_t tmp[YUANYANG_D];
	for (size_t i = 0; i < YUANYANG_D; i++) {
		tmp[i] = (int16_t) f[i]+YUANYANG_Q;
	}
	if (poly_inv_xn1_q(f_inv, tmp,YUANYANG_D)) {
		return 0;
	}
	for (size_t i = 0; i < YUANYANG_D; i++) {
		tmp[i] = (int16_t) g[i];
	}
	yuanyang_mul_mod_xn_plus_1_ntt_big(h, f_inv, (int16_t*)tmp);
	
	return 1;
}

static int
eval_at_one_is_even_mod2(const int8_t src[YUANYANG_D])
{
	unsigned parity;

	parity = 0;
	for (size_t u = 0; u < YUANYANG_D; u++) {
		parity ^= (unsigned)src[u] & 1u;
	}
	return parity == 0;
}

static int
drng_randombytes(void *ctx, unsigned char *buf, unsigned long long len_bytes)
{
	return get_random_number((DRNG_ctx *)ctx, buf, 8u * len_bytes);
}

static int ring_sampler(int8_t f[YUANYANG_D],prng *rng){
#if YUANYANG_LOGD==9
	const uint8_t gauss[]={30, 81, 111, 123, 127};
#elif YUANYANG_LOGD==10
	const uint8_t gauss[]={32, 85, 114, 125, 128};
#elif YUANYANG_LOGD==11
	const uint8_t gauss[]={34, 89, 117, 126, 128};
#endif
	for (size_t u = 0; u < YUANYANG_D; u++) {
		uint8_t random=prng_get_u8(rng);
		int sign=1-2*(random&1),value=0;
		random>>=1;
		for(size_t i=0;i<sizeof(gauss)/sizeof(*gauss);i++)
			value+=gauss[i]<=random;
		f[u]=value*sign;
	}
	return prng_status(rng);
}

void inverse_mod_2(int8_t *f,int8_t *finvint){
	int seven=0;
	uint32_t tmp[YUANYANG_D/2],finv[YUANYANG_D/2];
	for(size_t i=0;i<YUANYANG_D;i+=2){
		seven+=f[i];
	}
	finv[0]=seven&1;
	finv[1]=1-(seven&1); // f mod x^2+1 =f^-1
	for(size_t logd=1;logd<YUANYANG_LOGD;logd++){ // Newton iteration: finv=finv^2*f
		qntt_forward_partial(finv,logd); 
		int d=1<<logd;
		qntt_mul_int_partial(finv,finv,logd);
		for(int k=0;k<d;k++)
			finv[k]=finv[k]&1;
		qntt_forward_partial(finv,logd);
		for(int k=0;k<d;k++){
			int s=0;
#pragma omp simd
			for(int i=0;i<(1<<(YUANYANG_LOGD-logd));i++)
				s+=f[i*d+k];
			tmp[k]=s&1;
		}
		qntt_forward_partial(tmp,logd);
		qntt_mul_int_partial(finv,tmp,logd);
		for(int k=0;k<d;k++)
			finv[k]=finv[k]&1;
		if(logd+1<YUANYANG_LOGD)
			memset(finv+d,0,d*4);
	}
	/* 32 bits to 8 bits */
	for(size_t k=0;k<YUANYANG_D/2;k++){
		finvint[k]=finv[k];
	}
}

int
yy_kem_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	yy_kem_compact_sk decoded;
	int8_t g[YUANYANG_D];
	prng rng;

	if (pk_len_bytes != NULL) {
		*pk_len_bytes = yy_get_pk_len_bytes();
	}
	if (sk_len_bytes != NULL) {
		*sk_len_bytes = yy_get_sk_len_bytes();
	}
	if (pk == NULL || pk_len_bytes == NULL || sk == NULL || sk_len_bytes == NULL) {
		return YUANYANG_BADARG;
	}
	prng_init(&rng, drng_randombytes, &drng_algorithm);
	if (prng_status(&rng) != YUANYANG_SUCCESS) {
		return prng_status(&rng);
	}

	for (unsigned attempt = 0; attempt < YUANYANG_KEYGEN_MAX_ATTEMPTS; attempt++) {
		int rc;

		rc = ring_sampler(decoded.f, &rng);
		if (rc != YUANYANG_SUCCESS) {
			return rc;
		}
		if (eval_at_one_is_even_mod2(decoded.f)) {
			continue;
		}
		rc = ring_sampler(g, &rng);
		if (rc != YUANYANG_SUCCESS) {
			return rc;
		}

		if (!public_key_from_pair(decoded.h, decoded.f, g)) {
			continue;
		}

		for (size_t u = 0; u < YUANYANG_D; u++) {
			sk[u]=decoded.f[u];
		}
		inverse_mod_2(decoded.f,(int8_t*)sk+YUANYANG_D);
		yuanyang_encode_uniform(decoded.h,YUANYANG_D, YUANYANG_Q, 4,pk);
		uint8_t tmphash[512/8]; 
		pseudohash(512,pk,(*pk_len_bytes)*8,tmphash);
		memcpy(sk+3*YUANYANG_D/2,tmphash,YUANYANG_D/32);
		for(unsigned int i=0;i<YUANYANG_D/32;i++)
			sk[3*YUANYANG_D/2+YUANYANG_D/32+i]=prng_get_u8(&rng);
		memcpy(sk+3*YUANYANG_D/2+YUANYANG_D/32*2,pk,*pk_len_bytes);
		return rc;
	}
	return YUANYANG_KEYGEN_FAILED;
}
