/** 
 * \file parameters.h
 * \brief Parameters of the BRA_KEM IND-CCA2 scheme
 */

#ifndef BRA_PARAMETER_H
#define BRA_PARAMETER_H

#define BRA_SECRET_KEY_BYTES    1176 /**< Secret key size (sk_seed + sigma + pk) */
#define BRA_PUBLIC_KEY_BYTES    1078 /**< Public key size */
#define BRA_SHARED_SECRET_BYTES 64   /**< Shared secret size */
#define BRA_CIPHERTEXT_BYTES    2092 /**< Ciphertext size */

#define BRA_PARAM_Q    2   /**< Parameter q of the scheme (finite field GF(q^m)) */
#define BRA_PARAM_M    67  /**< Parameter m of the scheme (finite field GF(q^m)) */
#define BRA_PARAM_K    4   /**< Parameter k of the scheme (code dimension) */
#define BRA_PARAM_N    121 /**< Parameter n of the scheme (code length) */
#define BRA_PARAM_W_x  4   /**< Parameter omega_x of the scheme (weight of vectors) */
#define BRA_PARAM_W_y  5   /**< Parameter omega_y of the scheme (weight of vectors) */
#define BRA_PARAM_W_r1 5   /**< Parameter omega_r1 of the scheme (weight of vectors) */
#define BRA_PARAM_W_r2 5   /**< Parameter omega_r2 of the scheme (weight of vectors) */
#define BRA_PARAM_W_e  8   /**< Parameter omega_e of the scheme (weight of vectors) */
#define BRA_SECURITY   128 /**< Expected security level */

#define BRA_VEC_K_BYTES 34   /**< Number of bytes required to store a vector of size k */
#define BRA_VEC_N_BYTES 1014 /**< Number of bytes required to store a vector of size n */

#define SHA512_BYTES 64 /**< Size of SHA2_512 and SHA3_512 outputs */

#define SEEDEXPANDER_SEED_BYTES 64         /**< Seed size of the NGCC seed expander */
#define BRA_SALT_BYTES      64         /**< Salt size */
#define SEEDEXPANDER_MAX_LENGTH 4294967295 /**< Max length of the NGCC seed expander */

#endif
