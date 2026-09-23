#include "loong_kem.h"

#include "ct_util.h"
#include "loong_api_random.h"
#include "loong_hash.h"
#include "loong_pke.h"
#include "loong_status.h"
#include "parsing.h"

#include <stdlib.h>
#include <string.h>

static unsigned long long kem_message_bits(void)
{
	return (unsigned long long)LOONG_K * (unsigned long long)LOONG_M;
}

static void kem_clear_free(void *p, unsigned long long len)
{
	if (p != 0) {
		loong_secure_bzero(p, len);
		free(p);
	}
}

static int derive_g(unsigned char g[LOONG_G_BYTES], const unsigned char *pk,
                    unsigned long long pk_len_bytes, const unsigned char *m,
                    unsigned long long m_len_bytes, const unsigned char *salt,
                    unsigned long long salt_len_bytes)
{
	unsigned char id_pk[LOONG_ID_BYTES];
	int status;

	status = loong_hash_id(id_pk, pk, pk_len_bytes);
	if (status != LOONG_SUCCESS) {
		loong_secure_bzero(id_pk, sizeof(id_pk));
		return status;
	}
	status = loong_hash_g(g, id_pk, sizeof(id_pk), m, m_len_bytes, salt,
	                      salt_len_bytes);
	loong_secure_bzero(id_pk, sizeof(id_pk));
	return status;
}

int loong_kem_keygen(unsigned char *pk, unsigned long long pk_len_bytes,
                     unsigned char *sk, unsigned long long sk_len_bytes)
{
	unsigned char *sk_prime = 0;
	unsigned char *public_seed = 0;
	unsigned char *keygen_seed = 0;
	unsigned char xi[LOONG_XI_BYTES];
	int status = LOONG_ERR_ALLOC;

	if (pk == 0 || sk == 0) {
		return LOONG_ERR_NULL;
	}
	if (pk_len_bytes != LOONG_PK_BYTES || sk_len_bytes != LOONG_SK_BYTES) {
		return LOONG_ERR_BAD_LENGTH;
	}

	sk_prime = (unsigned char *)calloc((size_t)LOONG_SK_PRIME_BYTES, 1);
	public_seed = (unsigned char *)calloc((size_t)LOONG_PK_SEED_BYTES, 1);
	keygen_seed = (unsigned char *)calloc((size_t)LOONG_PK_SEED_BYTES, 1);
	if (sk_prime == 0 || public_seed == 0 || keygen_seed == 0) {
		goto cleanup;
	}

	status = loong_api_random_bytes(public_seed, LOONG_PK_SEED_BYTES);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = loong_api_random_bytes(keygen_seed, LOONG_PK_SEED_BYTES);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = loong_api_random_bytes(xi, LOONG_XI_BYTES);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}

	status = loong_pke_keygen_derandomized(pk, pk_len_bytes, sk_prime,
	                                       LOONG_SK_PRIME_BYTES, public_seed,
	                                       LOONG_PK_SEED_BYTES, keygen_seed,
	                                       LOONG_PK_SEED_BYTES);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = loong_secret_key_encode(sk, sk_len_bytes,
	                                 sk_prime + LOONG_SK_SEED_OFFSET,
	                                 LOONG_PK_SEED_BYTES,
	                                 sk_prime + LOONG_SK_X_OFFSET,
	                                 LOONG_X_OR_S_BYTES, pk, pk_len_bytes, xi,
	                                 sizeof(xi));

cleanup:
	if (status != LOONG_SUCCESS) {
		memset(pk, 0, (size_t)pk_len_bytes);
		memset(sk, 0, (size_t)sk_len_bytes);
	}
	kem_clear_free(sk_prime, LOONG_SK_PRIME_BYTES);
	kem_clear_free(public_seed, LOONG_PK_SEED_BYTES);
	kem_clear_free(keygen_seed, LOONG_PK_SEED_BYTES);
	loong_secure_bzero(xi, sizeof(xi));
	return status;
}

int loong_kem_encapsulate(const unsigned char *pk,
                          unsigned long long pk_len_bytes,
                          unsigned char *ss,
                          unsigned long long ss_len_bytes,
                          unsigned char *ct,
                          unsigned long long ct_len_bytes)
{
	unsigned char *m = 0;
	unsigned char *ct_core = 0;
	unsigned char salt[LOONG_SALT_BYTES];
	unsigned char g[LOONG_G_BYTES];
	unsigned long long m_len_bytes = loong_pke_get_message_len_bytes();
	int status = LOONG_ERR_ALLOC;

	if (pk == 0 || ss == 0 || ct == 0) {
		return LOONG_ERR_NULL;
	}
	if (pk_len_bytes != LOONG_PK_BYTES || ss_len_bytes != LOONG_SS_BYTES ||
	    ct_len_bytes != LOONG_CT_BYTES) {
		return LOONG_ERR_BAD_LENGTH;
	}

	m = (unsigned char *)calloc((size_t)m_len_bytes, 1);
	ct_core = (unsigned char *)calloc((size_t)LOONG_PKE_CT_BYTES, 1);
	if (m == 0 || ct_core == 0) {
		goto cleanup;
	}

	status = loong_api_random_bits(m, kem_message_bits());
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = loong_api_random_bytes(salt, sizeof(salt));
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = derive_g(g, pk, pk_len_bytes, m, m_len_bytes, salt, sizeof(salt));
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}

	status = loong_pke_encrypt_derandomized(
		ct_core, LOONG_PKE_CT_BYTES, pk, pk_len_bytes, m, m_len_bytes, g,
		LOONG_KEM_THETA_BYTES);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = loong_ciphertext_encode(ct, ct_len_bytes, ct_core,
	                                 LOONG_PKE_CT_BYTES, salt, sizeof(salt));
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = loong_hash_kdf(ss, ss_len_bytes, g, LOONG_KEM_G_SECRET_BYTES,
	                        ct_core, LOONG_PKE_CT_BYTES);

cleanup:
	if (status != LOONG_SUCCESS) {
		memset(ss, 0, (size_t)ss_len_bytes);
		memset(ct, 0, (size_t)ct_len_bytes);
	}
	kem_clear_free(m, m_len_bytes);
	kem_clear_free(ct_core, LOONG_PKE_CT_BYTES);
	loong_secure_bzero(salt, sizeof(salt));
	loong_secure_bzero(g, sizeof(g));
	return status;
}

int loong_kem_decapsulate(const unsigned char *sk,
                          unsigned long long sk_len_bytes,
                          const unsigned char *ct,
                          unsigned long long ct_len_bytes,
                          unsigned char *ss,
                          unsigned long long ss_len_bytes)
{
	loong_secret_key_view sk_view;
	loong_ciphertext_view ct_view;
	unsigned char *m = 0;
	unsigned char *ct_prime = 0;
	unsigned char g[LOONG_G_BYTES];
	unsigned char selected_secret[LOONG_KEM_G_SECRET_BYTES];
	unsigned long long m_len_bytes = loong_pke_get_message_len_bytes();
	int status;
	int dec_status;
	int enc_status;
	int valid;

	if (sk == 0 || ct == 0 || ss == 0) {
		return LOONG_ERR_NULL;
	}
	if (sk_len_bytes != LOONG_SK_BYTES || ct_len_bytes != LOONG_CT_BYTES ||
	    ss_len_bytes != LOONG_SS_BYTES) {
		return LOONG_ERR_BAD_LENGTH;
	}

	status = loong_secret_key_parse(&sk_view, sk, sk_len_bytes);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	status = loong_ciphertext_parse(&ct_view, ct, ct_len_bytes);
	if (status != LOONG_SUCCESS) {
		return status;
	}

	m = (unsigned char *)calloc((size_t)m_len_bytes, 1);
	ct_prime = (unsigned char *)calloc((size_t)LOONG_PKE_CT_BYTES, 1);
	if (m == 0 || ct_prime == 0) {
		status = LOONG_ERR_ALLOC;
		goto cleanup;
	}

	dec_status = loong_pke_decrypt(m, m_len_bytes, sk, LOONG_SK_PRIME_BYTES,
	                               ct_view.core, ct_view.core_len_bytes);
	if (dec_status != LOONG_SUCCESS) {
		memset(m, 0, (size_t)m_len_bytes);
	}

	status = derive_g(g, sk_view.pk, sk_view.pk_len_bytes, m, m_len_bytes,
	                  ct_view.salt, ct_view.salt_len_bytes);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}

	enc_status = loong_pke_encrypt_derandomized(
		ct_prime, LOONG_PKE_CT_BYTES, sk_view.pk, sk_view.pk_len_bytes, m,
		m_len_bytes, g, LOONG_KEM_THETA_BYTES);
	if (enc_status != LOONG_SUCCESS) {
		status = enc_status;
		goto cleanup;
	}

	valid = (dec_status == LOONG_SUCCESS) &&
	        loong_ct_equal(ct_prime, ct_view.core, LOONG_PKE_CT_BYTES);
	loong_ct_select(selected_secret, g, sk_view.xi, LOONG_KEM_G_SECRET_BYTES,
	                (unsigned char)valid);
	status = loong_hash_kdf(ss, ss_len_bytes, selected_secret,
	                        LOONG_KEM_G_SECRET_BYTES, ct_prime,
	                        LOONG_PKE_CT_BYTES);
	if (status == LOONG_SUCCESS && !valid) {
		status = LOONG_ERR_CRYPTO_REJECT;
	}

cleanup:
	if (status != LOONG_SUCCESS && status != LOONG_ERR_CRYPTO_REJECT) {
		memset(ss, 0, (size_t)ss_len_bytes);
	}
	kem_clear_free(m, m_len_bytes);
	kem_clear_free(ct_prime, LOONG_PKE_CT_BYTES);
	loong_secure_bzero(g, sizeof(g));
	loong_secure_bzero(selected_secret, sizeof(selected_secret));
	return status;
}
