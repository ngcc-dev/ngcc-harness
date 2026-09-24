/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "SIG_AlgorithmInstance.h"
#include "drng.h"
#include <string.h>
#include "tsuov.h"

// DRNG_ctx for generating pseudorandom numbers within the SIG scheme
extern DRNG_ctx drng_algorithm;

static int sig_random_seed(TSUOV_SEED seed)
{
    return get_random_number(&drng_algorithm, seed, (unsigned long long)TSUOV_SEED_LEN * 8);
}

unsigned long long sig_get_pk_len_bytes()
{
	return CRYPTO_PUBLICKEYBYTES;
}

unsigned long long sig_get_sk_len_bytes()
{
	return CRYPTO_SECRETKEYBYTES;
}

unsigned long long sig_get_sn_len_bytes()
{
	return CRYPTO_BYTES;
}

int sig_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
    TSUOV_SEED seed_sk ;
    TSUOV_SEED seed_pk ;
    static TSUOV_P3 P3 ; // to avoid huge array in stack,
                         // (but problematic in mult-thread environment)

    if (sig_random_seed(seed_sk) != 0) return -1;
    if (sig_random_seed(seed_pk) != 0) return -1;
    
    TSUOV_KeyGen (seed_sk, seed_pk, P3) ;
  
    size_t sk_pool_bits = 0 ;
    store_TSUOV_SEED(seed_sk, sk, &sk_pool_bits) ;
    store_TSUOV_SEED(seed_pk, sk, &sk_pool_bits) ;
    *sk_len_bytes = CRYPTO_SECRETKEYBYTES;
    size_t pk_pool_bits = 0 ;
    store_TSUOV_SEED(seed_pk, pk, &pk_pool_bits) ;
    store_TSUOV_P3  (P3,      pk, &pk_pool_bits) ;
    *pk_len_bytes = CRYPTO_PUBLICKEYBYTES;
    return 0 ;
}

int sig_sign(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *m, unsigned long long m_len_bytes,
	unsigned char *sn, unsigned long long *sn_len_bytes)
{
    size_t sk_pool_bits = 0 ;
    TSUOV_SEED seed_sk      ; restore_TSUOV_SEED(sk, &sk_pool_bits, seed_sk) ; // gen sk
  
    TSUOV_SEED seed_pk      ; restore_TSUOV_SEED(sk, &sk_pool_bits, seed_pk) ; // gen pk
    
    TSUOV_SEED seed_v       ;
    TSUOV_SEED seed_r       ;
    TSUOV_SEED seed_sol     ;
    if (sig_random_seed(seed_v)   != 0) return -1;
    if (sig_random_seed(seed_r)   != 0) return -1;
    if (sig_random_seed(seed_sol) != 0) return -1;
  
    TSUOV_SIGNATURE sig     ;
    TSUOV_Sign(seed_sk, seed_pk, seed_v, seed_r, seed_sol, m, m_len_bytes, sig) ;
  
    size_t sig_pool_bits = 0 ;
    store_TSUOV_SIGNATURE(sig, sn, &sig_pool_bits) ;
  

    *sn_len_bytes = CRYPTO_BYTES ;
  
    return 0 ;
}

int sig_verify(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *sn, unsigned long long sn_len_bytes,
	unsigned char *m, unsigned long long m_len_bytes)
{
    size_t pk_pool_bits = 0 ;
    TSUOV_SEED seed_pk      ; restore_TSUOV_SEED(pk, &pk_pool_bits, seed_pk) ;
    static TSUOV_P3 P3      ; // to avoid huge array in stack
                              // (but problematic in mult-thread environment)
  
    restore_TSUOV_P3(pk, &pk_pool_bits, P3) ;
  
    size_t sig_pool_bits = 0 ;
    TSUOV_SIGNATURE sig ; restore_TSUOV_SIGNATURE(sn, &sig_pool_bits, sig) ;
  
    if(TSUOV_Verify(seed_pk, P3, m, m_len_bytes, sig)){
      return 0;
    }else{
      return -1;
    }
}
