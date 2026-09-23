#include "parsing.h"

#include "loong_status.h"

#include <string.h>

static int require_component(const unsigned char *p,
                             unsigned long long got,
                             unsigned long long expected)
{
	if (got != expected) {
		return LOONG_ERR_BAD_LENGTH;
	}
	if (expected != 0 && p == 0) {
		return LOONG_ERR_NULL;
	}
	return LOONG_SUCCESS;
}

int loong_public_key_parse(loong_public_key_view *view, const unsigned char *pk,
                           unsigned long long pk_len_bytes)
{
	if (view == 0 || pk == 0) {
		return LOONG_ERR_NULL;
	}
	if (pk_len_bytes != LOONG_PK_BYTES) {
		return LOONG_ERR_BAD_LENGTH;
	}

	view->seed = pk + LOONG_PK_SEED_OFFSET;
	view->seed_len_bytes = LOONG_PK_SEED_BYTES;
	view->s = pk + LOONG_PK_S_OFFSET;
	view->s_len_bytes = LOONG_X_OR_S_BYTES;
	return LOONG_SUCCESS;
}

int loong_secret_key_parse(loong_secret_key_view *view, const unsigned char *sk,
                           unsigned long long sk_len_bytes)
{
	if (view == 0 || sk == 0) {
		return LOONG_ERR_NULL;
	}
	if (sk_len_bytes != LOONG_SK_BYTES) {
		return LOONG_ERR_BAD_LENGTH;
	}

	view->seed = sk + LOONG_SK_SEED_OFFSET;
	view->seed_len_bytes = LOONG_PK_SEED_BYTES;
	view->x = sk + LOONG_SK_X_OFFSET;
	view->x_len_bytes = LOONG_X_OR_S_BYTES;
	view->pk = sk + LOONG_SK_PK_OFFSET;
	view->pk_len_bytes = LOONG_PK_BYTES;
	view->xi = sk + LOONG_SK_XI_OFFSET;
	view->xi_len_bytes = LOONG_XI_BYTES;
	return LOONG_SUCCESS;
}

int loong_ciphertext_parse(loong_ciphertext_view *view,
                           const unsigned char *ct,
                           unsigned long long ct_len_bytes)
{
	if (view == 0 || ct == 0) {
		return LOONG_ERR_NULL;
	}
	if (ct_len_bytes != LOONG_CT_BYTES) {
		return LOONG_ERR_BAD_LENGTH;
	}

	view->core = ct + LOONG_CT_CORE_OFFSET;
	view->core_len_bytes = LOONG_PKE_CT_BYTES;
	view->c1_bit_offset = 0;
	view->c1_bit_len = LOONG_C1_BITS;
	view->c2_bit_offset = LOONG_C1_BITS;
	view->c2_bit_len = LOONG_C2_BITS;
	view->salt = ct + LOONG_CT_SALT_OFFSET;
	view->salt_len_bytes = LOONG_SALT_BYTES;
	return LOONG_SUCCESS;
}

int loong_public_key_encode(unsigned char *pk, unsigned long long pk_len_bytes,
                            const unsigned char *seed,
                            unsigned long long seed_len_bytes,
                            const unsigned char *s,
                            unsigned long long s_len_bytes)
{
	int status;

	if (pk == 0) {
		return LOONG_ERR_NULL;
	}
	if (pk_len_bytes != LOONG_PK_BYTES) {
		return LOONG_ERR_BAD_LENGTH;
	}
	status = require_component(seed, seed_len_bytes, LOONG_PK_SEED_BYTES);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	status = require_component(s, s_len_bytes, LOONG_X_OR_S_BYTES);
	if (status != LOONG_SUCCESS) {
		return status;
	}

	memcpy(pk + LOONG_PK_SEED_OFFSET, seed, LOONG_PK_SEED_BYTES);
	memcpy(pk + LOONG_PK_S_OFFSET, s, LOONG_X_OR_S_BYTES);
	return LOONG_SUCCESS;
}

int loong_secret_key_encode(unsigned char *sk, unsigned long long sk_len_bytes,
                            const unsigned char *seed,
                            unsigned long long seed_len_bytes,
                            const unsigned char *x,
                            unsigned long long x_len_bytes,
                            const unsigned char *pk,
                            unsigned long long pk_len_bytes,
                            const unsigned char *xi,
                            unsigned long long xi_len_bytes)
{
	int status;

	if (sk == 0) {
		return LOONG_ERR_NULL;
	}
	if (sk_len_bytes != LOONG_SK_BYTES) {
		return LOONG_ERR_BAD_LENGTH;
	}
	status = require_component(seed, seed_len_bytes, LOONG_PK_SEED_BYTES);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	status = require_component(x, x_len_bytes, LOONG_X_OR_S_BYTES);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	status = require_component(pk, pk_len_bytes, LOONG_PK_BYTES);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	status = require_component(xi, xi_len_bytes, LOONG_XI_BYTES);
	if (status != LOONG_SUCCESS) {
		return status;
	}

	memcpy(sk + LOONG_SK_SEED_OFFSET, seed, LOONG_PK_SEED_BYTES);
	memcpy(sk + LOONG_SK_X_OFFSET, x, LOONG_X_OR_S_BYTES);
	memcpy(sk + LOONG_SK_PK_OFFSET, pk, LOONG_PK_BYTES);
	memcpy(sk + LOONG_SK_XI_OFFSET, xi, LOONG_XI_BYTES);
	return LOONG_SUCCESS;
}

int loong_ciphertext_encode(unsigned char *ct, unsigned long long ct_len_bytes,
                            const unsigned char *core,
                            unsigned long long core_len_bytes,
                            const unsigned char *salt,
                            unsigned long long salt_len_bytes)
{
	int status;

	if (ct == 0) {
		return LOONG_ERR_NULL;
	}
	if (ct_len_bytes != LOONG_CT_BYTES) {
		return LOONG_ERR_BAD_LENGTH;
	}
	status = require_component(core, core_len_bytes, LOONG_PKE_CT_BYTES);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	status = require_component(salt, salt_len_bytes, LOONG_SALT_BYTES);
	if (status != LOONG_SUCCESS) {
		return status;
	}

	memcpy(ct + LOONG_CT_CORE_OFFSET, core, LOONG_PKE_CT_BYTES);
	memcpy(ct + LOONG_CT_SALT_OFFSET, salt, LOONG_SALT_BYTES);
	return LOONG_SUCCESS;
}
