/**
 * @file test_scloudplus.c
 * @brief Shared correctness test driver, kept outside the algorithm core.
 */

#include "encode.h"
#include "kem.h"
#include "KEM_AlgorithmInstance.h"
#include "matrix.h"
#include "pack.h"
#include "scloudplus_param_common.h"
#include "pke.h"
#include "sample.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



static void fill_bytes(uint8_t *out, size_t len, uint8_t seed)
{
	for (size_t i = 0; i < len; i++)
	{
		out[i] = (uint8_t)(seed + 17U * i + (uint8_t)(i >> 3));
	}
}

static void fill_q(uint16_t *out, size_t len, uint32_t seed)
{
	for (size_t i = 0; i < len; i++)
	{
		out[i] = (uint16_t)((seed + 37U * i + (uint32_t)(i * i)) &
							scloudplus_q_mask);
	}
}

static int require_equal_u8(const uint8_t *a, const uint8_t *b, size_t len,
							const char *label)
{
	if (memcmp(a, b, len) == 0)
	{
		return 1;
	}
	printf("%s mismatch\n", label);
	return 0;
}

static int require_equal_u16_q(const uint16_t *a, const uint16_t *b, size_t len,
							   const char *label)
{
	for (size_t i = 0; i < len; i++)
	{
		if (((a[i] ^ b[i]) & scloudplus_q_mask) != 0U)
		{
			printf("%s mismatch at %zu: %u != %u\n", label, i,
				   (unsigned)(a[i] & scloudplus_q_mask),
				   (unsigned)(b[i] & scloudplus_q_mask));
			return 0;
		}
	}
	return 1;
}

static int msg_roundtrip_test(void)
{
	uint8_t msg[scloudplus_ss];
	uint8_t dec[scloudplus_ss];
	uint16_t matrix[(size_t)scloudplus_mbar * scloudplus_nbar];

	fill_bytes(msg, sizeof(msg), 0x42U);
	memset(dec, 0, sizeof(dec));
	memset(matrix, 0, sizeof(matrix));
	printf("Testing MsgEnc/MsgDec\n");
	msg_encode(msg, matrix);
	msg_decode(matrix, dec);
	return require_equal_u8(msg, dec, sizeof(msg), "MsgEnc/MsgDec");
}

static int msg_noisy_roundtrip_test(void)
{
	uint8_t msg[scloudplus_ss];
	uint8_t dec[scloudplus_ss];
	uint16_t matrix[(size_t)scloudplus_mbar * scloudplus_nbar];
	const size_t used_coeffs = (size_t)mat_subm * scloudplus_bw_n;

	fill_bytes(msg, sizeof(msg), 0x5aU);
	memset(dec, 0, sizeof(dec));
	memset(matrix, 0, sizeof(matrix));
	printf("Testing noisy MsgEnc/MsgDec\n");
	msg_encode(msg, matrix);
	for (size_t i = 0; i < used_coeffs; i++)
	{
		const uint16_t delta = (i % 3U == 0U) ? 1U :
			(uint16_t)((i % 3U == 1U) ? UINT16_MAX : 0U);

		matrix[i] = (uint16_t)(matrix[i] + delta) & scloudplus_q_mask;
	}
	msg_decode(matrix, dec);
	return require_equal_u8(msg, dec, sizeof(msg), "noisy MsgEnc/MsgDec");
}

static int pk_roundtrip_test(void)
{
	const size_t count = (size_t)scloudplus_m * scloudplus_nbar;
	uint16_t *in = malloc(count * sizeof(uint16_t));
	uint16_t *out = malloc(count * sizeof(uint16_t));
	uint8_t *pk = malloc(scloudplus_pk);
	int ok = 0;

	if (!in || !out || !pk)
	{
		goto out;
	}
	fill_q(in, count, 5U);
	printf("Testing pk roundtrip\n");
	pack_pk(in, pk);
	unpack_pk(pk, out);
	ok = require_equal_u16_q(in, out, count, "pk roundtrip");

out:
	free(in);
	free(out);
	free(pk);
	return ok;
}

static int sk_roundtrip_test(void)
{
	const size_t count = (size_t)scloudplus_n * scloudplus_nbar;
	uint16_t *in = malloc(count * sizeof(uint16_t));
	uint16_t *out = malloc(count * sizeof(uint16_t));
	uint8_t *sk = malloc(scloudplus_pke_sk);
	int ok = 0;

	if (!in || !out || !sk)
	{
		goto out;
	}
	for (size_t i = 0; i < count; i++)
	{
		const int v = (int)(i % 3U) - 1;

		in[i] = (uint16_t)v;
	}
	printf("Testing sk roundtrip\n");
	pack_sk(in, sk);
	unpack_sk(sk, out);
	ok = memcmp(in, out, count * sizeof(uint16_t)) == 0;
	if (!ok)
	{
		printf("sk roundtrip mismatch\n");
	}

out:
	free(in);
	free(out);
	free(sk);
	return ok;
}

static int c1_roundtrip_test(void)
{
	const size_t count = (size_t)scloudplus_mbar * scloudplus_n;
	uint16_t *in = malloc(count * sizeof(uint16_t));
	uint16_t *red = malloc(count * sizeof(uint16_t));
	uint16_t *out = malloc(count * sizeof(uint16_t));
	uint8_t *buf = malloc(scloudplus_c1);
	int ok = 0;

	if (!in || !red || !out || !buf)
	{
		goto out;
	}
	fill_q(in, count, 17U);
	printf("Testing C1 q-direct roundtrip\n");
	reduce_c1(in, red);
	pack_c1(red, buf);
	unpack_c1(buf, out);
	reduce_c1(out, out);
	ok = require_equal_u16_q(red, out, count, "C1 roundtrip");

out:
	free(in);
	free(red);
	free(out);
	free(buf);
	return ok;
}

static int c2_roundtrip_test(void)
{
	const size_t count = (size_t)scloudplus_mbar * scloudplus_nbar;
	uint16_t in[count];
	uint16_t red[count];
	uint16_t out[count];
	uint8_t buf[scloudplus_c2];

	fill_q(in, count, 29U);
	printf("Testing C2 q-direct roundtrip\n");
	reduce_c2(in, red);
	pack_c2(red, buf);
	unpack_c2(buf, out);
	reduce_c2(out, out);
	return require_equal_u16_q(red, out, count, "C2 roundtrip");
}

static int noiseless_pke_test(void)
{
	uint8_t seedA[scloudplus_seedA_bytes];
	uint8_t s_seed[32];
	uint8_t sp_seed[32];
	uint8_t msg[scloudplus_ss];
	uint8_t dec[scloudplus_ss];
	uint16_t *S = calloc((size_t)scloudplus_n * scloudplus_nbar, sizeof(uint16_t));
	uint16_t *S1 = calloc((size_t)scloudplus_mbar * scloudplus_m, sizeof(uint16_t));
	uint16_t *B = calloc((size_t)scloudplus_m * scloudplus_nbar, sizeof(uint16_t));
	uint16_t *C1 = calloc((size_t)scloudplus_mbar * scloudplus_n, sizeof(uint16_t));
	uint16_t *C2 = calloc((size_t)scloudplus_mbar * scloudplus_nbar,
						  sizeof(uint16_t));
	uint16_t *D = calloc((size_t)scloudplus_mbar * scloudplus_nbar, sizeof(uint16_t));
	uint16_t *mu = calloc((size_t)scloudplus_mbar * scloudplus_nbar,
						  sizeof(uint16_t));
	int ok = 0;

	if (!S || !S1 || !B || !C1 || !C2 || !D || !mu)
	{
		goto out;
	}
	fill_bytes(seedA, sizeof(seedA), 0x80U);
	fill_bytes(s_seed, sizeof(s_seed), 0x11U);
	fill_bytes(sp_seed, sizeof(sp_seed), 0x22U);
	fill_bytes(msg, sizeof(msg), 0x33U);
	memset(dec, 0, sizeof(dec));

	printf("Testing noiseless PKE algebra\n");
	sample_s(s_seed, sizeof(s_seed), S);
	sample_sp(sp_seed, sizeof(sp_seed), S1);
	mul_as_e(seedA, S, B, B);
	mul_sa_e(seedA, S1, C1, C1);
	mul_sb_e(S1, B, C2, C2);
	msg_encode(msg, mu);
	mat_add(C2, mu, scloudplus_mbar * scloudplus_nbar, C2);
	mul_cs(C1, S, D);
	mat_sub(C2, D, scloudplus_mbar * scloudplus_nbar, D);
	reduce_c2(D, D);
	msg_decode(D, dec);
	ok = require_equal_u8(msg, dec, sizeof(msg), "noiseless PKE");

out:
	free(S);
	free(S1);
	free(B);
	free(C1);
	free(C2);
	free(D);
	free(mu);
	return ok;
}

static int pke_roundtrip_test(void)
{
	uint8_t *pk = malloc(scloudplus_pk);
	uint8_t *sk = malloc(scloudplus_pke_sk);
	uint8_t *ct = malloc(scloudplus_ctx);
	uint8_t msg[scloudplus_ss];
	uint8_t dec[scloudplus_ss];
	uint8_t coins[scloudplus_pke_enc_coins_bytes];
	int ok = 1;

	if (!pk || !sk || !ct)
	{
		ok = 0;
		goto out;
	}
	printf("Testing PKE 100\n");
	for (int i = 0; i < 100; i++)
	{
		fill_bytes(msg, sizeof(msg), (uint8_t)(0x50U + i));
		fill_bytes(coins, sizeof(coins), (uint8_t)(0x90U + i));
		if (pke_keygen(pk, sk) != 0)
		{
			printf("PKE keygen failed at iteration %d\n", i);
			ok = 0;
			break;
		}
		pke_enc(pk, msg, coins, ct);
		pke_dec(sk, ct, dec);
		if (memcmp(msg, dec, sizeof(msg)) != 0)
		{
			printf("PKE mismatch at iteration %d\n", i);
			ok = 0;
			break;
		}
	}

out:
	free(pk);
	free(sk);
	free(ct);
	return ok;
}

static int kem_roundtrip_test(void)
{
	uint8_t *pk = malloc(scloudplus_pk);
	uint8_t *sk = malloc(scloudplus_kem_sk);
	uint8_t *ct = malloc(scloudplus_ctx);
	uint8_t ss1[scloudplus_ss];
	uint8_t ss2[scloudplus_ss];
	int ok = 1;

	if (!pk || !sk || !ct)
	{
		ok = 0;
		goto out;
	}
	printf("Testing KEM 100\n");
	for (int i = 0; i < 100; i++)
	{
		if (scloud_kemkeygen(pk, sk) != 0 ||
			scloud_kemencaps(pk, ct, ss1) != 0 ||
			scloud_kemdecaps(sk, ct, ss2) != 0)
		{
			printf("KEM operation failed at iteration %d\n", i);
			ok = 0;
			break;
		}
		if (memcmp(ss1, ss2, sizeof(ss1)) != 0)
		{
			printf("KEM mismatch at iteration %d\n", i);
			ok = 0;
			break;
		}
	}

out:
	free(pk);
	free(sk);
	free(ct);
	return ok;
}


static int api_pkc_kem_test(void)
{
	const unsigned long long pk_expected = kem_get_pk_len_bytes();
	const unsigned long long sk_expected = kem_get_sk_len_bytes();
	const unsigned long long ct_expected = kem_get_ct_len_bytes();
	const unsigned long long ss_expected = kem_get_ss_len_bytes();
	uint8_t *pk = malloc((size_t)pk_expected);
	uint8_t *sk = malloc((size_t)sk_expected);
	uint8_t *ct = malloc((size_t)ct_expected);
	uint8_t *ss1 = malloc((size_t)ss_expected);
	uint8_t *ss2 = malloc((size_t)ss_expected);
	unsigned long long pk_len = 0;
	unsigned long long sk_len = 0;
	unsigned long long ct_len = 0;
	unsigned long long ss1_len = 0;
	unsigned long long ss2_len = 0;
	int ok = 1;

	if (pk_expected != scloudplus_pk || sk_expected != scloudplus_kem_sk ||
		ct_expected != scloudplus_ctx || ss_expected != scloudplus_ss)
	{
		printf("API_PKC size query mismatch\n");
		ok = 0;
		goto out;
	}
	if (!pk || !sk || !ct || !ss1 || !ss2)
	{
		ok = 0;
		goto out;
	}
	printf("Testing API_PKC KEM API\n");
	if (kem_keygen(pk, &pk_len, sk, &sk_len) != 0 ||
		pk_len != pk_expected || sk_len != sk_expected ||
		kem_enc(pk, pk_len, ss1, &ss1_len, ct, &ct_len) != 0 ||
		ss1_len != ss_expected || ct_len != ct_expected ||
		kem_dec(sk, sk_len, ct, ct_len, ss2, &ss2_len) != 0 ||
		ss2_len != ss_expected ||
		memcmp(ss1, ss2, (size_t)ss_expected) != 0)
	{
		printf("API_PKC KEM API mismatch\n");
		ok = 0;
	}

out:
	free(pk);
	free(sk);
	free(ct);
	free(ss1);
	free(ss2);
	return ok;
}

int main(void)
{
	int ok = 1;

	printf("%s\n", SYSTEM_NAME);
	printf("m=%d n=%d mbar=%d nbar=%d logq=%d A=packed10\n", scloudplus_m,
		   scloudplus_n, scloudplus_mbar, scloudplus_nbar, scloudplus_logq);

	ok &= msg_roundtrip_test();
	ok &= msg_noisy_roundtrip_test();
	ok &= pk_roundtrip_test();
	ok &= sk_roundtrip_test();
	ok &= c1_roundtrip_test();
	ok &= c2_roundtrip_test();
	ok &= noiseless_pke_test();
	ok &= pke_roundtrip_test();
	ok &= kem_roundtrip_test();
	ok &= api_pkc_kem_test();

	if (ok)
	{
		printf("All correctness tests PASSED\n");
		return 0;
	}
	printf("Correctness tests FAILED\n");
	return 1;
}
