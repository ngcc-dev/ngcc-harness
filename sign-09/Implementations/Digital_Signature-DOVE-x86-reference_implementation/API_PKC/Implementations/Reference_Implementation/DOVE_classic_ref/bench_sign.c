#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "drng.h"
#include "SIG_AlgorithmInstance.h"

DRNG_ctx drng_algorithm;

#define BENCH_ITERS 1000
#define MSG_LEN 56

int main(void)
{
	unsigned char seed[64];
	unsigned char m[MSG_LEN];
	unsigned char *pk, *sk, *sn;
	unsigned long long pk_len, sk_len, sn_len;
	clock_t t0, t1;
	int i, rtn;

	memset(seed, 0x42, sizeof(seed));
	init_random_number(&drng_algorithm, seed, sizeof(seed) * 8);

	pk_len = sig_get_pk_len_bytes();
	sk_len = sig_get_sk_len_bytes();
	sn_len = sig_get_sn_len_bytes();
	pk = (unsigned char *)calloc(pk_len, 1);
	sk = (unsigned char *)calloc(sk_len, 1);
	sn = (unsigned char *)calloc(sn_len, 1);
	memset(m, 0xAB, MSG_LEN);

	rtn = sig_keygen(pk, &pk_len, sk, &sk_len);
	if (rtn) {
		fprintf(stderr, "keygen failed: %d\n", rtn);
		return 1;
	}

	for (i = 0; i < 10; i++) {
		rtn = sig_sign(sk, sk_len, m, MSG_LEN, sn, &sn_len);
		if (rtn) {
			fprintf(stderr, "warmup sign failed: %d\n", rtn);
			return 1;
		}
	}

	t0 = clock();
	for (i = 0; i < BENCH_ITERS; i++) {
		rtn = sig_sign(sk, sk_len, m, MSG_LEN, sn, &sn_len);
		if (rtn) {
			fprintf(stderr, "sign failed at iter %d: %d\n", i, rtn);
			return 1;
		}
	}
	t1 = clock();

	rtn = sig_verify(pk, pk_len, sn, sn_len, m, MSG_LEN);
	if (rtn) {
		fprintf(stderr, "verify failed: %d\n", rtn);
		return 1;
	}

	printf("PK size: %llu bytes\n", pk_len);
	printf("SK size: %llu bytes\n", sk_len);
	printf("Sign x%d: %.6f sec (%.3f ms/sign)\n",
	       BENCH_ITERS,
	       (double)(t1 - t0) / CLOCKS_PER_SEC,
	       (double)(t1 - t0) / CLOCKS_PER_SEC / BENCH_ITERS * 1000.0);
	printf("verify ok\n");
	return 0;
}
