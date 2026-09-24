/**
 * @file parameters.h
 * @brief Parameters of the TriQ-128 scheme
 */

 #ifndef TRIQ_OPT_PARAMETERS_H
 #define TRIQ_OPT_PARAMETERS_H

 #include "api.h"

 #define CEIL_DIVIDE(a, b) (((a) / (b)) + ((a) % (b) == 0 ? 0 : 1))
 #define BITMASK(a, size)  ((1ULL << (a % size)) - 1)

 #define PARAM_N                     16301
 #define PARAM_N1                    42
 #define PARAM_N2                    384
 #define PARAM_N1N2                  16128
 #define PARAM_OMEGA                 67
 #define PARAM_OMEGA_E               106
 #define PARAM_OMEGA_R               67
 #define PARAM_SECURITY              128
 #define PARAM_SECURITY_BYTES        16
 #define PARAM_DFR_EXP               128

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

 #define PARAM_DELTA                 13
 #define PARAM_M                     8
 #define PARAM_GF_POLY               0x11D
 #define PARAM_GF_MUL_ORDER          255
 #define PARAM_K                     16
 #define PARAM_G                     27
 #define PARAM_FFT                   4

 #define RS_POLY_COEFS \
     217, 125, 229, 227, 54, 47, 236, 157, 230, 128, 74, 227, 194, 171, 106, 236, 178, 57, \
     3, 51, 165, 208, 64, 209, 204, 241, 1

 #define SEED_BYTES                  16
 #define SALT_BYTES                  32

 #define PARAM_N_MU 263478ULL
 #define UTILS_REJECTION_THRESHOLD             16773729

 #define PARAM_OMEGA_MAX                    106

 /* Bounded-density rejection parameters */
 #define PARAM_BD_L                   384
 #define PARAM_BD_GAMMA               7
 #define PARAM_BD_N_MAX               256

 
#define PARAM_N_MULT                16512
#define VEC_N_256_SIZE_64           (CEIL_DIVIDE(PARAM_N_MULT, 256) << 2)
#define VEC_N1N2_256_SIZE_64        (CEIL_DIVIDE(PARAM_N1N2, 256) << 2)
#endif // TRIQ_OPT_PARAMETERS_H

