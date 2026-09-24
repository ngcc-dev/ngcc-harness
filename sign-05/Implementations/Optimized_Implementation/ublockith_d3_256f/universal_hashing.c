#include "params.h"
#include "universal_hashing.h"
#include "utils.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static bf64_t compute_h1(const uint8_t* t, const uint8_t* x, unsigned int lambda,
                         unsigned int ell) {
  const bf64_t b_t = bf64_load(t);

  const unsigned int lambda_bytes  = lambda / 8;
  const unsigned int length_lambda = (ell + 3 * lambda - 1) / lambda;
  assert(lambda_bytes <= BF256_NUM_BYTES);

  uint8_t tmp[BF256_NUM_BYTES] = {0};
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

static inline void compute_h1_2(const uint8_t* t, const uint8_t* x0, const uint8_t* x1,
                                unsigned int lambda, unsigned int ell, bf64_t* h1_0,
                                bf64_t* h1_1) {
  const bf64_t b_t = bf64_load(t);

  const unsigned int lambda_bytes  = lambda / 8;
  const unsigned int length_lambda = (ell + 3 * lambda - 1) / lambda;
  assert(lambda_bytes <= BF256_NUM_BYTES);

  uint8_t tmp0[BF256_NUM_BYTES] = {0};
  uint8_t tmp1[BF256_NUM_BYTES] = {0};
  const size_t tail_bytes =
      (ell + lambda) % lambda == 0 ? lambda_bytes : ((ell + lambda) % lambda) / 8;
  memcpy(tmp0, x0 + (length_lambda - 1) * lambda_bytes, tail_bytes);
  memcpy(tmp1, x1 + (length_lambda - 1) * lambda_bytes, tail_bytes);

  bf64_t h0       = bf64_zero();
  bf64_t h1       = bf64_zero();
  bf64_t running_t = bf64_one();
  unsigned int i  = 0;
  for (; i < lambda_bytes; i += 8, running_t = bf64_mul(running_t, b_t)) {
    const size_t off = lambda_bytes - i - 8;
    h0 = bf64_add(h0, bf64_mul(running_t, bf64_load(tmp0 + off)));
    h1 = bf64_add(h1, bf64_mul(running_t, bf64_load(tmp1 + off)));
  }
  for (; i < length_lambda * lambda_bytes; i += 8, running_t = bf64_mul(running_t, b_t)) {
    const size_t off = length_lambda * lambda_bytes - i - 8;
    h0 = bf64_add(h0, bf64_mul(running_t, bf64_load(x0 + off)));
    h1 = bf64_add(h1, bf64_mul(running_t, bf64_load(x1 + off)));
  }

  *h1_0 = h0;
  *h1_1 = h1;
}

static inline void compute_h1_4(const uint8_t* t, const uint8_t* x0, const uint8_t* x1,
                                const uint8_t* x2, const uint8_t* x3, unsigned int lambda,
                                unsigned int ell, bf64_t* h1_0, bf64_t* h1_1,
                                bf64_t* h1_2, bf64_t* h1_3) {
  const bf64_t b_t = bf64_load(t);

  const unsigned int lambda_bytes  = lambda / 8;
  const unsigned int length_lambda = (ell + 3 * lambda - 1) / lambda;
  assert(lambda_bytes <= BF256_NUM_BYTES);

  uint8_t tmp0[BF256_NUM_BYTES] = {0};
  uint8_t tmp1[BF256_NUM_BYTES] = {0};
  uint8_t tmp2[BF256_NUM_BYTES] = {0};
  uint8_t tmp3[BF256_NUM_BYTES] = {0};
  const size_t tail_bytes =
      (ell + lambda) % lambda == 0 ? lambda_bytes : ((ell + lambda) % lambda) / 8;
  memcpy(tmp0, x0 + (length_lambda - 1) * lambda_bytes, tail_bytes);
  memcpy(tmp1, x1 + (length_lambda - 1) * lambda_bytes, tail_bytes);
  memcpy(tmp2, x2 + (length_lambda - 1) * lambda_bytes, tail_bytes);
  memcpy(tmp3, x3 + (length_lambda - 1) * lambda_bytes, tail_bytes);

  bf64_t h0 = bf64_zero();
  bf64_t h1 = bf64_zero();
  bf64_t h2 = bf64_zero();
  bf64_t h3 = bf64_zero();
  bf64_t running_t = bf64_one();
  unsigned int i = 0;
  for (; i < lambda_bytes; i += 8, running_t = bf64_mul(running_t, b_t)) {
    const size_t off = lambda_bytes - i - 8;
    h0 = bf64_add(h0, bf64_mul(running_t, bf64_load(tmp0 + off)));
    h1 = bf64_add(h1, bf64_mul(running_t, bf64_load(tmp1 + off)));
    h2 = bf64_add(h2, bf64_mul(running_t, bf64_load(tmp2 + off)));
    h3 = bf64_add(h3, bf64_mul(running_t, bf64_load(tmp3 + off)));
  }
  for (; i < length_lambda * lambda_bytes; i += 8, running_t = bf64_mul(running_t, b_t)) {
    const size_t off = length_lambda * lambda_bytes - i - 8;
    h0 = bf64_add(h0, bf64_mul(running_t, bf64_load(x0 + off)));
    h1 = bf64_add(h1, bf64_mul(running_t, bf64_load(x1 + off)));
    h2 = bf64_add(h2, bf64_mul(running_t, bf64_load(x2 + off)));
    h3 = bf64_add(h3, bf64_mul(running_t, bf64_load(x3 + off)));
  }

  *h1_0 = h0;
  *h1_1 = h1;
  *h1_2 = h2;
  *h1_3 = h3;
}

static inline void compute_h1_4_precomp(const vole_hash_precomp_t* ctx, const uint8_t* x0,
                                        const uint8_t* x1, const uint8_t* x2, const uint8_t* x3,
                                        bf64_t* h1_0, bf64_t* h1_1, bf64_t* h1_2, bf64_t* h1_3) {
  const unsigned int lambda_bytes = ctx->lambda_bytes;
  const unsigned int length_lambda = ctx->length_lambda;

  uint8_t tmp0[BF256_NUM_BYTES] = {0};
  uint8_t tmp1[BF256_NUM_BYTES] = {0};
  uint8_t tmp2[BF256_NUM_BYTES] = {0};
  uint8_t tmp3[BF256_NUM_BYTES] = {0};
  memcpy(tmp0, x0 + (size_t)(length_lambda - 1u) * lambda_bytes, ctx->last_bytes);
  memcpy(tmp1, x1 + (size_t)(length_lambda - 1u) * lambda_bytes, ctx->last_bytes);
  memcpy(tmp2, x2 + (size_t)(length_lambda - 1u) * lambda_bytes, ctx->last_bytes);
  memcpy(tmp3, x3 + (size_t)(length_lambda - 1u) * lambda_bytes, ctx->last_bytes);

  bf64_t h0 = bf64_zero();
  bf64_t h1 = bf64_zero();
  bf64_t h2 = bf64_zero();
  bf64_t h3 = bf64_zero();
  size_t word_idx = 0;

  for (unsigned int off = lambda_bytes; off != 0u; off -= 8u, ++word_idx) {
    const size_t woff = (size_t)off - 8u;
    const bf64_t coeff = ctx->t_pows[word_idx];
    h0 = bf64_add(h0, bf64_mul(coeff, bf64_load(tmp0 + woff)));
    h1 = bf64_add(h1, bf64_mul(coeff, bf64_load(tmp1 + woff)));
    h2 = bf64_add(h2, bf64_mul(coeff, bf64_load(tmp2 + woff)));
    h3 = bf64_add(h3, bf64_mul(coeff, bf64_load(tmp3 + woff)));
  }

  for (unsigned int blk = 1; blk < length_lambda; ++blk) {
    const size_t boff = (size_t)(length_lambda - 1u - blk) * lambda_bytes;
    const uint8_t* b0 = x0 + boff;
    const uint8_t* b1 = x1 + boff;
    const uint8_t* b2 = x2 + boff;
    const uint8_t* b3 = x3 + boff;
    for (unsigned int off = lambda_bytes; off != 0u; off -= 8u, ++word_idx) {
      const size_t woff = (size_t)off - 8u;
      const bf64_t coeff = ctx->t_pows[word_idx];
      h0 = bf64_add(h0, bf64_mul(coeff, bf64_load(b0 + woff)));
      h1 = bf64_add(h1, bf64_mul(coeff, bf64_load(b1 + woff)));
      h2 = bf64_add(h2, bf64_mul(coeff, bf64_load(b2 + woff)));
      h3 = bf64_add(h3, bf64_mul(coeff, bf64_load(b3 + woff)));
    }
  }

  *h1_0 = h0;
  *h1_1 = h1;
  *h1_2 = h2;
  *h1_3 = h3;
}

int vole_hash_precompute_init(vole_hash_precomp_t* ctx, const uint8_t* sd, unsigned int ell,
                              uint32_t lambda) {
  if (!ctx || !sd) {
    return 0;
  }

  memset(ctx, 0, sizeof(*ctx));
  ctx->sd = sd;
  ctx->lambda = (lambda == 256u) ? 256u : 128u;
  ctx->ell = ell;
  ctx->lambda_bytes = ctx->lambda / 8u;
  ctx->length_lambda = (ell + 3u * ctx->lambda - 1u) / ctx->lambda;
  ctx->last_bytes = ((ell + ctx->lambda) % ctx->lambda == 0u)
                        ? ctx->lambda_bytes
                        : ((ell + ctx->lambda) % ctx->lambda) / 8u;
  ctx->total_words = ((size_t)ctx->length_lambda * ctx->lambda_bytes) / 8u;

  ctx->t_pows = malloc(ctx->total_words * sizeof(*ctx->t_pows));
  if (!ctx->t_pows) {
    return 0;
  }

  {
    const uint8_t* t = sd + 5u * ctx->lambda_bytes;
    const bf64_t b_t = bf64_load(t);
    bf64_t running_t = bf64_one();
    for (size_t i = 0; i < ctx->total_words; ++i, running_t = bf64_mul(running_t, b_t)) {
      ctx->t_pows[i] = running_t;
    }
  }

  if (ctx->length_lambda > 1u) {
    const size_t pcount = (size_t)ctx->length_lambda - 1u;
    const uint8_t* s = sd + 4u * ctx->lambda_bytes;
    if (ctx->lambda == 256u) {
      ctx->s_pows_256 = malloc(pcount * sizeof(*ctx->s_pows_256));
      if (!ctx->s_pows_256) {
        vole_hash_precompute_clear(ctx);
        return 0;
      }
      bf256_t running_s = bf256_load(s);
      const bf256_t b_s = running_s;
      for (size_t i = 0; i < pcount; ++i, running_s = bf256_mul(running_s, b_s)) {
        ctx->s_pows_256[i] = running_s;
      }
    } else {
      ctx->s_pows_128 = malloc(pcount * sizeof(*ctx->s_pows_128));
      if (!ctx->s_pows_128) {
        vole_hash_precompute_clear(ctx);
        return 0;
      }
      bf128_t running_s = bf128_load(s);
      const bf128_t b_s = running_s;
      for (size_t i = 0; i < pcount; ++i, running_s = bf128_mul(running_s, b_s)) {
        ctx->s_pows_128[i] = running_s;
      }
    }
  }

  return 1;
}

void vole_hash_precompute_clear(vole_hash_precomp_t* ctx) {
  if (!ctx) {
    return;
  }
  free(ctx->t_pows);
  free(ctx->s_pows_128);
  free(ctx->s_pows_256);
  memset(ctx, 0, sizeof(*ctx));
}

static void vole_hash_128_4_precomp(uint8_t* h0_out, uint8_t* h1_out, uint8_t* h2_out,
                                    uint8_t* h3_out, const vole_hash_precomp_t* ctx,
                                    const uint8_t* x0, const uint8_t* x1, const uint8_t* x2,
                                    const uint8_t* x3) {
  const uint8_t* r0 = ctx->sd;
  const uint8_t* r1 = ctx->sd + 1 * BF128_NUM_BYTES;
  const uint8_t* r2 = ctx->sd + 2 * BF128_NUM_BYTES;
  const uint8_t* r3 = ctx->sd + 3 * BF128_NUM_BYTES;
  const uint8_t* x0_tail = x0 + (ctx->ell + 1 * BF128_NUM_BYTES * 8) / 8;
  const uint8_t* x1_tail = x1 + (ctx->ell + 1 * BF128_NUM_BYTES * 8) / 8;
  const uint8_t* x2_tail = x2 + (ctx->ell + 1 * BF128_NUM_BYTES * 8) / 8;
  const uint8_t* x3_tail = x3 + (ctx->ell + 1 * BF128_NUM_BYTES * 8) / 8;

  uint8_t tmp0[BF128_NUM_BYTES] = {0};
  uint8_t tmp1[BF128_NUM_BYTES] = {0};
  uint8_t tmp2[BF128_NUM_BYTES] = {0};
  uint8_t tmp3[BF128_NUM_BYTES] = {0};
  memcpy(tmp0, x0 + (size_t)(ctx->length_lambda - 1u) * BF128_NUM_BYTES, ctx->last_bytes);
  memcpy(tmp1, x1 + (size_t)(ctx->length_lambda - 1u) * BF128_NUM_BYTES, ctx->last_bytes);
  memcpy(tmp2, x2 + (size_t)(ctx->length_lambda - 1u) * BF128_NUM_BYTES, ctx->last_bytes);
  memcpy(tmp3, x3 + (size_t)(ctx->length_lambda - 1u) * BF128_NUM_BYTES, ctx->last_bytes);
  bf128_t h0_0 = bf128_load(tmp0);
  bf128_t h0_1 = bf128_load(tmp1);
  bf128_t h0_2 = bf128_load(tmp2);
  bf128_t h0_3 = bf128_load(tmp3);

  for (unsigned int i = 1; i != ctx->length_lambda; ++i) {
    const size_t off = (size_t)(ctx->length_lambda - 1u - i) * BF128_NUM_BYTES;
    const bf128_t coeff = ctx->s_pows_128[i - 1u];
    h0_0 = bf128_add(h0_0, bf128_mul(coeff, bf128_load(x0 + off)));
    h0_1 = bf128_add(h0_1, bf128_mul(coeff, bf128_load(x1 + off)));
    h0_2 = bf128_add(h0_2, bf128_mul(coeff, bf128_load(x2 + off)));
    h0_3 = bf128_add(h0_3, bf128_mul(coeff, bf128_load(x3 + off)));
  }

  bf64_t h1_0;
  bf64_t h1_1;
  bf64_t h1_2;
  bf64_t h1_3;
  compute_h1_4_precomp(ctx, x0, x1, x2, x3, &h1_0, &h1_1, &h1_2, &h1_3);
  const bf128_t br0 = bf128_load(r0);
  const bf128_t br1 = bf128_load(r1);
  const bf128_t br2 = bf128_load(r2);
  const bf128_t br3 = bf128_load(r3);

  bf128_t hh2_0 = bf128_add(bf128_mul(br0, h0_0), bf128_mul_64(br1, h1_0));
  bf128_t hh3_0 = bf128_add(bf128_mul(br2, h0_0), bf128_mul_64(br3, h1_0));
  bf128_t hh2_1 = bf128_add(bf128_mul(br0, h0_1), bf128_mul_64(br1, h1_1));
  bf128_t hh3_1 = bf128_add(bf128_mul(br2, h0_1), bf128_mul_64(br3, h1_1));
  bf128_t hh2_2 = bf128_add(bf128_mul(br0, h0_2), bf128_mul_64(br1, h1_2));
  bf128_t hh3_2 = bf128_add(bf128_mul(br2, h0_2), bf128_mul_64(br3, h1_2));
  bf128_t hh2_3 = bf128_add(bf128_mul(br0, h0_3), bf128_mul_64(br1, h1_3));
  bf128_t hh3_3 = bf128_add(bf128_mul(br2, h0_3), bf128_mul_64(br3, h1_3));

  bf128_store(h0_out, hh2_0);
  bf128_store(tmp0, hh3_0);
  memcpy(h0_out + BF128_NUM_BYTES, tmp0, UNIVERSAL_HASH_B);
  xor_u8_array(h0_out, x0_tail, h0_out, BF128_NUM_BYTES + UNIVERSAL_HASH_B);

  bf128_store(h1_out, hh2_1);
  bf128_store(tmp1, hh3_1);
  memcpy(h1_out + BF128_NUM_BYTES, tmp1, UNIVERSAL_HASH_B);
  xor_u8_array(h1_out, x1_tail, h1_out, BF128_NUM_BYTES + UNIVERSAL_HASH_B);

  bf128_store(h2_out, hh2_2);
  bf128_store(tmp2, hh3_2);
  memcpy(h2_out + BF128_NUM_BYTES, tmp2, UNIVERSAL_HASH_B);
  xor_u8_array(h2_out, x2_tail, h2_out, BF128_NUM_BYTES + UNIVERSAL_HASH_B);

  bf128_store(h3_out, hh2_3);
  bf128_store(tmp3, hh3_3);
  memcpy(h3_out + BF128_NUM_BYTES, tmp3, UNIVERSAL_HASH_B);
  xor_u8_array(h3_out, x3_tail, h3_out, BF128_NUM_BYTES + UNIVERSAL_HASH_B);
}

static void vole_hash_256_4_precomp(uint8_t* h0_out, uint8_t* h1_out, uint8_t* h2_out,
                                    uint8_t* h3_out, const vole_hash_precomp_t* ctx,
                                    const uint8_t* x0, const uint8_t* x1, const uint8_t* x2,
                                    const uint8_t* x3) {
  const uint8_t* r0 = ctx->sd;
  const uint8_t* r1 = ctx->sd + 1 * BF256_NUM_BYTES;
  const uint8_t* r2 = ctx->sd + 2 * BF256_NUM_BYTES;
  const uint8_t* r3 = ctx->sd + 3 * BF256_NUM_BYTES;
  const uint8_t* x0_tail = x0 + (ctx->ell + 1 * BF256_NUM_BYTES * 8) / 8;
  const uint8_t* x1_tail = x1 + (ctx->ell + 1 * BF256_NUM_BYTES * 8) / 8;
  const uint8_t* x2_tail = x2 + (ctx->ell + 1 * BF256_NUM_BYTES * 8) / 8;
  const uint8_t* x3_tail = x3 + (ctx->ell + 1 * BF256_NUM_BYTES * 8) / 8;

  uint8_t tmp0[BF256_NUM_BYTES] = {0};
  uint8_t tmp1[BF256_NUM_BYTES] = {0};
  uint8_t tmp2[BF256_NUM_BYTES] = {0};
  uint8_t tmp3[BF256_NUM_BYTES] = {0};
  memcpy(tmp0, x0 + (size_t)(ctx->length_lambda - 1u) * BF256_NUM_BYTES, ctx->last_bytes);
  memcpy(tmp1, x1 + (size_t)(ctx->length_lambda - 1u) * BF256_NUM_BYTES, ctx->last_bytes);
  memcpy(tmp2, x2 + (size_t)(ctx->length_lambda - 1u) * BF256_NUM_BYTES, ctx->last_bytes);
  memcpy(tmp3, x3 + (size_t)(ctx->length_lambda - 1u) * BF256_NUM_BYTES, ctx->last_bytes);
  bf256_t h0_0 = bf256_load(tmp0);
  bf256_t h0_1 = bf256_load(tmp1);
  bf256_t h0_2 = bf256_load(tmp2);
  bf256_t h0_3 = bf256_load(tmp3);

  for (unsigned int i = 1; i != ctx->length_lambda; ++i) {
    const size_t off = (size_t)(ctx->length_lambda - 1u - i) * BF256_NUM_BYTES;
    const bf256_t coeff = ctx->s_pows_256[i - 1u];
    h0_0 = bf256_add(h0_0, bf256_mul(coeff, bf256_load(x0 + off)));
    h0_1 = bf256_add(h0_1, bf256_mul(coeff, bf256_load(x1 + off)));
    h0_2 = bf256_add(h0_2, bf256_mul(coeff, bf256_load(x2 + off)));
    h0_3 = bf256_add(h0_3, bf256_mul(coeff, bf256_load(x3 + off)));
  }

  bf64_t h1_0;
  bf64_t h1_1;
  bf64_t h1_2;
  bf64_t h1_3;
  compute_h1_4_precomp(ctx, x0, x1, x2, x3, &h1_0, &h1_1, &h1_2, &h1_3);
  const bf256_t br0 = bf256_load(r0);
  const bf256_t br1 = bf256_load(r1);
  const bf256_t br2 = bf256_load(r2);
  const bf256_t br3 = bf256_load(r3);

  bf256_t hh2_0 = bf256_add(bf256_mul(br0, h0_0), bf256_mul_64(br1, h1_0));
  bf256_t hh3_0 = bf256_add(bf256_mul(br2, h0_0), bf256_mul_64(br3, h1_0));
  bf256_t hh2_1 = bf256_add(bf256_mul(br0, h0_1), bf256_mul_64(br1, h1_1));
  bf256_t hh3_1 = bf256_add(bf256_mul(br2, h0_1), bf256_mul_64(br3, h1_1));
  bf256_t hh2_2 = bf256_add(bf256_mul(br0, h0_2), bf256_mul_64(br1, h1_2));
  bf256_t hh3_2 = bf256_add(bf256_mul(br2, h0_2), bf256_mul_64(br3, h1_2));
  bf256_t hh2_3 = bf256_add(bf256_mul(br0, h0_3), bf256_mul_64(br1, h1_3));
  bf256_t hh3_3 = bf256_add(bf256_mul(br2, h0_3), bf256_mul_64(br3, h1_3));

  bf256_store(h0_out, hh2_0);
  bf256_store(tmp0, hh3_0);
  memcpy(h0_out + BF256_NUM_BYTES, tmp0, UNIVERSAL_HASH_B);
  xor_u8_array(h0_out, x0_tail, h0_out, BF256_NUM_BYTES + UNIVERSAL_HASH_B);

  bf256_store(h1_out, hh2_1);
  bf256_store(tmp1, hh3_1);
  memcpy(h1_out + BF256_NUM_BYTES, tmp1, UNIVERSAL_HASH_B);
  xor_u8_array(h1_out, x1_tail, h1_out, BF256_NUM_BYTES + UNIVERSAL_HASH_B);

  bf256_store(h2_out, hh2_2);
  bf256_store(tmp2, hh3_2);
  memcpy(h2_out + BF256_NUM_BYTES, tmp2, UNIVERSAL_HASH_B);
  xor_u8_array(h2_out, x2_tail, h2_out, BF256_NUM_BYTES + UNIVERSAL_HASH_B);

  bf256_store(h3_out, hh2_3);
  bf256_store(tmp3, hh3_3);
  memcpy(h3_out + BF256_NUM_BYTES, tmp3, UNIVERSAL_HASH_B);
  xor_u8_array(h3_out, x3_tail, h3_out, BF256_NUM_BYTES + UNIVERSAL_HASH_B);
}

void vole_hash_4_precomp(uint8_t* h0, uint8_t* h1, uint8_t* h2, uint8_t* h3,
                         const vole_hash_precomp_t* ctx, const uint8_t* x0, const uint8_t* x1,
                         const uint8_t* x2, const uint8_t* x3) {
  if (ctx->lambda == 256u) {
    vole_hash_256_4_precomp(h0, h1, h2, h3, ctx, x0, x1, x2, x3);
  } else {
    vole_hash_128_4_precomp(h0, h1, h2, h3, ctx, x0, x1, x2, x3);
  }
}

void vole_hash_2_precomp(uint8_t* h0, uint8_t* h1, const vole_hash_precomp_t* ctx,
                         const uint8_t* x0, const uint8_t* x1) {
  uint8_t throw2[BF256_NUM_BYTES + UNIVERSAL_HASH_B];
  uint8_t throw3[BF256_NUM_BYTES + UNIVERSAL_HASH_B];
  vole_hash_4_precomp(h0, h1, throw2, throw3, ctx, x0, x1, x0, x1);
}

void vole_hash_precomp(uint8_t* h0, const vole_hash_precomp_t* ctx, const uint8_t* x0) {
  uint8_t throw1[BF256_NUM_BYTES + UNIVERSAL_HASH_B];
  uint8_t throw2[BF256_NUM_BYTES + UNIVERSAL_HASH_B];
  uint8_t throw3[BF256_NUM_BYTES + UNIVERSAL_HASH_B];
  vole_hash_4_precomp(h0, throw1, throw2, throw3, ctx, x0, x0, x0, x0);
}

void vole_hash_256(uint8_t* h, const uint8_t* sd, const uint8_t* x, unsigned int ell) {
  const uint8_t* r0 = sd;
  const uint8_t* r1 = sd + 1 * BF256_NUM_BYTES;
  const uint8_t* r2 = sd + 2 * BF256_NUM_BYTES;
  const uint8_t* r3 = sd + 3 * BF256_NUM_BYTES;
  const uint8_t* s  = sd + 4 * BF256_NUM_BYTES;
  const uint8_t* t  = sd + 5 * BF256_NUM_BYTES;
  const uint8_t* x1 = x + (ell + 1 * BF256_NUM_BYTES * 8) / 8;

  const unsigned int length_lambda = (ell + 3 * BF256_NUM_BYTES * 8 - 1) / (BF256_NUM_BYTES * 8);

  uint8_t tmp[BF256_NUM_BYTES] = {0};
  memcpy(tmp, x + (length_lambda - 1) * BF256_NUM_BYTES,
         (ell + BF256_NUM_BYTES * 8) % (BF256_NUM_BYTES * 8) == 0
             ? BF256_NUM_BYTES
             : ((ell + BF256_NUM_BYTES * 8) % (BF256_NUM_BYTES * 8)) / 8);
  bf256_t h0 = bf256_load(tmp);

  const bf256_t b_s = bf256_load(s);
  bf256_t running_s = b_s;
  for (unsigned int i = 1; i != length_lambda; ++i, running_s = bf256_mul(running_s, b_s)) {
    h0 = bf256_add(h0,
                   bf256_mul(running_s, bf256_load(x + (length_lambda - 1 - i) * BF256_NUM_BYTES)));
  }

  bf64_t h1  = compute_h1(t, x, BF256_NUM_BYTES * 8, ell);
  bf256_t h2 = bf256_add(bf256_mul(bf256_load(r0), h0), bf256_mul_64(bf256_load(r1), h1));
  bf256_t h3 = bf256_add(bf256_mul(bf256_load(r2), h0), bf256_mul_64(bf256_load(r3), h1));

  bf256_store(h, h2);
  bf256_store(tmp, h3);
  memcpy(h + BF256_NUM_BYTES, tmp, UNIVERSAL_HASH_B);
  xor_u8_array(h, x1, h, BF256_NUM_BYTES + UNIVERSAL_HASH_B);
}

void vole_hash_256_2(uint8_t* h0_out, uint8_t* h1_out, const uint8_t* sd, const uint8_t* x0,
                     const uint8_t* x1, unsigned int ell) {
  const uint8_t* r0 = sd;
  const uint8_t* r1 = sd + 1 * BF256_NUM_BYTES;
  const uint8_t* r2 = sd + 2 * BF256_NUM_BYTES;
  const uint8_t* r3 = sd + 3 * BF256_NUM_BYTES;
  const uint8_t* s  = sd + 4 * BF256_NUM_BYTES;
  const uint8_t* t  = sd + 5 * BF256_NUM_BYTES;
  const uint8_t* x0_tail = x0 + (ell + 1 * BF256_NUM_BYTES * 8) / 8;
  const uint8_t* x1_tail = x1 + (ell + 1 * BF256_NUM_BYTES * 8) / 8;

  const unsigned int length_lambda = (ell + 3 * BF256_NUM_BYTES * 8 - 1) / (BF256_NUM_BYTES * 8);
  const size_t last_bytes = (ell + BF256_NUM_BYTES * 8) % (BF256_NUM_BYTES * 8) == 0
                                ? BF256_NUM_BYTES
                                : ((ell + BF256_NUM_BYTES * 8) % (BF256_NUM_BYTES * 8)) / 8;

  uint8_t tmp0[BF256_NUM_BYTES] = {0};
  uint8_t tmp1[BF256_NUM_BYTES] = {0};
  memcpy(tmp0, x0 + (length_lambda - 1) * BF256_NUM_BYTES, last_bytes);
  memcpy(tmp1, x1 + (length_lambda - 1) * BF256_NUM_BYTES, last_bytes);
  bf256_t h0_0 = bf256_load(tmp0);
  bf256_t h0_1 = bf256_load(tmp1);

  const bf256_t b_s = bf256_load(s);
  bf256_t running_s = b_s;
  for (unsigned int i = 1; i != length_lambda; ++i, running_s = bf256_mul(running_s, b_s)) {
    const size_t off = (length_lambda - 1 - i) * BF256_NUM_BYTES;
    h0_0 = bf256_add(h0_0, bf256_mul(running_s, bf256_load(x0 + off)));
    h0_1 = bf256_add(h0_1, bf256_mul(running_s, bf256_load(x1 + off)));
  }

  bf64_t h1_0;
  bf64_t h1_1;
  compute_h1_2(t, x0, x1, BF256_NUM_BYTES * 8, ell, &h1_0, &h1_1);
  const bf256_t br0 = bf256_load(r0);
  const bf256_t br1 = bf256_load(r1);
  const bf256_t br2 = bf256_load(r2);
  const bf256_t br3 = bf256_load(r3);

  bf256_t h2_0 = bf256_add(bf256_mul(br0, h0_0), bf256_mul_64(br1, h1_0));
  bf256_t h3_0 = bf256_add(bf256_mul(br2, h0_0), bf256_mul_64(br3, h1_0));
  bf256_t h2_1 = bf256_add(bf256_mul(br0, h0_1), bf256_mul_64(br1, h1_1));
  bf256_t h3_1 = bf256_add(bf256_mul(br2, h0_1), bf256_mul_64(br3, h1_1));

  bf256_store(h0_out, h2_0);
  bf256_store(tmp0, h3_0);
  memcpy(h0_out + BF256_NUM_BYTES, tmp0, UNIVERSAL_HASH_B);
  xor_u8_array(h0_out, x0_tail, h0_out, BF256_NUM_BYTES + UNIVERSAL_HASH_B);

  bf256_store(h1_out, h2_1);
  bf256_store(tmp1, h3_1);
  memcpy(h1_out + BF256_NUM_BYTES, tmp1, UNIVERSAL_HASH_B);
  xor_u8_array(h1_out, x1_tail, h1_out, BF256_NUM_BYTES + UNIVERSAL_HASH_B);
}

void vole_hash_256_4(uint8_t* h0_out, uint8_t* h1_out, uint8_t* h2_out, uint8_t* h3_out,
                     const uint8_t* sd, const uint8_t* x0, const uint8_t* x1,
                     const uint8_t* x2, const uint8_t* x3, unsigned int ell) {
  const uint8_t* r0 = sd;
  const uint8_t* r1 = sd + 1 * BF256_NUM_BYTES;
  const uint8_t* r2 = sd + 2 * BF256_NUM_BYTES;
  const uint8_t* r3 = sd + 3 * BF256_NUM_BYTES;
  const uint8_t* s  = sd + 4 * BF256_NUM_BYTES;
  const uint8_t* t  = sd + 5 * BF256_NUM_BYTES;
  const uint8_t* x0_tail = x0 + (ell + 1 * BF256_NUM_BYTES * 8) / 8;
  const uint8_t* x1_tail = x1 + (ell + 1 * BF256_NUM_BYTES * 8) / 8;
  const uint8_t* x2_tail = x2 + (ell + 1 * BF256_NUM_BYTES * 8) / 8;
  const uint8_t* x3_tail = x3 + (ell + 1 * BF256_NUM_BYTES * 8) / 8;

  const unsigned int length_lambda = (ell + 3 * BF256_NUM_BYTES * 8 - 1) / (BF256_NUM_BYTES * 8);
  const size_t last_bytes = (ell + BF256_NUM_BYTES * 8) % (BF256_NUM_BYTES * 8) == 0
                                ? BF256_NUM_BYTES
                                : ((ell + BF256_NUM_BYTES * 8) % (BF256_NUM_BYTES * 8)) / 8;

  uint8_t tmp0[BF256_NUM_BYTES] = {0};
  uint8_t tmp1[BF256_NUM_BYTES] = {0};
  uint8_t tmp2[BF256_NUM_BYTES] = {0};
  uint8_t tmp3[BF256_NUM_BYTES] = {0};
  memcpy(tmp0, x0 + (length_lambda - 1) * BF256_NUM_BYTES, last_bytes);
  memcpy(tmp1, x1 + (length_lambda - 1) * BF256_NUM_BYTES, last_bytes);
  memcpy(tmp2, x2 + (length_lambda - 1) * BF256_NUM_BYTES, last_bytes);
  memcpy(tmp3, x3 + (length_lambda - 1) * BF256_NUM_BYTES, last_bytes);
  bf256_t h0_0 = bf256_load(tmp0);
  bf256_t h0_1 = bf256_load(tmp1);
  bf256_t h0_2 = bf256_load(tmp2);
  bf256_t h0_3 = bf256_load(tmp3);

  const bf256_t b_s = bf256_load(s);
  bf256_t running_s = b_s;
  for (unsigned int i = 1; i != length_lambda; ++i, running_s = bf256_mul(running_s, b_s)) {
    const size_t off = (length_lambda - 1 - i) * BF256_NUM_BYTES;
    h0_0 = bf256_add(h0_0, bf256_mul(running_s, bf256_load(x0 + off)));
    h0_1 = bf256_add(h0_1, bf256_mul(running_s, bf256_load(x1 + off)));
    h0_2 = bf256_add(h0_2, bf256_mul(running_s, bf256_load(x2 + off)));
    h0_3 = bf256_add(h0_3, bf256_mul(running_s, bf256_load(x3 + off)));
  }

  bf64_t h1_0;
  bf64_t h1_1;
  bf64_t h1_2;
  bf64_t h1_3;
  compute_h1_4(t, x0, x1, x2, x3, BF256_NUM_BYTES * 8, ell, &h1_0, &h1_1, &h1_2, &h1_3);
  const bf256_t br0 = bf256_load(r0);
  const bf256_t br1 = bf256_load(r1);
  const bf256_t br2 = bf256_load(r2);
  const bf256_t br3 = bf256_load(r3);

  bf256_t hh2_0 = bf256_add(bf256_mul(br0, h0_0), bf256_mul_64(br1, h1_0));
  bf256_t hh3_0 = bf256_add(bf256_mul(br2, h0_0), bf256_mul_64(br3, h1_0));
  bf256_t hh2_1 = bf256_add(bf256_mul(br0, h0_1), bf256_mul_64(br1, h1_1));
  bf256_t hh3_1 = bf256_add(bf256_mul(br2, h0_1), bf256_mul_64(br3, h1_1));
  bf256_t hh2_2 = bf256_add(bf256_mul(br0, h0_2), bf256_mul_64(br1, h1_2));
  bf256_t hh3_2 = bf256_add(bf256_mul(br2, h0_2), bf256_mul_64(br3, h1_2));
  bf256_t hh2_3 = bf256_add(bf256_mul(br0, h0_3), bf256_mul_64(br1, h1_3));
  bf256_t hh3_3 = bf256_add(bf256_mul(br2, h0_3), bf256_mul_64(br3, h1_3));

  bf256_store(h0_out, hh2_0);
  bf256_store(tmp0, hh3_0);
  memcpy(h0_out + BF256_NUM_BYTES, tmp0, UNIVERSAL_HASH_B);
  xor_u8_array(h0_out, x0_tail, h0_out, BF256_NUM_BYTES + UNIVERSAL_HASH_B);

  bf256_store(h1_out, hh2_1);
  bf256_store(tmp1, hh3_1);
  memcpy(h1_out + BF256_NUM_BYTES, tmp1, UNIVERSAL_HASH_B);
  xor_u8_array(h1_out, x1_tail, h1_out, BF256_NUM_BYTES + UNIVERSAL_HASH_B);

  bf256_store(h2_out, hh2_2);
  bf256_store(tmp2, hh3_2);
  memcpy(h2_out + BF256_NUM_BYTES, tmp2, UNIVERSAL_HASH_B);
  xor_u8_array(h2_out, x2_tail, h2_out, BF256_NUM_BYTES + UNIVERSAL_HASH_B);

  bf256_store(h3_out, hh2_3);
  bf256_store(tmp3, hh3_3);
  memcpy(h3_out + BF256_NUM_BYTES, tmp3, UNIVERSAL_HASH_B);
  xor_u8_array(h3_out, x3_tail, h3_out, BF256_NUM_BYTES + UNIVERSAL_HASH_B);
}

void vole_hash(uint8_t* h, const uint8_t* sd, const uint8_t* x, unsigned int ell, uint32_t lambda) {
  (void)lambda;
  vole_hash_256(h, sd, x, ell);
}

void vole_hash_2(uint8_t* h0, uint8_t* h1, const uint8_t* sd, const uint8_t* x0,
                 const uint8_t* x1, unsigned int ell, uint32_t lambda) {
  (void)lambda;
  vole_hash_256_2(h0, h1, sd, x0, x1, ell);
}

void vole_hash_4(uint8_t* h0, uint8_t* h1, uint8_t* h2, uint8_t* h3,
                 const uint8_t* sd, const uint8_t* x0, const uint8_t* x1,
                 const uint8_t* x2, const uint8_t* x3, unsigned int ell, uint32_t lambda) {
  (void)lambda;
  vole_hash_256_4(h0, h1, h2, h3, sd, x0, x1, x2, x3, ell);
}

void zk_hash_256_init(zk_hash_256_ctx* ctx, const uint8_t* sd) {
  const uint8_t* s = sd + 2 * BF256_NUM_BYTES;
  const uint8_t* t = sd + 3 * BF256_NUM_BYTES;

  ctx->h0      = bf256_zero();
  ctx->h1      = bf256_zero();
  ctx->s       = bf256_load(s);
  ctx->s2      = bf256_mul(ctx->s, ctx->s);
  ctx->t       = bf64_load(t);
  ctx->sd      = sd;
  ctx->pending = 0;
}

void zk_hash_256_update(zk_hash_256_ctx* ctx, bf256_t v) {
  if (!ctx->pending) {
    ctx->pending_v = v;
    ctx->pending   = 1;
  } else {
    ctx->h0 = bf256_add(bf256_add(bf256_mul(ctx->h0, ctx->s2),
                                   bf256_mul(ctx->pending_v, ctx->s)), v);
    ctx->h1 = bf256_add(bf256_mul_64(
                bf256_add(bf256_mul_64(ctx->h1, ctx->t), ctx->pending_v),
                ctx->t), v);
    ctx->pending = 0;
  }
}

void zk_hash_256_finalize(uint8_t* h, zk_hash_256_ctx* ctx, bf256_t x1) {
  if (ctx->pending) {
    ctx->h0 = bf256_add(bf256_mul(ctx->h0, ctx->s), ctx->pending_v);
    ctx->h1 = bf256_add(bf256_mul_64(ctx->h1, ctx->t), ctx->pending_v);
    ctx->pending = 0;
  }
  const uint8_t* r0 = ctx->sd;
  const uint8_t* r1 = ctx->sd + BF256_NUM_BYTES;

  bf256_store(h, bf256_add(bf256_add(bf256_mul(bf256_load(r0), ctx->h0),
                                     bf256_mul(bf256_load(r1), ctx->h1)),
                           x1));
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
  ctx->s       = bf256_load(s);
  ctx->s2      = bf256_mul(ctx->s, ctx->s);
  ctx->t       = bf64_load(t);
  ctx->sd      = sd;
  ctx->pending = 0;
}

void zk_hash_256_3_update(zk_hash_256_3_ctx* ctx, bf256_t v_0, bf256_t v_1, bf256_t v_2) {
  if (!ctx->pending) {
    ctx->pending_v_0 = v_0;
    ctx->pending_v_1 = v_1;
    ctx->pending_v_2 = v_2;
    ctx->pending     = 1;
  } else {
    ctx->h0[0] = bf256_add(bf256_add(bf256_mul(ctx->h0[0], ctx->s2),
                                      bf256_mul(ctx->pending_v_0, ctx->s)), v_0);
    ctx->h1[0] = bf256_add(bf256_mul_64(
                   bf256_add(bf256_mul_64(ctx->h1[0], ctx->t), ctx->pending_v_0),
                   ctx->t), v_0);
    ctx->h0[1] = bf256_add(bf256_add(bf256_mul(ctx->h0[1], ctx->s2),
                                      bf256_mul(ctx->pending_v_1, ctx->s)), v_1);
    ctx->h1[1] = bf256_add(bf256_mul_64(
                   bf256_add(bf256_mul_64(ctx->h1[1], ctx->t), ctx->pending_v_1),
                   ctx->t), v_1);
    ctx->h0[2] = bf256_add(bf256_add(bf256_mul(ctx->h0[2], ctx->s2),
                                      bf256_mul(ctx->pending_v_2, ctx->s)), v_2);
    ctx->h1[2] = bf256_add(bf256_mul_64(
                   bf256_add(bf256_mul_64(ctx->h1[2], ctx->t), ctx->pending_v_2),
                   ctx->t), v_2);
    ctx->pending = 0;
  }
}

void zk_hash_256_3_raise_and_update(zk_hash_256_3_ctx* ctx, bf256_t v_1, bf256_t v_2) {
  zk_hash_256_3_update(ctx, bf256_zero(), v_1, v_2);
}

void zk_hash_256_3_finalize(uint8_t* h_0, uint8_t* h_1, uint8_t* h_2, zk_hash_256_3_ctx* ctx,
                            bf256_t x1_0, bf256_t x1_1, bf256_t x1_2) {
  if (ctx->pending) {
    ctx->h0[0] = bf256_add(bf256_mul(ctx->h0[0], ctx->s), ctx->pending_v_0);
    ctx->h1[0] = bf256_add(bf256_mul_64(ctx->h1[0], ctx->t), ctx->pending_v_0);
    ctx->h0[1] = bf256_add(bf256_mul(ctx->h0[1], ctx->s), ctx->pending_v_1);
    ctx->h1[1] = bf256_add(bf256_mul_64(ctx->h1[1], ctx->t), ctx->pending_v_1);
    ctx->h0[2] = bf256_add(bf256_mul(ctx->h0[2], ctx->s), ctx->pending_v_2);
    ctx->h1[2] = bf256_add(bf256_mul_64(ctx->h1[2], ctx->t), ctx->pending_v_2);
    ctx->pending = 0;
  }
  const bf256_t r0 = bf256_load(ctx->sd);
  const bf256_t r1 = bf256_load(ctx->sd + BF256_NUM_BYTES);

  bf256_store(h_0,
              bf256_add(bf256_add(bf256_mul(r0, ctx->h0[0]), bf256_mul(r1, ctx->h1[0])), x1_0));
  bf256_store(h_1,
              bf256_add(bf256_add(bf256_mul(r0, ctx->h0[1]), bf256_mul(r1, ctx->h1[1])), x1_1));
  bf256_store(h_2,
              bf256_add(bf256_add(bf256_mul(r0, ctx->h0[2]), bf256_mul(r1, ctx->h1[2])), x1_2));
}
