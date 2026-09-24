/* AVX2-specialized, namespaced copy of the local signing NTT layer. */
#define YUANYANG_SIGN_NTT_NO_DISPATCH 1
#define yuanyang_sign_ntt_precompute yuanyang_sign_ntt_precompute_avx2
#define yuanyang_sign_ntt_mul_single yuanyang_sign_ntt_mul_single_avx2
#define yuanyang_sign_ntt_mul_crt yuanyang_sign_ntt_mul_crt_avx2
#define yuanyang_sign_ntt_a_hat_mul yuanyang_sign_ntt_a_hat_mul_avx2
#define yuanyang_sign_ntt_b_hat_inv_mul_crt \
	yuanyang_sign_ntt_b_hat_inv_mul_crt_avx2
#define yuanyang_sign_ntt_u_hat_mul yuanyang_sign_ntt_u_hat_mul_avx2
#define yuanyang_sign_ntt_u_hat_mul_prepare \
	yuanyang_sign_ntt_u_hat_mul_prepare_avx2
#define yuanyang_sign_ntt_basis_mul yuanyang_sign_ntt_basis_mul_avx2
#define yuanyang_sign_ntt_basis_mul_with_z2_ntt \
	yuanyang_sign_ntt_basis_mul_with_z2_ntt_avx2
#define yuanyang_sign_ntt_delta2_weight \
	yuanyang_sign_ntt_delta2_weight_avx2

/* Keep all Q1 work inside the same AVX2 path without affecting verification. */
#define qntt_forward yuanyang_qntt_forward_avx2
#define qntt_forward_partial yuanyang_qntt_forward_partial_avx2
#define qntt_inverse_residue yuanyang_qntt_inverse_residue_avx2
#define qntt_encode_i64 yuanyang_qntt_encode_i64_avx2
#define qntt_encode_i32 yuanyang_qntt_encode_i32_avx2
#define qntt_center_i64 yuanyang_qntt_center_i64_avx2
#define qntt_to_montgomery yuanyang_qntt_to_montgomery_avx2
#define qntt_pointwise_mul_to yuanyang_qntt_pointwise_mul_to_avx2
#define qntt_pointwise_mul_to_partial \
	yuanyang_qntt_pointwise_mul_to_partial_avx2
#define qntt_pointwise_muladd_to yuanyang_qntt_pointwise_muladd_to_avx2
#define qntt_pointwise_montgomery_mul_to \
	yuanyang_qntt_pointwise_montgomery_mul_to_avx2
#define qntt_pointwise_montgomery_muladd_to \
	yuanyang_qntt_pointwise_montgomery_muladd_to_avx2

#include "sign_ntt.c"
