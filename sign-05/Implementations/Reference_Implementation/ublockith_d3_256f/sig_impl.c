/*
 * Shared sign/verify helpers used by AES/SM4/uBlock protocol-specific implementations.
 */

#include "sig_impl_internal.h"

/* verbosity flag controlling status prints in this module */
int sig_verbose = 0;

void sig_set_verbose(int v) {
  sig_verbose = v ? 1 : 0;
}

/* Lightweight debug for 128f investigations */
#define SIG_DEBUG_128F

#ifdef SIG_TIMING
uint64_t sig_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ((uint64_t)ts.tv_sec * 1000000000ULL) + (uint64_t)ts.tv_nsec;
}

void sig_print_timing(const char* label, uint64_t ns) {
  fprintf(stderr, "[sig] %s: %.3f ms\n", label, (double)ns / 1e6);
}
#endif

// UBLOCKITH.Sign: line 3, mu = H2_0(pk||msg; 2lambda), the output length is fixed in H2 functions
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

void hash_iv(uint8_t* iv, const uint8_t* iv_pre, unsigned int lambda) {
  (void)lambda;
  H4_context_t h4_ctx;
  H4_init(&h4_ctx);
  H4_update(&h4_ctx, iv_pre);
  H4_final(&h4_ctx, iv);
}

// UBLOCKITH.Sign: line 4 + line 5, (r, iv) = H3(owf_key, mu, rho), owf_key = sk->owf_key
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
  hash_iv(iv, iv_pre, lambda);
}

// UBLOCKITH.Sign: line 8
void hash_challenge_1(const params_t* params, uint8_t* chall_1,
                      const uint8_t* mu, const uint8_t* hcom, const uint8_t* c,
                      const uint8_t* iv, unsigned int lambda, unsigned int ell,
                      unsigned int tau) {
  (void)ell;
  const unsigned int lambda_bytes = lambda / 8;
  const unsigned int ell_hat_bytes = params->ell_hat_bytes;

  H2_context_t h2_ctx;
  H2_init(&h2_ctx);
  H2_update(&h2_ctx, mu, lambda_bytes * 2);
  H2_update(&h2_ctx, hcom, lambda_bytes * 2);
  H2_update(&h2_ctx, c, ell_hat_bytes * (tau - 1));
  H2_update(&h2_ctx, iv, IV_SIZE);
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
