/**
 * @file parameter_contract.c
 * @brief C99 compile-time contract for HARE instance parameters, backends,
 *        implementation profiles, and API sizes.
 *
 * Every expression depends only on public compile-time configuration.  A
 * violated contract makes the corresponding instance fail to compile rather
 * than allowing inconsistent buffer sizes or an unsupported implementation
 * tuple to reach runtime.
 */
#include "parameters.h"
#include "api.h"

#define HARE_CONTRACT_JOIN_(a, b) a##b
#define HARE_CONTRACT_JOIN(a, b) HARE_CONTRACT_JOIN_(a, b)
#define HARE_CONTRACT(expr) \
    typedef char HARE_CONTRACT_JOIN(hare_parameter_contract_, __LINE__)[(expr) ? 1 : -1]

#if defined(HARE_BACKEND_HAMMING)
#error "this package contains KR instances only"
#endif
#ifndef HARE_BACKEND_KR
#error "HARE_BACKEND_KR must be selected"
#endif

#if defined(HARE_X86_OPTIMIZED) && !defined(HARE_X86_SELECTED_GF2X)
#error "x86 optimized targets must select the packaged AVX2/PCLMUL GF2X implementation"
#endif

#if defined(HARE_ARM_SVE_OPTIMIZED)
#if (defined(HARE_ARM_PMULL_GF2X) + defined(HARE_ARM_GF2X_GENERIC)) != 1
#error "exactly one ARM GF2X base-multiply profile must be selected"
#endif
#endif

HARE_CONTRACT(PARAM_N > 0);
HARE_CONTRACT((PARAM_N & 1) == 1);
HARE_CONTRACT(PARAM_K > 0);
HARE_CONTRACT(PARAM_N1 >= PARAM_K);
HARE_CONTRACT(PARAM_N1 <= 255);
HARE_CONTRACT(PARAM_N2 > 0);
HARE_CONTRACT((PARAM_N2 % 128) == 0);
HARE_CONTRACT(PARAM_N1N2 == PARAM_N1 * PARAM_N2);
HARE_CONTRACT(PARAM_L1 == PARAM_N1N2);
HARE_CONTRACT(PARAM_L1 <= PARAM_N);
HARE_CONTRACT(PARAM_COMP_Q > 0);
HARE_CONTRACT(PARAM_COMP_K < PARAM_COMP_N);
HARE_CONTRACT(PARAM_COMP_Q == PARAM_L1 / PARAM_COMP_N);
HARE_CONTRACT(PARAM_L2 == PARAM_COMP_Q * PARAM_COMP_N);
HARE_CONTRACT(PARAM_L2 <= PARAM_L1);
HARE_CONTRACT(PARAM_NM == PARAM_COMP_K * PARAM_COMP_Q);
HARE_CONTRACT(PARAM_NV2 == PARAM_L1 - PARAM_L2);
HARE_CONTRACT(VEC_COMPRESSED_PAYLOAD_BITS == PARAM_NM + PARAM_NV2);
HARE_CONTRACT(VEC_COMPRESSED_PAYLOAD_BYTES == CEIL_DIVIDE(VEC_COMPRESSED_PAYLOAD_BITS, 8));
HARE_CONTRACT(VEC_N_SIZE_BYTES == CEIL_DIVIDE(PARAM_N, 8));
HARE_CONTRACT(VEC_N_SIZE_64 == CEIL_DIVIDE(PARAM_N, 64));
HARE_CONTRACT(VEC_N1N2_SIZE_64 == CEIL_DIVIDE(PARAM_N1N2, 64));

HARE_CONTRACT(PARAM_K == PARAM_SECURITY_BYTES);
HARE_CONTRACT(SEED_BYTES == PARAM_SECURITY_BYTES);
HARE_CONTRACT(SALT_BYTES == PARAM_SECURITY_BYTES);
HARE_CONTRACT(SHARED_SECRET_BYTES == PARAM_SECURITY_BYTES);
HARE_CONTRACT(PUBLIC_KEY_BYTES == SEED_BYTES + VEC_N_SIZE_BYTES);
HARE_CONTRACT(SECRET_KEY_BYTES == PUBLIC_KEY_BYTES + SEED_BYTES + PARAM_SECURITY_BYTES + SEED_BYTES);
HARE_CONTRACT(CIPHERTEXT_BYTES == VEC_N_SIZE_BYTES + VEC_COMPRESSED_PAYLOAD_BYTES + SALT_BYTES);

HARE_CONTRACT(CRYPTO_PUBLICKEYBYTES == PUBLIC_KEY_BYTES);
HARE_CONTRACT(CRYPTO_SECRETKEYBYTES == SECRET_KEY_BYTES);
HARE_CONTRACT(CRYPTO_CIPHERTEXTBYTES == CIPHERTEXT_BYTES);
HARE_CONTRACT(CRYPTO_BYTES == SHARED_SECRET_BYTES);

HARE_CONTRACT(PARAM_M == 8);

/*
 * Exact approved public parameter tuples.  This deliberately rejects a
 * cross-product assembled from individually valid n, n1, n2, weight, or alpha
 * values.  It protects all Reference, x86, and ARM instance directories from a
 * partial parameter edit silently producing a new, unevaluated distribution.
 */
#if defined(HARE_BACKEND_KR)
HARE_CONTRACT(PARAM_COMP_N == 51);
HARE_CONTRACT(PARAM_COMP_K == 41);
HARE_CONTRACT(PARAM_COMP_R == 2);
HARE_CONTRACT(
    (PARAM_SECURITY_BYTES == 16 && PARAM_N == 20899 && PARAM_K == 16 &&
     PARAM_OMEGA == 79 && PARAM_OMEGA_R == 79 && PARAM_OMEGA_E == 145 &&
     PARAM_N1 == 32 && PARAM_N2 == 640 && PARAM_ALPHA == 11) ||
    (PARAM_SECURITY_BYTES == 32 && PARAM_N == 52379 && PARAM_K == 32 &&
     PARAM_OMEGA == 131 && PARAM_OMEGA_R == 131 && PARAM_OMEGA_E == 224 &&
     PARAM_N1 == 81 && PARAM_N2 == 640 && PARAM_ALPHA == 9) ||
    (PARAM_SECURITY_BYTES == 48 && PARAM_N == 104869 && PARAM_K == 48 &&
     PARAM_OMEGA == 193 && PARAM_OMEGA_R == 193 && PARAM_OMEGA_E == 361 &&
     PARAM_N1 == 91 && PARAM_N2 == 1152 && PARAM_ALPHA == 15) ||
    (PARAM_SECURITY_BYTES == 64 && PARAM_N == 173981 && PARAM_K == 64 &&
     PARAM_OMEGA == 259 && PARAM_OMEGA_R == 259 && PARAM_OMEGA_E == 449 &&
     PARAM_N1 == 151 && PARAM_N2 == 1152 && PARAM_ALPHA == 13));
#endif

enum { hare_parameter_contract_anchor = 0 };
