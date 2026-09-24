/*
 * AVX2-specialized copy of the local NTT translation unit. Public symbols are
 * namespaced so the generic and optimized implementations can coexist.
 */
#define qntt_forward_partial yuanyang_qntt_forward_partial_avx2
#define qntt_forward yuanyang_qntt_forward_avx2
#define qntt_inverse_partial yuanyang_qntt_inverse_partial_avx2
#define qntt_inverse yuanyang_qntt_inverse_avx2
#define qntt_inverse_residue yuanyang_qntt_inverse_residue_avx2
#define qntt_encode_i64 yuanyang_qntt_encode_i64_avx2
#define qntt_encode_i32 yuanyang_qntt_encode_i32_avx2
#define qntt_center_i64 yuanyang_qntt_center_i64_avx2
#define qntt_to_montgomery yuanyang_qntt_to_montgomery_avx2
#define qntt_pointwise_mul_to_partial \
	yuanyang_qntt_pointwise_mul_to_partial_avx2
#define qntt_pointwise_mul_to yuanyang_qntt_pointwise_mul_to_avx2
#define qntt_pointwise_muladd_to yuanyang_qntt_pointwise_muladd_to_avx2
#define qntt_pointwise_montgomery_mul_to \
	yuanyang_qntt_pointwise_montgomery_mul_to_avx2
#define qntt_pointwise_montgomery_muladd_to \
	yuanyang_qntt_pointwise_montgomery_muladd_to_avx2
#define qntt_pointwise_mul yuanyang_qntt_pointwise_mul_avx2
#define qntt_pointwise_mul_partial yuanyang_qntt_pointwise_mul_partial_avx2
#define qntt_mul yuanyang_qntt_mul_avx2
#define qntt_mul_int_partial yuanyang_qntt_mul_int_partial_avx2
#define qntt_mul_int yuanyang_qntt_mul_int_avx2
#define yuanyang_mul_mod_xn_plus_1_ntt_big \
	yuanyang_mul_mod_xn_plus_1_ntt_big_avx2

#include "ntt.c"
