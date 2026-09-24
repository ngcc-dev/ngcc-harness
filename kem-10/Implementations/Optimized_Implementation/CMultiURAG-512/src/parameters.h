/**
 * \file parameters.h
 * \brief Parameters of the CMULTIURAG_KEM IND-CCA2 scheme
 */

#ifndef CMULTIURAG_PARAMETER_H
#define CMULTIURAG_PARAMETER_H

#define CMULTIURAG_SECRET_KEY_BYTES    29021 /**< Secret key size (sk_seed + sigma + pk) */
#define CMULTIURAG_PUBLIC_KEY_BYTES    28866 /**< Public key size (S_bytes + pk_seed) */
#define CMULTIURAG_SHARED_SECRET_BYTES 64    /**< Shared secret size */
#define CMULTIURAG_CIPHERTEXT_BYTES    40926 /**< Ciphertext size (U + V + salt) */

#define CMULTIURAG_PARAM_Q     2    /**< Parameter q of the scheme (finite field GF(q^m)) */
#define CMULTIURAG_PARAM_M     181  /**< Parameter m of the scheme (extension degree) */
#define CMULTIURAG_PARAM_N     67   /**< Parameter n of the scheme (matrix dimension for H) */
#define CMULTIURAG_PARAM_K     4    /**< Parameter k of the scheme (AG code dimension) */
#define CMULTIURAG_PARAM_T     181  /**< Parameter t of the scheme (AG generator weight) */
#define CMULTIURAG_PARAM_N1    19   /**< Parameter N1 of the scheme (syndrome columns) */
#define CMULTIURAG_PARAM_N2    21   /**< Parameter N2 of the scheme (ciphertext columns) */
#define CMULTIURAG_PARAM_W1    12   /**< Parameter w1 of the scheme (secret key rank weight) */
#define CMULTIURAG_PARAM_W2    14   /**< Parameter w2 of the scheme (encryption noise weight) */
#define CMULTIURAG_PARAM_R     168  /**< Parameter r = w1*w2 (max error weight for decoding) */
#define CMULTIURAG_PARAM_N1N2  399  /**< N1*N2 (AG code length) */
#define CMULTIURAG_SECURITY    512  /**< Expected security level */

#define CMULTIURAG_VEC_K_BYTES    91    /**< Bytes for a vector of size k: ceil(m*k/8) */
#define CMULTIURAG_MAT_NN1_BYTES  28802 /**< Bytes for matrix S (n x N1): ceil(m*n*N1/8) */
#define CMULTIURAG_MAT_NN2_BYTES  31834 /**< Bytes for matrix U (n x N2): ceil(m*n*N2/8) */
#define CMULTIURAG_MAT_N1N2_BYTES 9028  /**< Bytes for matrix V (N1 x N2): ceil(m*N1*N2/8) */

#define SHA512_BYTES 64 /**< Size of SHA2_512 and SHA3_512 outputs */

#define SEEDEXPANDER_SEED_BYTES 64         /**< Seed size of the NGCC seed expander */
#define CMULTIURAG_SALT_BYTES  64     /**< Salt size */
#define SEEDEXPANDER_MAX_LENGTH 4294967295 /**< Max length of the NGCC seed expander */

#endif
