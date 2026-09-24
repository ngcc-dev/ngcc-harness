/*
 *  SPDX-License-Identifier: MIT
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include "instances.h"
#include "universal_hashing.h"
#include "utils.h"

#include <assert.h>
#include <string.h>

static bf64_t compute_h1(const uint8_t* t, const uint8_t* x, unsigned int lambda,
                         unsigned int ell) {
  const bf64_t b_t = bf64_load(t);

  unsigned int lambda_bytes        = lambda / 8;
  const unsigned int length_lambda = (ell + 3 * lambda - 1) / lambda;

  uint8_t tmp[MAX_LAMBDA_BYTES] = {0};
  memcpy(tmp, x + (length_lambda - 1) * lambda_bytes,
         (ell + lambda) % lambda == 0 ? lambda_bytes : ((ell + lambda) % lambda) / 8);

  bf64_t h1        = bf64_zero();
  bf64_t running_t = bf64_one();
  unsigned int i   = 0;
  for (; i < lambda_bytes; i += 8, running_t = bf64_mul(running_t, b_t)) {
    h1 = bf64_add(h1, bf64_mul(running_t, bf64_load(tmp + (lambda_bytes - i - 8))));
  }
  for (; i < length_lambda * lambda_bytes; i += 8, running_t = bf64_mul(running_t, b_t)) {
    h1 = bf64_add(h1, bf64_mul(running_t, bf64_load(x + (length_lambda * lambda_bytes - i - 8))));
  }

  return h1;
}

void vole_hash_128(uint8_t* h, const uint8_t* sd, const uint8_t* x, unsigned int ell) {
  const uint8_t* r0 = sd;
  const uint8_t* r1 = sd + 1 * BF128_NUM_BYTES;
  const uint8_t* r2 = sd + 2 * BF128_NUM_BYTES;
  const uint8_t* r3 = sd + 3 * BF128_NUM_BYTES;
  const uint8_t* s  = sd + 4 * BF128_NUM_BYTES;
  const uint8_t* t  = sd + 5 * BF128_NUM_BYTES;
  const uint8_t* x1 = x + (ell + 2 * BF128_NUM_BYTES * 8) / 8;

  const unsigned int length_lambda = (ell + 3 * BF128_NUM_BYTES * 8 - 1) / (BF128_NUM_BYTES * 8);

  uint8_t tmp[BF128_NUM_BYTES] = {0};
  memcpy(tmp, x + (length_lambda - 1) * BF128_NUM_BYTES,
         (ell + BF128_NUM_BYTES * 8) % (BF128_NUM_BYTES * 8) == 0
             ? BF128_NUM_BYTES
             : ((ell + BF128_NUM_BYTES * 8) % (BF128_NUM_BYTES * 8)) / 8);
  bf128_t h0;
  bf128_load(&h0, tmp);

  bf128_t b_s;
  bf128_load(&b_s, s);
  bf128_t running_s = b_s;
  for (unsigned int i = 1; i != length_lambda; ++i, bf128_mul_inplace(&running_s, &b_s)) {
    bf128_t xi;
    bf128_load(&xi, x + (length_lambda - 1 - i) * BF128_NUM_BYTES);

    bf128_mul_inplace(&xi, &running_s);
    bf128_add_inplace(&h0, &xi);
  }

  const bf64_t h1 = compute_h1(t, x, BF128_NUM_BYTES * 8, ell);

  bf128_t tmp0;
  bf128_t tmp1;
  bf128_load(&tmp0, r0);
  bf128_load(&tmp1, r1);
  bf128_mul_64_inplace(&tmp1, h1);
  bf128_mul_inplace(&tmp0, &h0);
  bf128_t h2;
  bf128_add(&h2, &tmp0, &tmp1);

  bf128_load(&tmp0, r2);
  bf128_load(&tmp1, r3);
  bf128_mul_64_inplace(&tmp1, h1);
  bf128_mul_inplace(&tmp0, &h0);
  bf128_t h3;
  bf128_add(&h3, &tmp0, &tmp1);

  bf128_store(h, &h2);
  bf128_store(tmp, &h3);
  memcpy(h + BF128_NUM_BYTES, tmp, UNIVERSAL_HASH_B);
  xor_u8_array(h, x1, h, BF128_NUM_BYTES + UNIVERSAL_HASH_B);
}

void vole_hash_192(uint8_t* h, const uint8_t* sd, const uint8_t* x, unsigned int ell) {
  const uint8_t* r0 = sd;
  const uint8_t* r1 = sd + 1 * BF192_NUM_BYTES;
  const uint8_t* r2 = sd + 2 * BF192_NUM_BYTES;
  const uint8_t* r3 = sd + 3 * BF192_NUM_BYTES;
  const uint8_t* s  = sd + 4 * BF192_NUM_BYTES;
  const uint8_t* t  = sd + 5 * BF192_NUM_BYTES;
  const uint8_t* x1 = x + (ell + 2 * BF192_NUM_BYTES * 8) / 8;

  const unsigned int length_lambda = (ell + 3 * BF192_NUM_BYTES * 8 - 1) / (BF192_NUM_BYTES * 8);

  uint8_t tmp[BF192_NUM_BYTES] = {0};
  memcpy(tmp, x + (length_lambda - 1) * BF192_NUM_BYTES,
         (ell + BF192_NUM_BYTES * 8) % (BF192_NUM_BYTES * 8) == 0
             ? BF192_NUM_BYTES
             : ((ell + BF192_NUM_BYTES * 8) % (BF192_NUM_BYTES * 8)) / 8);
  bf192_t h0;
  bf192_load(&h0, tmp);

  bf192_t b_s;
  bf192_load(&b_s, s);
  bf192_t running_s = b_s;
  for (unsigned int i = 1; i != length_lambda; ++i, bf192_mul_inplace(&running_s, &b_s)) {
    bf192_t xi;
    bf192_load(&xi, x + (length_lambda - 1 - i) * BF192_NUM_BYTES);

    bf192_mul_inplace(&xi, &running_s);
    bf192_add_inplace(&h0, &xi);
  }

  const bf64_t h1 = compute_h1(t, x, BF192_NUM_BYTES * 8, ell);

  bf192_t tmp0;
  bf192_t tmp1;
  bf192_load(&tmp0, r0);
  bf192_load(&tmp1, r1);
  bf192_mul_64_inplace(&tmp1, h1);
  bf192_mul_inplace(&tmp0, &h0);
  bf192_t h2;
  bf192_add(&h2, &tmp0, &tmp1);

  bf192_load(&tmp0, r2);
  bf192_load(&tmp1, r3);
  bf192_mul_64_inplace(&tmp1, h1);
  bf192_mul_inplace(&tmp0, &h0);
  bf192_t h3;
  bf192_add(&h3, &tmp0, &tmp1);

  bf192_store(h, &h2);
  bf192_store(tmp, &h3);
  memcpy(h + BF192_NUM_BYTES, tmp, UNIVERSAL_HASH_B);
  xor_u8_array(h, x1, h, BF192_NUM_BYTES + UNIVERSAL_HASH_B);
}

void vole_hash_256(uint8_t* h, const uint8_t* sd, const uint8_t* x, unsigned int ell) {
  const uint8_t* r0 = sd;
  const uint8_t* r1 = sd + 1 * BF256_NUM_BYTES;
  const uint8_t* r2 = sd + 2 * BF256_NUM_BYTES;
  const uint8_t* r3 = sd + 3 * BF256_NUM_BYTES;
  const uint8_t* s  = sd + 4 * BF256_NUM_BYTES;
  const uint8_t* t  = sd + 5 * BF256_NUM_BYTES;
  const uint8_t* x1 = x + (ell + 2 * BF256_NUM_BYTES * 8) / 8;

  const unsigned int length_lambda = (ell + 3 * BF256_NUM_BYTES * 8 - 1) / (BF256_NUM_BYTES * 8);

  uint8_t tmp[BF256_NUM_BYTES] = {0};
  memcpy(tmp, x + (length_lambda - 1) * BF256_NUM_BYTES,
         (ell + BF256_NUM_BYTES * 8) % (BF256_NUM_BYTES * 8) == 0
             ? BF256_NUM_BYTES
             : ((ell + BF256_NUM_BYTES * 8) % (BF256_NUM_BYTES * 8)) / 8);
  bf256_t h0;
  bf256_load(&h0, tmp);

  bf256_t b_s;
  bf256_load(&b_s, s);
  bf256_t running_s = b_s;
  for (unsigned int i = 1; i != length_lambda; ++i, bf256_mul_inplace(&running_s, &b_s)) {
    bf256_t xi;
    bf256_load(&xi, x + (length_lambda - 1 - i) * BF256_NUM_BYTES);

    bf256_mul_inplace(&xi, &running_s);
    bf256_add_inplace(&h0, &xi);
  }

  const bf64_t h1 = compute_h1(t, x, BF256_NUM_BYTES * 8, ell);

  bf256_t tmp0;
  bf256_t tmp1;
  bf256_load(&tmp0, r0);
  bf256_load(&tmp1, r1);
  bf256_mul_64_inplace(&tmp1, h1);
  bf256_mul_inplace(&tmp0, &h0);
  bf256_t h2;
  bf256_add(&h2, &tmp0, &tmp1);

  bf256_load(&tmp0, r2);
  bf256_load(&tmp1, r3);
  bf256_mul_64_inplace(&tmp1, h1);
  bf256_mul_inplace(&tmp0, &h0);
  bf256_t h3;
  bf256_add(&h3, &tmp0, &tmp1);

  bf256_store(h, &h2);
  bf256_store(tmp, &h3);
  memcpy(h + BF256_NUM_BYTES, tmp, UNIVERSAL_HASH_B);
  xor_u8_array(h, x1, h, BF256_NUM_BYTES + UNIVERSAL_HASH_B);
}

void vole_hash(uint8_t* h, const uint8_t* sd, const uint8_t* x, unsigned int ell, uint32_t lambda) {
  switch (lambda) {
  case 256:
    vole_hash_256(h, sd, x, ell);
    break;
  case 192:
    vole_hash_192(h, sd, x, ell);
    break;
  default:
    vole_hash_128(h, sd, x, ell);
    break;
  }
}

/*
 * SYDO reference extensions.
 *
 * SYDO reference universal hashing code only exposes the SYDO 128/192/256-bit
 * variants.  SYDO needs generic 160/256/512-bit field hash helpers for VOLE
 * checking and Quicksilver, so keep those helpers in the universal_hashing
 * module rather than as a separate duplicate source file.
 */
#include "universal_hashing_impl.inc"

void zk_hash_128_init(zk_hash_128_ctx* ctx, const uint8_t* sd) {
  const uint8_t* s = sd + 2 * BF128_NUM_BYTES;
  const uint8_t* t = sd + 3 * BF128_NUM_BYTES;

  ctx->h0 = bf128_zero();
  ctx->h1 = bf128_zero();
  bf128_load(&ctx->s, s);
  ctx->t  = bf64_load(t);
  ctx->sd = sd;
}

void zk_hash_128_update(zk_hash_128_ctx* ctx, const bf128_t* v) {
  bf128_mul_inplace(&ctx->h0, &ctx->s);
  bf128_add_inplace(&ctx->h0, v);
  bf128_mul_64_inplace(&ctx->h1, ctx->t);
  bf128_add_inplace(&ctx->h1, v);
}

void zk_hash_128_finalize(uint8_t* h, zk_hash_128_ctx* ctx, const bf128_t* x1) {
  bf128_t r0;
  bf128_t r1;
  bf128_load(&r0, ctx->sd);
  bf128_load(&r1, ctx->sd + BF128_NUM_BYTES);

  bf128_mul_inplace(&r0, &ctx->h0);
  bf128_mul_inplace(&r1, &ctx->h1);

  bf128_add_inplace(&r0, &r1);
  bf128_add_inplace(&r0, x1);
  bf128_store(h, &r0);
}

void zk_hash_128_3_init(zk_hash_128_3_ctx* ctx, const uint8_t* sd) {
  const uint8_t* s = sd + 2 * BF128_NUM_BYTES;
  const uint8_t* t = sd + 3 * BF128_NUM_BYTES;

  ctx->h0[0] = bf128_zero();
  ctx->h0[1] = bf128_zero();
  ctx->h0[2] = bf128_zero();
  ctx->h1[0] = bf128_zero();
  ctx->h1[1] = bf128_zero();
  ctx->h1[2] = bf128_zero();
  bf128_load(&ctx->s, s);
  ctx->t  = bf64_load(t);
  ctx->sd = sd;
}

void zk_hash_128_3_update(zk_hash_128_3_ctx* ctx, const bf128_t* v_0, const bf128_t* v_1,
                          const bf128_t* v_2) {
  bf128_mul_inplace(&ctx->h0[0], &ctx->s);
  bf128_add_inplace(&ctx->h0[0], v_0);
  bf128_mul_64_inplace(&ctx->h1[0], ctx->t);
  bf128_add_inplace(&ctx->h1[0], v_0);
  bf128_mul_inplace(&ctx->h0[1], &ctx->s);
  bf128_add_inplace(&ctx->h0[1], v_1);
  bf128_mul_64_inplace(&ctx->h1[1], ctx->t);
  bf128_add_inplace(&ctx->h1[1], v_1);
  bf128_mul_inplace(&ctx->h0[2], &ctx->s);
  bf128_add_inplace(&ctx->h0[2], v_2);
  bf128_mul_64_inplace(&ctx->h1[2], ctx->t);
  bf128_add_inplace(&ctx->h1[2], v_2);
}

void zk_hash_128_3_raise_and_update(zk_hash_128_3_ctx* ctx, const bf128_t* v_1,
                                    const bf128_t* v_2) {
  const bf128_t zero = bf128_zero();
  zk_hash_128_3_update(ctx, &zero, v_1, v_2);
}

void zk_hash_128_3_finalize(uint8_t* h_0, uint8_t* h_1, uint8_t* h_2, zk_hash_128_3_ctx* ctx,
                            const bf128_t* x1_0, const bf128_t* x1_1, const bf128_t* x1_2) {
  bf128_t r0;
  bf128_t r1;
  bf128_load(&r0, ctx->sd);
  bf128_load(&r1, ctx->sd + BF128_NUM_BYTES);

  bf128_t t0;
  bf128_t t1;
  bf128_mul(&t0, &r0, &ctx->h0[0]);
  bf128_mul(&t1, &r1, &ctx->h1[0]);
  bf128_add_inplace(&t0, &t1);
  bf128_add_inplace(&t0, x1_0);
  bf128_store(h_0, &t0);
  bf128_mul(&t0, &r0, &ctx->h0[1]);
  bf128_mul(&t1, &r1, &ctx->h1[1]);
  bf128_add_inplace(&t0, &t1);
  bf128_add_inplace(&t0, x1_1);
  bf128_store(h_1, &t0);
  bf128_mul(&t0, &r0, &ctx->h0[2]);
  bf128_mul(&t1, &r1, &ctx->h1[2]);
  bf128_add_inplace(&t0, &t1);
  bf128_add_inplace(&t0, x1_2);
  bf128_store(h_2, &t0);
}

void zk_hash_192_init(zk_hash_192_ctx* ctx, const uint8_t* sd) {
  const uint8_t* s = sd + 2 * BF192_NUM_BYTES;
  const uint8_t* t = sd + 3 * BF192_NUM_BYTES;

  ctx->h0 = bf192_zero();
  ctx->h1 = bf192_zero();
  bf192_load(&ctx->s, s);
  ctx->t  = bf64_load(t);
  ctx->sd = sd;
}

void zk_hash_192_update(zk_hash_192_ctx* ctx, const bf192_t* v) {
  bf192_mul_inplace(&ctx->h0, &ctx->s);
  bf192_add_inplace(&ctx->h0, v);
  bf192_mul_64_inplace(&ctx->h1, ctx->t);
  bf192_add_inplace(&ctx->h1, v);
}

void zk_hash_192_finalize(uint8_t* h, zk_hash_192_ctx* ctx, const bf192_t* x1) {
  bf192_t r0;
  bf192_t r1;
  bf192_load(&r0, ctx->sd);
  bf192_load(&r1, ctx->sd + BF192_NUM_BYTES);

  bf192_mul_inplace(&r0, &ctx->h0);
  bf192_mul_inplace(&r1, &ctx->h1);

  bf192_add_inplace(&r0, &r1);
  bf192_add_inplace(&r0, x1);
  bf192_store(h, &r0);
}

void zk_hash_192_3_init(zk_hash_192_3_ctx* ctx, const uint8_t* sd) {
  const uint8_t* s = sd + 2 * BF192_NUM_BYTES;
  const uint8_t* t = sd + 3 * BF192_NUM_BYTES;

  ctx->h0[0] = bf192_zero();
  ctx->h0[1] = bf192_zero();
  ctx->h0[2] = bf192_zero();
  ctx->h1[0] = bf192_zero();
  ctx->h1[1] = bf192_zero();
  ctx->h1[2] = bf192_zero();
  bf192_load(&ctx->s, s);
  ctx->t  = bf64_load(t);
  ctx->sd = sd;
}

void zk_hash_192_3_update(zk_hash_192_3_ctx* ctx, const bf192_t* v_0, const bf192_t* v_1,
                          const bf192_t* v_2) {
  bf192_mul_inplace(&ctx->h0[0], &ctx->s);
  bf192_add_inplace(&ctx->h0[0], v_0);
  bf192_mul_64_inplace(&ctx->h1[0], ctx->t);
  bf192_add_inplace(&ctx->h1[0], v_0);
  bf192_mul_inplace(&ctx->h0[1], &ctx->s);
  bf192_add_inplace(&ctx->h0[1], v_1);
  bf192_mul_64_inplace(&ctx->h1[1], ctx->t);
  bf192_add_inplace(&ctx->h1[1], v_1);
  bf192_mul_inplace(&ctx->h0[2], &ctx->s);
  bf192_add_inplace(&ctx->h0[2], v_2);
  bf192_mul_64_inplace(&ctx->h1[2], ctx->t);
  bf192_add_inplace(&ctx->h1[2], v_2);
}

void zk_hash_192_3_raise_and_update(zk_hash_192_3_ctx* ctx, const bf192_t* v_1,
                                    const bf192_t* v_2) {
  const bf192_t zero = bf192_zero();
  zk_hash_192_3_update(ctx, &zero, v_1, v_2);
}

void zk_hash_192_3_finalize(uint8_t* h_0, uint8_t* h_1, uint8_t* h_2, zk_hash_192_3_ctx* ctx,
                            const bf192_t* x1_0, const bf192_t* x1_1, const bf192_t* x1_2) {
  bf192_t r0;
  bf192_t r1;
  bf192_load(&r0, ctx->sd);
  bf192_load(&r1, ctx->sd + BF192_NUM_BYTES);

  bf192_t t0;
  bf192_t t1;
  bf192_mul(&t0, &r0, &ctx->h0[0]);
  bf192_mul(&t1, &r1, &ctx->h1[0]);
  bf192_add_inplace(&t0, &t1);
  bf192_add_inplace(&t0, x1_0);
  bf192_store(h_0, &t0);
  bf192_mul(&t0, &r0, &ctx->h0[1]);
  bf192_mul(&t1, &r1, &ctx->h1[1]);
  bf192_add_inplace(&t0, &t1);
  bf192_add_inplace(&t0, x1_1);
  bf192_store(h_1, &t0);
  bf192_mul(&t0, &r0, &ctx->h0[2]);
  bf192_mul(&t1, &r1, &ctx->h1[2]);
  bf192_add_inplace(&t0, &t1);
  bf192_add_inplace(&t0, x1_2);
  bf192_store(h_2, &t0);
}

void zk_hash_256_init(zk_hash_256_ctx* ctx, const uint8_t* sd) {
  const uint8_t* s = sd + 2 * BF256_NUM_BYTES;
  const uint8_t* t = sd + 3 * BF256_NUM_BYTES;

  ctx->h0 = bf256_zero();
  ctx->h1 = bf256_zero();
  bf256_load(&ctx->s, s);
  ctx->t  = bf64_load(t);
  ctx->sd = sd;
}

void zk_hash_256_update(zk_hash_256_ctx* ctx, const bf256_t* v) {
  bf256_mul_inplace(&ctx->h0, &ctx->s);
  bf256_add_inplace(&ctx->h0, v);
  bf256_mul_64_inplace(&ctx->h1, ctx->t);
  bf256_add_inplace(&ctx->h1, v);
}

void zk_hash_256_finalize(uint8_t* h, zk_hash_256_ctx* ctx, const bf256_t* x1) {
  bf256_t r0;
  bf256_t r1;
  bf256_load(&r0, ctx->sd);
  bf256_load(&r1, ctx->sd + BF256_NUM_BYTES);

  bf256_mul_inplace(&r0, &ctx->h0);
  bf256_mul_inplace(&r1, &ctx->h1);

  bf256_add_inplace(&r0, &r1);
  bf256_add_inplace(&r0, x1);
  bf256_store(h, &r0);
}

void zk_hash_256_3_init(zk_hash_256_3_ctx* ctx, const uint8_t* sd) {
  const uint8_t* s = sd + 2 * BF256_NUM_BYTES;
  const uint8_t* t = sd + 3 * BF256_NUM_BYTES;

  ctx->h0[0] = bf256_zero();
  ctx->h0[1] = bf256_zero();
  ctx->h0[2] = bf256_zero();
  ctx->h1[0] = bf256_zero();
  ctx->h1[1] = bf256_zero();
  ctx->h1[2] = bf256_zero();
  bf256_load(&ctx->s, s);
  ctx->t  = bf64_load(t);
  ctx->sd = sd;
}

void zk_hash_256_3_update(zk_hash_256_3_ctx* ctx, const bf256_t* v_0, const bf256_t* v_1,
                          const bf256_t* v_2) {
  bf256_mul_inplace(&ctx->h0[0], &ctx->s);
  bf256_add_inplace(&ctx->h0[0], v_0);
  bf256_mul_64_inplace(&ctx->h1[0], ctx->t);
  bf256_add_inplace(&ctx->h1[0], v_0);
  bf256_mul_inplace(&ctx->h0[1], &ctx->s);
  bf256_add_inplace(&ctx->h0[1], v_1);
  bf256_mul_64_inplace(&ctx->h1[1], ctx->t);
  bf256_add_inplace(&ctx->h1[1], v_1);
  bf256_mul_inplace(&ctx->h0[2], &ctx->s);
  bf256_add_inplace(&ctx->h0[2], v_2);
  bf256_mul_64_inplace(&ctx->h1[2], ctx->t);
  bf256_add_inplace(&ctx->h1[2], v_2);
}

void zk_hash_256_3_raise_and_update(zk_hash_256_3_ctx* ctx, const bf256_t* v_1,
                                    const bf256_t* v_2) {
  const bf256_t zero = bf256_zero();
  zk_hash_256_3_update(ctx, &zero, v_1, v_2);
}

void zk_hash_256_3_finalize(uint8_t* h_0, uint8_t* h_1, uint8_t* h_2, zk_hash_256_3_ctx* ctx,
                            const bf256_t* x1_0, const bf256_t* x1_1, const bf256_t* x1_2) {
  bf256_t r0;
  bf256_t r1;
  bf256_load(&r0, ctx->sd);
  bf256_load(&r1, ctx->sd + BF256_NUM_BYTES);

  bf256_t t0;
  bf256_t t1;
  bf256_mul(&t0, &r0, &ctx->h0[0]);
  bf256_mul(&t1, &r1, &ctx->h1[0]);
  bf256_add_inplace(&t0, &t1);
  bf256_add_inplace(&t0, x1_0);
  bf256_store(h_0, &t0);
  bf256_mul(&t0, &r0, &ctx->h0[1]);
  bf256_mul(&t1, &r1, &ctx->h1[1]);
  bf256_add_inplace(&t0, &t1);
  bf256_add_inplace(&t0, x1_1);
  bf256_store(h_1, &t0);
  bf256_mul(&t0, &r0, &ctx->h0[2]);
  bf256_mul(&t1, &r1, &ctx->h1[2]);
  bf256_add_inplace(&t0, &t1);
  bf256_add_inplace(&t0, x1_2);
  bf256_store(h_2, &t0);
}

void leaf_hash_128(uint8_t* h, const uint8_t* uhash, const uint8_t* x) {
  bf128_t x0;
  bf384_t x1;
  bf384_t u;
  bf128_load(&x0, x);
  bf384_load(&x1, x + BF128_NUM_BYTES);
  bf384_load(&u, uhash);

  bf384_mul_128_inplace(&u, &x0);
  bf384_add_inplace(&u, &x1);
  bf384_store(h, &u);
}

void leaf_hash_192(uint8_t* h, const uint8_t* uhash, const uint8_t* x) {
  bf192_t x0;
  bf576_t x1;
  bf576_t u;
  bf192_load(&x0, x);
  bf576_load(&x1, x + BF192_NUM_BYTES);
  bf576_load(&u, uhash);

  bf576_mul_192_inplace(&u, &x0);
  bf576_add_inplace(&u, &x1);
  bf576_store(h, &u);
}

void leaf_hash_256(uint8_t* h, const uint8_t* uhash, const uint8_t* x) {
  bf256_t x0;
  bf768_t x1;
  bf768_t u;
  bf256_load(&x0, x);
  bf768_load(&x1, x + BF256_NUM_BYTES);
  bf768_load(&u, uhash);

  bf768_mul_256_inplace(&u, &x0);
  bf768_add_inplace(&u, &x1);
  bf768_store(h, &u);
}
