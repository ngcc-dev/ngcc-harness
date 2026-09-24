#pragma once

#include <stdlib.h>
#include <string.h>
#include "tsuov_params.h"
#include "Fql.h"
#include "matrix.h"
#include "mgf.h"
#include "auxfunc.h"

/* =====================================================================
   pseudo random generator (SM3 / auxfunc)
   ===================================================================== */

#define TSUOV_PRG_CTX  MGF_CTX_s
#define PRG_init       PRG_init_sm3
#define PRG_yield      PRG_yield_sm3
#define PRG_final      PRG_final_sm3
#define PRG_copy       PRG_copy_sm3
#define RejSampPRG     RejSampPRG_sm3
#define TSUOV_PRG2_CTX MGF_CTX_s
#define PRG2_init      PRG_init_sm3
#define PRG2_yield     PRG_yield_sm3
#define PRG2_final     PRG_final_sm3
#define PRG2_copy      PRG_copy_sm3

/* =====================================================================
   TSUOV functions
   ===================================================================== */

// SECRET       KEY : (sk_seed,        ,   )
// SIGNING      KEY : (sk_seed, pk_seed,   )
// VERIFICATION KEY : (       , pk_seed, P3)
//

typedef uint8_t TSUOV_SEED [TSUOV_SEED_LEN] ;
typedef uint8_t TSUOV_SALT [TSUOV_SALT_LEN] ;

TYPEDEF_STRUCT(TSUOV_SIGNATURE,
  TSUOV_SALT r  ;
  MATRIX_kxN s  ;
) ;

extern void TSUOV_KeyGen(
  const TSUOV_SEED sk_seed,      // input
  const TSUOV_SEED pk_seed,      // input
  TSUOV_P3         P3            // output
) ;

extern void TSUOV_Sign(
  // ---------------------------------------------------
  const TSUOV_SEED seed_sk,        // input (signing key)
  const TSUOV_SEED seed_pk,        // input (signing key)
  // ---------------------------------------------------
  const TSUOV_SEED seed_v,         // input (random (F_q)^v)
  const TSUOV_SEED seed_r,         // input (random byte)
  const TSUOV_SEED seed_sol,       // input (random (F_q)^*)
  // ---------------------------------------------------
  const uint8_t    message[],      // input (message)
  const size_t     message_length, // input (message)
  // ---------------------------------------------------
  TSUOV_SIGNATURE  sig             // output
) ;

extern int TSUOV_Verify(         // NG:0, OK:1,
  const TSUOV_SEED      pk_seed, // input
  const TSUOV_P3        P3,      // input
                                 //
  const uint8_t         Msg[],   // input
  const size_t          Msg_len, // input
                                 //
  const TSUOV_SIGNATURE sig      // input
) ;

/* =====================================================================
   memory I/O
   ===================================================================== */

inline static void store_TSUOV_SEED (
  const TSUOV_SEED   seed,      // input
  uint8_t          * pool,      // output
  size_t           * pool_bits  // must be 8*n
){
  size_t index = (*pool_bits >> 3) ;
  memcpy(pool+index, seed, TSUOV_SEED_LEN) ;
  *pool_bits += (TSUOV_SEED_LEN<<3) ;
}

inline static void restore_TSUOV_SEED (
  const uint8_t * pool,      // input
  size_t        * pool_bits, // must be 8*n
  TSUOV_SEED      seed       // output
){
  size_t index = (*pool_bits >> 3) ;
  memcpy(seed, pool+index, TSUOV_SEED_LEN) ;
  *pool_bits += (TSUOV_SEED_LEN<<3) ;
}

inline static void store_TSUOV_SALT (
  const TSUOV_SALT   salt,      // input
  uint8_t          * pool,      // output
  size_t           * pool_bits  // must be 8*n
){
  size_t index = (*pool_bits >> 3) ;
  memcpy(pool+index, salt, TSUOV_SALT_LEN) ;
  *pool_bits += (TSUOV_SALT_LEN<<3) ;
}

inline static void restore_TSUOV_SALT (
  const uint8_t * pool,      // input
  size_t        * pool_bits, // must be 8*n
  TSUOV_SALT      salt       // output
){
  size_t index = (*pool_bits >> 3) ;
  memcpy(salt, pool+index, TSUOV_SALT_LEN) ;
  *pool_bits += (TSUOV_SALT_LEN<<3) ;
}

extern void store_TSUOV_P3(
  const TSUOV_P3   P3,        // input
  uint8_t        * pool,      // output
  size_t         * pool_bits  //
);

extern void restore_TSUOV_P3(
  const uint8_t * pool,      // input
  size_t        * pool_bits, // input (current bit index)
  TSUOV_P3        P3         // output
);

inline static void store_TSUOV_SIGNATURE(
  const TSUOV_SIGNATURE   sig,      // input
  uint8_t               * pool,     // output
  size_t                * pool_bits // output (current bit index) // must be 8*n
){
  store_TSUOV_SALT (sig->r, pool, pool_bits) ;
    for(size_t k=0; k<TSUOV_k; k++)
    for(size_t l=0; l<TSUOV_L; l++)
      for(size_t i=0; i<TSUOV_N; i++) 
        store_Fq(sig->s[k][l][i], pool, pool_bits) ;
  return ;
}

inline static void restore_TSUOV_SIGNATURE(
  const uint8_t   * pool,      // input
  size_t          * pool_bits, // input (current bit index) // must be 8*n
  TSUOV_SIGNATURE   sig        // output
){
  restore_TSUOV_SALT (pool, pool_bits, sig->r) ;
  for(size_t k=0; k<TSUOV_k; k++)
    for(size_t l=0; l<TSUOV_L; l++)
      for(size_t i=0; i<TSUOV_N; i++) 
        sig->s[k][l][i] = restore_Fq(pool, pool_bits) ;

  for(size_t i = 0; i < TSUOV_k; i++) {
    for(size_t j = 0; j < TSUOV_L; j++) {
      memset(&(sig->s[i][j][TSUOV_N]), 0,  (BLOCK_ALIGNED_BYTES(TSUOV_N) - TSUOV_N) * sizeof(uint8_t));
    }
  }
  return ;
}
