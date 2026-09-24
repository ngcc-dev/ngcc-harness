#include <stdlib.h>
#include <string.h>

#include "random_oracle.h"
#include "params.h"
#include "auxfunc.h"

static const uint8_t domain_sep_H1   = 1;
static const uint8_t domain_sep_H2_0 = 8 + 0;
static const uint8_t domain_sep_H2_1 = 8 + 1;
static const uint8_t domain_sep_H2_2 = 8 + 2;
static const uint8_t domain_sep_H2_3 = 8 + 3;
static const uint8_t domain_sep_H3   = 3;
static const uint8_t domain_sep_H4   = 4;

static int sm3hash_msg_plus_domain_byte_local(const uint8_t* msg, size_t msg_len,
                                              uint8_t domain_sep, uint8_t* output) {
  const size_t total_len = msg_len + 1u;
  uint8_t stack_buf[1024];
  uint8_t* buf = total_len <= sizeof(stack_buf) ? stack_buf : malloc(total_len);
  if (!buf) {
    return 0;
  }
  if (msg_len) {
    memcpy(buf, msg, msg_len);
  }
  buf[msg_len] = domain_sep;
  const int ret = sm3hash(256, buf, (unsigned long long)total_len * 8ULL, output) == 0;
  if (buf != stack_buf) {
    free(buf);
  }
  return ret;
}

static int pseudoXOF_leq256_byte_aligned_local(unsigned long long output_len_bits,
                                               const uint8_t* msg, size_t msg_len,
                                               uint8_t* output) {
  if (output_len_bits == 0u || output_len_bits > 256u || (output_len_bits & 7u) != 0u) {
    return 0;
  }

  const size_t total_len = msg_len + 4u;
  uint8_t stack_buf[1024];
  uint8_t* buf = total_len <= sizeof(stack_buf) ? stack_buf : malloc(total_len);
  if (!buf) {
    return 0;
  }
  if (msg_len) {
    memcpy(buf, msg, msg_len);
  }
  buf[msg_len + 0u] = 0u;
  buf[msg_len + 1u] = 0u;
  buf[msg_len + 2u] = 0u;
  buf[msg_len + 3u] = 1u;

  uint8_t digest[32];
  const int ret = sm3hash(256, buf, (unsigned long long)total_len * 8ULL, digest) == 0;
  if (ret) {
    memcpy(output, digest, (size_t)(output_len_bits / 8u));
  }
  if (buf != stack_buf) {
    free(buf);
  }
  return ret;
}

static int hash_with_domain_sep(const uint8_t* msg, size_t msg_len, uint8_t domain_sep,
                                uint8_t* output, size_t output_bytes_len) {
  const int output_bits = (int)(output_bytes_len * 8u);
  if (output_bits == 256) {
    return sm3hash_msg_plus_domain_byte_local(msg, msg_len, domain_sep, output);
  }

  const size_t total_len = msg_len + 1u;
  uint8_t stack_buf[1024];
  uint8_t* buf = total_len <= sizeof(stack_buf) ? stack_buf : malloc(total_len);
  if (!buf) {
    return 0;
  }
  if (msg_len) {
    memcpy(buf, msg, msg_len);
  }
  buf[msg_len] = domain_sep;
  const unsigned long long bit_len = (unsigned long long)total_len * 8ULL;

  int ret = (output_bits == 512 || output_bits == 768 || output_bits == 1024)
                ? pseudohash(output_bits, buf, bit_len, output)
                : ((output_bits > 0 && output_bits <= 256)
                       ? !pseudoXOF_leq256_byte_aligned_local((unsigned long long)output_bits, buf,
                                                              total_len, output)
                       : pseudoXOF((unsigned long long)output_bits, buf, bit_len, output));
  if (buf != stack_buf) {
    free(buf);
  }
  return ret == 0;
}

/* reusable hash helpers */
void new_hash_init(new_hash_context_t* ctx) {
  ctx->buffer = NULL;
  ctx->buffer_len = 0;
  ctx->buffer_cap = 0;
  ctx->output = NULL;
  ctx->output_bytes_len = 0;
  ctx->output_off = 0;
  ctx->finalized = 0;
}

void new_hash_reset(new_hash_context_t* ctx) {
  ctx->buffer_len = 0;
  ctx->output_off = 0;
  ctx->finalized = 0;
}

// input src with length, allocate more space if needed
void new_hash_update(new_hash_context_t* ctx, const uint8_t* input, size_t input_bytes_len) {
  if (input_bytes_len == 0) {
    return;
  }
  if (!input || ctx->buffer_len > SIZE_MAX - input_bytes_len - 1) {
    return;
  }
  const size_t needed = ctx->buffer_len + input_bytes_len + 1;
  if (needed > ctx->buffer_cap) {
    size_t new_cap = ctx->buffer_cap ? ctx->buffer_cap : 64;
    while (new_cap < needed) {
      if (new_cap > SIZE_MAX / 2) {
        new_cap = needed;
        break;
      }
      new_cap *= 2;
    }
    uint8_t* new_buf = realloc(ctx->buffer, new_cap);
    if (!new_buf) {
      return;
    }
    ctx->buffer = new_buf;
    ctx->buffer_cap = new_cap;
  }
  memcpy(ctx->buffer + ctx->buffer_len, input, input_bytes_len);
  ctx->buffer_len += input_bytes_len;
}

// finalize the hash with domain separation and output length in bits
// put a domain_sep byte at the end of the message before hashing,
// then even the inputs are the same, different domain_sep will lead to different outputs
void new_hash_final(new_hash_context_t* ctx, uint8_t domain_sep, size_t output_bytes_len) {
  if (ctx->output_bytes_len < output_bytes_len || ctx->output == NULL) {
    uint8_t* new_out = realloc(ctx->output, output_bytes_len);
    if (!new_out) {
      ctx->finalized = 0;
      return;
    }
    ctx->output = new_out;
  }
  ctx->output_bytes_len = output_bytes_len;

  const uint8_t* msg = ctx->buffer_len ? ctx->buffer : (const uint8_t*)"";
  if (!hash_with_domain_sep(msg, ctx->buffer_len, domain_sep, ctx->output, output_bytes_len)) {
    ctx->finalized = 0;
    return;
  }
  ctx->output_off = 0;
  ctx->finalized = 1;
}

// squeeze outputs with claimed length into dst, used when splited result is needed
void new_hash_squeeze(new_hash_context_t* ctx, uint8_t* dst, size_t len) {
  if (!dst || !len) {
    return;
  }
  const size_t remaining = ctx->output_bytes_len > ctx->output_off
                               ? ctx->output_bytes_len - ctx->output_off
                               : 0;
  const size_t take = len > remaining ? remaining : len;
  if (take) {
    memcpy(dst, ctx->output + ctx->output_off, take);
  }
  ctx->output_off += take;
  if (len > take) {
    memset(dst + take, 0, len - take);
  }
}

void new_hash_clear(new_hash_context_t* ctx) {
  if (ctx->buffer) {
    free(ctx->buffer);
  }
  if (ctx->output) {
    free(ctx->output);
  }
  ctx->buffer = NULL;
  ctx->buffer_len = 0;
  ctx->buffer_cap = 0;
  ctx->output = NULL;
  ctx->output_bytes_len = 0;
  ctx->output_off = 0;
  ctx->finalized = 0;
}

// H_1, used for leaf commit in BAVC.Commit
// H1 output length is 2 * Nblock bits; see H1_final_keep for the active PRG block width.
void H1_init(H1_context_t* ctx) {
  new_hash_init(ctx);
}

void H1_reset(H1_context_t* ctx) {
  new_hash_reset(ctx);
}

void H1_update(H1_context_t* ctx, const uint8_t* src, size_t bytes_len) {
  new_hash_update(ctx, src, bytes_len);
}

void H1_final_keep(const params_t* params, H1_context_t* ctx, uint8_t* digest) {
  size_t out_bytes_len = 2u * params->lambda_bytes;
  new_hash_final(ctx, domain_sep_H1, out_bytes_len);
  new_hash_squeeze(ctx, digest, out_bytes_len);
}

void H1_clear(H1_context_t* ctx) {
  new_hash_clear(ctx);
}

// define output len
void H1_final(const params_t* params, H1_context_t* ctx, uint8_t* digest) {
  H1_final_keep(params, ctx, digest);
  H1_clear(ctx);
}

// H_2
static size_t H2_output_len(const params_t* params, int which) {
  switch (which) {
    case 0:
      return (size_t)(2u * params->lambda_bytes);
    case 1:
      return (size_t)(5u * params->lambda_f_bytes + 8u);
    case 2:
      return (size_t)(3u * params->lambda_f_bytes + 8u);
    case 3:
      return (size_t)(params->lambda_f_bytes);
    default:
      return 0;
  }
}

void H2_init(H2_context_t* ctx) {
  new_hash_init(ctx);
}

void H2_update(H2_context_t* ctx, const uint8_t* src, size_t bytes_len) {
  new_hash_update(ctx, src, bytes_len);
}

void H2_0_final(const params_t* params, H2_context_t* ctx, uint8_t* digest) {
  const size_t out_bytes_len = H2_output_len(params, 0);
  new_hash_final(ctx, domain_sep_H2_0, out_bytes_len);
  new_hash_squeeze(ctx, digest, out_bytes_len);
  new_hash_clear(ctx);
}

void H2_1_final(const params_t* params, H2_context_t* ctx, uint8_t* digest) {
  const size_t out_bytes_len = H2_output_len(params, 1);
  new_hash_final(ctx, domain_sep_H2_1, out_bytes_len);
  new_hash_squeeze(ctx, digest, out_bytes_len);
  new_hash_clear(ctx);
}

void H2_2_final(const params_t* params, H2_context_t* ctx, uint8_t* digest) {
  const size_t out_bytes_len = H2_output_len(params, 2);
  new_hash_final(ctx, domain_sep_H2_2, out_bytes_len);
  new_hash_squeeze(ctx, digest, out_bytes_len);
  new_hash_clear(ctx);
}

void H2_3_final(const params_t* params, H2_context_t* ctx, uint8_t* digest) {
  const size_t out_bytes_len = H2_output_len(params, 3);
  new_hash_final(ctx, domain_sep_H2_3, out_bytes_len);
  new_hash_squeeze(ctx, digest, out_bytes_len);
  new_hash_clear(ctx);
}

void H2_3_final_u32_le(const params_t* params, const H2_context_t* ctx, uint32_t ctr,
                       uint8_t* digest) {
  const size_t out_bytes_len = H2_output_len(params, 3);
  const size_t base_len = ctx->buffer_len;
  const size_t msg_len = base_len + 4u;
  uint8_t stack_buf[1024];
  uint8_t* msg = msg_len <= sizeof(stack_buf) ? stack_buf : malloc(msg_len);
  if (!msg) {
    memset(digest, 0, out_bytes_len);
    return;
  }
  if (base_len) {
    memcpy(msg, ctx->buffer, base_len);
  }
  msg[base_len + 0] = (uint8_t)(ctr);
  msg[base_len + 1] = (uint8_t)(ctr >> 8);
  msg[base_len + 2] = (uint8_t)(ctr >> 16);
  msg[base_len + 3] = (uint8_t)(ctr >> 24);
  if (!hash_with_domain_sep(msg, msg_len, domain_sep_H2_3, digest, out_bytes_len)) {
    memset(digest, 0, out_bytes_len);
  }
  if (msg != stack_buf) {
    free(msg);
  }
}

void H2_3_final_u32_le_batch4(const params_t* params, const H2_context_t* ctx,
                              uint32_t ctr_base, uint8_t* const digest[4],
                              size_t lane_count) {
  {
    const size_t out_bytes_len = H2_output_len(params, 3);
    const int output_bits = (int)(out_bytes_len * 8u);
    const size_t base_len = ctx->buffer_len;
    const size_t lane_msg_len = base_len + 5u; /* base || ctr_le32 || domain_sep */
    const size_t stack_stride = 512u;
    const size_t lane_stride = lane_msg_len <= stack_stride ? stack_stride : lane_msg_len;
    uint8_t stack_lane_buf[4u * stack_stride];
    uint8_t* lane_buf = lane_count == 0u
                            ? NULL
                            : (lane_msg_len <= stack_stride ? stack_lane_buf
                                                            : malloc(lane_count * lane_stride));
    const unsigned long long bit_len = (unsigned long long)lane_msg_len * 8ULL;

    if (lane_buf != NULL) {
      bool ok = true;
      for (size_t lane = 0; lane < lane_count; ++lane) {
        uint8_t* dst = lane_buf + lane * lane_stride;
        if (base_len) {
          memcpy(dst, ctx->buffer, base_len);
        }
        const uint32_t ctr = ctr_base + (uint32_t)lane;
        dst[base_len + 0] = (uint8_t)(ctr);
        dst[base_len + 1] = (uint8_t)(ctr >> 8);
        dst[base_len + 2] = (uint8_t)(ctr >> 16);
        dst[base_len + 3] = (uint8_t)(ctr >> 24);
        dst[base_len + 4] = domain_sep_H2_3;

        int rc = 0;
        if (output_bits == 256) {
          rc = sm3hash(output_bits, dst, bit_len, digest[lane]);
        } else if (output_bits == 512 || output_bits == 768 || output_bits == 1024) {
          rc = pseudohash(output_bits, dst, bit_len, digest[lane]);
        } else if (output_bits > 0 && output_bits <= 256) {
          rc = pseudoXOF_leq256_byte_aligned_local((unsigned long long)output_bits, dst,
                                                   lane_msg_len, digest[lane])
                   ? 0
                   : -1;
        } else {
          rc = pseudoXOF((unsigned long long)output_bits, dst, bit_len, digest[lane]);
        }
        if (rc != 0) {
          ok = false;
          break;
        }
      }

      if (lane_buf != stack_lane_buf) {
        free(lane_buf);
      }
      if (ok) {
        return;
      }
    }
  }
  for (size_t lane = 0; lane < lane_count; ++lane) {
    H2_3_final_u32_le(params, ctx, ctr_base + (uint32_t)lane, digest[lane]);
  }
}

// H_3
void H3_init(H3_context_t* ctx) {
  new_hash_init(ctx);
}

void H3_update(H3_context_t* ctx, const uint8_t* input, size_t bytes_len) {
  new_hash_update(ctx, input, bytes_len);
}

void H3_final(const params_t* params, H3_context_t* ctx, uint8_t* digest, uint8_t* iv) {
  const size_t out_bytes_len = params->lambda_bytes + IV_PRE_SIZE;
  new_hash_final(ctx, domain_sep_H3, out_bytes_len);
  new_hash_squeeze(ctx, digest, params->lambda_bytes);
  new_hash_squeeze(ctx, iv, IV_PRE_SIZE);
  new_hash_clear(ctx);
}

// H_4
void H4_init(H4_context_t* ctx) {
  new_hash_init(ctx);
}

void H4_update(H4_context_t* ctx, const uint8_t* iv) {
  new_hash_update(ctx, iv, IV_PRE_SIZE);
}

void H4_final(H4_context_t* ctx, uint8_t* iv) {
  new_hash_final(ctx, domain_sep_H4, IV_SIZE);
  new_hash_squeeze(ctx, iv, IV_SIZE);
  new_hash_clear(ctx);
}
