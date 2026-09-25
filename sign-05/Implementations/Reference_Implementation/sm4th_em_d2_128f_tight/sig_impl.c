#include "sig_impl_internal.h"
#include "utils_sm4/sm4.h"
#include "sm4th_sm4_128.h"
#include "auxfunc.h"

int sig_verbose = 0;

// sm4th.Sign line 3: mu = H2_0(pk || msg; 2 * lambda protocol bits).
// The concrete output length is selected in H2_output_len().
void hash_mu(const params_t* params, uint8_t* mu, const uint8_t* owf_input,
             size_t owf_input_size, const uint8_t* owf_output, size_t owf_output_size,
             const uint8_t* msg, size_t msglen, unsigned int lambda) {
  (void)lambda;
  H2_context_t h1_ctx;
  H2_init(&h1_ctx);
  H2_update(&h1_ctx, owf_input, owf_input_size);
  H2_update(&h1_ctx, owf_output, owf_output_size);
  H2_update(&h1_ctx, msg, msglen);
  H2_0_final(params, &h1_ctx, mu);
}

void hash_iv(const params_t* params, uint8_t* iv, const uint8_t* iv_pre,
             unsigned int lambda) {
  (void)lambda;
  H4_context_t h4_ctx;
  H4_init(&h4_ctx);
  H4_update(params, &h4_ctx, iv_pre);
  H4_final(params, &h4_ctx, iv);
}

// sm4th.Sign: line 4 + line 5, (r, iv) = H3(owf_key, mu, rho), owf_key = sk->owf_key
void hash_r_iv(const params_t* params, uint8_t* root_key, uint8_t* iv_pre,
               uint8_t* iv, const uint8_t* owf_key, const uint8_t* mu,
               const uint8_t* rho, size_t rho_size, unsigned int lambda) {
  const unsigned int lambda_bytes = params->lambda_bytes;

  {
    H3_context_t h3_ctx;
    H3_init(&h3_ctx);
    H3_update(&h3_ctx, owf_key, lambda_bytes);
    H3_update(&h3_ctx, mu, lambda_bytes * 2);
    H3_update(&h3_ctx, rho, rho_size);

    // root_kay is r, used as GGM tree root
    H3_final(params, &h3_ctx, root_key, iv_pre);
  }

  // Given iv_pre, compute iv = H4(iv_pre)
  hash_iv(params, iv, iv_pre, lambda);
}

// sm4th.Sign: line 8
void hash_challenge_1(const params_t* params, uint8_t* chall_1,
                      const uint8_t* mu, const uint8_t* hcom, const uint8_t* c,
                      const uint8_t* iv, unsigned int lambda, unsigned int ell,
                      unsigned int tau) {
  (void)ell;
  const unsigned int lambda_bytes = lambda / 8;
  const unsigned int ell_hat_bytes = params->ell_hat_bytes;
  const unsigned int hcom_bytes = params->n_leaf * params->prg_block_bytes;

  H2_context_t h2_ctx;
  H2_init(&h2_ctx);
  H2_update(&h2_ctx, mu, lambda_bytes * 2);
  H2_update(&h2_ctx, hcom, hcom_bytes);
  H2_update(&h2_ctx, c, ell_hat_bytes * (tau - 1));
  H2_update(&h2_ctx, iv, params->iv_bytes);
  H2_1_final(params, &h2_ctx, chall_1);
}

void hash_challenge_2_init(H2_context_t* h2_ctx, const uint8_t* chall_1,
                           const uint8_t* u_tilde, unsigned int lambda) {
  const unsigned int lambda_bytes = lambda / 8;
  const unsigned int u_tilde_bytes = lambda_bytes + UNIVERSAL_HASH_B;

  H2_init(h2_ctx);
  H2_update(h2_ctx, chall_1, 5 * lambda_bytes + 8);
  H2_update(h2_ctx, u_tilde, u_tilde_bytes);
}

void hash_challenge_2_update_v_tilde(H2_context_t* h2_ctx, const uint8_t* v_tilde,
                                     unsigned int lambda) {
  const unsigned int lambda_bytes = lambda / 8;
  const unsigned int v_tilde_bytes = lambda_bytes + UNIVERSAL_HASH_B;

  H2_update(h2_ctx, v_tilde, v_tilde_bytes);
}

void hash_challenge_2_update_v_tilde_batch(H2_context_t* h2_ctx, const uint8_t* v_tilde_batch,
                                           size_t lanes, unsigned int lambda) {
  const unsigned int lambda_bytes = lambda / 8;
  const size_t v_tilde_bytes = (size_t)lambda_bytes + UNIVERSAL_HASH_B;
  H2_update(h2_ctx, v_tilde_batch, lanes * v_tilde_bytes);
}

// ell = 1280
void hash_challenge_2_finalize(const params_t* params, uint8_t* chall_2,
                               H2_context_t* h2_ctx, const uint8_t* d,
                               const unsigned int lambda, unsigned int ell) {
  (void)lambda;
  const unsigned int ell_bytes = ell / 8;
  H2_update(h2_ctx, d, ell_bytes);
  H2_2_final(params, h2_ctx, chall_2);
}

void hash_challenge_3_init(H2_context_t* h2_ctx, const uint8_t* chall_2,
                           const uint8_t* a_tilde, unsigned int lambda,
                           unsigned int deg) {
  const unsigned int lambda_bytes = lambda / 8;
  const unsigned int num_a_tilde = deg - 1;

  H2_init(h2_ctx);
  H2_update(h2_ctx, chall_2, 3 * lambda_bytes + 8);
  H2_update(h2_ctx, a_tilde, lambda_bytes * num_a_tilde);
}

void hash_challenge_3_final(const params_t* params, uint8_t* chall_3,
                            const H2_context_t* ctx, uint32_t ctr,
                            unsigned int lambda) {
  (void)lambda;
  H2_3_final_u32_le(params, ctx, ctr, chall_3);
}

void hash_challenge_3_final_batch4(const params_t* params, uint8_t* const chall_3[4],
                                   const H2_context_t* ctx, uint32_t ctr_base,
                                   size_t lane_count, unsigned int lambda) {
  (void)lambda;
  H2_3_final_u32_le_batch4(params, ctx, ctr_base, chall_3, lane_count);
}

void hash_challenge_3(const params_t* params, uint8_t* chall_3,
                      const uint8_t* chall_2, const uint8_t* a_tilde,
                      const uint8_t* ctr, unsigned int lambda) {
  H2_context_t h2_ctx;
  hash_challenge_3_init(&h2_ctx, chall_2, a_tilde, lambda, params->deg);
  H2_update(&h2_ctx, ctr, sizeof(uint32_t));
  H2_3_final(params, &h2_ctx, chall_3);
}

bool check_challenge_3(const uint8_t* chall_3, unsigned int start, unsigned int lambda) {
  if (start >= lambda) {
    return true;
  }

  unsigned int byte_i = start / 8;
  const unsigned int start_bit = start & 7u;
  if (start_bit) {
    const uint8_t mask = (uint8_t)(0xFFu << start_bit);
    if (chall_3[byte_i] & mask) {
      return false;
    }
    ++byte_i;
  }

  const unsigned int full_end = lambda / 8;
  for (; byte_i < full_end; ++byte_i) {
    if (chall_3[byte_i]) {
      return false;
    }
  }

  const unsigned int tail_bits = lambda & 7u;
  if (tail_bits) {
    const uint8_t mask = (uint8_t)((1u << tail_bits) - 1u);
    if (chall_3[full_end] & mask) {
      return false;
    }
  }

  return true;
}

void free_pointer_array(uint8_t*** ptr) {
  free((*ptr)[0]);
  free(*ptr);
  *ptr = NULL;
}
static inline uint8_t chall3_get_bits4(const uint8_t* chall_3, unsigned int bit_pos,
                                       unsigned int lambda_bits) {
  const unsigned int byte_pos = bit_pos >> 3;
  const unsigned int shift = bit_pos & 7u;
  const unsigned int chall_bytes = (lambda_bits + 7u) >> 3;

  uint16_t word = chall_3[byte_pos];
  if (byte_pos + 1u < chall_bytes) {
    word |= (uint16_t)chall_3[byte_pos + 1u] << 8;
  }
  return (uint8_t)((word >> shift) & 0x0Fu);
}

/* Branchless masked xor for 4 lanes packed as batch[ lane * stride + j ].
 * bits low 4 bits indicate whether to xor u_tilde into each lane. */
static inline void xor_u_tilde_masked_batch4(uint8_t* batch, size_t stride,
                                             const uint8_t* u_tilde, size_t utilde_bytes,
                                             uint8_t bits) {
  const uint8_t m0 = (uint8_t)-(int)((bits >> 0) & 1u);
  const uint8_t m1 = (uint8_t)-(int)((bits >> 1) & 1u);
  const uint8_t m2 = (uint8_t)-(int)((bits >> 2) & 1u);
  const uint8_t m3 = (uint8_t)-(int)((bits >> 3) & 1u);
  uint8_t* q0 = batch + 0u * stride;
  uint8_t* q1 = batch + 1u * stride;
  uint8_t* q2 = batch + 2u * stride;
  uint8_t* q3 = batch + 3u * stride;

  for (size_t j = 0; j < utilde_bytes; ++j) {
    const uint8_t u = u_tilde[j];
    q0[j] ^= (uint8_t)(u & m0);
    q1[j] ^= (uint8_t)(u & m1);
    q2[j] ^= (uint8_t)(u & m2);
    q3[j] ^= (uint8_t)(u & m3);
  }
}

/* Must match random_oracle.c H2_3 domain separator. */
#define H2_DOMAIN_SEP_3 11u

typedef struct {
  size_t base_len;
  size_t lane_msg_len;
  size_t lane_stride;
  int output_bits;
  uint8_t stack_lane_buf[4u * 512u];
  uint8_t* lane_buf;
  uint8_t* msg4[4];
} chall3_batch4_portable_ctx_t;

static bool chall3_batch4_portable_init(const params_t* params, const H2_context_t* h2_ctx,
                                        chall3_batch4_portable_ctx_t* batch_ctx) {
  const size_t base_len = h2_ctx->buffer_len;
  const size_t lane_msg_len = base_len + 4u + 1u; /* base || ctr_le32 || domain_sep */
  const size_t lane_stride = lane_msg_len <= 512u ? 512u : lane_msg_len;
  const size_t total_bytes = 4u * lane_stride;
  uint8_t* lane_buf =
      total_bytes <= sizeof(batch_ctx->stack_lane_buf) ? batch_ctx->stack_lane_buf : malloc(total_bytes);
  if (!lane_buf) {
    return false;
  }

  batch_ctx->base_len = base_len;
  batch_ctx->lane_msg_len = lane_msg_len;
  batch_ctx->lane_stride = lane_stride;
  batch_ctx->output_bits = (int)(params->lambda_f_bytes * 8u);
  batch_ctx->lane_buf = lane_buf;

  for (size_t lane = 0; lane < 4u; ++lane) {
    uint8_t* dst = lane_buf + lane * lane_stride;
    if (base_len) {
      memcpy(dst, h2_ctx->buffer, base_len);
    }
    dst[base_len + 4] = H2_DOMAIN_SEP_3;
    batch_ctx->msg4[lane] = dst;
  }
  return true;
}

static bool chall3_batch4_portable_hash(chall3_batch4_portable_ctx_t* batch_ctx, uint32_t ctr_base,
                                        uint8_t* const digest[4]) {
  const size_t base_len = batch_ctx->base_len;
  const unsigned long long bit_len = (unsigned long long)batch_ctx->lane_msg_len * 8ULL;

  for (size_t lane = 0; lane < 4u; ++lane) {
    uint8_t* dst = batch_ctx->msg4[lane];
    const uint32_t ctr = ctr_base + (uint32_t)lane;
    dst[base_len + 0] = (uint8_t)(ctr);
    dst[base_len + 1] = (uint8_t)(ctr >> 8);
    dst[base_len + 2] = (uint8_t)(ctr >> 16);
    dst[base_len + 3] = (uint8_t)(ctr >> 24);

    int rc = 0;
    if (batch_ctx->output_bits == 256) {
      rc = sm3hash(batch_ctx->output_bits, dst, bit_len, digest[lane]);
    } else if (batch_ctx->output_bits == 512 || batch_ctx->output_bits == 768 ||
               batch_ctx->output_bits == 1024) {
      rc = pseudohash(batch_ctx->output_bits, dst, bit_len, digest[lane]);
    } else {
      rc = pseudoXOF((unsigned long long)batch_ctx->output_bits, dst, bit_len, digest[lane]);
    }
    if (rc != 0) {
      return false;
    }
  }
  return true;
}

static void chall3_batch4_portable_clear(chall3_batch4_portable_ctx_t* batch_ctx) {
  if (batch_ctx->lane_buf && batch_ctx->lane_buf != batch_ctx->stack_lane_buf) {
    free(batch_ctx->lane_buf);
  }
  batch_ctx->lane_buf = NULL;
}

void sm4th_sign(const params_t* params, uint8_t* sig, const uint8_t* msg, size_t msg_len,
                const uint8_t* owf_key, const uint8_t* owf_input, const uint8_t* owf_output,
                const uint8_t* witness, const uint8_t* rho, size_t rholen) {
  const char* proto_name = "sm4th";
  SIG_TSTART(t_sign_total);
  const unsigned int ell           = params->ell;
  const unsigned int ell_bytes     = params->ell_bytes;
  const unsigned int lambda        = params->lambda;
  const unsigned int lambda_f      = params->lambda_f;
  const unsigned int lambda_f_bytes = params->lambda_f_bytes;
  const unsigned int tau           = params->tau;
  const unsigned int ell_hat       = params->ell_hat_bits;
  const unsigned int ell_hat_bytes = params->ell_hat_bytes;
  const unsigned int w_grind       = params->w_grind;
  const unsigned int utilde_bytes  = params->utilde_bytes;

  SIG_PRINT("[%s sign] start\n", proto_name);

  uint8_t* mu = malloc(params->lambda_bytes * 2);
  assert(mu);
  SIG_TSTART(t_hash_mu);
  hash_mu(params, mu, owf_input, SM4_BLOCK_SIZE, owf_output, SM4_BLOCK_SIZE, msg, msg_len, lambda);
  SIG_TEND("sign.hash_mu", t_hash_mu);
  SIG_PRINT("[%s sign] hash_mu done\n", proto_name);

  uint8_t* rootkey = malloc(params->lambda_prg_bytes);
  uint8_t iv[IV_SIZE];
  assert(rootkey);
  SIG_TSTART(t_hash_r_iv);
  hash_r_iv(params, rootkey, signature_iv_pre(params, sig), iv, owf_key, mu, rho, rholen, lambda);
  SIG_TEND("sign.hash_r_iv", t_hash_r_iv);
  SIG_PRINT("[%s sign] hash_r_iv done\n", proto_name);

  bavc_t bavc;
  uint8_t* u = malloc(ell_hat_bytes);
  assert(u);
  uint8_t** V = malloc(lambda_f * sizeof(uint8_t*));
  assert(V);
  V[0] = calloc(lambda_f, ell_hat_bytes);
  assert(V[0]);
  for (unsigned int i = 1; i < lambda_f; ++i) {
    V[i] = V[0] + i * ell_hat_bytes;
  }
  SIG_TSTART(t_vole_commit);
  vole_commit(params, rootkey, iv, ell_hat, &bavc, signature_c(params, sig, 0), u, V);
  SIG_TEND("sign.vole_commit", t_vole_commit);
  SIG_PRINT("[%s sign] vole_commit done\n", proto_name);

  uint8_t* chall_1 = malloc(5u * lambda_f_bytes + 8u);
  assert(chall_1);
  SIG_TSTART(t_chall_1);
  hash_challenge_1(params, chall_1, mu, bavc.h, signature_c(params, sig, 0), iv, lambda, ell, tau);
  SIG_TEND("sign.hash_challenge_1.pseudoXOF_5lambda+64", t_chall_1);
  SIG_PRINT("[%s sign] chall_1 done\n", proto_name);

  SIG_TSTART(t_u_tilde);
  vole_hash(signature_u_tilde(params, sig), chall_1, u, ell, lambda_f);
  SIG_TEND("sign.vole_hash_u_tilde", t_u_tilde);
  SIG_PRINT("[%s sign] u_tilde done\n", proto_name);

  H2_context_t chall_2_ctx;
  SIG_TSTART(t_chall_2_total);
  hash_challenge_2_init(&chall_2_ctx, chall_1, signature_u_tilde(params, sig), lambda_f);
  vole_hash_precomp_t vh_precomp_sign;
  const int has_vh_precomp_sign =
      vole_hash_precompute_init(&vh_precomp_sign, chall_1, ell, lambda_f);
  assert(has_vh_precomp_sign);
  (void)has_vh_precomp_sign;

  {
    uint8_t V_tilde_batch[4u * utilde_bytes];
    uint8_t* V_tilde0 = V_tilde_batch + 0u * utilde_bytes;
    uint8_t* V_tilde1 = V_tilde_batch + 1u * utilde_bytes;
    uint8_t* V_tilde2 = V_tilde_batch + 2u * utilde_bytes;
    uint8_t* V_tilde3 = V_tilde_batch + 3u * utilde_bytes;
    unsigned int i = 0;
    for (; i + 3 < lambda_f; i += 4) {
      vole_hash_4_precomp(V_tilde0, V_tilde1, V_tilde2, V_tilde3, &vh_precomp_sign, V[i],
                          V[i + 1], V[i + 2], V[i + 3]);
      hash_challenge_2_update_v_tilde_batch(&chall_2_ctx, V_tilde_batch, 4u, lambda_f);
    }
    if (i + 1 < lambda_f) {
      vole_hash_2_precomp(V_tilde0, V_tilde1, &vh_precomp_sign, V[i], V[i + 1]);
      hash_challenge_2_update_v_tilde_batch(&chall_2_ctx, V_tilde_batch, 2u, lambda_f);
      i += 2;
    }
    if (i < lambda_f) {
      vole_hash_precomp(V_tilde0, &vh_precomp_sign, V[i]);
      hash_challenge_2_update_v_tilde(&chall_2_ctx, V_tilde0, lambda_f);
    }
  }
  vole_hash_precompute_clear(&vh_precomp_sign);

  xor_u8_array(witness, u, signature_d(params, sig), ell_bytes);

  uint8_t* chall_2 = malloc(3u * lambda_f_bytes + 8u);
  assert(chall_2);
  hash_challenge_2_finalize(params, chall_2, &chall_2_ctx, signature_d(params, sig), lambda_f, ell);
  SIG_TEND("sign.hash_challenge_2.pseudoXOF_3lambda+64", t_chall_2_total);
  SIG_PRINT("[%s sign] chall_2 done\n", proto_name);

  uint8_t* a0_tilde = malloc(lambda_f_bytes);
  assert(a0_tilde);
  SIG_TSTART(t_sm4_prove);
  sm4_128_prover(params, a0_tilde, signature_a1_tilde(params, sig), witness, u + ell_bytes,
            V, owf_input, owf_output, chall_2);
  SIG_TEND("sign.sm4_prove", t_sm4_prove);
  SIG_PRINT("[%s sign] sm4_prove done\n", proto_name);

  free_pointer_array(&V);
  free(u);
  u = NULL;

  H2_context_t chall_3_ctx;
  SIG_TSTART(t_chall_3);
  hash_challenge_3_init(&chall_3_ctx, chall_2, signature_a1_tilde(params, sig), lambda_f,
                        params->deg);

  uint8_t chall_3_batch[4][params->lambda_f_bytes];
  uint8_t* chall_3_ptrs[4] = {
      chall_3_batch[0], chall_3_batch[1], chall_3_batch[2], chall_3_batch[3]};
  uint16_t* decoded_chall_3 = malloc(sizeof(uint16_t) * tau);
  assert(decoded_chall_3);

  uint32_t ctr = 0;
  chall3_batch4_portable_ctx_t chall3_batch_ctx;
  const bool use_chall3_batch_portable =
      chall3_batch4_portable_init(params, &chall_3_ctx, &chall3_batch_ctx);
  for (;;) {
    if (!use_chall3_batch_portable ||
        !chall3_batch4_portable_hash(&chall3_batch_ctx, ctr, chall_3_ptrs)) {
      hash_challenge_3_final_batch4(params, chall_3_ptrs, &chall_3_ctx, ctr, 4u, lambda);
    }

    bool done = false;
    for (size_t lane = 0; lane < 4u; ++lane) {
      const uint8_t* chall_3 = chall_3_ptrs[lane];
      if (!check_challenge_3(chall_3, lambda_f - w_grind, lambda_f)) {
        continue;
      }
      if (!decode_all_chall_3(params, decoded_chall_3, chall_3)) {
        continue;
      }
      if (bavc_open(params, signature_decom_i(params, sig), &bavc, decoded_chall_3)) {
        memcpy(signature_chall_3(params, sig), chall_3, lambda_f_bytes);
        ctr += (uint32_t)lane;
        done = true;
        break;
      }
    }

    if (done) {
      break;
    }
    ctr += 4u;
  }
  if (use_chall3_batch_portable) {
    chall3_batch4_portable_clear(&chall3_batch_ctx);
  }
  free(decoded_chall_3);
  SIG_TEND("sign.challenge_3_loop.pseudoXOF_lambda", t_chall_3);
  SIG_PRINT("[%s sign] challenge_3 loop done\n", proto_name);
  SIG_TSTART(t_sign_cleanup);
  new_hash_clear(&chall_3_ctx);
  bavc_clear(&bavc);

  ctr = htole32(ctr);
  memcpy(signature_ctr(params, sig), &ctr, sizeof(ctr));

  free(chall_2);
  free(chall_1);
  free(a0_tilde);
  free(rootkey);
  free(mu);

  SIG_TEND("sign.total", t_sign_total);
  SIG_PRINT("[%s sign] end\n", proto_name);
}

int sm4th_verify(const params_t* params, const uint8_t* msg, size_t msglen,
                 const uint8_t* sig, const uint8_t* owf_input, const uint8_t* owf_output) {
  const char* proto_name = "sm4th";
  SIG_TSTART(t_verify_total);
  const unsigned int ell           = params->ell;
  const unsigned int lambda        = params->lambda;
  const unsigned int lambda_f      = params->lambda_f;
  const unsigned int lambda_f_bytes = params->lambda_f_bytes;
  const unsigned int tau           = params->tau;
  const unsigned int ell_hat       = params->ell_hat_bits;
  const unsigned int ell_hat_bytes = params->ell_hat_bytes;
  const unsigned int utilde_bytes  = params->utilde_bytes;

  SIG_PRINT("[%s verify] start\n", proto_name);

  SIG_TSTART(t_check_chall3);
  if (!check_challenge_3(dsignature_chall_3(params, sig), lambda_f - params->w_grind, lambda_f)) {
    SIG_TEND("verify.check_challenge_3", t_check_chall3);
    return -1;
  }
  SIG_TEND("verify.check_challenge_3", t_check_chall3);

  uint8_t* mu = malloc(params->lambda_bytes * 2);
  assert(mu);
  SIG_TSTART(t_hash_mu_v);
  hash_mu(params, mu, owf_input, SM4_BLOCK_SIZE, owf_output, SM4_BLOCK_SIZE, msg, msglen, lambda);
  SIG_TEND("verify.hash_mu", t_hash_mu_v);
  SIG_PRINT("[%s verify] hash_mu done\n", proto_name);

  uint8_t iv[IV_SIZE];
  SIG_TSTART(t_hash_iv);
  hash_iv(params, iv, dsignature_iv_pre(params, sig), lambda);
  SIG_TEND("verify.hash_iv", t_hash_iv);
  SIG_PRINT("[%s verify] hash_iv done\n", proto_name);

  uint8_t** q = malloc(lambda_f * sizeof(uint8_t*));
  assert(q);
  q[0] = calloc(lambda_f, ell_hat_bytes);
  assert(q[0]);
  for (unsigned int i = 1; i < lambda_f; ++i) {
    q[i] = q[0] + i * ell_hat_bytes;
  }
  uint8_t* hcom = malloc(params->n_leaf * params->prg_block_bytes);
  assert(hcom);

  SIG_TSTART(t_vole_reconstruct);
  if (!vole_reconstruct(params, hcom, q, iv, dsignature_chall_3(params, sig),
                        dsignature_decom_i(params, sig), dsignature_c(params, sig, 0), ell_hat)) {
    free_pointer_array(&q);
    free(hcom);
    free(mu);
    SIG_TEND("verify.vole_reconstruct", t_vole_reconstruct);
    return -1;
  }
  SIG_TEND("verify.vole_reconstruct", t_vole_reconstruct);
  SIG_PRINT("[%s verify] vole_reconstruct done\n", proto_name);

  uint8_t* chall_1 = malloc(5u * lambda_f_bytes + 8u);
  assert(chall_1);
  SIG_TSTART(t_chall1_v);
  hash_challenge_1(params, chall_1, mu, hcom, dsignature_c(params, sig, 0), iv, lambda, ell, tau);
  SIG_TEND("verify.hash_challenge_1.pseudoXOF_5lambda+64", t_chall1_v);
  SIG_PRINT("[%s verify] chall_1 done\n", proto_name);

  H2_context_t chall_2_ctx;
  hash_challenge_2_init(&chall_2_ctx, chall_1, dsignature_u_tilde(params, sig), lambda_f);
  vole_hash_precomp_t vh_precomp;
  const int has_vh_precomp = vole_hash_precompute_init(&vh_precomp, chall_1, ell, lambda_f);
  assert(has_vh_precomp);
  (void)has_vh_precomp;

  {
    const uint8_t* chall_3 = dsignature_chall_3(params, sig);
    uint8_t Q_tilde_batch[4u * utilde_bytes];
    uint8_t* Q_tilde0 = Q_tilde_batch + 0u * utilde_bytes;
    uint8_t* Q_tilde1 = Q_tilde_batch + 1u * utilde_bytes;
    uint8_t* Q_tilde2 = Q_tilde_batch + 2u * utilde_bytes;
    uint8_t* Q_tilde3 = Q_tilde_batch + 3u * utilde_bytes;
    const uint8_t* u_tilde = dsignature_u_tilde(params, sig);
    unsigned int i = 0;
    for (; i + 3 < lambda_f; i += 4) {
      vole_hash_4_precomp(Q_tilde0, Q_tilde1, Q_tilde2, Q_tilde3, &vh_precomp, q[i], q[i + 1],
                          q[i + 2], q[i + 3]);
      const uint8_t bits = chall3_get_bits4(chall_3, i, lambda_f);
      xor_u_tilde_masked_batch4(Q_tilde_batch, utilde_bytes, u_tilde, utilde_bytes, bits);
      hash_challenge_2_update_v_tilde_batch(&chall_2_ctx, Q_tilde_batch, 4u, lambda_f);
    }
    if (i + 1 < lambda_f) {
      vole_hash_2_precomp(Q_tilde0, Q_tilde1, &vh_precomp, q[i], q[i + 1]);
      const uint8_t bits = (uint8_t)(chall3_get_bits4(chall_3, i, lambda_f) & 0x03u);
      xor_u_tilde_masked_batch4(Q_tilde_batch, utilde_bytes, u_tilde, utilde_bytes, bits);
      hash_challenge_2_update_v_tilde_batch(&chall_2_ctx, Q_tilde_batch, 2u, lambda_f);
      i += 2;
    }
    if (i < lambda_f) {
      vole_hash_precomp(Q_tilde0, &vh_precomp, q[i]);
      const uint8_t bits = (uint8_t)(chall3_get_bits4(chall_3, i, lambda_f) & 0x01u);
      xor_u_tilde_masked_batch4(Q_tilde_batch, utilde_bytes, u_tilde, utilde_bytes, bits);
      hash_challenge_2_update_v_tilde(&chall_2_ctx, Q_tilde0, lambda_f);
    }
  }
  vole_hash_precompute_clear(&vh_precomp);

  uint8_t* chall_2 = malloc(3u * lambda_f_bytes + 8u);
  assert(chall_2);
  hash_challenge_2_finalize(params, chall_2, &chall_2_ctx, dsignature_d(params, sig), lambda_f, ell);
  SIG_PRINT("[%s verify] chall_2 done\n", proto_name);

  const uint8_t* d = dsignature_d(params, sig);
  uint8_t* a0_tilde = malloc(lambda_f_bytes);
  assert(a0_tilde);
  SIG_TSTART(t_sm4_verify);
  sm4_128_verifier(params, a0_tilde, d, q, owf_input, owf_output, chall_2, dsignature_chall_3(params, sig), dsignature_a1_tilde(params, sig));
  SIG_TEND("verify.sm4_verify", t_sm4_verify);
  SIG_PRINT("[%s verify] sm4_verify done\n", proto_name);
  free_pointer_array(&q);

  uint8_t* chall_3 = malloc(lambda_f_bytes);
  assert(chall_3);
  SIG_TSTART(t_hash_chall3);
  hash_challenge_3(params, chall_3, chall_2, dsignature_a1_tilde(params, sig),
                   dsignature_ctr(params, sig), lambda_f);
  SIG_TEND("verify.hash_challenge_3.pseudoXOF_lambda", t_hash_chall3);
  SIG_PRINT("[%s verify] chall_3 done\n", proto_name);

  int result = memcmp(chall_3, dsignature_chall_3(params, sig), lambda_f_bytes) == 0 ? 0 : -1;
  SIG_PRINT("[%s verify] end\n", proto_name);
  SIG_TEND("verify.total", t_verify_total);
  free(chall_3);
  free(a0_tilde);
  free(chall_2);
  free(chall_1);
  free(hcom);
  free(mu);
  return result;
}
