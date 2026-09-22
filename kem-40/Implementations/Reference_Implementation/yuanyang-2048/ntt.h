#ifndef YY_NTT_H
#define YY_NTT_H

#include "params.h"


/* 
 * In-place direct negacyclic NTT modulo QNTT. Input must be QNTT residues. 
 */
void qntt_forward(uint32_t a[YUANYANG_D]);
void qntt_forward_partial(uint32_t *a,int logn);

/*
 * In-place inverse direct negacyclic NTT modulo QNTT.
 * If modq is non-zero, reduce centered QNTT representatives modulo Yuanyang q;
 * otherwise return centered representatives modulo QNTT in the uint32_t slots.
 */
void qntt_inverse(uint32_t a[YUANYANG_D], int modq);

/* In-place inverse NTT that leaves coefficients as QNTT residues. */
void qntt_inverse_residue(uint32_t a[YUANYANG_D]);

/* Convert coefficient-domain signed inputs to QNTT residues. */
void qntt_encode_i64(
	uint32_t out[YUANYANG_D],
	const int64_t a[YUANYANG_D]);

void qntt_encode_i32(
	uint32_t out[YUANYANG_D],
	const int32_t a[YUANYANG_D]);

/* Convert QNTT residues to centered signed representatives. */
void qntt_center_i64(
	int64_t out[YUANYANG_D],
	const uint32_t a[YUANYANG_D]);

/* Convert normal QNTT residues in place to Montgomery representation. */
void qntt_to_montgomery(uint32_t a[YUANYANG_D]);

/* Pointwise product of two polynomials already in NTT form. */
void qntt_pointwise_mul_to(
	uint32_t out[YUANYANG_D],
	const uint32_t a[YUANYANG_D],
	const uint32_t b[YUANYANG_D]);

/* Pointwise sum of two products, with all operands already in NTT form. */
void qntt_pointwise_muladd_to(
	uint32_t out[YUANYANG_D],
	const uint32_t a0[YUANYANG_D],
	const uint32_t b0[YUANYANG_D],
	const uint32_t a1[YUANYANG_D],
	const uint32_t b1[YUANYANG_D]);

/*
 * Pointwise products with key-dependent operands already stored in Montgomery
 * representation.  The other operands and outputs use normal residues.
 */
void qntt_pointwise_montgomery_mul_to(
	uint32_t out[YUANYANG_D],
	const uint32_t a_montgomery[YUANYANG_D],
	const uint32_t b[YUANYANG_D]);

void qntt_pointwise_montgomery_muladd_to(
	uint32_t out[YUANYANG_D],
	const uint32_t a0_montgomery[YUANYANG_D],
	const uint32_t b0[YUANYANG_D],
	const uint32_t a1_montgomery[YUANYANG_D],
	const uint32_t b1[YUANYANG_D]);

/* In-place pointwise product of two polynomials already in NTT form. */
void qntt_pointwise_mul(
	uint32_t a[YUANYANG_D],
	const uint32_t b[YUANYANG_D]);

void qntt_pointwise_mul_partial(
	uint32_t *a,
	const uint32_t *b,int logn);

/* Compatibility wrappers used by the existing KEM code. */
void qntt_mul(uint32_t a[YUANYANG_D], uint32_t b[YUANYANG_D]);

void qntt_mul_int(uint32_t a[YUANYANG_D], uint32_t b[YUANYANG_D]);
void qntt_mul_int_partial(uint32_t *a, uint32_t *b,int logn);

void yuanyang_mul_mod_xn_plus_1_ntt_big(
	uint16_t out[YUANYANG_D],
	const uint16_t h[YUANYANG_D],
	const int16_t s1[YUANYANG_D]);

#endif
