#include <stdint.h>
#include "../sign.h"
#include "../params.h"
#include "../poly.h"
#include "../polyvec.h"
#include "../polyfix.h"
#include "../polymat.h"
#include "../fft.h"
#include "../ntt.h"
#include "cpucycles.h"
#include "speed_print.h"

#define NTESTS 10000

uint64_t t[NTESTS];

int main(void)
{
	int i;
	size_t smlen;
	uint8_t pk[CRYPTO_PUBLICKEYBYTES];
	uint8_t sk[CRYPTO_SECRETKEYBYTES];
	uint8_t sm[CRYPTO_SIGNATUREBYTES + CRHBYTES];
	uint8_t b;
	uint8_t seedbuf[CRHBYTES], seed[SEEDBYTES];
	polyfixvecl y1;
	polyfixveck y2;
	polyvecl_1 s1;
	polyvecl_1 mat[K];
	polyveck e;
	poly s0;
	poly *a = &s0;
	poly_complex_fp32_16 input;

	for (i = 0; i < NTESTS; ++i) {
		t[i] = cpucycles();
		polyveckl_ternary_p(&s0, &s1, &e, seedbuf, i);
	}
	print_results("Ternary Sample:", t, NTESTS);

	for (i = 0; i < NTESTS; ++i) {
		t[i] = cpucycles();
		polymatkl_1_expand(mat, seed);
	}
	print_results("Matrix Expansion:", t, NTESTS);

	for (i = 0; i < NTESTS; ++i) {
		t[i] = cpucycles();
		polyfixveclk_sample_hyperball(&y1, &y2, &b, seedbuf, i);
	}
	print_results("Hyperball Sample y:", t, NTESTS);

	for (i = 0; i < NTESTS; ++i) {
		t[i] = cpucycles();
		polymatkl_1_pointwise_montgomery(&e, mat, &s1);
	}
	print_results("Matrix Pointwise Montgomery:", t, NTESTS);

	for (i = 0; i < NTESTS; ++i) {
		t[i] = cpucycles();
		poly_ntt(a);
	}
	print_results("NTT:", t, NTESTS);	

	for (i = 0; i < NTESTS; ++i) {
		t[i] = cpucycles();
		poly_mul_mont(a);
	}
	print_results("Multiplication MONT:", t, NTESTS);		

	for (i = 0; i < NTESTS; ++i) {
		t[i] = cpucycles();
		fft_init_and_bitrev(&input, a);  // ✓ 正确的指针传递
		fft(&input);
	}
	print_results("FFT:", t, NTESTS);	

	for (i = 0; i < NTESTS; ++i) {
		t[i] = cpucycles();
		polyveckl_sqsing_value(&s0, &s1, &e);
	}
	print_results("N(s):", t, NTESTS);	

	for(i = 0; i < NTESTS; ++i) {
		t[i] = cpucycles();
		crypto_sign_keypair(pk, sk);
	}
	print_results("Keypair:", t, NTESTS);

	for(i = 0; i < NTESTS; ++i) {
		t[i] = cpucycles();
		crypto_sign_signature(sm, &smlen, sm, CRHBYTES, sk);
	}
	print_results("Sign:", t, NTESTS);

	for(i = 0; i < NTESTS; ++i) {
		t[i] = cpucycles();
		crypto_sign_verify(sm, CRYPTO_SIGNATUREBYTES, sm, CRHBYTES, pk);
	}
	print_results("Verify:", t, NTESTS);

	return 0;
	}