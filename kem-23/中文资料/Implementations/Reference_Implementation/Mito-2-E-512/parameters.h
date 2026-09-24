/**
* @file parameters.h
* @brief Parameters of the MITO-KEM IND-CCA2 scheme
 */

#ifndef MITO_PARAMETERS_H
#define MITO_PARAMETERS_H

#define MITO_2_E // only change it to MITO_1 or MITO_1_E or MITO_2_E
#define lambda 512 // must be in {128, 256, 512}

 /*****************************DO NOT CHANGE ANYTHING UNDER THIS LINE*****************************/

#ifdef MITO_1
#define PARAM_L                     2            ///< Define the parameter l of the scheme
#if lambda == 128
#define ALGORITHM_NAME "MITO_1_128"
#define PARAM_N                     15373        ///< Define the parameter n of the scheme
#define PARAM_N1                    40           ///< Define the parameter n1 of the scheme (length of Reed–Solomon code)
#define PARAM_N2                    384          ///< Define the parameter n2 of the scheme (length of Duplicated Reed–Muller code)
static const int PARAM_OMEGA[] = { 44, 41, 47, 40 }; ///< Define the parameter omega of the scheme
static const int PARAM_OMEGA_R[] = { 42, 45, 44, 45 }; ///< Define the parameter omega_r of the scheme
#define PARAM_OMEGA_E               85           ///< Define the parameter omega_e of the scheme
#define PARAM_G                     25           ///< Define the size of the generator polynomial of the Reed–Solomon code
#define PARAM_FFT                   4            ///< Exponent for additive FFT (2^PARAM_FFT points)
#define RS_POLY_COEFS                                                                                                \
    0xC1, 0x6C, 0xC7, 0xD0, 0xAD, 0x4F, 0x2D, 0x85, 0xFB, 0x7D, 0x2C, 0xA7, 0xC6,\
    0x96, 0xAE, 0xFC, 0xDA, 0x08, 0xC5, 0xC3, 0x14, 0x21, 0xC5, 0xF4, 0x01  
    ///< Coefficients of the Reed–Solomon generator polynomial
#elif lambda == 256
#define ALGORITHM_NAME "MITO_1_256"
#define PARAM_N                     33827        ///< Define the parameter n of the scheme
#define PARAM_N1                    88           ///< Define the parameter n1 of the scheme (length of Reed–Solomon code)
#define PARAM_N2                    384          ///< Define the parameter n2 of the scheme (length of Duplicated Reed–Muller code)
static const int PARAM_OMEGA[] = { 65, 63, 72, 69 }; ///< Define the parameter omega of the scheme
static const int PARAM_OMEGA_R[] = { 63, 66, 71, 74 }; ///< Define the parameter omega_r of the scheme
#define PARAM_OMEGA_E               133          ///< Define the parameter omega_e of the scheme
#define PARAM_G                     57           ///< Define the size of the generator polynomial of the Reed–Solomon code
#define PARAM_FFT                   5            ///< Exponent for additive FFT (2^PARAM_FFT points)
#define RS_POLY_COEFS                                                                                                \
    0x61, 0xF8, 0x89, 0x20, 0x8E, 0xA7, 0x38, 0xD4, 0xFA, 0x89, 0x68, 0x51, 0xC2,\
    0x42, 0x1B, 0x19, 0xC6, 0x84, 0x8A, 0x75, 0x76, 0xC1, 0xDE, 0x09, 0x53, 0x51,\
    0x8F, 0xB2, 0xD7, 0x39, 0x91, 0x4A, 0xF3, 0x19, 0xF4, 0xE4, 0xFF, 0x2D, 0xBF,\
    0xC9, 0xE9, 0x9B, 0xEF, 0x8C, 0xFF, 0x10, 0x22, 0xE8, 0x97, 0x93, 0x8A, 0xE5,\
    0xD1, 0x67, 0xEC, 0x68, 0x01
#elif lambda == 512
#define ALGORITHM_NAME "MITO_1_512"
#define PARAM_N                     106261        ///< Define the parameter n of the scheme
#define PARAM_N1                    166           ///< Define the parameter n1 of the scheme (length of Reed–Solomon code)
#define PARAM_N2                    640           ///< Define the parameter n2 of the scheme (length of Duplicated Reed–Muller code)
static const int PARAM_OMEGA[] = { 127, 126, 143,131 }; ///< Define the parameter omega of the scheme
static const int PARAM_OMEGA_R[] = { 123, 134, 137, 138 }; ///< Define the parameter omega_r of the scheme
#define PARAM_OMEGA_E               262           ///< Define the parameter omega_e of the scheme
#define PARAM_G                     103           ///< Define the size of the generator polynomial of the Reed–Solomon code
#define PARAM_FFT                   6             ///< Exponent for additive FFT (2^PARAM_FFT points)
#define RS_POLY_COEFS                                                                                                \
    0x92, 0x83, 0x54, 0xF0, 0x52, 0x0A, 0x51, 0xD8, 0xB8, 0x3B, 0xC6, 0x22, 0x5B,\
    0xAC, 0x67, 0xC6, 0x86, 0xDC, 0x9D, 0xAA, 0xBB, 0x05, 0xA4, 0xD4, 0x9E, 0x26,\
    0x09, 0xE4, 0x7F, 0xAE, 0x9D, 0x67, 0x1F, 0x5B, 0x98, 0x57, 0x1F, 0x92, 0x61,\
    0x45, 0x17, 0x01, 0x1A, 0x5D, 0x93, 0x9E, 0x06, 0x01, 0x74, 0x8A, 0xD2, 0x91,\
    0xB2, 0x4C, 0xB2, 0x43, 0x75, 0x30, 0x63, 0xAD, 0x07, 0xE9, 0x4C, 0x20, 0x04,\
    0x58, 0x43, 0xBC, 0x0A, 0xC9, 0xC8, 0xDF, 0xC6, 0x56, 0x87, 0xFB, 0x07, 0x2A,\
    0xD1, 0x9B, 0x8A, 0xB4, 0xA7, 0xD7, 0xE0, 0x01, 0x5F, 0x06, 0x97, 0x5C, 0x72,\
    0x6F, 0x75, 0x17, 0x94, 0x50, 0x63, 0xB7, 0x49, 0xEB, 0xC8, 0x8D, 0x01
#else
#error "lambda must be in {128,256,512}"
#endif
#elif defined(MITO_1_E)
#define PARAM_L                     2            ///< Define the parameter l of the scheme
#if lambda == 128
#define ALGORITHM_NAME "MITO_1_E_128"
#define PARAM_N                     14621        ///< Define the parameter n of the scheme
#define PARAM_N1                    38           ///< Define the parameter n1 of the scheme (length of Reed–Solomon code)
#define PARAM_N2                    384          ///< Define the parameter n2 of the scheme (length of Duplicated Reed–Muller code)
static const int PARAM_OMEGA[] = { 44, 41, 47, 40 }; ///< Define the parameter omega of the scheme
static const int PARAM_OMEGA_R[] = { 42, 45, 44, 45 }; ///< Define the parameter omega_r of the scheme
#define PARAM_OMEGA_E               86           ///< Define the parameter omega_e of the scheme
#define PARAM_G                     23           ///< Define the size of the generator polynomial of the Reed–Solomon code
#define PARAM_FFT                   4            ///< Exponent for additive FFT (2^PARAM_FFT points)
#define RS_POLY_COEFS                                                                                                \
    0x47, 0x6E, 0x33, 0x4B, 0x49, 0xCB, 0xD9, 0x1F, 0x6E, 0x75, 0xB1, 0xB2, 0xA1,\
    0xF4, 0xED, 0x75, 0xCB, 0x43, 0xCF, 0x6C, 0xF6, 0xB2, 0x01
#elif lambda == 256
#define ALGORITHM_NAME "MITO_1_E_256"
#define PARAM_N                     32261        ///< Define the parameter n of the scheme
#define PARAM_N1                    84           ///< Define the parameter n1 of the scheme (length of Reed–Solomon code)
#define PARAM_N2                    384          ///< Define the parameter n2 of the scheme (length of Duplicated Reed–Muller code)
static const int PARAM_OMEGA[] = { 69, 64, 72, 64 }; ///< Define the parameter omega of the scheme
static const int PARAM_OMEGA_R[] = { 63, 73, 66, 72 }; ///< Define the parameter omega_r of the scheme
#define PARAM_OMEGA_E               132          ///< Define the parameter omega_e of the scheme
#define PARAM_G                     53           ///< Define the size of the generator polynomial of the Reed–Solomon code
#define PARAM_FFT                   5            ///< Exponent for additive FFT (2^PARAM_FFT points)
#define RS_POLY_COEFS                                                                                                \
    0x88, 0x7E, 0x8C, 0x27, 0xE7, 0x52, 0xE8, 0xE9, 0xC0, 0xD3, 0x42, 0xCC, 0xF8,\
    0x6F, 0x73, 0x98, 0x13, 0xA9, 0x29, 0xD5, 0xAF, 0xDF, 0x97, 0x56, 0xED, 0x06,\
    0xE6, 0x3D, 0xE8, 0xB9, 0x7E, 0xFD, 0x4E, 0xD4, 0xF3, 0xFC, 0x36, 0x0E, 0x27,\
    0x9E, 0x42, 0xAF, 0x5D, 0x38, 0xAF, 0x08, 0x48, 0xA0, 0xAE, 0xE1, 0x14, 0xED,\
    0x01
#elif lambda == 512
#define ALGORITHM_NAME "MITO_1_E_512"
#define PARAM_N                     102437        ///< Define the parameter n of the scheme
#define PARAM_N1                    160           ///< Define the parameter n1 of the scheme (length of Reed–Solomon code)
#define PARAM_N2                    640           ///< Define the parameter n2 of the scheme (length of Duplicated Reed–Muller code)
static const int PARAM_OMEGA[] = { 128, 123, 140, 136 }; ///< Define the parameter omega of the scheme
static const int PARAM_OMEGA_R[] = { 126, 129, 136, 141 }; ///< Define the parameter omega_r of the scheme
#define PARAM_OMEGA_E               261           ///< Define the parameter omega_e of the scheme
#define PARAM_G                     97            ///< Define the size of the generator polynomial of the Reed–Solomon code
#define PARAM_FFT                   6             ///< Exponent for additive FFT (2^PARAM_FFT points)
#define RS_POLY_COEFS                                                                                                \
    0x61, 0xC8, 0x90, 0x81, 0x3C, 0x76, 0x46, 0x63, 0x3C, 0xEE, 0x88, 0x13, 0x90,\
    0x6B, 0xE1, 0x27, 0x8A, 0xB0, 0xAF, 0x42, 0x1B, 0xB4, 0xC6, 0xAD, 0xD1, 0x18,\
    0xC5, 0x87, 0x4F, 0x73, 0xB0, 0xD1, 0x09, 0xF9, 0x5E, 0x61, 0x45, 0x3E, 0x51,\
    0xAA, 0xB6, 0xE8, 0x8E, 0x9D, 0x3F, 0x12, 0x5A, 0x33, 0xB4, 0x18, 0xFD, 0xA5,\
    0x27, 0xBA, 0x62, 0xAF, 0xD3, 0x77, 0x08, 0xC2, 0x3C, 0xFD, 0xB0, 0x4E, 0x38,\
    0xB5, 0x06, 0x22, 0xCB, 0x4C, 0x30, 0x5B, 0x85, 0xEC, 0x82, 0x53, 0xD3, 0x5C,\
    0x83, 0x6A, 0xAB, 0x29, 0x34, 0x10, 0x05, 0x6C, 0xB0, 0xB1, 0xEA, 0xE8, 0x50,\
    0x60, 0xA8, 0xD3, 0x11, 0x90, 0x01
#else
#error "lambda must be in {128,256,512}"
#endif
#elif defined(MITO_2_E)
#define PARAM_L                     4            ///< Define the parameter l of the scheme
#if lambda == 128
#define ALGORITHM_NAME "MITO_2_E_128"
#define PARAM_N                     10253        ///< Define the parameter n of the scheme
#define PARAM_N1                    40           ///< Define the parameter n1 of the scheme (length of Reed–Solomon code)
#define PARAM_N2                    256          ///< Define the parameter n2 of the scheme (length of Duplicated Reed–Muller code)
static const int PARAM_OMEGA[] = { 25, 25, 22, 21 }; ///< Define the parameter omega of the scheme
static const int PARAM_OMEGA_R[] = { 22, 23, 25,25 }; ///< Define the parameter omega_r of the scheme
#define PARAM_OMEGA_E               39           ///< Define the parameter omega_e of the scheme
#define PARAM_G                     25           ///< Define the size of the generator polynomial of the Reed–Solomon code
#define PARAM_FFT                   4            ///< Exponent for additive FFT (2^PARAM_FFT points)
#define RS_POLY_COEFS                                                                                                \
    0xC1, 0x6C, 0xC7, 0xD0, 0xAD, 0x4F, 0x2D, 0x85, 0xFB, 0x7D, 0x2C, 0xA7, 0xC6,\
    0x96, 0xAE, 0xFC, 0xDA, 0x08, 0xC5, 0xC3, 0x14, 0x21, 0xC5, 0xF4, 0x01
#elif lambda == 256
#define ALGORITHM_NAME "MITO_2_E_256"
#define PARAM_N                     21523        ///< Define the parameter n of the scheme
#define PARAM_N1                    84           ///< Define the parameter n1 of the scheme (length of Reed–Solomon code)
#define PARAM_N2                    256          ///< Define the parameter n2 of the scheme (length of Duplicated Reed–Muller code)
static const int PARAM_OMEGA[] = { 38, 38, 33, 33 }; ///< Define the parameter omega of the scheme
static const int PARAM_OMEGA_R[] = { 34, 34, 38,38 }; ///< Define the parameter omega_r of the scheme
#define PARAM_OMEGA_E               68           ///< Define the parameter omega_e of the scheme
#define PARAM_G                     53           ///< Define the size of the generator polynomial of the Reed–Solomon code
#define PARAM_FFT                   5            ///< Exponent for additive FFT (2^PARAM_FFT points)
#define RS_POLY_COEFS                                                                                                \
    0x88, 0x7E, 0x8C, 0x27, 0xE7, 0x52, 0xE8, 0xE9, 0xC0, 0xD3, 0x42, 0xCC, 0xF8,\
    0x6F, 0x73, 0x98, 0x13, 0xA9, 0x29, 0xD5, 0xAF, 0xDF, 0x97, 0x56, 0xED, 0x06,\
    0xE6, 0x3D, 0xE8, 0xB9, 0x7E, 0xFD, 0x4E, 0xD4, 0xF3, 0xFC, 0x36, 0x0E, 0x27,\
    0x9E, 0x42, 0xAF, 0x5D, 0x38, 0xAF, 0x08, 0x48, 0xA0, 0xAE, 0xE1, 0x14, 0xED,\
    0x01
#elif lambda == 512
#define ALGORITHM_NAME "MITO_2_E_512"
#define PARAM_N                     65293         ///< Define the parameter n of the scheme
#define PARAM_N1                    170           ///< Define the parameter n1 of the scheme (length of Reed–Solomon code)
#define PARAM_N2                    384           ///< Define the parameter n2 of the scheme (length of Duplicated Reed–Muller code)
static const int PARAM_OMEGA[] = { 72, 72, 66, 62 }; ///< Define the parameter omega of the scheme
static const int PARAM_OMEGA_R[] = { 65, 66, 70, 73 }; ///< Define the parameter omega_r of the scheme
#define PARAM_OMEGA_E               135           ///< Define the parameter omega_e of the scheme
#define PARAM_G                     107           ///< Define the size of the generator polynomial of the Reed–Solomon code
#define PARAM_FFT                   6             ///< Exponent for additive FFT (2^PARAM_FFT points)
#define RS_POLY_COEFS                                                                                                \
    0x6F, 0x12, 0xDE, 0x98, 0x54, 0x97, 0xDA, 0x31, 0x55, 0xBC, 0x54, 0x28, 0x80,\
    0x4C, 0x81, 0x40, 0xFB, 0x21, 0xCD, 0x80, 0xC1, 0x41, 0xD4, 0x5D, 0xC0, 0x80,\
    0x54, 0xCC, 0x30, 0x8B, 0xC2, 0xD2, 0x52, 0xBF, 0x57, 0x03, 0xB6, 0x63, 0x6A,\
    0x21, 0xE0, 0x74, 0x7F, 0x90, 0x10, 0x02, 0xFA, 0xEF, 0x2C, 0x5B, 0xF8, 0x51,\
    0x0F, 0xD3, 0x09, 0x36, 0x05, 0xF1, 0xEF, 0xBB, 0x20, 0x91, 0x6F, 0xDB, 0x37,\
    0x02, 0x7F, 0xD7, 0xAC, 0xCB, 0xD2, 0x42, 0xC8, 0xDE, 0xC8, 0x01, 0xE4, 0x56,\
    0x73, 0x55, 0xD2, 0xBC, 0x7D, 0xD1, 0x35, 0xD3, 0x6B, 0x32, 0x5B, 0xCE, 0x11,\
    0xB4, 0x5A, 0xBC, 0x59, 0xA4, 0xB8, 0x51, 0x96, 0x0D, 0xA7, 0xFE, 0x01, 0xEA,\
    0xEF, 0x26, 0x01
#else
#error "lambda must be in {128,256,512}"
#endif
#else
#error "undefined parameters sets!"
#endif

#define PARAM_N1N2                  (PARAM_N1 * PARAM_N2)        ///< Define the length in bits of the concatenated code
#define PARAM_M                     8           ///< Define the degree m of the Galois field GF(2^m)
#define PARAM_GF_POLY               0x11D       ///< Generator polynomial of GF(2^PARAM_M) in hexadecimal form
#define PARAM_GF_MUL_ORDER          255         ///< Size of the multiplicative group of GF(2^PARAM_M) (2^PARAM_M−1)
#define PARAM_DELTA                 (PARAM_G / 2)          ///< Define the error-correcting capacity (delta) of the Reed–Solomon code
#define PARAM_ALPHA                 7           ///< Define the distance threshold

#define PARAM_K_BYTES               (lambda / 8)          ///< Define the security level in bytes
#define SEED_BYTES                  64          ///< Define the size of the seed in bytes
#define SALT_BYTES                  32          ///< Define the size of a salt in bytes

#define VEC_N_SIZE_BYTES            ((PARAM_N + 7) / 8)         ///< Size of array to store PARAM_N bits in bytes
#define VEC_N1_SIZE_BYTES           PARAM_N1                    ///< Size of array to store PARAM_N1 bits in bytes
#define VEC_N1N2_SIZE_BYTES         ((PARAM_N1N2 + 7) / 8)      ///< Size of array to store PARAM_N1N2 bits in bytes

#define VEC_N_SIZE_64               ((PARAM_N + 63) / 64)       ///< Size of array to store PARAM_N bits in 64-bit words
#define VEC_N1_SIZE_64              ((PARAM_N1 + 7) / 8)        ///< Size of array to store PARAM_N1 bits in 64-bit words
#define VEC_N1N2_SIZE_64            ((PARAM_N1N2 + 63) / 64)    ///< Size of array to store PARAM_N1N2 bits in 64-bit words

#define SHARED_SECRET_BYTES         64            ///< Define the size of the shared secret in bytes
#define PUBLIC_KEY_BYTES            (PARAM_L * VEC_N_SIZE_BYTES +  SEED_BYTES)  ///< Define the size of the public key in bytes
#define SECRET_KEY_BYTES            (PUBLIC_KEY_BYTES + 2 * SEED_BYTES + PARAM_K_BYTES)   ///< Define the size of the secret key in bytes
#define CIPHERTEXT_BYTES            (PARAM_L * VEC_N_SIZE_BYTES + VEC_N1N2_SIZE_BYTES + SALT_BYTES)  ///< Define the size of the ciphertext in bytes
                                                 
#endif // MITO_PARAMETERS_H
