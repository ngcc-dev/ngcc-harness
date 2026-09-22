#ifndef YUANYANG_512_INNER_H
#define YUANYANG_512_INNER_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "yuanyang_params.h"

#ifndef YUANYANG_PREFIX
#define YUANYANG_PREFIX   yuanyang
#endif

#define Zf(name)             Zf_(YUANYANG_PREFIX, name)
#define Zf_(prefix, name)    Zf__(prefix, name)
#define Zf__(prefix, name)   prefix ## _ ## name

#include "fpr.h"

typedef struct prng prng;

/*
 * Error codes
*/
#define YUANYANG_SUCCESS             0
#define YUANYANG_INVALID_SIGNATURE  -1
#define YUANYANG_NOT_IMPLEMENTED    -2
#define YUANYANG_BADARG             -3
#define YUANYANG_MEMORY_ERROR       -4
#define YUANYANG_KEYGEN_FAILED      -5
#define YUANYANG_SIGN_FAILED        -6

/*
 * Sizes of various elements that we handle. We mostly handle polynomial of
 * degree d, but the specification describes computations on matrices. These
 * macros aims at making use of flattened matrices, while referencing to their
 * block (M00, M01, ...) for a better coherence between specification and
 * implementation, without performance loss.
*/
#define YUANYANG_MAT2_POLYS      4u
#define YUANYANG_MAT2_SIZE       (YUANYANG_MAT2_POLYS * YUANYANG_D)
#define YUANYANG_MAT2_00         0u
#define YUANYANG_MAT2_01         1u
#define YUANYANG_MAT2_10         2u
#define YUANYANG_MAT2_11         3u
#define YUANYANG_MAT2_INDEX(r, c) (((size_t)(r) << 1) + (size_t)(c))
#define YUANYANG_MAT2_POLY(m, i) ((m) + ((size_t)(i) * YUANYANG_D))

/*
 * Minimal representation of the secret key. All other information can be
 * derived from these values by key expansion. Keeping this compact form
 * separate leaves the serialized key layout explicit.
 */
typedef struct {
	uint16_t h[YUANYANG_D];
	int8_t f[YUANYANG_D];
	int8_t g[YUANYANG_D];
	int8_t F[YUANYANG_D];
	int8_t G[YUANYANG_D];
} yuanyang_compact_sk;

/*
 * Expanded private key used by the signing implementation.
 */
typedef struct {
	yuanyang_compact_sk compact;
	fpr A_hat[YUANYANG_MAT2_SIZE];
	fpr sigma_delta[YUANYANG_MAT2_SIZE];
	fpr u_hat[YUANYANG_D];
} yuanyang_expanded_sk;

typedef struct {
	unsigned attempts;
	unsigned f_not_invertible_mod_q;
	unsigned security_loss_reject;
	unsigned fg_not_int8;
	unsigned ntrusolve_gcd;
	unsigned ntrusolve_reduce;
	unsigned ntrusolve_limit;
	unsigned ntrusolve_other;
	unsigned ntrusolve_ref;
	unsigned perturbation;
} yuanyang_keygen_stats;

typedef struct {
	uint64_t attempts;
	uint64_t delta1_reject;
	uint64_t delta2_reject;
	uint64_t s2_range_reject;
	uint64_t squared_norm_reject;
	uint64_t compression_reject;
	uint64_t success;
	uint64_t max_attempts_exhausted;
} yuanyang_sign_stats;

typedef int (*yuanyang_randombytes)(
	void *ctx, unsigned char *buf, unsigned long long len_bytes);

uint16_t yuanyang_load_u16_le(const unsigned char *src);
int16_t yuanyang_center_lift_q(uint16_t x);

int yuanyang_encode_public_key(
	unsigned char *pk, unsigned long long pk_len_bytes,
	const uint16_t h[YUANYANG_D]);

int yuanyang_decode_public_key(
	uint16_t h[YUANYANG_D],
	const unsigned char *pk, unsigned long long pk_len_bytes);

int yuanyang_decode_signature_s1(
	int16_t s1[YUANYANG_D],
	const unsigned char *sn, unsigned long long sn_len_bytes);

int yuanyang_encode_signature_s1(
	unsigned char *sn, unsigned long long sn_len_bytes,
	const int16_t s1[YUANYANG_D]);

int yuanyang_decode_private_key(
	yuanyang_expanded_sk *decoded,
	const unsigned char *sk, unsigned long long sk_len_bytes);

int yuanyang_encode_private_key(
	unsigned char *sk, unsigned long long *sk_len_bytes,
	const yuanyang_expanded_sk *decoded);

int yuanyang_hash_message_with_public_key(
	unsigned char seed[32],
	const unsigned char *m, unsigned long long m_len_bytes,
	const uint16_t h[YUANYANG_D]);

int yuanyang_hash_to_challenge(
	uint16_t c[YUANYANG_D],
	const unsigned char salt[YUANYANG_SALT_BYTES],
	const unsigned char seed[32]);

void yuanyang_mul_mod_xn_plus_1(
	uint16_t out[YUANYANG_D],
	const uint16_t h[YUANYANG_D],
	const int16_t s1[YUANYANG_D]);

int yuanyang_pairgen(
	int8_t f[YUANYANG_D],
	int8_t g[YUANYANG_D], prng *rng);


int yuanyang_expand_private_key(
	yuanyang_expanded_sk *expanded,
	const yuanyang_compact_sk *compact);

void yuanyang_derive_b_hat_inv_fft(
	fpr dst_fft[YUANYANG_MAT2_SIZE],
	const fpr B_fft[YUANYANG_MAT2_SIZE],
	const fpr u_hat_fft[YUANYANG_D]);

int yuanyang_verify_core(
	const unsigned char *pk, unsigned long long pk_len_bytes,
	const unsigned char *sn, unsigned long long sn_len_bytes,
	const unsigned char *m, unsigned long long m_len_bytes);

int yuanyang_keygen_core(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes);

int yuanyang_keygen_core_with_stats(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes,
	yuanyang_keygen_stats *stats);

int yuanyang_sign_core(
	const unsigned char *sk, unsigned long long sk_len_bytes,
	const unsigned char *m, unsigned long long m_len_bytes,
	unsigned char *sn, unsigned long long *sn_len_bytes);

int yuanyang_sign_core_with_stats(
	const unsigned char *sk, unsigned long long sk_len_bytes,
	const unsigned char *m, unsigned long long m_len_bytes,
	unsigned char *sn, unsigned long long *sn_len_bytes,
	yuanyang_sign_stats *stats);

/*
 * FFT functions
 *
 * Falcon FFT representation:
 *   Re(f(w_j)) is stored in slot rev(j)/2,
 *   Im(f(w_j)) is stored in slot rev(j)/2 + N/2.
 * The representation has N fpr words for N/2 complex values.
 */

 /*
 * Compute FFT in-place: the source array should contain a real
 * polynomial (N coefficients); its storage area is reused to store
 * the FFT representation of that polynomial (N/2 complex numbers).
 */
void Zf(FFT)(fpr *f, unsigned logn);

/*
 * Compute the inverse FFT in-place: the source array should contain the
 * FFT representation of a real polynomial (N/2 elements); the resulting
 * real polynomial (N coefficients of type 'fpr') is written over the
 * array.
 */
void Zf(iFFT)(fpr *f, unsigned logn);

/*
 * Add polynomial b to polynomial a. a and b MUST NOT overlap. This
 * function works in both normal and FFT representations.
 */
void Zf(poly_add)(fpr *restrict a, const fpr *restrict b, unsigned logn);

/*
 * Subtract polynomial b from polynomial a. a and b MUST NOT overlap. This
 * function works in both normal and FFT representations.
 */
void Zf(poly_sub)(fpr *restrict a, const fpr *restrict b, unsigned logn);

/*
 * Negate polynomial a. This function works in both normal and FFT
 * representations.
 */
void Zf(poly_neg)(fpr *a, unsigned logn);

/*
 * Multiply polynomial a with polynomial b. a and b MUST NOT overlap.
 * This function works only in FFT representation.
 */
void Zf(poly_mul_fft)(fpr *restrict a, const fpr *restrict b, unsigned logn);

/*
 * Multiply polynomial with a real constant. This function works in both
 * normal and FFT representations.
 */
void Zf(poly_mulconst)(fpr *a, fpr x, unsigned logn);

#endif
