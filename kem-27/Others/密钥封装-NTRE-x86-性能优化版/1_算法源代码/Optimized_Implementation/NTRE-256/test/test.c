#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "KEM_AlgorithmInstance.h"
#include "params.h"
#include "poly.h"
#include "drng.h"
#include "counter.h"

#define SEED_LEN_BYTES 64

DRNG_ctx drng_algorithm;

#if defined(__aarch64__)
#define COUNTER_UNIT_STR "ticks"
#elif defined(__x86_64__) || defined(__i386__)
#define COUNTER_UNIT_STR "cycles"
#else
#error "counter unsupported on this architecture"
#endif

static int TEST_CBD_PRIME(void)
{
	unsigned char msg[NTRE_MSGBYTES] = {0};
	unsigned char coins[NTRE_ERROR_RANDOMBYTES] = {0};
	unsigned char decoded[NTRE_MSGBYTES] = {0};
	poly e;

	for (int i = 0; i < NTRE_MSGBYTES; i++)
		msg[i] = (unsigned char)(0xA5 ^ i);
	for (int i = 0; i < NTRE_ERROR_RANDOMBYTES; i++)
		coins[i] = (unsigned char)(0x3C + 17 * i);

	poly_cbd1_prime(&e, msg, coins);
	poly_msg_mod2_to_bytes(decoded, &e);
	return memcmp(msg, decoded, NTRE_MSGBYTES) != 0;
}

static int TEST_CCA_KEM(void)
{
	unsigned char pk[NTRE_PUBLICKEYBYTES] = {0};
	unsigned char sk[NTRE_SECRETKEYBYTES] = {0};
	unsigned char ct[NTRE_CIPHERTEXTBYTES] = {0};
	unsigned char ss[NTRE_SSBYTES] = {0};
	unsigned char dss[NTRE_SSBYTES] = {0};
	unsigned long long pk_len, sk_len, ss_len, ct_len;

	int fail_cnt = 0;
	int dec_err  = 0;
	int tamper_err = 0;

	printf("============ CCA_KEM ENCAP DECAP TEST ============\n");

	kem_keygen(pk, &pk_len, sk, &sk_len);
	if (pk_len != NTRE_PUBLICKEYBYTES || sk_len != NTRE_SECRETKEYBYTES)
		fail_cnt++;

	for (int j = 0; j < TEST_LOOP_COUNT; j++)
	{
		kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len);
		if (ss_len != NTRE_SSBYTES || ct_len != NTRE_CIPHERTEXTBYTES)
			fail_cnt++;

		if (kem_dec(sk, sk_len, ct, ct_len, dss, &ss_len) != 0)
			dec_err++;
		if (ss_len != NTRE_SSBYTES)
			fail_cnt++;

		if (memcmp(ss, dss, NTRE_SSBYTES) != 0)
		{
			printf("ss[%d]  : ", j);
			for (int i = 0; i < NTRE_SSBYTES; i++) printf("%02X", ss[i]);
			printf("\n");

			printf("dss[%d] : ", j);
			for (int i = 0; i < NTRE_SSBYTES; i++) printf("%02X", dss[i]);
			printf("\n");

			fail_cnt++;
		}
	}
	ct[0] ^= 1;
	memset(dss, 0xFF, sizeof dss);
	if (kem_dec(sk, sk_len, ct, ct_len, dss, &ss_len) != -1)
		tamper_err++;
	for (int i = 0; i < NTRE_SSBYTES; i++)
		if (dss[i] != 0)
			tamper_err++;

	printf("count: %d  (kem_dec errors: %d, tamper errors: %d)\n", fail_cnt, dec_err, tamper_err);
	return fail_cnt + dec_err + tamper_err;
}

static void TEST_CCA_KEM_CLOCK(void)
{
	unsigned char pk[NTRE_PUBLICKEYBYTES] = {0};
	unsigned char sk[NTRE_SECRETKEYBYTES] = {0};
	unsigned char ct[NTRE_CIPHERTEXTBYTES] = {0};
	unsigned char ss[NTRE_SSBYTES] = {0};
	unsigned char dss[NTRE_SSBYTES] = {0};
	unsigned long long pk_len, sk_len, ss_len, ct_len;

	unsigned long long kcycles, ecycles, dcycles;
	unsigned long long count1, count2;

	printf("========= CCA KEM ENCAP DECAP SPEED TEST =========\n");

	kcycles = 0;
	for (int i = 0; i < TEST_LOOP_COUNT; i++)
	{
		count1 = counter();
		kem_keygen(pk, &pk_len, sk, &sk_len);
		count2 = counter();
		kcycles += count2 - count1 - countergap;
	}
	printf("  KEYGEN runs in ................. %8lld %s", kcycles / TEST_LOOP_COUNT, COUNTER_UNIT_STR);
	printf("\n");

	ecycles = 0;
	dcycles = 0;
	for (int i = 0; i < TEST_LOOP_COUNT; i++)
	{
		count1 = counter();
		kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len);
		count2 = counter();
		ecycles += count2 - count1 - countergap;

		count1 = counter();
		kem_dec(sk, sk_len, ct, ct_len, dss, &ss_len);
		count2 = counter();
		dcycles += count2 - count1 - countergap;
	}

	printf("  ENCAP  runs in ................. %8lld %s\n", ecycles / TEST_LOOP_COUNT, COUNTER_UNIT_STR);
	printf("  DECAP  runs in ................. %8lld %s\n", dcycles / TEST_LOOP_COUNT, COUNTER_UNIT_STR);
}

int main(void)
{
	unsigned char seed[SEED_LEN_BYTES];
	for (int i = 0; i < SEED_LEN_BYTES / 4; i++)
		memcpy(seed + 4 * i, "test", 4);
	init_random_number(&drng_algorithm, seed, SEED_LEN_BYTES);

	printf("================= BENCHMARK INFO =================\n");
	setup_counter();
	printf("ITERATIONS: %d\n", TEST_LOOP_COUNT);
	printf("COUNTERGAP: %lld %s\n", countergap, COUNTER_UNIT_STR);
	printf("=================== PARAMETERS ===================\n");
	printf("ALGORITHM_NAME  : %s\n", NTRE_ALGNAME);
	printf("PUBLICKEYBYTES  : %d\n", NTRE_PUBLICKEYBYTES);
	printf("SECRETKEYBYTES  : %d\n", NTRE_SECRETKEYBYTES);
	printf("CIPHERTEXTBYTES : %d\n", NTRE_CIPHERTEXTBYTES);
	printf("SHAREDSECRETBYTES: %d\n", NTRE_SSBYTES);

	if (TEST_CBD_PRIME() != 0) {
		printf("CBD'_1 mod-2 recovery failed\n");
		return 1;
	}
	if (TEST_CCA_KEM() != 0)
		return 1;
	TEST_CCA_KEM_CLOCK();

	return 0;
}
