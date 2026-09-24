/**
* @file parameters.h
* @brief Parameters of the QUBE-KEM IND-CCA2 scheme
 */

#ifndef QUBE_PARAMETERS_H
#define QUBE_PARAMETERS_H

#include "api.h"

#define CEIL_DIVIDE(a, b) (((a) / (b)) + ((a) % (b) == 0 ? 0 : 1)) /*!< Divide a by b and ceil the result*/
#define BITMASK(a, size)  ((1UL << (a % size)) - 1)                /*!< Create a mask*/

#define PARAM_N                     135851       ///< Define the parameter n of the scheme
#define PARAM_N1                    142          ///< Define the parameter n1 of the scheme (length of Reed–Solomon code)
#define PARAM_N2                    1024         ///< Define the parameter n2 of the scheme (length of Duplicated Reed–Muller code)
#define PARAM_N1N2                  145408       ///< Define the length in bits of the concatenated code
#define PARAM_OMEGA_X1              291          ///< Define the parameter omega of the scheme
#define PARAM_OMEGA_X2              291          ///< Define the parameter omega of the scheme
#define PARAM_OMEGA_Y1              301          ///< Define the parameter omega of the scheme
#define PARAM_OMEGA_R11             103          ///< Define the parameter omega of the scheme
#define PARAM_OMEGA_R21             113          ///< Define the parameter omega of the scheme
#define PARAM_OMEGA_R22             113          ///< Define the parameter omega of the scheme
#define PARAM_OMEGA_E               324          ///< Define the parameter omega_e of the scheme
#define PARAM_SECURITY              512         ///< Define the security level corresponding to the chosen parameters
#define PARAM_SECURITY_BYTES        64          ///< Define the security level in bytes
#define PARAM_DFR_EXP               512         ///< Define the decryption failure rate exponent

#define PARAM_OMEGA_MAX_1             324          ///< Define the parameter omega of the scheme
#define PARAM_OMEGA_MAX_2             113          ///< Define the parameter omega of the scheme

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

#define PARAM_N_MULT                147456      
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
    117, 46, 245, 193, 47, 239, 220, 7, 37, 145, 167, 198, 110, 10, 75, 73, 142, 249, 134, 240, 185, 134, 77, 143, 39, 110, 211, 194, 111, 127, 229, 137, 182, 229, 90, 183, 232, 190, 173, 174, 246, 191, 48, 175, 218, 167, 5, 166, 9, 222, 121, 66, 15, 17, 197, 156, 60, 166, 225, 48, 36, 64, 138, 212, 250, 74, 182, 131, 162, 223, 158, 172, 46, 240, 189, 64,31, 165, 1                                                     ///< Coefficients of the Reed–Solomon generator polynomial
#define SEED_BYTES                  32          ///< Define the size of the seed in bytes
#define SALT_BYTES                  16          ///< Define the size of a salt in bytes

#define PARAM_N_MU 31615ULL   ///<  Define a precomputed multiplier for Barrett reduction mu = floor(2^32 / PARAM_N)
#define UTILS_REJECTION_THRESHOLD             16709673  ///< Rejection threshold for uniform sampling in [0, PARAM_N)

#endif // QUBE_PARAMETERS_H
