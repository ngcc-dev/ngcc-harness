#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drng.h"
#include "SIG_AlgorithmInstance.h"

extern DRNG_ctx drng_algorithm;

static int
require_failure(const char *name, int rc)
{
	if (rc == 0) {
		fprintf(stderr, "%s unexpectedly succeeded\n", name);
		return 0;
	}
	return 1;
}

static int
seed_drng(void)
{
	unsigned char seed[64];

	for (size_t i = 0; i < sizeof seed; i++) {
		seed[i] = (unsigned char)(0xA5u ^ (unsigned)i);
	}
	return init_random_number(&drng_algorithm, seed, sizeof seed);
}

int
main(void)
{
	static const unsigned char initial_message[] =
		"YuanYang reference functional and malformed-input test";
	unsigned char *pk, *sk, *sn, *bad;
	unsigned char message[sizeof initial_message];
	unsigned long long pk_len, sk_len, sn_len;
	size_t pk_cap, sk_cap, sn_cap;
	int ok, rc;

	pk_cap = (size_t)sig_get_pk_len_bytes();
	sk_cap = (size_t)sig_get_sk_len_bytes();
	sn_cap = (size_t)sig_get_sn_len_bytes();
	pk = malloc(pk_cap);
	sk = malloc(sk_cap);
	sn = malloc(sn_cap);
	bad = malloc(sn_cap);
	if (pk == NULL || sk == NULL || sn == NULL || bad == NULL) {
		fprintf(stderr, "allocation failed\n");
		free(pk);
		free(sk);
		free(sn);
		free(bad);
		return 1;
	}
	if (seed_drng() != 0) {
		fprintf(stderr, "DRNG initialization failed\n");
		return 1;
	}
	memcpy(message, initial_message, sizeof message);
	pk_len = pk_cap;
	sk_len = sk_cap;
	rc = sig_keygen(pk, &pk_len, sk, &sk_len);
	if (rc != 0 || pk_len != pk_cap || sk_len != sk_cap) {
		fprintf(stderr, "keygen failed: rc=%d pk=%llu sk=%llu\n",
			rc, pk_len, sk_len);
		return 1;
	}
	sn_len = sn_cap;
	rc = sig_sign(sk, sk_len, message, sizeof message - 1u, sn, &sn_len);
	if (rc != 0 || sn_len != sn_cap) {
		fprintf(stderr, "sign failed: rc=%d sn=%llu\n", rc, sn_len);
		return 1;
	}
	if (sig_verify(pk, pk_len, sn, sn_len,
		message, sizeof message - 1u) != 0)
	{
		fprintf(stderr, "valid signature rejected\n");
		return 1;
	}

	ok = 1;
	message[0] ^= 1u;
	ok &= require_failure("modified message",
		sig_verify(pk, pk_len, sn, sn_len, message, sizeof message - 1u));
	message[0] ^= 1u;
	memcpy(bad, sn, sn_cap);
	bad[0] ^= 1u;
	ok &= require_failure("modified signature",
		sig_verify(pk, pk_len, bad, sn_len, message, sizeof message - 1u));
	memset(bad, 0xFF, sn_cap);
	ok &= require_failure("malformed signature encoding",
		sig_verify(pk, pk_len, bad, sn_len, message, sizeof message - 1u));
	ok &= require_failure("truncated signature",
		sig_verify(pk, pk_len, sn, sn_len - 1u,
			message, sizeof message - 1u));
	ok &= require_failure("truncated public key",
		sig_verify(pk, pk_len - 1u, sn, sn_len,
			message, sizeof message - 1u));
	sn_len = sn_cap;
	ok &= require_failure("truncated secret key",
		sig_sign(sk, sk_len - 1u, message, sizeof message - 1u,
			sn, &sn_len));
	ok &= require_failure("null public key",
		sig_verify(NULL, pk_len, sn, sn_cap,
			message, sizeof message - 1u));
	ok &= require_failure("null signature",
		sig_verify(pk, pk_len, NULL, sn_cap,
			message, sizeof message - 1u));
	ok &= require_failure("null message",
		sig_verify(pk, pk_len, sn, sn_cap, NULL, 0));

	printf("functional/malformed tests: %s\n", ok ? "pass" : "fail");
	free(pk);
	free(sk);
	free(sn);
	free(bad);
	return ok ? 0 : 1;
}
