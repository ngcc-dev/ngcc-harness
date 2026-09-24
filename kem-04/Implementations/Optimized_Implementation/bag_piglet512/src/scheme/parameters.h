/** 
 * \file parameters.h
 * \brief Parameters of the BAG-PIGLET IND-CCA2 scheme
 */

#include "api.h"

#ifndef BAG_PIGLET_PARAMETER_H
#define BAG_PIGLET_PARAMETER_H


#define PARAM_Q 2 /**< Parameter q of the scheme (finite field GF(q^m)) */
#define PARAM_M 131/**< Parameter m of the scheme (finite field GF(q^m)) */ //(m-k+espilon)2
#define PARAM_K 4 /**< Parameter k of the scheme (code dimension) */
#define PARAM_N 125 /**< Parameter n of the scheme (code length) */
#define PARAM_N_ 130
#define PARAM_N_1 2
#define PARAM_epsilon 92
#define PARAM_delta (PARAM_N_-PARAM_K+PARAM_epsilon)/2
#define PARAM_K_Gabidulin (PARAM_K+PARAM_epsilon)
#define PARAM_E_Gabidulin (PARAM_N_-PARAM_K_Gabidulin)/2
#define PARAM_SECURITY 512 /**< Expected security level */

#define SECRET_KEY_BYTES CRYPTO_SECRETKEYBYTES /**< Secret key size */
#define PUBLIC_KEY_BYTES CRYPTO_PUBLICKEYBYTES /**< Public key size */
#define SHARED_SECRET_BYTES CRYPTO_BYTES /**< Shared secret size */
#define CIPHERTEXT_BYTES CRYPTO_CIPHERTEXTBYTES /**< Ciphertext size */

#define VEC_K_BYTES (PARAM_M*PARAM_K+7)/8 /**< Number of bytes required to store a vector of size k */
#define VEC_N_1_N_BYTES (PARAM_M*PARAM_N*PARAM_N_1+7)/8 /**< Number of bytes required to store a vector of size n */
#define VEC_N_BYTES (PARAM_M*PARAM_N+7)/8 /**< Number of bytes required to store a vector of size n */
#define FFI_ELT_BYTES (PARAM_M+7)/8 /**< Number of bytes required to store an element of GF(q^m) */
#define FFI_VEC_K_BYTES PARAM_K * FFI_ELT_BYTES /**< Number of bytes required to store a vector of size k using the ffi_vec implementation */
#define FFI_VEC_N_BYTES PARAM_N * FFI_ELT_BYTES /**< Number of bytes required to store a vector of size n using the ffi_vec implementation */

#define SEEDEXPANDER_SEED_BYTES 64 /**< Seed size at this security level */
#define PARAMS_SALT_SIZE 16
#define PARAMS_ID_SIZE SEEDEXPANDER_SEED_BYTES
#define PARAMS_PREKEY_SIZE SEEDEXPANDER_SEED_BYTES
#define PARAMS_RAND_SIZE SEEDEXPANDER_SEED_BYTES
#define CCAKEM_KEY_SIZE SEEDEXPANDER_SEED_BYTES

#define VEC_BLOCK_N_BYTES (PARAM_M*PARAM_N*BLOCK+7)/8
#define VEC_CT_BYTES (2*PARAM_M*PARAM_N*PARAM_N_1+7)/8
#define CPAPKE_CT_SIZE VEC_CT_BYTES
#define CPAPKE_PK_SIZE (SEEDEXPANDER_SEED_BYTES+VEC_BLOCK_N_BYTES)
#define CPAPKE_SK_SIZE (SEEDEXPANDER_SEED_BYTES+VEC_N_BYTES)
#define CPAPKE_PT_SIZE VEC_K_BYTES

#define CCAKEM_CT_SIZE (CPAPKE_CT_SIZE+PARAMS_SALT_SIZE)
#define CCAKEM_PK_SIZE CPAPKE_PK_SIZE
#define CCAKEM_SK_SIZE SEEDEXPANDER_SEED_BYTES

#define CCAPKE_PK_SIZE CCAKEM_PK_SIZE
#define CCAPKE_SK_SIZE CCAKEM_SK_SIZE
#define PARAMS_SEED_SIZE PARAMS_RAND_SIZE

#define SHA512_BYTES 64 /**< Size of SHA512 output */


#define SEEDEXPANDER_MAX_LENGTH 196608 /**< Max length of the NIST seed expander */

// #define PARAM_W_E 1 /**< Parameter omega_r of the scheme (weight of vectors) */
// #define WEIGHT_X_VALUES {1,1,1}
// #define WEIGHT_Y_VALUES {1,1,1}
// #define WEIGHT_R1_VALUES {1,1,1}
// #define WEIGHT_R2_VALUES {1,1,1}

/**< Parameter omega_r of the scheme (weight of vectors) */
#define WEIGHT_X_VALUES 8
#define WEIGHT_Y1_VALUES 8
#define WEIGHT_Y2_VALUES 8

#define WEIGHT_R1_VALUES 4
#define WEIGHT_R21_VALUES 4
#define WEIGHT_R22_VALUES 5
#define WEIGHT_E_VALUES 5 

#define WEIGHT_TOTAL_1 (WEIGHT_X_VALUES+WEIGHT_Y1_VALUES+WEIGHT_Y2_VALUES)
#define WEIGHT_TOTAL_2 (WEIGHT_R1_VALUES+WEIGHT_R21_VALUES+WEIGHT_R22_VALUES+WEIGHT_E_VALUES)


// #define PARAM_W_E 2 /**< Parameter omega_r of the scheme (weight of vectors) */
// #define WEIGHT_X_VALUES {2,2,2}
// #define WEIGHT_Y_VALUES {2,2,2}
// #define WEIGHT_R1_VALUES {2,2,2}
// #define WEIGHT_R2_VALUES {2,2,2}

// #define PARAM_W_E 5 /**< Parameter omega_r of the scheme (weight of vectors) */
// #define WEIGHT_X_VALUES {4}
// #define WEIGHT_Y_VALUES {4}
// #define WEIGHT_R1_VALUES {5}
// #define WEIGHT_R2_VALUES {5}


#endif
