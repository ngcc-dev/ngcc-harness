/**
 * @file parameters.h
 * @brief Parameters of the TriQ-256 scheme
 */

#ifndef TRIQ_OPT_PARAMETERS_H
#define TRIQ_OPT_PARAMETERS_H

#include "api.h"

#define CEIL_DIVIDE(a, b) (((a) / (b)) + ((a) % (b) == 0 ? 0 : 1))
#define BITMASK(a, size)  ((1ULL << (a % size)) - 1)

#define PARAM_N                     50363
#define PARAM_N1                    64
#define PARAM_N2                    768
#define PARAM_N1N2                  49152
#define PARAM_OMEGA                 132
#define PARAM_OMEGA_E               234
#define PARAM_OMEGA_R               132
#define PARAM_SECURITY              256
#define PARAM_SECURITY_BYTES        32
#define PARAM_DFR_EXP               256

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

#define PARAM_DELTA                 16
#define PARAM_M                     8
#define PARAM_GF_POLY               0x11D
#define PARAM_GF_MUL_ORDER          255
#define PARAM_K                     32
#define PARAM_G                     33
#define PARAM_FFT                   5

#define RS_POLY_COEFS \
    45, 216, 239, 24, 253, 104, 27, 40, 107, 50, 163, 210, 227, 134, 224, 158, \
    119, 13, 158, 1, 238, 164, 82, 43, 15, 232, 246, 142, 50, 189, 29, 232, 1

#define SEED_BYTES                  32
#define SALT_BYTES                  32

#define PARAM_N_MU 85284ULL
#define UTILS_REJECTION_THRESHOLD             16770879

#define PARAM_OMEGA_MAX                    234

/* Bounded-density rejection parameters */
#define PARAM_BD_L                   768
#define PARAM_BD_GAMMA               9
#define PARAM_BD_N_MAX               256


#define PARAM_N_MULT                50688
#define VEC_N_256_SIZE_64           (CEIL_DIVIDE(PARAM_N_MULT, 256) << 2)
#define VEC_N1N2_256_SIZE_64        (CEIL_DIVIDE(PARAM_N1N2, 256) << 2)
#endif // TRIQ_OPT_PARAMETERS_H

