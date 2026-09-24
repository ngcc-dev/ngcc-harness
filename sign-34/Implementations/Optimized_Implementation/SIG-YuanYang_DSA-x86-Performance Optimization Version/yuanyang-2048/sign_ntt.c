#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ntt.h"
#include "sign_ntt.h"
#include "sign_ntttable.h"

#if YUANYANG_HAVE_AVX2_IMPL && !defined(YUANYANG_SIGN_NTT_NO_DISPATCH)
#include "cpu_features.h"
#include "sign_ntt_avx2.h"
#define YUANYANG_SIGN_NTT_USE_AVX2() \
	(yuanyang_cpu_get_path() == YUANYANG_CPU_AVX2)
#else
#define YUANYANG_SIGN_NTT_USE_AVX2() 0
#endif

#if YUANYANG_SIGN_NTT_DEBUG
#include <stdio.h>
#endif

/*
 * Signing-only NTT layer.
 *
 * The shared ntt.c file provides Q1 = QNTT arithmetic used by the single-prime signature products. Some signing products need more headroom;
 * this file adds Q2 with the same direct negacyclic layout and combines Q1/Q2
 * residues by CRT only at those wider products.
 */

#define YY_SIGN_NTT_Q1        UINT32_C(2147352577)
#define YY_SIGN_NTT_Q1_HALF   UINT32_C(1073676288)

#define YY_SIGN_NTT_Q1_INV_Q2_MONTY Q2NTT_Q1_INV_Q2_MONTY
#define YY_SIGN_NTT_WEIGHT_SCALE_BITS 43u

static uint32_t
q2_norm_i64(int64_t x)
{
	uint64_t ux, sign, mag, hi;
	uint32_t r, d, neg_r;

	/* Q2NTT = 2^31 - 94207; three fixed folds cover int64_t. */
	ux = (uint64_t)x;
	sign = (uint64_t)0 - (uint64_t)(x < 0);
	mag = (ux ^ sign) - sign;
	for (unsigned i = 0; i < 3u; i++) {
		hi = mag >> 31;
		mag = (mag & UINT64_C(0x7FFFFFFF)) + hi * UINT64_C(94207);
	}
	r = (uint32_t)mag;
	d = r - Q2NTT;
	r = d + (Q2NTT & (uint32_t)-(d >> 31));
	neg_r = (Q2NTT - r) & (uint32_t)-(r != 0u);
	return (r & (uint32_t)~sign) | (neg_r & (uint32_t)sign);
}

static uint32_t
q2_norm_i32(int32_t x)
{
	if (x >= 0 && x < (int32_t)Q2NTT) {
		return (uint32_t)x;
	}
	if (x < 0 && x > -(int32_t)Q2NTT) {
		return Q2NTT + (uint32_t)x;
	}
	return q2_norm_i64(x);
}

static yy_u128
u128_add(yy_u128 a, yy_u128 b)
{
	yy_u128 z;

	z.lo = a.lo + b.lo;
	z.hi = a.hi + b.hi + (uint64_t)(z.lo < a.lo);
	return z;
}

static yy_u128
u128_sub(yy_u128 a, yy_u128 b)
{
	yy_u128 z;

	z.lo = a.lo - b.lo;
	z.hi = a.hi - b.hi - (uint64_t)(a.lo < b.lo);
	return z;
}

static int
u128_cmp(yy_u128 a, yy_u128 b)
{
	if (a.hi != b.hi) {
		return a.hi > b.hi ? 1 : -1;
	}
	if (a.lo != b.lo) {
		return a.lo > b.lo ? 1 : -1;
	}
	return 0;
}

static void
u128_add_assign(yy_u128 *a, yy_u128 b)
{
	*a = u128_add(*a, b);
}

static uint64_t
u128_lshift_to_u64(yy_u128 a, unsigned shift)
{
	if (shift == 0u) {
		return a.lo;
	}
	if (shift < 64u) {
		return a.lo << shift;
	}
	return 0;
}

static uint64_t
u128_pow2_scale_rne(yy_u128 a, unsigned mul_bits, unsigned div_bits)
{
	if (mul_bits >= div_bits) {
		return u128_lshift_to_u64(a, mul_bits - div_bits);
	}
	return yy_u128_rshift_rne(a, div_bits - mul_bits);
}

static void
dot_add_i32_i64(yy_u128 *pos, yy_u128 *neg, int32_t a, int64_t b)
{
	uint64_t sign;
	uint64_t aa;
	uint64_t bb;
	yy_u128 prod;

	if (a == 0 || b == 0) {
		return;
	}
	sign = ((uint64_t)(a < 0) ^ (uint64_t)(b < 0)) & 1u;
	aa = yy_fpr_abs_i64((int64_t)a);
	bb = yy_fpr_abs_i64(b);
	prod = yy_mul64_wide(aa, bb);
	if (sign) {
		u128_add_assign(neg, prod);
	} else {
		u128_add_assign(pos, prod);
	}
}

static inline uint32_t
q2_add(uint32_t a, uint32_t b)
{
	uint32_t d;

	d = a + b - Q2NTT;
	d += Q2NTT & -(d >> 31);
	return d;
}

static inline uint32_t
q2_sub(uint32_t a, uint32_t b)
{
	uint32_t d;

	d = a - b;
	d += Q2NTT & -(d >> 31);
	return d;
}

static inline uint32_t
q2_montymul(uint32_t a, uint32_t b)
{
	uint64_t z;
	uint32_t m;
	uint32_t d;

	z = (uint64_t)a * b;
	m = (uint32_t)z * Q2NTT_Q0I;
	z = (z + (uint64_t)m * Q2NTT) >> 32;
	d = (uint32_t)z - Q2NTT;
	d += Q2NTT & -(d >> 31);
	return d;
}

static void
q2_forward(uint32_t a[YUANYANG_D])
{
#define Q2_FORWARD_STAGE(M) do {                                    \
		const unsigned ht = YUANYANG_D / (2u * (M));                \
		for (unsigned i = 0, j1 = 0; i < (M); i++, j1 += 2u * ht) { \
			const uint32_t s = q2ntt_GMb[(M) + i];                  \
			const unsigned j2 = j1 + ht;                            \
			_Pragma("omp simd")                                     \
			for (unsigned j = j1; j < j2; j++) {                    \
				const uint32_t u = a[j];                            \
				const uint32_t v = q2_montymul(a[j + ht], s);       \
				a[j] = q2_add(u, v);                                \
				a[j + ht] = q2_sub(u, v);                           \
			}                                                       \
		}                                                           \
	} while (0)
	Q2_FORWARD_STAGE(1u << 0);
	Q2_FORWARD_STAGE(1u << 1);
	Q2_FORWARD_STAGE(1u << 2);
	Q2_FORWARD_STAGE(1u << 3);
	Q2_FORWARD_STAGE(1u << 4);
	Q2_FORWARD_STAGE(1u << 5);
	Q2_FORWARD_STAGE(1u << 6);
	Q2_FORWARD_STAGE(1u << 7);
	Q2_FORWARD_STAGE(1u << 8);
	Q2_FORWARD_STAGE(1u << 9);
	Q2_FORWARD_STAGE(1u << 10);
#undef Q2_FORWARD_STAGE
}

static void
q2_inverse_residue(uint32_t a[YUANYANG_D])
{
#define Q2_INVERSE_STAGE(HM) do {                                    \
		const unsigned st = YUANYANG_D / (2u * (HM));                \
		for (unsigned i = 0, j1 = 0; i < (HM); i++, j1 += 2u * st) { \
			const uint32_t s = q2ntt_iGMb[(HM) + i];                 \
			const unsigned j2 = j1 + st;                             \
			_Pragma("omp simd")                                      \
			for (unsigned j = j1; j < j2; j++) {                     \
				const uint32_t u = a[j];                             \
				const uint32_t v = a[j + st];                        \
				a[j] = q2_add(u, v);                                 \
				a[j + st] = q2_montymul(q2_sub(u, v), s);            \
			}                                                        \
		}                                                            \
	} while (0)
	Q2_INVERSE_STAGE(1u << 10);
	Q2_INVERSE_STAGE(1u << 9);
	Q2_INVERSE_STAGE(1u << 8);
	Q2_INVERSE_STAGE(1u << 7);
	Q2_INVERSE_STAGE(1u << 6);
	Q2_INVERSE_STAGE(1u << 5);
	Q2_INVERSE_STAGE(1u << 4);
	Q2_INVERSE_STAGE(1u << 3);
	Q2_INVERSE_STAGE(1u << 2);
	Q2_INVERSE_STAGE(1u << 1);
#undef Q2_INVERSE_STAGE
	{
		const unsigned t = YUANYANG_D >> 1;
		const uint32_t scaled_twiddle = q2_montymul(q2ntt_iGMb[1],
			Q2NTT_INV_N_MONTY);
#pragma omp simd
		for (unsigned j = 0; j < t; j++) {
			const uint32_t u = a[j];
			const uint32_t v = a[j + t];
			a[j] = q2_montymul(q2_add(u, v), Q2NTT_INV_N_MONTY);
			a[j + t] = q2_montymul(q2_sub(u, v), scaled_twiddle);
		}
	}
}

static void
negacyclic_mul_q2(
	uint32_t out[YUANYANG_D],
	const int64_t a[YUANYANG_D],
	const int64_t b[YUANYANG_D])
{
	uint32_t aa[YUANYANG_D];
	uint32_t bb[YUANYANG_D];

#pragma omp simd
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		aa[i] = q2_norm_i64(a[i]);
		bb[i] = q2_norm_i64(b[i]);
	}

	q2_forward(aa);
	q2_forward(bb);
#pragma omp simd
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		aa[i] = q2_montymul(q2_montymul(aa[i], Q2NTT_R2), bb[i]);
	}
	q2_inverse_residue(aa);

#pragma omp simd
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		out[i] = aa[i];
	}
}

static void
q2_forward_i64(
	uint32_t out[YUANYANG_D],
	const int64_t a[YUANYANG_D])
{
#pragma omp simd
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		out[i] = q2_norm_i64(a[i]);
	}
	q2_forward(out);
}

static void
q2_to_montgomery(uint32_t a[YUANYANG_D])
{
#pragma omp simd
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		a[i] = q2_montymul(a[i], Q2NTT_R2);
	}
}

static void
q2_forward_i32(
	uint32_t out[YUANYANG_D],
	const int32_t a[YUANYANG_D])
{
#pragma omp simd
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		out[i] = q2_norm_i32(a[i]);
	}
	q2_forward(out);
}

static void
q2_mul_ntt_residue(
	uint32_t out[YUANYANG_D],
	const uint32_t a_ntt_montgomery[YUANYANG_D],
	uint32_t b_ntt[YUANYANG_D])
{
#pragma omp simd
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		b_ntt[i] = q2_montymul(a_ntt_montgomery[i], b_ntt[i]);
	}
	q2_inverse_residue(b_ntt);
#pragma omp simd
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		out[i] = b_ntt[i];
	}
}

static void
q2_mul_montgomery_pretransformed_residue(
	uint32_t out[YUANYANG_D],
	const uint32_t a_ntt_montgomery[YUANYANG_D],
	const uint32_t b_ntt[YUANYANG_D])
{
#pragma omp simd
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		out[i] = q2_montymul(a_ntt_montgomery[i], b_ntt[i]);
	}
	q2_inverse_residue(out);
}

static void
q2_muladd_montgomery_pretransformed_residue(
	uint32_t out[YUANYANG_D],
	const uint32_t a0_ntt_montgomery[YUANYANG_D],
	const uint32_t b0_ntt[YUANYANG_D],
	const uint32_t a1_ntt_montgomery[YUANYANG_D],
	const uint32_t b1_ntt[YUANYANG_D])
{
#pragma omp simd
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		uint32_t p0;
		uint32_t p1;

		p0 = q2_montymul(a0_ntt_montgomery[i], b0_ntt[i]);
		p1 = q2_montymul(a1_ntt_montgomery[i], b1_ntt[i]);
		out[i] = q2_add(p0, p1);
	}
	q2_inverse_residue(out);
}

static void
q2_mul_ntt_i64_residue(
	uint32_t out[YUANYANG_D],
	const uint32_t a_ntt[YUANYANG_D],
	const int64_t b[YUANYANG_D])
{
	uint32_t b_ntt[YUANYANG_D];

	q2_forward_i64(b_ntt, b);
	q2_mul_ntt_residue(out, a_ntt, b_ntt);
}

static void
crt_combine_residues(
	int64_t out[YUANYANG_D],
	const uint32_t r1[YUANYANG_D],
	const uint32_t r2[YUANYANG_D])
{
	/*
	 * Reconstruct the centered integer represented by residues modulo Q1 and
	 * Q2. The Q1 residue comes from ntt.c, while Q2 is local to this file.
	 */
	const uint64_t q1 = YY_SIGN_NTT_Q1;
	const uint64_t q2 = Q2NTT;
	const uint64_t modulus = q1 * q2;
	const uint64_t half = modulus >> 1;

	for (unsigned i = 0; i < YUANYANG_D; i++) {
		uint32_t diff;
		uint32_t t;
		uint64_t x;

		diff = r2[i] >= r1[i]
			? r2[i] - r1[i]
			: (uint32_t)((uint64_t)r2[i] + q2 - r1[i]);
		t = q2_montymul(diff, YY_SIGN_NTT_Q1_INV_Q2_MONTY);
		x = (uint64_t)r1[i] + q1 * t;
		if (x > half) {
			out[i] = -(int64_t)(modulus - x);
		} else {
			out[i] = (int64_t)x;
		}
	}
}

static void
crt_precompute_i64(
	yuanyang_sign_ntt_crt_poly *dst,
	const int64_t a[YUANYANG_D])
{
	/* Precompute the same coefficient vector under both NTT primes. */
	qntt_encode_i64(dst->q1, a);
	qntt_forward(dst->q1);
	q2_forward_i64(dst->q2, a);
	qntt_to_montgomery(dst->q1);
	q2_to_montgomery(dst->q2);
}

static void
crt_precompute_i32(
	yuanyang_sign_ntt_crt_poly *dst,
	const int32_t a[YUANYANG_D])
{
	/* Precompute the same coefficient vector under both NTT primes. */
	qntt_encode_i32(dst->q1, a);
	qntt_forward(dst->q1);
	q2_forward_i32(dst->q2, a);
}

static void
q1_precompute_i64(
	yuanyang_sign_ntt_q1_poly *dst,
	const int64_t a[YUANYANG_D])
{
	qntt_encode_i64(dst->q1, a);
	qntt_forward(dst->q1);
}

static void
q1_precompute_i32(
	yuanyang_sign_ntt_q1_poly *dst,
	const int32_t a[YUANYANG_D])
{
	qntt_encode_i32(dst->q1, a);
	qntt_forward(dst->q1);
}

static void
q1_mul_pretransformed_residue(
	uint32_t out[YUANYANG_D],
	const uint32_t a_ntt[YUANYANG_D],
	const uint32_t b_ntt[YUANYANG_D])
{
	qntt_pointwise_mul_to(out, a_ntt, b_ntt);
	qntt_inverse_residue(out);
}

static void
q1_mul_montgomery_pretransformed_residue(
	uint32_t out[YUANYANG_D],
	const uint32_t a_ntt_montgomery[YUANYANG_D],
	const uint32_t b_ntt[YUANYANG_D])
{
	qntt_pointwise_montgomery_mul_to(out, a_ntt_montgomery, b_ntt);
	qntt_inverse_residue(out);
}

static void
q1_muladd_montgomery_pretransformed_residue(
	uint32_t out[YUANYANG_D],
	const uint32_t a0_ntt_montgomery[YUANYANG_D], const uint32_t b0_ntt[YUANYANG_D],
	const uint32_t a1_ntt_montgomery[YUANYANG_D], const uint32_t b1_ntt[YUANYANG_D])
{
	qntt_pointwise_montgomery_muladd_to(out, a0_ntt_montgomery, b0_ntt,
		a1_ntt_montgomery, b1_ntt);
	qntt_inverse_residue(out);
}

static void
q1_mul_montgomery_pretransformed_centered(
	int64_t out[YUANYANG_D], const uint32_t a_ntt_montgomery[YUANYANG_D],
	const uint32_t b_ntt[YUANYANG_D])
{
	uint32_t residue[YUANYANG_D];
	q1_mul_montgomery_pretransformed_residue(residue, a_ntt_montgomery, b_ntt);
	qntt_center_i64(out, residue);
}

static void
q1_muladd_montgomery_pretransformed_centered(
	int64_t out[YUANYANG_D],
	const uint32_t a0_ntt_montgomery[YUANYANG_D], const uint32_t b0_ntt[YUANYANG_D],
	const uint32_t a1_ntt_montgomery[YUANYANG_D], const uint32_t b1_ntt[YUANYANG_D])
{
	uint32_t residue[YUANYANG_D];
	q1_muladd_montgomery_pretransformed_residue(residue,
		a0_ntt_montgomery, b0_ntt, a1_ntt_montgomery, b1_ntt);
	qntt_center_i64(out, residue);
}

static void
q1_mul_pretransformed_centered(
	int64_t out[YUANYANG_D],
	const uint32_t a_ntt[YUANYANG_D],
	const uint32_t b_ntt[YUANYANG_D])
{
	uint32_t residue[YUANYANG_D];

	q1_mul_pretransformed_residue(residue, a_ntt, b_ntt);
	qntt_center_i64(out, residue);
}

static void
q1_mul_ntt_i64_residue(
	uint32_t out[YUANYANG_D],
	const uint32_t a_ntt[YUANYANG_D],
	const int64_t b[YUANYANG_D])
{
	uint32_t b_ntt[YUANYANG_D];

	qntt_encode_i64(b_ntt, b);
	qntt_forward(b_ntt);
	q1_mul_montgomery_pretransformed_residue(out, a_ntt, b_ntt);
}

static void
q1_mul_ntt_i32_centered(
	int64_t out[YUANYANG_D],
	const uint32_t a_ntt[YUANYANG_D],
	const int32_t b[YUANYANG_D])
{
	uint32_t b_ntt[YUANYANG_D];

	qntt_encode_i32(b_ntt, b);
	qntt_forward(b_ntt);
	q1_mul_montgomery_pretransformed_centered(out, a_ntt, b_ntt);
}

static void
q1_mul_i64_residue(
	uint32_t out[YUANYANG_D],
	const int64_t a[YUANYANG_D],
	const int64_t b[YUANYANG_D])
{
	uint32_t a_ntt[YUANYANG_D];
	uint32_t b_ntt[YUANYANG_D];

	qntt_encode_i64(a_ntt, a);
	qntt_encode_i64(b_ntt, b);
	qntt_forward(a_ntt);
	qntt_forward(b_ntt);
	q1_mul_pretransformed_residue(out, a_ntt, b_ntt);
}

static void
q1_mul_i64_centered(
	int64_t out[YUANYANG_D],
	const int64_t a[YUANYANG_D],
	const int64_t b[YUANYANG_D])
{
	uint32_t residue[YUANYANG_D];

	q1_mul_i64_residue(residue, a, b);
	qntt_center_i64(out, residue);
}

static void
q1_neg_precomp(
	yuanyang_sign_ntt_q1_poly *dst,
	const yuanyang_sign_ntt_q1_poly *src)
{
#pragma omp simd
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		dst->q1[i] = src->q1[i] == 0
			? 0
			: (uint32_t)(YY_SIGN_NTT_Q1 - src->q1[i]);
	}
}

static void
mul_crt_precomp_i64(
	int64_t out[YUANYANG_D],
	const yuanyang_sign_ntt_crt_poly *a_ntt,
	const int64_t b[YUANYANG_D])
{
	/* Multiply precomputed CRT operand by a coefficient-domain vector. */
	uint32_t r1[YUANYANG_D], r2[YUANYANG_D];

	q1_mul_ntt_i64_residue(r1, a_ntt->q1, b);
	q2_mul_ntt_i64_residue(r2, a_ntt->q2, b);
	crt_combine_residues(out, r1, r2);
}

static void
mul_crt_pretransformed(
	int64_t out[YUANYANG_D],
	const yuanyang_sign_ntt_crt_poly *a_ntt,
	const yuanyang_sign_ntt_crt_poly *b_ntt)
{
	/* Multiply two CRT-precomputed operands and reconstruct signed integers. */
	uint32_t r1[YUANYANG_D], r2[YUANYANG_D];

	q1_mul_montgomery_pretransformed_residue(r1, a_ntt->q1, b_ntt->q1);
	q2_mul_montgomery_pretransformed_residue(r2, a_ntt->q2, b_ntt->q2);
	crt_combine_residues(out, r1, r2);
}

static void
mul_crt_pretransformed_add2(
	int64_t out[YUANYANG_D],
	const yuanyang_sign_ntt_crt_poly *a0_ntt,
	const yuanyang_sign_ntt_crt_poly *b0_ntt,
	const yuanyang_sign_ntt_crt_poly *a1_ntt,
	const yuanyang_sign_ntt_crt_poly *b1_ntt)
{
	/* Sum two CRT products in the NTT domain before reconstructing. */
	uint32_t r1[YUANYANG_D], r2[YUANYANG_D];

	q1_muladd_montgomery_pretransformed_residue(r1, a0_ntt->q1, b0_ntt->q1,
		a1_ntt->q1, b1_ntt->q1);
	q2_muladd_montgomery_pretransformed_residue(r2, a0_ntt->q2, b0_ntt->q2,
		a1_ntt->q2, b1_ntt->q2);
	crt_combine_residues(out, r1, r2);
}

static void
mul_q1_precomp_i32(
	int64_t out[YUANYANG_D],
	const yuanyang_sign_ntt_q1_poly *a_ntt,
	const int32_t b[YUANYANG_D])
{
	q1_mul_ntt_i32_centered(out, a_ntt->q1, b);
}

static void
mul_q1_pretransformed(
	int64_t out[YUANYANG_D],
	const yuanyang_sign_ntt_q1_poly *a_ntt,
	const yuanyang_sign_ntt_q1_poly *b_ntt)
{
	q1_mul_pretransformed_centered(out, a_ntt->q1, b_ntt->q1);
}

void
yuanyang_sign_ntt_mul_single(
	int64_t out[YUANYANG_D],
	const int64_t a[YUANYANG_D],
	const int64_t b[YUANYANG_D])
{
#if YUANYANG_HAVE_AVX2_IMPL && !defined(YUANYANG_SIGN_NTT_NO_DISPATCH)
	if (YUANYANG_SIGN_NTT_USE_AVX2()) {
		yuanyang_sign_ntt_mul_single_avx2(out, a, b);
		return;
	}
#endif
	q1_mul_i64_centered(out, a, b);
}

void
yuanyang_sign_ntt_mul_crt(
	int64_t out[YUANYANG_D],
	const int64_t a[YUANYANG_D],
	const int64_t b[YUANYANG_D])
{
	uint32_t r1[YUANYANG_D], r2[YUANYANG_D];

#if YUANYANG_HAVE_AVX2_IMPL && !defined(YUANYANG_SIGN_NTT_NO_DISPATCH)
	if (YUANYANG_SIGN_NTT_USE_AVX2()) {
		yuanyang_sign_ntt_mul_crt_avx2(out, a, b);
		return;
	}
#endif
	q1_mul_i64_residue(r1, a, b);
	negacyclic_mul_q2(r2, a, b);
	crt_combine_residues(out, r1, r2);
}

static int64_t
div_pow2_i64_rne(int64_t x, unsigned bits)
{
	uint64_t neg;
	uint64_t mag;
	uint64_t q;
	uint64_t r;
	uint64_t half;

	if (bits == 0u) {
		return x;
	}
	neg = (uint64_t)(x < 0);
	mag = yy_fpr_abs_i64(x);
	q = mag >> bits;
	r = mag & ((UINT64_C(1) << bits) - 1u);
	half = UINT64_C(1) << (bits - 1u);
	if (r > half || (r == half && (q & 1u) != 0)) {
		q++;
	}
	return neg ? -(int64_t)q : (int64_t)q;
}

#if YUANYANG_SIGN_NTT_DEBUG
static void
poly_mul_crt_scaled(
	int64_t out[YUANYANG_D],
	const int64_t a[YUANYANG_D],
	const int64_t b[YUANYANG_D],
	unsigned bits)
{
	int64_t prod[YUANYANG_D];

	yuanyang_sign_ntt_mul_crt(prod, a, b);
#pragma omp simd
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		out[i] = div_pow2_i64_rne(prod[i], bits);
	}
}
#endif

static void
poly_mul_crt_scaled_precomp(
	int64_t out[YUANYANG_D],
	const yuanyang_sign_ntt_crt_poly *a_ntt,
	const int64_t b[YUANYANG_D],
	unsigned bits)
{
	int64_t prod[YUANYANG_D];

	mul_crt_precomp_i64(prod, a_ntt, b);
#pragma omp simd
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		out[i] = div_pow2_i64_rne(prod[i], bits);
	}
}

static void
poly_add(
	int64_t out[YUANYANG_D],
	const int64_t a[YUANYANG_D],
	const int64_t b[YUANYANG_D])
{
#pragma omp simd
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		out[i] = a[i] + b[i];
	}
}

#if YUANYANG_SIGN_NTT_DEBUG
static int64_t
poly_max_abs(const int64_t a[YUANYANG_D])
{
	int64_t max;

	max = 0;
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		int64_t x;

		x = a[i] < 0 ? -a[i] : a[i];
		if (x > max) {
			max = x;
		}
	}
	return max;
}

static void
debug_report_inverse_corr_residual(
	const char *label,
	const int64_t g[YUANYANG_D],
	const int64_t corr[YUANYANG_D],
	unsigned bits)
{
	int64_t gc[YUANYANG_D];
	int64_t residual[YUANYANG_D];

	poly_mul_crt_scaled(gc, g, corr, bits);
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		residual[i] = corr[i] - g[i] - gc[i];
	}
	printf("%s: g_max=%lld corr_max=%lld residual_max=%lld\n",
		label,
		(long long)poly_max_abs(g),
		(long long)poly_max_abs(corr),
		(long long)poly_max_abs(residual));
}
#endif

static void
poly_inverse_correction_scaled_precomp(
	int64_t out_corr[YUANYANG_D],
	const yuanyang_sign_ntt_crt_poly *g_ntt,
	const int64_t g[YUANYANG_D],
	unsigned bits)
{
	int64_t power[YUANYANG_D];
	int64_t next_power[YUANYANG_D];

	memset(out_corr, 0, sizeof(int64_t) * YUANYANG_D);
	memcpy(power, g, sizeof power);
	for (unsigned iter = 0; iter < YUANYANG_SIGN_DELTA_NTT_INVERSE_ITERS;
		iter++)
	{
		for (unsigned i = 0; i < YUANYANG_D; i++) {
			out_corr[i] += power[i];
		}
		if (iter + 1u < YUANYANG_SIGN_DELTA_NTT_INVERSE_ITERS) {
			poly_mul_crt_scaled_precomp(next_power, g_ntt, power, bits);
			memcpy(power, next_power, sizeof power);
		}
	}
}

static void
poly_apply_inverse_correction_scaled_precomp(
	int64_t out[YUANYANG_D],
	const yuanyang_sign_ntt_crt_poly *corr_ntt,
	const int64_t in[YUANYANG_D],
	unsigned bits)
{
	int64_t correction[YUANYANG_D];

	poly_mul_crt_scaled_precomp(correction, corr_ntt, in, bits);
#pragma omp simd
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		out[i] = in[i] + correction[i];
	}
}

void
yuanyang_sign_ntt_precompute(
	yuanyang_sign_ntt_precomp *dst,
	const yuanyang_expanded_sk *expanded)
{
#if YUANYANG_HAVE_AVX2_IMPL && !defined(YUANYANG_SIGN_NTT_NO_DISPATCH)
	if (YUANYANG_SIGN_NTT_USE_AVX2()) {
		yuanyang_sign_ntt_precompute_avx2(dst, expanded);
		return;
	}
#endif
	/*
	 * Convert all key-dependent signing products to coefficient-domain
	 * fixed-point integers, then precompute their Q1 or CRT NTT forms.
	 */
	int64_t f[YUANYANG_D], g[YUANYANG_D];
	int64_t F[YUANYANG_D], G[YUANYANG_D];
	int64_t uf_q11[YUANYANG_D], ug_q11[YUANYANG_D];
	int64_t *adj00, *adj01, *adj10, *adj11;

	for (unsigned i = 0; i < YUANYANG_D; i++) {
		dst->a00_q22[i] = fpr_rint_to_scaled(
			YUANYANG_MAT2_POLY(expanded->A_hat, YUANYANG_MAT2_00)[i],
			YUANYANG_A_HAT_PRECISION_BITS);
		dst->a10_q22[i] = fpr_rint_to_scaled(
			YUANYANG_MAT2_POLY(expanded->A_hat, YUANYANG_MAT2_10)[i],
			YUANYANG_A_HAT_PRECISION_BITS);
		dst->a11_q22[i] = fpr_rint_to_scaled(
			YUANYANG_MAT2_POLY(expanded->A_hat, YUANYANG_MAT2_11)[i],
			YUANYANG_A_HAT_PRECISION_BITS);
		dst->u_hat_q11[i] = fpr_rint_to_scaled(expanded->u_hat[i],
			YUANYANG_U_HAT_PRECISION_BITS);
		f[i] = expanded->compact.f[i];
		g[i] = expanded->compact.g[i];
		F[i] = expanded->compact.F[i];
		G[i] = expanded->compact.G[i];
		for (unsigned j = 0; j < YUANYANG_MAT2_POLYS; j++) {
			YUANYANG_MAT2_POLY(dst->delta_g_qD, j)[i] =
				fpr_rint_to_scaled(
					fpr_mul(YUANYANG_MAT2_POLY(
						expanded->sigma_delta, j)[i],
						fpr_inv_sigma_sq),
					YUANYANG_SIGN_DELTA_NTT_SCALE_BITS);
		}
	}

	q1_precompute_i64(&dst->u_hat_ntt, dst->u_hat_q11);
	q1_precompute_i64(&dst->basis_ntt[YUANYANG_MAT2_00], f);
	q1_precompute_i64(&dst->basis_ntt[YUANYANG_MAT2_01], F);
	q1_precompute_i64(&dst->basis_ntt[YUANYANG_MAT2_10], g);
	q1_precompute_i64(&dst->basis_ntt[YUANYANG_MAT2_11], G);
	mul_q1_pretransformed(uf_q11, &dst->u_hat_ntt,
		&dst->basis_ntt[YUANYANG_MAT2_00]);
	mul_q1_pretransformed(ug_q11, &dst->u_hat_ntt,
		&dst->basis_ntt[YUANYANG_MAT2_10]);
	qntt_to_montgomery(dst->u_hat_ntt.q1);
	for (unsigned j = 0; j < YUANYANG_MAT2_POLYS; j++) {
		qntt_to_montgomery(dst->basis_ntt[j].q1);
	}

	adj00 = YUANYANG_MAT2_POLY(dst->b_hat_inv_adj, YUANYANG_MAT2_00);
	adj01 = YUANYANG_MAT2_POLY(dst->b_hat_inv_adj, YUANYANG_MAT2_01);
	adj10 = YUANYANG_MAT2_POLY(dst->b_hat_inv_adj, YUANYANG_MAT2_10);
	adj11 = YUANYANG_MAT2_POLY(dst->b_hat_inv_adj, YUANYANG_MAT2_11);
	/*
	 * B_hat^{-1} is represented as its adjugate. The q denominator is
	 * applied when the caller converts the exact numerator back to fpr.
	 */
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		int64_t F_hat_q11;
		int64_t G_hat_q11;

		F_hat_q11 = (int64_t)expanded->compact.F[i]
			* (int64_t)YUANYANG_U_HAT_SCALE + uf_q11[i];
		G_hat_q11 = (int64_t)expanded->compact.G[i]
			* (int64_t)YUANYANG_U_HAT_SCALE + ug_q11[i];
		adj00[i] = G_hat_q11;
		adj01[i] = -F_hat_q11;
		adj10[i] = -(int64_t)expanded->compact.g[i];
		adj11[i] = expanded->compact.f[i];
	}

	crt_precompute_i64(&dst->a_hat_ntt[0], dst->a00_q22);
	crt_precompute_i64(&dst->a_hat_ntt[1], dst->a10_q22);
	crt_precompute_i64(&dst->a_hat_ntt[2], dst->a11_q22);
	crt_precompute_i64(&dst->b_hat_inv_top_ntt[0], adj00);
	crt_precompute_i64(&dst->b_hat_inv_top_ntt[1], adj01);
	q1_neg_precomp(&dst->b_hat_inv_bottom_ntt[0],
		&dst->basis_ntt[YUANYANG_MAT2_10]);
	dst->b_hat_inv_bottom_ntt[1] = dst->basis_ntt[YUANYANG_MAT2_00];
	for (unsigned j = 0; j < YUANYANG_MAT2_POLYS; j++) {
		crt_precompute_i64(&dst->delta_g_ntt[j],
			YUANYANG_MAT2_POLY(dst->delta_g_qD, j));
	}
	poly_inverse_correction_scaled_precomp(dst->delta_inv_corr0_qD,
		&dst->delta_g_ntt[YUANYANG_MAT2_00],
		YUANYANG_MAT2_POLY(dst->delta_g_qD, YUANYANG_MAT2_00),
		YUANYANG_SIGN_DELTA_NTT_SCALE_BITS);
	poly_inverse_correction_scaled_precomp(dst->delta_inv_corr1_qD,
		&dst->delta_g_ntt[YUANYANG_MAT2_11],
		YUANYANG_MAT2_POLY(dst->delta_g_qD, YUANYANG_MAT2_11),
		YUANYANG_SIGN_DELTA_NTT_SCALE_BITS);
	crt_precompute_i64(&dst->delta_inv_corr_ntt[0],
		dst->delta_inv_corr0_qD);
	crt_precompute_i64(&dst->delta_inv_corr_ntt[1],
		dst->delta_inv_corr1_qD);
#if YUANYANG_SIGN_NTT_DEBUG
	for (unsigned j = 0; j < YUANYANG_MAT2_POLYS; j++) {
		printf("delta_g[%u] max=%lld\n", j,
			(long long)poly_max_abs(YUANYANG_MAT2_POLY(
				dst->delta_g_qD, j)));
	}
	debug_report_inverse_corr_residual("delta den0 inverse-corr",
		YUANYANG_MAT2_POLY(dst->delta_g_qD, YUANYANG_MAT2_00),
		dst->delta_inv_corr0_qD,
		YUANYANG_SIGN_DELTA_NTT_SCALE_BITS);
	debug_report_inverse_corr_residual("delta den1 inverse-corr",
		YUANYANG_MAT2_POLY(dst->delta_g_qD, YUANYANG_MAT2_11),
		dst->delta_inv_corr1_qD,
		YUANYANG_SIGN_DELTA_NTT_SCALE_BITS);
#endif
}

void
yuanyang_sign_ntt_a_hat_mul(
	int64_t p0_q22[YUANYANG_D],
	int64_t p1_q22[YUANYANG_D],
	const yuanyang_sign_ntt_precomp *precomp,
	const int32_t x0[YUANYANG_D],
	const int32_t x1[YUANYANG_D])
{
#if YUANYANG_HAVE_AVX2_IMPL && !defined(YUANYANG_SIGN_NTT_NO_DISPATCH)
	if (YUANYANG_SIGN_NTT_USE_AVX2()) {
		yuanyang_sign_ntt_a_hat_mul_avx2(p0_q22, p1_q22,
			precomp, x0, x1);
		return;
	}
#endif
	/* A_hat*x is the only presampler product that currently requires CRT. */
	yuanyang_sign_ntt_crt_poly x0_ntt, x1_ntt;

	crt_precompute_i32(&x0_ntt, x0);
	crt_precompute_i32(&x1_ntt, x1);
	mul_crt_pretransformed(p0_q22, &precomp->a_hat_ntt[0], &x0_ntt);
	mul_crt_pretransformed_add2(p1_q22,
		&precomp->a_hat_ntt[1], &x0_ntt,
		&precomp->a_hat_ntt[2], &x1_ntt);
}

void
yuanyang_sign_ntt_b_hat_inv_mul_crt(
	int64_t hat0_num_q11_L[YUANYANG_D],
	int64_t hat1_num_q11_L[YUANYANG_D],
	const yuanyang_sign_ntt_precomp *precomp,
	const int32_t centered0_L[YUANYANG_D],
	const int32_t centered1_L[YUANYANG_D])
{
#if YUANYANG_HAVE_AVX2_IMPL && !defined(YUANYANG_SIGN_NTT_NO_DISPATCH)
	if (YUANYANG_SIGN_NTT_USE_AVX2()) {
		yuanyang_sign_ntt_b_hat_inv_mul_crt_avx2(
			hat0_num_q11_L, hat1_num_q11_L, precomp,
			centered0_L, centered1_L);
		return;
	}
#endif
	/*
	 * Compute adj(B_hat)*(c-p)*L. The top row can exceed Q1 and uses CRT;
	 * the bottom row fits a single QNTT product and is scaled by 2^11 here.
	 */
	int64_t bottom[YUANYANG_D];
	yuanyang_sign_ntt_crt_poly centered0_ntt, centered1_ntt;
	const int64_t u_scale = YUANYANG_U_HAT_SCALE;

	crt_precompute_i32(&centered0_ntt, centered0_L);
	crt_precompute_i32(&centered1_ntt, centered1_L);
	mul_crt_pretransformed_add2(hat0_num_q11_L,
		&precomp->b_hat_inv_top_ntt[0], &centered0_ntt,
		&precomp->b_hat_inv_top_ntt[1], &centered1_ntt);
	q1_muladd_montgomery_pretransformed_centered(bottom,
		precomp->b_hat_inv_bottom_ntt[0].q1, centered0_ntt.q1,
		precomp->b_hat_inv_bottom_ntt[1].q1, centered1_ntt.q1);
#pragma omp simd
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		hat1_num_q11_L[i] = bottom[i] * u_scale;
	}
}

void
yuanyang_sign_ntt_u_hat_mul(
	int64_t out_q11[YUANYANG_D],
	const yuanyang_sign_ntt_precomp *precomp,
	const int32_t z[YUANYANG_D])
{
#if YUANYANG_HAVE_AVX2_IMPL && !defined(YUANYANG_SIGN_NTT_NO_DISPATCH)
	if (YUANYANG_SIGN_NTT_USE_AVX2()) {
		yuanyang_sign_ntt_u_hat_mul_avx2(out_q11, precomp, z);
		return;
	}
#endif
	/* Compute u_hat*z at the u_hat fixed-point scale. */
	mul_q1_precomp_i32(out_q11, &precomp->u_hat_ntt, z);
}

void
yuanyang_sign_ntt_u_hat_mul_prepare(
	int64_t out_q11[YUANYANG_D],
	yuanyang_sign_ntt_q1_poly *z_ntt,
	const yuanyang_sign_ntt_precomp *precomp,
	const int32_t z[YUANYANG_D])
{
#if YUANYANG_HAVE_AVX2_IMPL && !defined(YUANYANG_SIGN_NTT_NO_DISPATCH)
	if (YUANYANG_SIGN_NTT_USE_AVX2()) {
		yuanyang_sign_ntt_u_hat_mul_prepare_avx2(out_q11, z_ntt,
			precomp, z);
		return;
	}
#endif
	/* Compute u_hat*z and retain z in Q1 NTT form for later row products. */
	q1_precompute_i32(z_ntt, z);
	q1_mul_montgomery_pretransformed_centered(out_q11,
		precomp->u_hat_ntt.q1, z_ntt->q1);
}

void
yuanyang_sign_ntt_basis_mul_with_z2_ntt(
	int32_t out0[YUANYANG_D],
	int32_t out1[YUANYANG_D],
	const yuanyang_sign_ntt_precomp *precomp,
	const int32_t z1[YUANYANG_D],
	const yuanyang_sign_ntt_q1_poly *z2_ntt)
{
#if YUANYANG_HAVE_AVX2_IMPL && !defined(YUANYANG_SIGN_NTT_NO_DISPATCH)
	if (YUANYANG_SIGN_NTT_USE_AVX2()) {
		yuanyang_sign_ntt_basis_mul_with_z2_ntt_avx2(out0, out1,
			precomp, z1, z2_ntt);
		return;
	}
#endif
	/* Compute B*(z1,z2) in the coefficient domain. */
	int64_t row0[YUANYANG_D], row1[YUANYANG_D];
	yuanyang_sign_ntt_q1_poly z1_ntt;

	q1_precompute_i32(&z1_ntt, z1);
	q1_muladd_montgomery_pretransformed_centered(row0,
		precomp->basis_ntt[YUANYANG_MAT2_00].q1, z1_ntt.q1,
		precomp->basis_ntt[YUANYANG_MAT2_01].q1, z2_ntt->q1);
	q1_muladd_montgomery_pretransformed_centered(row1,
		precomp->basis_ntt[YUANYANG_MAT2_10].q1, z1_ntt.q1,
		precomp->basis_ntt[YUANYANG_MAT2_11].q1, z2_ntt->q1);
#pragma omp simd
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		out0[i] = (int32_t)row0[i];
		out1[i] = (int32_t)row1[i];
	}
}

void
yuanyang_sign_ntt_basis_mul(
	int32_t out0[YUANYANG_D],
	int32_t out1[YUANYANG_D],
	const yuanyang_sign_ntt_precomp *precomp,
	const int32_t z1[YUANYANG_D],
	const int32_t z2[YUANYANG_D])
{
	yuanyang_sign_ntt_q1_poly z2_ntt;

#if YUANYANG_HAVE_AVX2_IMPL && !defined(YUANYANG_SIGN_NTT_NO_DISPATCH)
	if (YUANYANG_SIGN_NTT_USE_AVX2()) {
		yuanyang_sign_ntt_basis_mul_avx2(out0, out1, precomp, z1, z2);
		return;
	}
#endif
	q1_precompute_i32(&z2_ntt, z2);
	yuanyang_sign_ntt_basis_mul_with_z2_ntt(out0, out1, precomp,
		z1, &z2_ntt);
}

fpr
yuanyang_sign_ntt_delta2_weight(
	const yuanyang_sign_ntt_precomp *precomp,
	const int32_t err0[YUANYANG_D],
	const int32_t err1[YUANYANG_D])
{
#if YUANYANG_HAVE_AVX2_IMPL && !defined(YUANYANG_SIGN_NTT_NO_DISPATCH)
	if (YUANYANG_SIGN_NTT_USE_AVX2()) {
		return yuanyang_sign_ntt_delta2_weight_avx2(precomp, err0, err1);
	}
#endif
	/*
	 * Fixed-point Jacobi quadratic form for Delta2. G=Sigma_delta/sigma^2
	 * is stored at YUANYANG_SIGN_DELTA_NTT_SCALE_BITS, products are rounded
	 * back to that scale after each CRT multiplication, and the final dot
	 * product is converted back to the default fpr format.
	 */
	int64_t rhs0[YUANYANG_D], rhs1[YUANYANG_D];
	int64_t action0[YUANYANG_D], action1[YUANYANG_D];
	int64_t g01_action1[YUANYANG_D], g10_action0[YUANYANG_D];
	int64_t num0[YUANYANG_D], num1[YUANYANG_D];
	int64_t next0[YUANYANG_D], next1[YUANYANG_D];
	yuanyang_sign_ntt_crt_poly err0_ntt, err1_ntt;
	yy_u128 dot_pos, dot_neg, dot_mag;
	int dot_cmp;
	fpr weight;

	crt_precompute_i32(&err0_ntt, err0);
	crt_precompute_i32(&err1_ntt, err1);
	mul_crt_pretransformed_add2(rhs0,
		&precomp->delta_g_ntt[YUANYANG_MAT2_00], &err0_ntt,
		&precomp->delta_g_ntt[YUANYANG_MAT2_01], &err1_ntt);
	mul_crt_pretransformed_add2(rhs1,
		&precomp->delta_g_ntt[YUANYANG_MAT2_10], &err0_ntt,
		&precomp->delta_g_ntt[YUANYANG_MAT2_11], &err1_ntt);

	/*
	 * The Jacobi action starts from zero, so the first off-diagonal terms
	 * are known to be zero.  Start with one diagonal correction step and run
	 * the full recurrence only for the remaining iterations.
	 */
	if (YUANYANG_T_JACOBI_ITERATIONS == 0u) {
		memset(action0, 0, sizeof action0);
		memset(action1, 0, sizeof action1);
	} else {
		poly_apply_inverse_correction_scaled_precomp(
			action0, &precomp->delta_inv_corr_ntt[0], rhs0,
			YUANYANG_SIGN_DELTA_NTT_SCALE_BITS);
		poly_apply_inverse_correction_scaled_precomp(
			action1, &precomp->delta_inv_corr_ntt[1], rhs1,
			YUANYANG_SIGN_DELTA_NTT_SCALE_BITS);

		for (unsigned iter = 1; iter < YUANYANG_T_JACOBI_ITERATIONS;
			iter++)
		{
			poly_mul_crt_scaled_precomp(g01_action1,
				&precomp->delta_g_ntt[YUANYANG_MAT2_01],
				action1, YUANYANG_SIGN_DELTA_NTT_SCALE_BITS);
			poly_mul_crt_scaled_precomp(g10_action0,
				&precomp->delta_g_ntt[YUANYANG_MAT2_10],
				action0, YUANYANG_SIGN_DELTA_NTT_SCALE_BITS);
			poly_add(num0, rhs0, g01_action1);
			poly_add(num1, rhs1, g10_action0);
			poly_apply_inverse_correction_scaled_precomp(
				next0, &precomp->delta_inv_corr_ntt[0], num0,
				YUANYANG_SIGN_DELTA_NTT_SCALE_BITS);
			poly_apply_inverse_correction_scaled_precomp(
				next1, &precomp->delta_inv_corr_ntt[1], num1,
				YUANYANG_SIGN_DELTA_NTT_SCALE_BITS);
			memcpy(action0, next0, sizeof action0);
			memcpy(action1, next1, sizeof action1);
		}
	}

	dot_pos.hi = 0;
	dot_pos.lo = 0;
	dot_neg.hi = 0;
	dot_neg.lo = 0;
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		dot_add_i32_i64(&dot_pos, &dot_neg, err0[i], action0[i]);
		dot_add_i32_i64(&dot_pos, &dot_neg, err1[i], action1[i]);
	}
	dot_cmp = u128_cmp(dot_pos, dot_neg);
	if (dot_cmp <= 0) {
#if YUANYANG_SIGN_NTT_DEBUG
		uint64_t abs_weight_raw;

		dot_mag = dot_cmp == 0
			? (yy_u128){ 0, 0 }
			: u128_sub(dot_neg, dot_pos);
		abs_weight_raw = u128_pow2_scale_rne(dot_mag,
			YY_SIGN_NTT_WEIGHT_SCALE_BITS + YUANYANG_LOGD - 1u,
			YUANYANG_SIGN_DELTA_NTT_SCALE_BITS);
		printf("ntt_weight: nonpositive dot_abs=0x%016llx%016llx "
			"abs_raw=%llu\n",
			(unsigned long long)dot_mag.hi,
			(unsigned long long)dot_mag.lo,
			(unsigned long long)abs_weight_raw);
#endif
		return fpr_zero;
	}
	dot_mag = u128_sub(dot_pos, dot_neg);
	weight = fpr_from_scaled_i64((int64_t)u128_pow2_scale_rne(dot_mag,
		YY_SIGN_NTT_WEIGHT_SCALE_BITS + YUANYANG_LOGD - 1u,
		YUANYANG_SIGN_DELTA_NTT_SCALE_BITS),
		YY_SIGN_NTT_WEIGHT_SCALE_BITS);
#if YUANYANG_SIGN_NTT_DEBUG
	printf("ntt_weight: %.17g\n", weight.v);
#endif
	return weight;
}
