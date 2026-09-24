/*
 * Universal hashing (512-only variant) for vistrutith_d3_512s.
 */

#ifndef UNIVERSAL_HASHING_H
#define UNIVERSAL_HASHING_H

#include <stdint.h>

#include "fields.h"

void vole_hash_512(uint8_t* h, const uint8_t* sd, const uint8_t* x, unsigned int ell);
void vole_hash_512_2(uint8_t* h0, uint8_t* h1, const uint8_t* sd, const uint8_t* x0,
                     const uint8_t* x1, unsigned int ell);
void vole_hash_512_4(uint8_t* h0, uint8_t* h1, uint8_t* h2, uint8_t* h3, const uint8_t* sd,
                     const uint8_t* x0, const uint8_t* x1, const uint8_t* x2, const uint8_t* x3,
                     unsigned int ell);

// Compatibility wrappers retained for call sites that still use generic names.
void vole_hash(uint8_t* h, const uint8_t* sd, const uint8_t* x, unsigned int ell, uint32_t lambda);
void vole_hash_2(uint8_t* h0, uint8_t* h1, const uint8_t* sd, const uint8_t* x0,
                 const uint8_t* x1, unsigned int ell, uint32_t lambda);
void vole_hash_4(uint8_t* h0, uint8_t* h1, uint8_t* h2, uint8_t* h3, const uint8_t* sd,
                 const uint8_t* x0, const uint8_t* x1, const uint8_t* x2, const uint8_t* x3,
                 unsigned int ell, uint32_t lambda);

typedef struct {
  const uint8_t* sd;
  unsigned int ell;
  unsigned int length_lambda;
  size_t last_bytes;
  size_t total_words;
  bf64_t* t_pows;
  bf512_t* s_pows_512;
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
  bf512_t h0;
  bf512_t h1;
  bf512_t s;
  bf512_t s2;
  bf64_t t;
  const uint8_t* sd;
  bf512_t pending_v;
  int pending;
} zk_hash_512_ctx;

void zk_hash_512_init(zk_hash_512_ctx* ctx, const uint8_t* sd);
void zk_hash_512_update(zk_hash_512_ctx* ctx, bf512_t v);
void zk_hash_512_finalize(uint8_t* h, zk_hash_512_ctx* ctx, bf512_t x1);

typedef struct {
  bf512_t h0[2];
  bf512_t h1[2];
  bf512_t s;
  bf512_t s2;
  bf64_t t;
  const uint8_t* sd;
  bf512_t pending_v_0;
  bf512_t pending_v_1;
  int pending;
} zk_hash_512_2_ctx;

void zk_hash_512_2_init(zk_hash_512_2_ctx* ctx, const uint8_t* sd);
void zk_hash_512_2_update(zk_hash_512_2_ctx* ctx, bf512_t v_0, bf512_t v_1);
void zk_hash_512_2_raise_and_update(zk_hash_512_2_ctx* ctx, bf512_t v_1);
void zk_hash_512_2_finalize(uint8_t* h_0, uint8_t* h_1, zk_hash_512_2_ctx* ctx, bf512_t x1_0,
                            bf512_t x1_1);

typedef struct {
  bf512_t h0[3];
  bf512_t h1[3];
  bf512_t s;
  bf512_t s2;
  bf64_t t;
  const uint8_t* sd;
  bf512_t pending_v_0;
  bf512_t pending_v_1;
  bf512_t pending_v_2;
  int pending;
} zk_hash_512_3_ctx;

void zk_hash_512_3_init(zk_hash_512_3_ctx* ctx, const uint8_t* sd);
void zk_hash_512_3_update(zk_hash_512_3_ctx* ctx, bf512_t v_0, bf512_t v_1, bf512_t v_2);
void zk_hash_512_3_raise_and_update(zk_hash_512_3_ctx* ctx, bf512_t v_1, bf512_t v_2);
void zk_hash_512_3_finalize(uint8_t* h_0, uint8_t* h_1, uint8_t* h_2, zk_hash_512_3_ctx* ctx,
                            bf512_t x1_0, bf512_t x1_1, bf512_t x1_2);

#endif
