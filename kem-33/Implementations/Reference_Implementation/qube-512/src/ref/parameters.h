/**
 * @file parameters.h
 * @brief QUBE-512 parameters from Algorithm specifications.pdf.
 */

#ifndef QUBE_PARAMETERS_H
#define QUBE_PARAMETERS_H

#include <stdint.h>

#define CEIL_DIVIDE(a, b) (((a) / (b)) + (((a) % (b)) == 0 ? 0 : 1))
#define QUBE_TAIL_MASK(bits) ((((bits) % 64) == 0) ? UINT64_MAX : ((UINT64_C(1) << ((bits) % 64)) - 1U))

#define PARAM_N              135851
#define PARAM_N1             150
#define PARAM_N2             896
#define PARAM_N1N2           134400
#define PARAM_OMEGA_X1       291
#define PARAM_OMEGA_X2       291
#define PARAM_OMEGA_Y1       301
#define PARAM_OMEGA_R11      103
#define PARAM_OMEGA_R21      113
#define PARAM_OMEGA_R22      113
#define PARAM_OMEGA_E        324
#define PARAM_SECURITY       512
#define PARAM_SECURITY_BYTES 64
#define PARAM_DFR_EXP        512
#define PARAM_OMEGA_MAX      324

#define PARAM_M              8
#define PARAM_GF_POLY        0x11D
#define PARAM_GF_MUL_ORDER   255
#define PARAM_K              PARAM_SECURITY_BYTES
#define PARAM_DELTA          ((PARAM_N1 - PARAM_K) / 2)
#define PARAM_G              (2 * PARAM_DELTA + 1)
#define PARAM_FFT            6

#define SEED_BYTES           PARAM_SECURITY_BYTES
#define SALT_BYTES           PARAM_SECURITY_BYTES

#define VEC_N_SIZE_BYTES     CEIL_DIVIDE(PARAM_N, 8)
#define VEC_N_SIZE_64        CEIL_DIVIDE(PARAM_N, 64)
#define VEC_N1_SIZE_BYTES    PARAM_N1
#define VEC_N1_SIZE_64       CEIL_DIVIDE(PARAM_N1, 8)
#define VEC_N1N2_SIZE_BYTES  CEIL_DIVIDE(PARAM_N1N2, 8)
#define VEC_N1N2_SIZE_64     CEIL_DIVIDE(PARAM_N1N2, 64)
#define VEC_K_SIZE_BYTES     PARAM_K

#define PUBLIC_KEY_BYTES     (SEED_BYTES + 2 * VEC_N_SIZE_BYTES)
#define SECRET_KEY_BYTES     (SEED_BYTES + PUBLIC_KEY_BYTES + SEED_BYTES)
#define CIPHERTEXT_BYTES     (VEC_N_SIZE_BYTES + VEC_N1N2_SIZE_BYTES + SALT_BYTES)
#define SHARED_SECRET_BYTES  PARAM_SECURITY_BYTES

#define EXPECTED_PUBLIC_KEY_BYTES    34028
#define EXPECTED_SECRET_KEY_BYTES    34156
#define EXPECTED_CIPHERTEXT_BYTES    33846
#define EXPECTED_SHARED_SECRET_BYTES 64

typedef char qube_pk_len_check[(PUBLIC_KEY_BYTES == EXPECTED_PUBLIC_KEY_BYTES) ? 1 : -1];
typedef char qube_sk_len_check[(SECRET_KEY_BYTES == EXPECTED_SECRET_KEY_BYTES) ? 1 : -1];
typedef char qube_ct_len_check[(CIPHERTEXT_BYTES == EXPECTED_CIPHERTEXT_BYTES) ? 1 : -1];
typedef char qube_ss_len_check[(SHARED_SECRET_BYTES == EXPECTED_SHARED_SECRET_BYTES) ? 1 : -1];
typedef char qube_code_len_check[(PARAM_N1 * PARAM_N2 == PARAM_N1N2) ? 1 : -1];

#endif  // QUBE_PARAMETERS_H
