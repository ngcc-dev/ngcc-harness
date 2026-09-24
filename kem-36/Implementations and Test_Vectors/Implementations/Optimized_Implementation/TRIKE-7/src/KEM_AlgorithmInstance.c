#include <string.h>
#include <stdlib.h>

#include "KEM_AlgorithmInstance.h"
#include "auxfunc.h"
#include "drng.h"

#include "trike_types.h"
#include "decoder.h"
#include "sample.h"
#include "gf2x.h"

// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

// Returns the public key size in bytes.
unsigned long long kem_get_pk_len_bytes()
{
	return sizeof(public_key_t);
}

// Returns the secret key size in bytes.
unsigned long long kem_get_sk_len_bytes()
{
	return sizeof(secret_key_t);
}

// Returns the shared secret size in bytes.
unsigned long long kem_get_ss_len_bytes()
{
	return sizeof(shared_secret_t);
}

// Returns the ciphertext size in bytes.
unsigned long long kem_get_ct_len_bytes()
{
	return sizeof(ciphertext_t);
}

// Computes t0 = (h0*r1 + h1)/(t1 + r1) over GF2X.
static inline void calculate_t0(
	uint8_t *t0, const uint8_t *h0, const uint8_t *h1,
	const uint8_t *t1, const uint8_t *r1)
{
	uint8_t *tmp1 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *tmp2 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	trike_setz(tmp1, PADDED_R_SIZE_BYTES);
	trike_setz(tmp2, PADDED_R_SIZE_BYTES);

	gf2x_add(tmp1, t1, r1);
	gf2x_inv(tmp2, tmp1);

	gf2x_mul(tmp1, h0, r1);
	gf2x_add(tmp1, tmp1, h1);

	gf2x_mul(t0, tmp1, tmp2);

	free(tmp2);
	free(tmp1);
}

// Computes r2 = (t0*t2 + h2)/(t0 + h0) over GF2X.
static inline void calculate_r2(
	uint8_t *r2, const uint8_t *h0, const uint8_t *h2,
	const uint8_t *t0, const uint8_t *t2)
{
	uint8_t *tmp1 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *tmp2 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	trike_setz(tmp1, PADDED_R_SIZE_BYTES);
	trike_setz(tmp2, PADDED_R_SIZE_BYTES);

	gf2x_add(tmp1, t0, h0);
	gf2x_inv(tmp2, tmp1);

	gf2x_mul(tmp1, t0, t2);
	gf2x_add(tmp1, tmp1, h2);

	gf2x_mul(r2, tmp1, tmp2);

	free(tmp2);
	free(tmp1);
}

// XORs two message buffers of length M_SIZE_BYTES.
static inline void msgxor(uint8_t *out, const uint8_t *in1, const uint8_t *in2)
{
	for (size_t i = 0; i < M_SIZE_BYTES; i++)
	{
		out[i] = in1[i] ^ in2[i];
	}
}

// Computes u = e0 + e1*r1 + e2*r2 or v = e0 + e1*t1 + e2*t2 over GF2X.
static inline void calculate_uv(
	uint8_t *u, const uint8_t *e0, const uint8_t *e1, const uint8_t *e2,
	const uint8_t *r1, const uint8_t *r2)
{
	uint8_t *tmp = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	trike_setz(tmp, PADDED_R_SIZE_BYTES);
	
	trike_assign(e0, u, R_ZMM_SIZE_BYTES, PADDED_R_SIZE_BYTES);

	gf2x_mul(tmp, e1, r1);
	gf2x_add(u, u, tmp);
	gf2x_mul(tmp, e2, r2);
	gf2x_add(u, u, tmp);
	
	free(tmp);
}

// Computes s = (h0 + t0)*u + t0*v over GF2X.
static inline void calculate_s(
	uint8_t *s, const uint8_t *u, const uint8_t *v,
	const uint8_t *h0, const uint8_t *t0)
{
	uint8_t *tmp = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	trike_setz(tmp, PADDED_R_SIZE_BYTES);

	gf2x_add(tmp, h0, t0);
	gf2x_mul(s, tmp, u);
	gf2x_mul(tmp, t0, v);
	gf2x_add(s, s, tmp);

	free(tmp);
}

// Compares two byte vectors and returns 0 only when they are equal.
static inline uint8_t compare_vec(const uint8_t *vec1, const uint8_t *vec2, size_t len_bytes)
{
#ifdef AVX512_AVAILABLE
	uint8_t result = -1;
	for (size_t i = 0; i < len_bytes; i += 64)
	{
		__m512i v1 = _mm512_load_si512((__m512i *)(vec1 + i));
		__m512i v2 = _mm512_load_si512((__m512i *)(vec2 + i));
		result &= _mm512_cmpeq_epi64_mask(v1, v2);
	}
	return -(result != 0xFF);
#else
	return -(memcmp(vec1, vec2, len_bytes) != 0);
#endif
}

// Hashes input bits and truncates the result to M_SIZE_BYTES.
static inline void hash_length_m(const uint8_t *input, size_t input_len, uint8_t *output)
{
	uint8_t temp[64] = {0};
	pseudohash(512, input, input_len, temp);
	memcpy(output, temp, M_SIZE_BYTES);
}


// Generates the public and secret keys.
int kem_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	public_key_t *local_pk = (public_key_t *)pk;
	secret_key_t *local_sk = (secret_key_t *)sk;

	uint8_t *h0 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *h1 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *h2 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *t0 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *t1 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *t2 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *r1 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *r2 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);

	trike_setz(h0, PADDED_R_SIZE_BYTES);
	trike_setz(h1, PADDED_R_SIZE_BYTES);
	trike_setz(h2, PADDED_R_SIZE_BYTES);
	trike_setz(t0, PADDED_R_SIZE_BYTES);
	trike_setz(t1, PADDED_R_SIZE_BYTES);
	trike_setz(t2, PADDED_R_SIZE_BYTES);
	trike_setz(r1, PADDED_R_SIZE_BYTES);
	trike_setz(r2, PADDED_R_SIZE_BYTES);

	uint8_t seed[M_SIZE_BYTES] = {0};
	uint8_t sigma[M_SIZE_BYTES] = {0};
	uint8_t sigma2[M_SIZE_BYTES] = {0};
	uint32_t h0_idx[PARAM_D] = {0};
	uint32_t h1_idx[PARAM_D] = {0};
	uint32_t h2_idx[PARAM_D] = {0};

	get_random_number(&drng_algorithm, seed, PARAM_M);
	get_random_number(&drng_algorithm, sigma2, PARAM_M);
	get_random_number(&drng_algorithm, sigma, PARAM_M);

	generate_secret_key(h0, h1, h2, h0_idx, h1_idx, h2_idx, seed);

	generate_hash_vectors(t1, t2, r1, sigma);

	calculate_t0(t0, h0, h1, t1, r1);
	calculate_r2(r2, h0, h2, t0, t2);

	memcpy(local_pk->r2, r2, R_SIZE_BYTES);
	memcpy(local_pk->sigma, sigma, M_SIZE_BYTES);

	memcpy(local_sk->h0_idx, h0_idx, PARAM_D * sizeof(uint32_t));
	memcpy(local_sk->h1_idx, h1_idx, PARAM_D * sizeof(uint32_t));
	memcpy(local_sk->h2_idx, h2_idx, PARAM_D * sizeof(uint32_t));
	memcpy(local_sk->h0, h0, R_SIZE_BYTES);
	memcpy(local_sk->t0, t0, R_SIZE_BYTES);
	memcpy(local_sk->r2, r2, R_SIZE_BYTES);
	memcpy(local_sk->sigma, sigma, M_SIZE_BYTES);
	memcpy(local_sk->sigma2, sigma2, M_SIZE_BYTES);

	free(r2);
	free(r1);
	free(t2);
	free(t1);
	free(t0);
	free(h2);
	free(h1);
	free(h0);

	*pk_len_bytes = sizeof(public_key_t);
	*sk_len_bytes = sizeof(secret_key_t);
	return 0;
}

// Encapsulates a shared secret using the given public key.
int kem_enc(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes,
	unsigned char *ct, unsigned long long *ct_len_bytes)
{
	public_key_t *local_pk = (public_key_t *)pk;
	ciphertext_t *local_ct = (ciphertext_t *)ct;

	uint8_t *e = (uint8_t *)aligned_alloc(64, N_ZMM_SIZE_BYTES);
	uint8_t *e0 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *e1 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *e2 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *t1 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *t2 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *r1 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *r2 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *u = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *v = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *hash_input = (uint8_t *)calloc(M_SIZE_BYTES + sizeof(ciphertext_t), sizeof(uint8_t));

	trike_setz(e, N_ZMM_SIZE_BYTES);
	trike_setz(t1, PADDED_R_SIZE_BYTES);
	trike_setz(t2, PADDED_R_SIZE_BYTES);
	trike_setz(r1, PADDED_R_SIZE_BYTES);
	trike_setz(r2, PADDED_R_SIZE_BYTES);

	uint8_t msg[M_SIZE_BYTES] = {0}, msg2[M_SIZE_BYTES] = {0}, err_hash[M_SIZE_BYTES] = {0};
	uint8_t sigma[M_SIZE_BYTES] = {0};

	get_random_number(&drng_algorithm, msg, PARAM_M);

	memcpy(sigma, local_pk->sigma, M_SIZE_BYTES);
	memcpy(r2, local_pk->r2, R_SIZE_BYTES);

	generate_hash_vectors(t1, t2, r1, sigma);

	generate_error_vector(e, e + R_ZMM_SIZE_BYTES, e + 2 * R_ZMM_SIZE_BYTES, msg, r2);

	trike_assign(e, e0, R_ZMM_SIZE_BYTES, PADDED_R_SIZE_BYTES);
	trike_assign(e + R_ZMM_SIZE_BYTES, e1, R_ZMM_SIZE_BYTES, PADDED_R_SIZE_BYTES);
	trike_assign(e + 2 * R_ZMM_SIZE_BYTES, e2, R_ZMM_SIZE_BYTES, PADDED_R_SIZE_BYTES);

	calculate_uv(u, e0, e1, e2, r1, r2);

	calculate_uv(v, e0, e1, e2, t1, t2);

	hash_length_m(e, N_ZMM_SIZE_BYTES * 8, err_hash);

	msgxor(msg2, msg, err_hash);

	memcpy(local_ct->u, u, R_SIZE_BYTES);
	memcpy(local_ct->v, v, R_SIZE_BYTES);
	memcpy(local_ct->c2, msg2, M_SIZE_BYTES);

	memcpy(hash_input, msg, M_SIZE_BYTES);
	memcpy(hash_input + M_SIZE_BYTES, local_ct, sizeof(ciphertext_t));

	hash_length_m(hash_input, (M_SIZE_BYTES + sizeof(ciphertext_t)) * 8, ss);

	free(hash_input);
	free(v);
	free(u);
	free(r2);
	free(r1);
	free(t2);
	free(t1);
	free(e2);
	free(e1);
	free(e0);
	free(e);

	*ss_len_bytes = sizeof(shared_secret_t);
	*ct_len_bytes = sizeof(ciphertext_t);

	return 0;
}

// Decapsulates the shared secret from ciphertext using the secret key.
int kem_dec(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *ct, unsigned long long ct_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes)
{
	secret_key_t *local_sk = (secret_key_t *)sk;
	ciphertext_t *local_ct = (ciphertext_t *)ct;

	uint32_t h0_idx[PARAM_D] = {0}, h1_idx[PARAM_D] = {0}, h2_idx[PARAM_D] = {0};
	uint8_t sigma[M_SIZE_BYTES] = {0}, sigma2[M_SIZE_BYTES] = {0}, err_hash[M_SIZE_BYTES] = {0};
	uint8_t msg[M_SIZE_BYTES] = {0}, msg2[M_SIZE_BYTES] = {0};

	uint8_t *h0 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *t0 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *t1 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *t2 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *r1 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *r2 = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *u = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *v = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *s = (uint8_t *)aligned_alloc(64, PADDED_R_SIZE_BYTES);
	uint8_t *e = (uint8_t *)aligned_alloc(64, N_ZMM_SIZE_BYTES);
	uint8_t *e_calc = (uint8_t *)aligned_alloc(64, N_ZMM_SIZE_BYTES);
	uint8_t *hash_input = (uint8_t *)calloc(M_SIZE_BYTES + sizeof(ciphertext_t), sizeof(uint8_t));

	trike_setz(h0, PADDED_R_SIZE_BYTES);
	trike_setz(t0, PADDED_R_SIZE_BYTES);
	trike_setz(t1, PADDED_R_SIZE_BYTES);
	trike_setz(t2, PADDED_R_SIZE_BYTES);
	trike_setz(r1, PADDED_R_SIZE_BYTES);
	trike_setz(r2, PADDED_R_SIZE_BYTES);
	trike_setz(u, PADDED_R_SIZE_BYTES);
	trike_setz(v, PADDED_R_SIZE_BYTES);
	trike_setz(s, PADDED_R_SIZE_BYTES);
	trike_setz(e, N_ZMM_SIZE_BYTES);
	trike_setz(e_calc, N_ZMM_SIZE_BYTES);

	memcpy(h0_idx, local_sk->h0_idx, PARAM_D * sizeof(uint32_t));
	memcpy(h1_idx, local_sk->h1_idx, PARAM_D * sizeof(uint32_t));
	memcpy(h2_idx, local_sk->h2_idx, PARAM_D * sizeof(uint32_t));
	memcpy(h0, local_sk->h0, R_SIZE_BYTES);
	memcpy(sigma, local_sk->sigma, M_SIZE_BYTES);
	memcpy(sigma2, local_sk->sigma2, M_SIZE_BYTES);
	memcpy(t0, local_sk->t0, R_SIZE_BYTES);
	memcpy(r2, local_sk->r2, R_SIZE_BYTES);
	memcpy(u, local_ct->u, R_SIZE_BYTES);
	memcpy(v, local_ct->v, R_SIZE_BYTES);
	memcpy(msg2, local_ct->c2, M_SIZE_BYTES);

	generate_hash_vectors(t1, t2, r1, sigma);

	calculate_s(s, u, v, h0, t0);

	decode(e, e + R_ZMM_SIZE_BYTES, e + 2 * R_ZMM_SIZE_BYTES, s, h0_idx, h1_idx, h2_idx);

	hash_length_m(e, N_ZMM_SIZE_BYTES * 8, err_hash);

	msgxor(msg, msg2, err_hash);

	generate_error_vector(e_calc, e_calc + R_ZMM_SIZE_BYTES, e_calc + 2 * R_ZMM_SIZE_BYTES, msg, r2);

	uint8_t mask = compare_vec(e, e_calc, N_ZMM_SIZE_BYTES);

	for (size_t i = 0; i < M_SIZE_BYTES; i++)
	{
		msg[i] = (msg[i] & ~mask) | (sigma2[i] & mask);
	}

	memcpy(hash_input, msg, M_SIZE_BYTES);
	memcpy(hash_input + M_SIZE_BYTES, local_ct, sizeof(ciphertext_t));

	hash_length_m(hash_input, (M_SIZE_BYTES + sizeof(ciphertext_t)) * 8, ss);

	free(hash_input);
	free(e_calc);
	free(e);
	free(s);
	free(v);
	free(u);
	free(r2);
	free(r1);
	free(t2);
	free(t1);
	free(t0);

	*ss_len_bytes = sizeof(shared_secret_t);
	return 0;
}