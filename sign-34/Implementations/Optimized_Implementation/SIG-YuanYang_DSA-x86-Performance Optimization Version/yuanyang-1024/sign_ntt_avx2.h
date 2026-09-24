#ifndef YUANYANG_1024_SIGN_NTT_AVX2_H
#define YUANYANG_1024_SIGN_NTT_AVX2_H

#include "sign_ntt.h"

void yuanyang_sign_ntt_precompute_avx2(
	yuanyang_sign_ntt_precomp *dst, const yuanyang_expanded_sk *expanded);
void yuanyang_sign_ntt_mul_single_avx2(
	int64_t *out, const int64_t *a, const int64_t *b);
void yuanyang_sign_ntt_mul_crt_avx2(
	int64_t *out, const int64_t *a, const int64_t *b);
void yuanyang_sign_ntt_a_hat_mul_avx2(
	int64_t *p0, int64_t *p1, const yuanyang_sign_ntt_precomp *precomp,
	const int32_t *x0, const int32_t *x1);
void yuanyang_sign_ntt_b_hat_inv_mul_crt_avx2(
	int64_t *hat0, int64_t *hat1, const yuanyang_sign_ntt_precomp *precomp,
	const int32_t *centered0, const int32_t *centered1);
void yuanyang_sign_ntt_u_hat_mul_avx2(
	int64_t *out, const yuanyang_sign_ntt_precomp *precomp,
	const int32_t *z);
void yuanyang_sign_ntt_u_hat_mul_prepare_avx2(
	int64_t *out, yuanyang_sign_ntt_q1_poly *z_ntt,
	const yuanyang_sign_ntt_precomp *precomp, const int32_t *z);
void yuanyang_sign_ntt_basis_mul_avx2(
	int32_t *out0, int32_t *out1, const yuanyang_sign_ntt_precomp *precomp,
	const int32_t *z1, const int32_t *z2);
void yuanyang_sign_ntt_basis_mul_with_z2_ntt_avx2(
	int32_t *out0, int32_t *out1, const yuanyang_sign_ntt_precomp *precomp,
	const int32_t *z1, const yuanyang_sign_ntt_q1_poly *z2_ntt);
fpr yuanyang_sign_ntt_delta2_weight_avx2(
	const yuanyang_sign_ntt_precomp *precomp,
	const int32_t *err0, const int32_t *err1);

#endif
