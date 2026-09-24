/** 
 * \file parameters.h
 * \brief Parameters of the BRQC_KEM IND-CCA2 scheme
 */

 #ifndef BRQC_PARAMETER_H
 #define BRQC_PARAMETER_H
 
 #define BRQC_SECRET_KEY_BYTES    2066  /**< Secret key size (sk_seed + sigma + pk) */
 #define BRQC_PUBLIC_KEY_BYTES    1954  /**< Public key size */
 #define BRQC_SHARED_SECRET_BYTES 64    /**< Shared secret size */
 #define BRQC_CIPHERTEXT_BYTES    3844  /**< Ciphertext size */
 
 #define BRQC_PARAM_Q    2    /**< Parameter q of the scheme (finite field GF(q^m)) */
 #define BRQC_PARAM_M    127  /**< Parameter m of the scheme (finite field GF(q^m)) */
 #define BRQC_PARAM_K    3    /**< Parameter k of the scheme (code dimension) */
 #define BRQC_PARAM_N    119  /**< Parameter n of the scheme (code length) */
 #define BRQC_PARAM_W_x  5    /**< Parameter omega_x of the scheme (weight of vectors) */
 #define BRQC_PARAM_W_y  5    /**< Parameter omega_y of the scheme (weight of vectors) */
 #define BRQC_PARAM_W_r1 5    /**< Parameter omega_r1 of the scheme (weight of vectors) */
 #define BRQC_PARAM_W_r2 5    /**< Parameter omega_r2 of the scheme (weight of vectors) */
 #define BRQC_PARAM_W_e  8    /**< Parameter omega_e  of the scheme (weight of vectors) */
 #define BRQC_SECURITY   128  /**< Expected security level */
 
 #define BRQC_VEC_K_BYTES 48   /**< Number of bytes required to store a vector of size k */
 #define BRQC_VEC_N_BYTES 1890 /**< Number of bytes required to store a vector of size n */
 #define SHA512_BYTES         64   /**< Size of SHA2_512 and SHA3_512 outputs */
 
 #define SEEDEXPANDER_SEED_BYTES 64         /**< Seed size of the NGCC seed expander */
 #define BRQC_SALT_BYTES     64         /**< Salt size */
 #define SEEDEXPANDER_MAX_LENGTH 4294967295 /**< Max length of the NGCC seed expander */
 
 #endif
 
 
