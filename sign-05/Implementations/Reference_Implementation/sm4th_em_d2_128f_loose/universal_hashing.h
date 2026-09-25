/*
 *  This file implements the universal hashing functions.
 * We use multiple lanes of hashing to amortize the cost of field multiplications, and also provide precomputation for the case where the same seed is used for multiple hashes.
 */

#ifndef UNIVERSAL_HASHING_H
#define UNIVERSAL_HASHING_H

#include <stdint.h>

#include "macros.h"
#include "fields.h"

void vole_hash(uint8_t* h, const uint8_t* sd, const uint8_t* x, unsigned int ell, uint32_t lambda);
void vole_hash_2(uint8_t* h0, uint8_t* h1, const uint8_t* sd, const uint8_t* x0,
                 const uint8_t* x1, unsigned int ell, uint32_t lambda);
void vole_hash_4(uint8_t* h0, uint8_t* h1, uint8_t* h2, uint8_t* h3,
                 const uint8_t* sd, const uint8_t* x0, const uint8_t* x1,
                 const uint8_t* x2, const uint8_t* x3, unsigned int ell, uint32_t lambda);

typedef struct {
  const uint8_t* sd;
  uint32_t lambda;
  unsigned int ell;
  unsigned int length_lambda;
  unsigned int lambda_bytes;
  size_t total_words;
  size_t last_bytes;
  bf64_t* t_pows;
  bf128_t* s_pows_128;
} vole_hash_precomp_t;

int vole_hash_precompute_init(vole_hash_precomp_t* ctx, const uint8_t* sd, unsigned int ell,
                              uint32_t lambda);
void vole_hash_precompute_clear(vole_hash_precomp_t* ctx);
void vole_hash_precomp(uint8_t* h0, const vole_hash_precomp_t* ctx, const uint8_t* x0);
void vole_hash_2_precomp(uint8_t* h0, uint8_t* h1, const vole_hash_precomp_t* ctx,
                         const uint8_t* x0, const uint8_t* x1);
void vole_hash_4_precomp(uint8_t* h0, uint8_t* h1, uint8_t* h2, uint8_t* h3,
                         const vole_hash_precomp_t* ctx, const uint8_t* x0, const uint8_t* x1,
                         const uint8_t* x2, const uint8_t* x3);

typedef struct {
  bf128_t h0;
  bf128_t h1;
  bf128_t s;
  bf128_t s2;       /* s^2, precomputed in init */
  bf64_t t;
  const uint8_t* sd;
  bf128_t pending_v;/* buffered value waiting for the next call */
  int pending;      /* 1 if pending_v holds an unconsumed value */
} zk_hash_128_ctx;

void zk_hash_128_init(zk_hash_128_ctx* ctx, const uint8_t* sd);
void zk_hash_128_update(zk_hash_128_ctx* ctx, bf128_t v);
void zk_hash_128_finalize(uint8_t* h, zk_hash_128_ctx* ctx, bf128_t x1);

typedef struct {
  bf128_t h0[2];
  bf128_t h1[2];
  bf128_t s;
  bf128_t s2;
  bf64_t t;
  const uint8_t* sd;
  bf128_t pending_v_0;
  bf128_t pending_v_1;
  int pending;
} zk_hash_128_2_ctx;

void zk_hash_128_2_init(zk_hash_128_2_ctx* ctx, const uint8_t* sd);
void zk_hash_128_2_update(zk_hash_128_2_ctx* ctx, bf128_t v_0, bf128_t v_1);
void zk_hash_128_2_raise_and_update(zk_hash_128_2_ctx* ctx, bf128_t v_1);
void zk_hash_128_2_finalize(uint8_t* h_0, uint8_t* h_1, zk_hash_128_2_ctx* ctx, bf128_t x1_0,
                            bf128_t x1_1);

typedef struct {
  bf128_t h0[3];
  bf128_t h1[3];
  bf128_t s;
  bf128_t s2;
  bf64_t t;
  const uint8_t* sd;
  bf128_t pending_v_0;
  bf128_t pending_v_1;
  bf128_t pending_v_2;
  int pending;
} zk_hash_128_3_ctx;

void zk_hash_128_3_init(zk_hash_128_3_ctx* ctx, const uint8_t* sd);
void zk_hash_128_3_update(zk_hash_128_3_ctx* ctx, bf128_t v_0, bf128_t v_1, bf128_t v_2);
void zk_hash_128_3_raise_and_update(zk_hash_128_3_ctx* ctx, bf128_t v_1, bf128_t v_2);
void zk_hash_128_3_finalize(uint8_t* h_0, uint8_t* h_1, uint8_t* h_2,
                            zk_hash_128_3_ctx* ctx, bf128_t x1_0, bf128_t x1_1,
                            bf128_t x1_2);


#endif
