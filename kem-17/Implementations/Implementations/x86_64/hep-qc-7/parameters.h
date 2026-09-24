/**
* @file parameters.h
* @brief Parameters of the HEP-QC-KEM IND-CCA2 scheme
 */

#ifndef HEP_QC_PARAMETERS_H
#define HEP_QC_PARAMETERS_H

#include "api.h"

#define CEIL_DIVIDE(a, b) (((a) / (b)) + ((a) % (b) == 0 ? 0 : 1)) /*!< Divide a by b and ceil the result*/
#define BITMASK(a, size)  ((1UL << (a % size)) - 1)                /*!< Create a mask*/

#define PARAM_N                     197123      ///< Define the parameter n of the scheme
#define PARAM_N1                    220         ///< Define the parameter n1 of the scheme (length of Reed-Solomon code)
#define PARAM_N2                    896         ///< Define the parameter n2 of the scheme (length of Duplicated Reed-Muller code)
#define PARAM_N1N2                  197120      ///< Define the length in bits of the concatenated code
#define PARAM_OMEGA                 261         ///< Define the parameter omega of the scheme
#define PARAM_OMEGA_E               297         ///< Define the parameter omega_e of the scheme
#define PARAM_OMEGA_R               297         ///< Define the parameter omega_r of the scheme
#define PARAM_SECURITY              512         ///< Define the security level corresponding to the chosen parameters
#define PARAM_SECURITY_BYTES        64          ///< Define the security level in bytes
#define PARAM_DFR_EXP               512         ///< Define the decryption failure rate corresponding to the chosen parameters

#define SECRET_KEY_BYTES            CRYPTO_SECRETKEYBYTES   ///< Define the size of the secret key in bytes
#define PUBLIC_KEY_BYTES            CRYPTO_PUBLICKEYBYTES   ///< Define the size of the public key in bytes
#define SHARED_SECRET_BYTES         CRYPTO_BYTES            ///< Define the size of the shared secret in bytes
#define CIPHERTEXT_BYTES            CRYPTO_CIPHERTEXTBYTES  ///< Define the size of the ciphertext in bytes

#define VEC_N_SIZE_BYTES            CEIL_DIVIDE(PARAM_N, 8)     ///< Size of array to store PARAM_N bits in bytes
#define VEC_K_SIZE_BYTES            PARAM_K                     ///< Size of array to store PARAM_K bits in bytes
#define VEC_N1_SIZE_BYTES           PARAM_N1                    ///< Size of array to store PARAM_N1 bits in bytes
#define VEC_N1N2_SIZE_BYTES         CEIL_DIVIDE(PARAM_N1N2, 8)  ///< Size of array to store PARAM_N1N2 bits in bytes

#define VEC_N_SIZE_64               CEIL_DIVIDE(PARAM_N, 64)    ///< Size of array to store PARAM_N bits in 64-bit words
#define VEC_N_SIZE_64_BIT           VEC_N_SIZE_64 * 64          ///< Size of bits to store PARAM_N bits in 64-bit words
#define VEC_N1_SIZE_64              CEIL_DIVIDE(PARAM_N1, 8)    ///< Size of array to store PARAM_N1 bits in 64-bit words
#define VEC_N1N2_SIZE_64            CEIL_DIVIDE(PARAM_N1N2, 64) ///< Size of array to store PARAM_N1N2 bits in 64-bit words

#define PARAM_DELTA                 78          ///< Define the error-correcting capacity (delta) of the Reed-Solomon code
#define PARAM_M                     8           ///< Define the degree m of the Galois field GF(2^m)
#define PARAM_GF_POLY               0x11D       ///< Generator polynomial of GF(2^PARAM_M) in hexadecimal form
#define PARAM_GF_MUL_ORDER          255         ///< Size of the multiplicative group of GF(2^PARAM_M) (2^PARAM_M−1)
#define PARAM_K                     64          ///< Define the size of the information bits of the Reed-Solomon code
#define MSG_BIT                     512         ///< Define the size of the message bits
#define PARAM_G                     157         ///< Define the size of the generator polynomial of the Reed-Solomon code
#define PARAM_FFT                   7           ///< Exponent for additive FFT (2^PARAM_FFT points)

#define CTX_LEN                     26

#define RS_POLY_COEFS                                                                                                  \
    64, 54, 224, 12, 82, 107, 255, 93, 85, 254, 61, 80, 40, 28, 11, 6, 35, 138,\
    200, 72, 182, 203, 116, 129, 244, 213, 66, 122, 154, 247, 10, 233, 143, 190,\
250, 166, 115, 151, 122, 229, 223, 192, 66, 94, 202, 130, 174, 26, 47, 150, 23,\
240, 231, 129, 46, 110, 2, 60, 66, 67, 210, 24, 90, 101, 194, 127, 255, 110, 24,\
44, 2, 70, 25, 252, 53, 101, 148, 146, 37, 216, 251, 206, 227, 82, 76, 18, 180,\
68, 125, 250, 30, 177, 198, 205, 186, 198, 148, 255, 210, 226, 237, 77, 220,\
205, 240, 255, 153, 155, 221, 187, 232, 206, 223, 55, 175, 127, 250, 63, 47,\
222, 100, 177, 69, 132, 141, 64, 145, 87, 56, 204, 169, 216, 126, 41, 84, 121,\
126, 84, 178, 253, 8, 253, 169, 100, 154, 233, 128, 172, 51, 23, 163, 67, 32,\
139, 2, 77, 1                  ///< Coefficients of the Reed-Solomon generator polynomial

#define SEED_BYTES                  32          ///< Define the size of the seed in bytes
#define SALT_BYTES                  16          ///< Define the size of a salt in bytes

#define PARAM_N_MU 21788ULL   ///<  Define a precomputed multiplier for Barrett reduction mu = floor(2^32 / PARAM_N)
#define UTILS_REJECTION_THRESHOLD             16952578 ///< Rejection threshold for uniform sampling in [0, PARAM_N). ceil(2^24 / PARAM_N) * PARAM_N

#endif // HEP_QC_PARAMETERS_H