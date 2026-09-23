#ifndef LOONG_PARAMETERS_H
#define LOONG_PARAMETERS_H

#define LOONG_INSTANCE_NAME "BAG-Loong-512"

#define LOONG_Q 2
#define LOONG_N 104
#define LOONG_M 97
#define LOONG_N_PRIME 97
#define LOONG_N1 15
#define LOONG_N2 15
#define LOONG_EPSILON 73
#define LOONG_K 6

#define LOONG_SECURITY_BITS 512
#define LOONG_QUANTUM_SECURITY_BITS 256

#define LOONG_AXY_00 7
#define LOONG_AXY_01 4
#define LOONG_AXY_10 4
#define LOONG_AXY_11 7

#define LOONG_AR_00 7
#define LOONG_AR_01 4
#define LOONG_AR_10 4
#define LOONG_AR_11 7

#define LOONG_LAMBDA_BYTES 64
#define LOONG_PK_SEED_BYTES 128
#define LOONG_X_OR_S_BYTES 18915
#define LOONG_PK_BYTES 19043
#define LOONG_SK_PRIME_BYTES 19043
#define LOONG_XI_BYTES 64
#define LOONG_SK_BYTES 38150
#define LOONG_PKE_CT_BYTES 21644
#define LOONG_SALT_BYTES 16
#define LOONG_CT_BYTES 21660
#define LOONG_SS_BYTES 64

#define LOONG_ID_BYTES 32
#define LOONG_G_BYTES 64

#define LOONG_FIELD_POLY_DEGREE 97
#define LOONG_FIELD_POLY_NEEDS_ERRATUM 0

#define LOONG_PARAM_STATIC_ASSERT(cond, name) typedef char loong_param_static_assert_##name[(cond) ? 1 : -1]

LOONG_PARAM_STATIC_ASSERT(LOONG_Q == 2, q_matches_baseline);
LOONG_PARAM_STATIC_ASSERT(LOONG_N == 104, n_matches_baseline);
LOONG_PARAM_STATIC_ASSERT(LOONG_M == 97, m_matches_baseline);
LOONG_PARAM_STATIC_ASSERT(LOONG_N_PRIME == 97, n_prime_matches_baseline);
LOONG_PARAM_STATIC_ASSERT(LOONG_N1 == 15, n1_matches_baseline);
LOONG_PARAM_STATIC_ASSERT(LOONG_N2 == 15, n2_matches_baseline);
LOONG_PARAM_STATIC_ASSERT(LOONG_EPSILON == 73, epsilon_matches_baseline);
LOONG_PARAM_STATIC_ASSERT(LOONG_K == 6, k_matches_baseline);
LOONG_PARAM_STATIC_ASSERT(LOONG_PK_BYTES == 19043, pk_bytes_matches_baseline);
LOONG_PARAM_STATIC_ASSERT(LOONG_SK_BYTES == 38150, sk_bytes_matches_baseline);
LOONG_PARAM_STATIC_ASSERT(LOONG_PKE_CT_BYTES == 21644, pke_ct_bytes_matches_baseline);
LOONG_PARAM_STATIC_ASSERT(LOONG_CT_BYTES == 21660, api_ct_bytes_matches_baseline);
LOONG_PARAM_STATIC_ASSERT(LOONG_SS_BYTES == 64, ss_bytes_matches_baseline);
LOONG_PARAM_STATIC_ASSERT(LOONG_FIELD_POLY_DEGREE == 97, field_poly_degree_matches_m);
LOONG_PARAM_STATIC_ASSERT(LOONG_FIELD_POLY_NEEDS_ERRATUM == 0, field_poly_baseline_confirmed);

#define LOONG_CEIL_DIV(a, b) (((a) + (b) - 1) / (b))
#define LOONG_VEC_N_N1_BITS ((unsigned long long)LOONG_N * LOONG_N1 * LOONG_M)
#define LOONG_PKE_CT_BITS \
	(((unsigned long long)LOONG_N * LOONG_N2 + (unsigned long long)LOONG_N1 * LOONG_N2) * LOONG_M)
#define LOONG_VEC_N_N1_BYTES LOONG_CEIL_DIV(LOONG_VEC_N_N1_BITS, 8ULL)
#define LOONG_PKE_CT_DERIVED_BYTES LOONG_CEIL_DIV(LOONG_PKE_CT_BITS, 8ULL)

#endif
