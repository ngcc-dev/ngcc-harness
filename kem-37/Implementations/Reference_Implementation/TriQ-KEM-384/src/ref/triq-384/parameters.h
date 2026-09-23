/**
 * @file parameters.h
 * @brief Parameters of the TriQ-384 scheme
 */

#ifndef TRIQ_PARAMETERS_H
#define TRIQ_PARAMETERS_H

#include "api.h"

#define CEIL_DIVIDE(a, b) (((a) / (b)) + ((a) % (b) == 0 ? 0 : 1))
#define BITMASK(a, size)  ((1ULL << (a % size)) - 1)

#define PARAM_N                     97651
#define PARAM_N1                    108
#define PARAM_N2                    896
#define PARAM_N1N2                  96768
#define PARAM_OMEGA                 196
#define PARAM_OMEGA_E               350
#define PARAM_OMEGA_R               196
#define PARAM_SECURITY              384
#define PARAM_SECURITY_BYTES        48
#define PARAM_DFR_EXP               384

#define SECRET_KEY_BYTES            CRYPTO_SECRETKEYBYTES
#define PUBLIC_KEY_BYTES            CRYPTO_PUBLICKEYBYTES
#define SHARED_SECRET_BYTES         CRYPTO_BYTES
#define CIPHERTEXT_BYTES            CRYPTO_CIPHERTEXTBYTES

#define VEC_N_SIZE_BYTES            CEIL_DIVIDE(PARAM_N, 8)
#define VEC_K_SIZE_BYTES            PARAM_K
#define VEC_N1_SIZE_BYTES           PARAM_N1
#define VEC_N1N2_SIZE_BYTES         CEIL_DIVIDE(PARAM_N1N2, 8)

#define VEC_N_SIZE_64               CEIL_DIVIDE(PARAM_N, 64)
#define VEC_N1_SIZE_64              CEIL_DIVIDE(PARAM_N1, 8)
#define VEC_N1N2_SIZE_64            CEIL_DIVIDE(PARAM_N1N2, 64)

#define PARAM_DELTA                 30
#define PARAM_M                     8
#define PARAM_GF_POLY               0x11D
#define PARAM_GF_MUL_ORDER          255
#define PARAM_K                     48
#define PARAM_G                     61
#define PARAM_FFT                   6

#define RS_POLY_COEFS \
    193, 91, 190, 154, 101, 58, 231, 197, 152, 88, 73, 62, 169, 88, 188, 23, \
    36, 202, 63, 20, 102, 230, 131, 141, 214, 45, 101, 94, 62, 65, 66, 46, \
    131, 42, 187, 9, 122, 3, 19, 118, 6, 154, 14, 193, 79, 251, 124, 18, \
    186, 244, 166, 235, 167, 108, 41, 19, 76, 48, 42, 208, 1

#define SEED_BYTES                  48
#define SALT_BYTES                  32

#define PARAM_N_MU 43984ULL
#define UTILS_REJECTION_THRESHOLD             16698321

#define PARAM_OMEGA_MAX                    350

/* Bounded-density rejection parameters */
#define PARAM_BD_L                   896
#define PARAM_BD_GAMMA               9
#define PARAM_BD_N_MAX               256

#endif // TRIQ_PARAMETERS_H
