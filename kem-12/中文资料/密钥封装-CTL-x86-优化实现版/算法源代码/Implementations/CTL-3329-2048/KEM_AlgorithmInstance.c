#include <stdlib.h>
#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include "ctl.h"

extern DRNG_ctx drng_algorithm;

unsigned long long kem_get_pk_len_bytes()
{
	return ctl_3329_2048_encode_public_key(NULL, 0, NULL);
}

unsigned long long kem_get_sk_len_bytes()
{
	ctl_3329_2048_private_key sk;
	return ctl_3329_2048_encode_private_key(NULL, 0, &sk, 0);
}

unsigned long long kem_get_ss_len_bytes()
{
	return 48;
}

unsigned long long kem_get_ct_len_bytes()
{
	return ctl_3329_2048_encode_ciphertext(NULL, 0, NULL);
}

int kem_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	ctl_3329_2048_private_key sk_struct;
	ctl_3329_2048_public_key pk_struct;
	uint8_t *tmp = (uint8_t *)malloc(CTL_3329_2048_TMP_KEYGEN);
	int rtn;

	if (tmp == NULL) {
		return -1;
	}

	*pk_len_bytes = ctl_3329_2048_encode_public_key(NULL, 0, NULL);
	*sk_len_bytes = ctl_3329_2048_encode_private_key(NULL, 0, NULL, 0);

	rtn = ctl_3329_2048_keygen(&sk_struct, tmp, CTL_3329_2048_TMP_KEYGEN);
	if (rtn != 0) {
		free(tmp);
		return -1;
	}

	ctl_3329_2048_get_public_key(&pk_struct, &sk_struct);

	ctl_3329_2048_encode_public_key(pk, *pk_len_bytes, &pk_struct);
	ctl_3329_2048_encode_private_key(sk, *sk_len_bytes, &sk_struct, 0);

	free(tmp);
	return 0;
}

int kem_enc(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes,
	unsigned char *ct, unsigned long long *ct_len_bytes)
{
	ctl_3329_2048_public_key pk_struct;
	ctl_3329_2048_ciphertext ct_struct;
	uint8_t *tmp = (uint8_t *)malloc(CTL_3329_2048_TMP_ENCAPS);
	uint8_t seed[48];
	int rtn;

	if (tmp == NULL) {
		return -1;
	}

	*ss_len_bytes = 48;
	*ct_len_bytes = ctl_3329_2048_encode_ciphertext(NULL, 0, NULL);

	rtn = ctl_3329_2048_decode_public_key(&pk_struct, pk, pk_len_bytes);
	if (rtn == 0) {
		free(tmp);
		return -1;
	}

	get_random_number(&drng_algorithm, seed, 48 * 8);

	rtn = ctl_3329_2048_encapsulate_explicit_seed(
		ss, *ss_len_bytes, &ct_struct, &pk_struct, seed, tmp, CTL_3329_2048_TMP_ENCAPS);
	if (rtn != 0) {
		free(tmp);
		return -1;
	}

	ctl_3329_2048_encode_ciphertext(ct, *ct_len_bytes, &ct_struct);

	free(tmp);
	return 0;
}

int kem_dec(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *ct, unsigned long long ct_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes)
{
	ctl_3329_2048_private_key sk_struct;
	ctl_3329_2048_ciphertext ct_struct;
	uint8_t *tmp_decode = (uint8_t *)malloc(CTL_3329_2048_TMP_DECODE_PRIV);
	uint8_t *tmp_decaps = (uint8_t *)malloc(CTL_3329_2048_TMP_DECAPS);
	int rtn;

	if (tmp_decode == NULL || tmp_decaps == NULL) {
		free(tmp_decode);
		free(tmp_decaps);
		return -1;
	}

	*ss_len_bytes = 48;

	rtn = ctl_3329_2048_decode_private_key(&sk_struct, sk, sk_len_bytes, tmp_decode, CTL_3329_2048_TMP_DECODE_PRIV);
	if (rtn == 0) {
		free(tmp_decode);
		free(tmp_decaps);
		return -1;
	}

	rtn = ctl_3329_2048_decode_ciphertext(&ct_struct, ct, ct_len_bytes);
	if (rtn == 0) {
		free(tmp_decode);
		free(tmp_decaps);
		return -1;
	}

	rtn = ctl_3329_2048_decapsulate(ss, *ss_len_bytes, &ct_struct, &sk_struct, tmp_decaps, CTL_3329_2048_TMP_DECAPS);
	if (rtn != 0) {
		free(tmp_decode);
		free(tmp_decaps);
		return -1;
	}

	free(tmp_decode);
	free(tmp_decaps);
	return 0;
}