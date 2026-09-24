/**
* @file parameters.h
* @brief Parameters of the QUBE-KEM IND-CCA2 scheme
 */

#ifndef QUBE_PARAMETERS_H
#define QUBE_PARAMETERS_H

#include "api.h"

#define CEIL_DIVIDE(a, b) (((a) / (b)) + ((a) % (b) == 0 ? 0 : 1)) /*!< Divide a by b and ceil the result*/
#define BITMASK(a, size)  ((1UL << (a % size)) - 1)                /*!< Create a mask*/

#define PARAM_N                     82757       ///< Define the parameter n of the scheme
#define PARAM_N1                    96          ///< Define the parameter n1 of the scheme (length of Reed–Solomon code)
#define PARAM_N2                    896         ///< Define the parameter n2 of the scheme (length of Duplicated Reed–Muller code)
#define PARAM_N1N2                  86016       ///< Define the length in bits of the concatenated code
#define PARAM_OMEGA_X1              219          ///< Define the parameter omega of the scheme
#define PARAM_OMEGA_X2              219          ///< Define the parameter omega of the scheme
#define PARAM_OMEGA_Y1              226          ///< Define the parameter omega of the scheme
#define PARAM_OMEGA_R11             78          ///< Define the parameter omega of the scheme
#define PARAM_OMEGA_R21             85          ///< Define the parameter omega of the scheme
#define PARAM_OMEGA_R22             85          ///< Define the parameter omega of the scheme
#define PARAM_OMEGA_E               244          ///< Define the parameter omega_e of the scheme
#define PARAM_SECURITY              384         ///< Define the security level corresponding to the chosen parameters
#define PARAM_SECURITY_BYTES        48          ///< Define the security level in bytes
#define PARAM_DFR_EXP               384         ///< Define the decryption failure rate exponent

#define PARAM_OMEGA_MAX_1             244          ///< Define the parameter omega of the scheme
#define PARAM_OMEGA_MAX_2             85          ///< Define the parameter omega of the scheme

#define SECRET_KEY_BYTES            CRYPTO_SECRETKEYBYTES   ///< Define the size of the secret key in bytes
#define PUBLIC_KEY_BYTES            CRYPTO_PUBLICKEYBYTES   ///< Define the size of the public key in bytes
#define SHARED_SECRET_BYTES         CRYPTO_BYTES            ///< Define the size of the shared secret in bytes
#define CIPHERTEXT_BYTES            CRYPTO_CIPHERTEXTBYTES  ///< Define the size of the ciphertext in bytes

#define VEC_N_SIZE_BYTES            CEIL_DIVIDE(PARAM_N, 8)     ///< Size of array to store PARAM_N bits in bytes
#define VEC_K_SIZE_BYTES            PARAM_K                     ///< Size of array to store PARAM_K bits in bytes
#define VEC_N1_SIZE_BYTES           PARAM_N1                    ///< Size of array to store PARAM_N1 bits in bytes
#define VEC_N1N2_SIZE_BYTES         CEIL_DIVIDE(PARAM_N1N2, 8)  ///< Size of array to store PARAM_N1N2 bits in bytes

#define VEC_N_SIZE_64               CEIL_DIVIDE(PARAM_N, 64)    ///< Size of array to store PARAM_N bits in 64-bit words
#define VEC_N1_SIZE_64              CEIL_DIVIDE(PARAM_N1, 8)    ///< Size of array to store PARAM_N1 bits in 64-bit words
#define VEC_N1N2_SIZE_64            CEIL_DIVIDE(PARAM_N1N2, 64) ///< Size of array to store PARAM_N1N2 bits in 64-bit words

#define PARAM_N_MULT                98304      
///< Define the size (in 64-bit words) for PARAM_N as 256-bit elements
#define VEC_N_256_SIZE_64           (CEIL_DIVIDE(PARAM_N_MULT, 256) << 2)   ///< 256-bit block count for PARAM_N
#define VEC_N1N2_256_SIZE_64        (CEIL_DIVIDE(PARAM_N1N2, 256) << 2)     ///< 256-bit block count for PARAM_N1N2
#define VEC_N_256_NUM_WORDS (VEC_N_256_SIZE_64 >> 2)

#define PARAM_DELTA                 ((PARAM_N1-PARAM_SECURITY_BYTES)/2)          ///< Define the error-correcting capacity (delta) of the Reed–Solomon code
#define PARAM_M                     8           ///< Define the degree m of the Galois field GF(2^m)
#define PARAM_GF_POLY               0x11D       ///< Generator polynomial of GF(2^PARAM_M) in hexadecimal form
#define PARAM_GF_MUL_ORDER          255         ///< Size of the multiplicative group of GF(2^PARAM_M) (2^PARAM_M−1)
#define PARAM_K                     PARAM_SECURITY_BYTES          ///< Define the size of the information bits of the Reed–Solomon code
#define PARAM_G                     (2*PARAM_DELTA+1)          ///< Define the size of the generator polynomial of the Reed–Solomon code
#define PARAM_FFT                   4           ///< Exponent for additive FFT (2^PARAM_FFT points)


#define RS_POLY_COEFS                                                                                                \
    228, 231, 214, 81, 113, 204, 19, 169, 10, 244, 117, 219, 130, 12, 160, 151, 195, 170, 150, 151, 251, 218, 245, 166, 149, 183, 109, 176, 148, 218, 21, 161, 240, 25, 15, 71, 62, 5, 17, 32, 157, 194, 73, 195, 218, 14, 12, 122, 1                                                   ///< Coefficients of the Reed–Solomon generator polynomial
#define SEED_BYTES                  32          ///< Define the size of the seed in bytes
#define SALT_BYTES                  16          ///< Define the size of a salt in bytes

#define PARAM_N_MU 51898ULL   ///<  Define a precomputed multiplier for Barrett reduction mu = floor(2^32 / PARAM_N)
#define UTILS_REJECTION_THRESHOLD             16716914  ///< Rejection threshold for uniform sampling in [0, PARAM_N)

#endif // QUBE_PARAMETERS_H
