#ifndef YUANYANG_1024_SIGN_NTT_H
#define YUANYANG_1024_SIGN_NTT_H

#include <stdint.h>

#include "yuanyang_inner.h"

#ifndef YUANYANG_SIGN_NTT_DEBUG
#define YUANYANG_SIGN_NTT_DEBUG 0
#endif

#ifndef YUANYANG_SIGN_DELTA_NTT_SCALE_BITS
#define YUANYANG_SIGN_DELTA_NTT_SCALE_BITS 43u
#endif

#ifndef YUANYANG_SIGN_DELTA_NTT_INVERSE_ITERS
#define YUANYANG_SIGN_DELTA_NTT_INVERSE_ITERS 6u
#endif

#define YUANYANG_EXTRABITS 12u

typedef struct {
	uint32_t q1[YUANYANG_D];
} yuanyang_sign_ntt_q1_poly;

typedef struct {
	uint32_t q1[YUANYANG_D];
	uint32_t q2[YUANYANG_D];
} yuanyang_sign_ntt_crt_poly;

typedef struct {
	/* NTT arrays in this key precomputation use Montgomery representation. */
	int64_t a00_q22[YUANYANG_D];
	int64_t a10_q22[YUANYANG_D];
	int64_t a11_q22[YUANYANG_D];
	int64_t u_hat_q11[YUANYANG_D];
	int64_t b_hat_inv_adj[YUANYANG_MAT2_SIZE];
	int64_t delta_g_qD[YUANYANG_MAT2_SIZE];
	int64_t delta_inv_corr0_qD[YUANYANG_D];
	int64_t delta_inv_corr1_qD[YUANYANG_D];
	yuanyang_sign_ntt_crt_poly a_hat_ntt[3];
	yuanyang_sign_ntt_q1_poly u_hat_ntt;
	yuanyang_sign_ntt_crt_poly b_hat_inv_top_ntt[2];
	yuanyang_sign_ntt_q1_poly b_hat_inv_bottom_ntt[2];
	yuanyang_sign_ntt_q1_poly basis_ntt[YUANYANG_MAT2_POLYS];
	yuanyang_sign_ntt_crt_poly delta_g_ntt[YUANYANG_MAT2_POLYS];
	yuanyang_sign_ntt_crt_poly delta_inv_corr_ntt[2];
} yuanyang_sign_ntt_precomp;

typedef struct {
	int32_t p0_scaled[YUANYANG_D];
	int32_t p1_scaled[YUANYANG_D];
} yuanyang_sign_presample;

typedef struct {
	yuanyang_sign_presample *slots;
	size_t capacity;
	size_t head;
	size_t count;
} yuanyang_sign_presample_buffer;

void yuanyang_sign_ntt_precompute(
	yuanyang_sign_ntt_precomp *dst,
	const yuanyang_expanded_sk *expanded);

void yuanyang_sign_presample_buffer_init(
	yuanyang_sign_presample_buffer *buf,
	yuanyang_sign_presample *slots,
	size_t capacity);

int yuanyang_sign_presample_buffer_fill_ntt(
	yuanyang_sign_presample_buffer *buf,
	const yuanyang_sign_ntt_precomp *ntt_precomp,
	prng *rng,
	size_t target_count);

int yuanyang_sign_core_precomputed_ntt(
	const yuanyang_expanded_sk *expanded,
	const yuanyang_sign_ntt_precomp *ntt_precomp,
	const unsigned char *m, unsigned long long m_len_bytes,
	unsigned char *sn, unsigned long long *sn_len_bytes,
	yuanyang_sign_stats *stats);

int yuanyang_sign_core_precomputed_ntt_buffered(
	const yuanyang_expanded_sk *expanded,
	const yuanyang_sign_ntt_precomp *ntt_precomp,
	yuanyang_sign_presample_buffer *presamples,
	const unsigned char *m, unsigned long long m_len_bytes,
	unsigned char *sn, unsigned long long *sn_len_bytes,
	yuanyang_sign_stats *stats);

void yuanyang_sign_ntt_mul_single(
	int64_t out[YUANYANG_D],
	const int64_t a[YUANYANG_D],
	const int64_t b[YUANYANG_D]);

void yuanyang_sign_ntt_mul_crt(
	int64_t out[YUANYANG_D],
	const int64_t a[YUANYANG_D],
	const int64_t b[YUANYANG_D]);

void yuanyang_sign_ntt_a_hat_mul(
	int64_t p0_q22[YUANYANG_D],
	int64_t p1_q22[YUANYANG_D],
	const yuanyang_sign_ntt_precomp *precomp,
	const int32_t x0[YUANYANG_D],
	const int32_t x1[YUANYANG_D]);

void yuanyang_sign_ntt_b_hat_inv_mul_crt(
	int64_t hat0_num_q11_L[YUANYANG_D],
	int64_t hat1_num_q11_L[YUANYANG_D],
	const yuanyang_sign_ntt_precomp *precomp,
	const int32_t centered0_L[YUANYANG_D],
	const int32_t centered1_L[YUANYANG_D]);

void yuanyang_sign_ntt_u_hat_mul(
	int64_t out_q11[YUANYANG_D],
	const yuanyang_sign_ntt_precomp *precomp,
	const int32_t z[YUANYANG_D]);

void yuanyang_sign_ntt_u_hat_mul_prepare(
	int64_t out_q11[YUANYANG_D],
	yuanyang_sign_ntt_q1_poly *z_ntt,
	const yuanyang_sign_ntt_precomp *precomp,
	const int32_t z[YUANYANG_D]);

void yuanyang_sign_ntt_basis_mul(
	int32_t out0[YUANYANG_D],
	int32_t out1[YUANYANG_D],
	const yuanyang_sign_ntt_precomp *precomp,
	const int32_t z1[YUANYANG_D],
	const int32_t z2[YUANYANG_D]);

void yuanyang_sign_ntt_basis_mul_with_z2_ntt(
	int32_t out0[YUANYANG_D],
	int32_t out1[YUANYANG_D],
	const yuanyang_sign_ntt_precomp *precomp,
	const int32_t z1[YUANYANG_D],
	const yuanyang_sign_ntt_q1_poly *z2_ntt);

fpr yuanyang_sign_ntt_delta2_weight(
	const yuanyang_sign_ntt_precomp *precomp,
	const int32_t err0[YUANYANG_D],
	const int32_t err1[YUANYANG_D]);

#endif
