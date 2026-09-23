/**
 * @file parameters.h
 * @brief Parameters of the TriQ-512 scheme
 */

#ifndef TRIQ_PARAMETERS_H
#define TRIQ_PARAMETERS_H

#include "api.h"

#define CEIL_DIVIDE(a, b) (((a) / (b)) + ((a) % (b) == 0 ? 0 : 1))
#define BITMASK(a, size)  ((1ULL << (a % size)) - 1)

#define PARAM_N                     157627
#define PARAM_N1                    136
#define PARAM_N2                    1152
#define PARAM_N1N2                  156672
#define PARAM_OMEGA                 260
#define PARAM_OMEGA_E               475
#define PARAM_OMEGA_R               260
#define PARAM_SECURITY              512
#define PARAM_SECURITY_BYTES        64
#define PARAM_DFR_EXP               512

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

#define PARAM_DELTA                 36
#define PARAM_M                     8
#define PARAM_GF_POLY               0x11D
#define PARAM_GF_MUL_ORDER          255
#define PARAM_K                     64
#define PARAM_G                     73
#define PARAM_FFT                   7

#define RS_POLY_COEFS \
    120, 227, 198, 174, 170, 155, 255, 12, 75, 190, 37, 137, 101, 193, 134, 124, \
    216, 8, 6, 254, 132, 86, 179, 98, 30, 3, 79, 197, 179, 209, 246, 52, \
    220, 195, 41, 64, 171, 165, 2, 173, 85, 216, 233, 230, 175, 245, 179, 176, \
    172, 71, 229, 168, 131, 236, 176, 154, 97, 129, 201, 170, 26, 78, 171, 218, \
    255, 167, 3, 54, 103, 30, 247, 179, 1

#define SEED_BYTES                  64
#define SALT_BYTES                  32

#define PARAM_N_MU 27246ULL
#define UTILS_REJECTION_THRESHOLD             16708462

#define PARAM_OMEGA_MAX                    475

/* Bounded-density rejection parameters */
#define PARAM_BD_L                   1152
#define PARAM_BD_GAMMA               9
#define PARAM_BD_N_MAX               256

#endif // TRIQ_PARAMETERS_H
