/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include "params.h"
#include "poly.h"
#include "auxfunc.h"
#include "symmetric.h"
#include <stdint.h>
#include <string.h>






























extern DRNG_ctx drng_algorithm;




unsigned long long kem_get_pk_len_bytes()
{
	return NTRUOAEP_PUBLICKEYBYTES;
}

unsigned long long kem_get_sk_len_bytes()
{
	return NTRUOAEP_SECRETKEYBYTES;
}

unsigned long long kem_get_ss_len_bytes()
{
	return NTRUOAEP_SSBYTES;
}

unsigned long long kem_get_ct_len_bytes()
{
	return NTRUOAEP_CIPHERTEXTBYTES;
}

int kem_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	uint8_t buf[NTRUOAEP_N / 4];

	poly f, finv;
	poly g;
	poly h, hinv;

	do {
		get_random_number(&drng_algorithm, buf, NTRUOAEP_N / 4 * 8);



		poly_cbd1(&f, buf);
		poly_triple(&f, &f);
		f.coeffs[0] += 1;
		poly_ntt(&f, &f);
	} while((poly_is_invertible(&f)));

	poly_baseinv(&finv, &f);

	do {
		get_random_number(&drng_algorithm, buf, NTRUOAEP_N / 4 * 8);



		poly_cbd1(&g, buf);
		poly_triple(&g, &g);
		poly_ntt(&g, &g);

	} while((poly_is_invertible(&g)));


	poly_basemul(&h, &g, &finv);
	poly_baseinv(&hinv, &h);









    poly_tobytes(pk, &h);


    poly_tobytes(sk, &f);
    poly_tobytes(sk + NTRUOAEP_POLYBYTES, &hinv);
    hash_f(sk + 2 * NTRUOAEP_POLYBYTES, pk);

	*pk_len_bytes = NTRUOAEP_PUBLICKEYBYTES;
	*sk_len_bytes = NTRUOAEP_SECRETKEYBYTES;

	return 0;
}

int kem_enc(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes,
	unsigned char *ct, unsigned long long *ct_len_bytes)
{

	uint8_t msg[NTRUOAEP_N / 4];
	uint8_t randomness[NTRUOAEP_N / 4];
	uint8_t sigma[KeyConfirmation_BYTES];
	uint8_t buff[NTRUOAEP_SYMBYTES + NTRUOAEP_N / 2];
	int8_t fail = 0;

	poly p_c, p_h, p_s, p_t, p_mul;

	get_random_number(&drng_algorithm, randomness, NTRUOAEP_N / 4 * 8);

	memset(msg, 0, NTRUOAEP_N/8);







	poly_cbd1(&p_t, randomness);

	short_poly_to_bytes(buff + NTRUOAEP_SYMBYTES + NTRUOAEP_N / 4, &p_t);

	hash_g(randomness, buff + NTRUOAEP_SYMBYTES + NTRUOAEP_N / 4);

	poly_sotp(&p_s, msg, randomness);

	short_poly_to_bytes(buff + NTRUOAEP_SYMBYTES, &p_s);


	poly_ntt(&p_s, &p_s);


	poly_ntt(&p_t, &p_t);

	poly_frombytes(&p_h, pk);

	hash_f(buff, pk);



	poly_basemul(&p_mul, &p_h, &p_s);
	poly_baseadd(&p_c, &p_mul, &p_t);



	poly_tobytes(ct, &p_c);


	hash_h_prime(sigma, buff);


	memcpy(ct + NTRUOAEP_POLYBYTES, sigma, NTRUOAEP_SYMBYTES);
	memcpy(ss, sigma + NTRUOAEP_SYMBYTES, NTRUOAEP_SSBYTES);

	*ct_len_bytes = NTRUOAEP_CIPHERTEXTBYTES;
	*ss_len_bytes = NTRUOAEP_SSBYTES;

	return (int)fail;
}

int kem_dec(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *ct, unsigned long long ct_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes)
{
	uint8_t r[NTRUOAEP_N / 4];
	uint8_t buff[NTRUOAEP_SYMBYTES + NTRUOAEP_N / 2];
	uint8_t sigma[KeyConfirmation_BYTES];
	uint8_t ssbuf[NTRUOAEP_N / 8];
	int8_t fail = 0;

	poly p_c, p_f, p_hinv;
	poly p_s, p_t;
	poly p_tmp;


	poly_frombytes(&p_c, ct);
	poly_frombytes(&p_f, sk);
	poly_frombytes(&p_hinv, sk + NTRUOAEP_POLYBYTES);


	poly_basemul(&p_tmp, &p_c, &p_f);
	poly_invntt(&p_tmp, &p_tmp);
	poly_crepmod3(&p_t, &p_tmp);




	poly_ntt(&p_tmp, &p_t);
	poly_sub(&p_c, &p_c, &p_tmp);
	poly_basemul(&p_s, &p_c, &p_hinv);

	poly_invntt_normalized(&p_s, &p_s);


	for (int i = 0; i < NTRUOAEP_N; i++) fail |= (p_s.coeffs[i] > 1 || p_s.coeffs[i] < -1);

	short_poly_to_bytes(buff + NTRUOAEP_SYMBYTES + NTRUOAEP_N / 4, &p_t);

	hash_g(r, buff + NTRUOAEP_SYMBYTES + NTRUOAEP_N / 4);

	fail |= (poly_sotp_inv(ssbuf, &p_s, r));

	memcpy(buff, sk+NTRUOAEP_POLYBYTES * 2, NTRUOAEP_SYMBYTES);

	short_poly_to_bytes(buff + NTRUOAEP_SYMBYTES, &p_s);

	hash_h_prime(sigma, buff);

	for (int i = NTRUOAEP_POLYBYTES; i < NTRUOAEP_CIPHERTEXTBYTES; i++) {
		fail |= (ct[i] ^ sigma[i - NTRUOAEP_POLYBYTES]);
	}

	memcpy(ss, sigma + NTRUOAEP_SYMBYTES, NTRUOAEP_SSBYTES);

	*ss_len_bytes = NTRUOAEP_SSBYTES;

	return fail;
}
