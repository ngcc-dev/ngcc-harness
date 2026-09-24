#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "api.h"
#include "params.h"
#include "sign.h"
#include "poly.h"
#include "polyvec.h"
#include "packing.h"
#include "randombytes.h"
#include "hashkdf.h"
#include "api.h"
#include "pspm.h"
#include <string.h>

#include "sample.h"


/*************************************************
 * generate a pair of public key pk and secret key sk,
 * where pk = rho|t1
 *       sk = rho|key|hash(pk)|s1|s2|t0
 **************************************************/
int32_t msig_keygen(uint8_t *pk, uint8_t *sk)
{
	int32_t i;
	ALIGN(32)
	uint8_t buf[3 * SEEDBYTES + CRHBYTES]; // buf = r|rho|key|hash(pk)
	uint8_t *rnd, *rho, *key, *hashpk;
	uint8_t nonce = 0;
	polyvecl mat[PARAM_K];
	polyvecl s1, s1hat;
	polyveck s2, t, t1, t0;
	rnd = buf;
	rho = &buf[SEEDBYTES];
	key = &buf[2 * SEEDBYTES];
	hashpk = &buf[3 * SEEDBYTES];

	randombytes(buf, SEEDBYTES);

	KDF(buf, 3 * SEEDBYTES, buf, SEEDBYTES);

	expand_mat(mat, rho);

	polyvecl_uniform_eta1(&s1, rnd, nonce);

	nonce += PARAM_L;
	polyveck_uniform_eta2(&s2, rnd, nonce);
	s1hat = s1;

	polyvecl_ntt(&s1hat);

	for (i = 0; i < PARAM_K; ++i)
	{
		polyvecl_pointwise_acc_montgomery(&t.vec[i], mat + i, &s1hat); // output coefficient < PARAM_L * Q in absolute value
		poly_invntt_montgomery(t.vec + i);							   // output coefficient < 0.6 * Q in absolute value
	}
	polyveck_add(&t, &t, &s2); // output coefficient < Q in absoulte value
	polyveck_amodq(&t);
	polyveck_power2round(&t1, &t0, &t);

	pack_pk(pk, rho, &t1);

	KDF(hashpk, CRHBYTES, pk, SIG_PUBLICKEYBYTES);

	pack_sk(sk, rho, key, hashpk, &s1, &s2, &t0);

	return 0;
}

/*************************************************
 * create a signature sm on message m, where
 * sm = z|h|c
 **************************************************/
int32_t msig_sign(uint8_t *sk, uint8_t *m, int32_t mlen, uint8_t *sm, int32_t *smlen)
{
	int i, n;
	uint8_t rho[SEEDBYTES], cseed[SEEDBYTES];
	uint8_t *buf, *key, *hashpk;
	uint16_t nonce = 0;
	poly c, ct;
	polyvecl mat[PARAM_K], y, yhat, z, cs1;
	polyveck t0, w, w1;
	polyveck h, tmp;
	uint8_t s1_table[PARAM_L][PARAM_N * 3];
	s2Word s2_table[PARAM_K][PARAM_N * 3];

	buf = (uint8_t *)malloc(SEEDBYTES + CRHBYTES + mlen);
	if (buf == NULL)
		return -1;
	key = buf;
	hashpk = &buf[SEEDBYTES];
	unpack_sk(rho, key, hashpk, s1_table, s2_table, &t0, sk);

	for (i = 0; i < mlen; i++)
		buf[SEEDBYTES + CRHBYTES + i] = m[i];

	KDF(hashpk, CRHBYTES, hashpk, CRHBYTES + mlen);

	expand_mat(mat, rho);

rej:
	polyvecl_uniform_gamma1(&y, key, nonce);
	nonce += PARAM_L;
	yhat = y;
	polyvecl_ntt(&yhat);
	
	for (i = 0; i < PARAM_K; ++i)
	{
		polyvecl_pointwise_acc_montgomery(w.vec + i, mat + i, &yhat); // output coefficient < PARAM_L * Q in absolute value
		poly_invntt_montgomery(w.vec + i);							  // output coefficient  < 0.6*Q in aboslute value
	}
	polyveck_amodq(&w);
	polyveck_decompose(&w1, &tmp, &w);
	challenge(cseed, hashpk, &w1);
	unpack_c(&c, cseed);

	for (i = 0; i < PARAM_L; i++)
		if (poly_emulate_z(&z.vec[i],&y.vec[i], &c, s1_table[i]))
			goto rej;

	for (i = 0; i < PARAM_K; i++) 
		if (poly_emulate_w0_cs2(&tmp.vec[i], &tmp.vec[i], &c, s2_table[i]))
			goto rej;

	for (i = 0; i < PARAM_K; i++) {
		poly_emulate_ct(&ct, &c, &t0.vec[i]);
		poly_add(&tmp.vec[i], &tmp.vec[i], &ct);
		if (poly_chknorm(&tmp.vec[i], GAMMA3))
			goto rej;
	}

	polyveck_amodq(&tmp);
	n = polyveck_make_hint(&h, &tmp, &w1);

	if (n > OMEGA || n == -1)
		goto rej;

	*smlen = pack_sig(sm, &z, cseed, &h);

	free(buf);
	return 0;
}

int32_t msig_verf(uint8_t *pk,
				  uint8_t *sm, int32_t smlen,
				  uint8_t *m, int32_t mlen)
{
	int i;
	uint8_t rho[SEEDBYTES], cseed[SEEDBYTES], tcseed[SEEDBYTES];
	uint8_t *buf;
	poly c, chat;
	polyvecl mat[PARAM_K], z;
	polyveck t1, w1, h, tmp1, tmp2, tmp3;

	if(smlen < SIG_MIN_SIZE_PACKED || smlen > SIG_MAX_SIZE_PACKED)
    	return 1;

	unpack_pk(rho, &t1, pk);
	unpack_sig(&z, &h, cseed, sm);
	if (polyvecl_chknorm(&z, GAMMA1 - BETA1))
		return 1;

	buf = (uint8_t *)malloc(CRHBYTES + mlen);
	if (buf == NULL)
		return 1;
	for (i = 0; i < mlen; i++)
		buf[CRHBYTES + i] = m[i];

	KDF(buf, CRHBYTES, pk, SIG_PUBLICKEYBYTES);
	KDF(buf, CRHBYTES, buf, CRHBYTES + mlen);

	expand_mat(mat, rho);

	polyvecl_ntt(&z);
	for (i = 0; i < PARAM_K; ++i)
		polyvecl_pointwise_acc_montgomery(tmp1.vec + i, mat + i, &z); // output coefficient  <= PARAM_L*Q in absolute value
	polyveck_invntt_montgomery(&tmp1);

	unpack_c(&c, cseed);
	emulate_ct(&tmp3, &c, &t1);
	polyveck_shiftl(&tmp3, PARAM_D);
	polyveck_sub(&tmp1, &tmp1, &tmp3);
	polyveck_g_reduce(&tmp1);
	polyveck_use_hint(&w1, &tmp1, &h);
	challenge(tcseed, buf, &w1);

	for (i = 0; i < SEEDBYTES; ++i)
		if (cseed[i] != tcseed[i]) 
			return 1;
			
	polyveck_subw(&tmp2, &tmp1, &w1);
	polyveck_cmodq(&tmp2);
	if (polyveck_chknorm(&tmp2, GAMMA3))
		return 1;

	free(buf);
	return 0;
}
