/*
* General internal definitions for the signature implementation, not specific to any particular protocol.
*/

#ifndef SIG_IMPL_INTERNAL_H
#define SIG_IMPL_INTERNAL_H

#include "sig_impl.h"
#include "random_oracle.h"
#include "universal_hashing.h"
#include "utils.h"
#include "vole.h"
#include "sig_timing.h"

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern int sig_verbose;

#define SIG_PRINT(...) do { if (sig_verbose) fprintf(stderr, __VA_ARGS__); } while(0)

static inline uint8_t* signature_c(const params_t* params, uint8_t* base_ptr,
                                   unsigned int index) {
  return base_ptr + index * params->ell_hat_bytes;
}

static inline uint8_t* signature_u_tilde(const params_t* params, uint8_t* base_ptr) {
  return base_ptr + (params->tau - 1) * params->ell_hat_bytes;
}

static inline uint8_t* signature_d(const params_t* params, uint8_t* base_ptr) {
  return base_ptr + (params->tau - 1) * params->ell_hat_bytes + params->utilde_bytes;
}

static inline uint8_t* signature_a1_tilde(const params_t* params, uint8_t* base_ptr) {
  return base_ptr + (params->tau - 1) * params->ell_hat_bytes + params->utilde_bytes +
         params->ell_bytes;
}

static inline uint8_t* signature_decom_i(const params_t* params, uint8_t* base_ptr) {
  return base_ptr + (params->tau - 1) * params->ell_hat_bytes + params->utilde_bytes +
         params->ell_bytes + (params->deg - 1) * params->lambda_f_bytes;
}

static inline uint8_t* signature_a2_tilde(const params_t* params, uint8_t* base_ptr) {
  return signature_a1_tilde(params, base_ptr) + params->lambda_f_bytes;
}

static inline uint8_t* signature_chall_3(const params_t* params, uint8_t* base_ptr) {
  return base_ptr + params->sig_size - sizeof(uint32_t) - params->lambda_iv_bytes -
         params->lambda_f_bytes;
}

static inline uint8_t* signature_iv_pre(const params_t* params, uint8_t* base_ptr) {
  return base_ptr + params->sig_size - sizeof(uint32_t) - params->lambda_iv_bytes;
}

static inline uint8_t* signature_ctr(const params_t* params, uint8_t* base_ptr) {
  return base_ptr + params->sig_size - sizeof(uint32_t);
}

static inline const uint8_t* dsignature_c(const params_t* params, const uint8_t* base_ptr,
                                          unsigned int index) {
  return base_ptr + index * params->ell_hat_bytes;
}

static inline const uint8_t* dsignature_u_tilde(const params_t* params,
                                                const uint8_t* base_ptr) {
  return base_ptr + (params->tau - 1) * params->ell_hat_bytes;
}

static inline const uint8_t* dsignature_d(const params_t* params,
                                          const uint8_t* base_ptr) {
  return base_ptr + (params->tau - 1) * params->ell_hat_bytes + params->utilde_bytes;
}

static inline const uint8_t* dsignature_a1_tilde(const params_t* params,
                                                 const uint8_t* base_ptr) {
  return base_ptr + (params->tau - 1) * params->ell_hat_bytes + params->utilde_bytes +
         params->ell_bytes;
}

static inline const uint8_t* dsignature_decom_i(const params_t* params,
                                                const uint8_t* base_ptr) {
  return base_ptr + (params->tau - 1) * params->ell_hat_bytes + params->utilde_bytes +
         params->ell_bytes + (params->deg - 1) * params->lambda_f_bytes;
}

static inline const uint8_t* dsignature_a2_tilde(const params_t* params,
                                                 const uint8_t* base_ptr) {
  return dsignature_a1_tilde(params, base_ptr) + params->lambda_f_bytes;
}

static inline const uint8_t* dsignature_chall_3(const params_t* params,
                                                const uint8_t* base_ptr) {
  return base_ptr + params->sig_size - sizeof(uint32_t) - params->lambda_iv_bytes -
         params->lambda_f_bytes;
}

static inline const uint8_t* dsignature_iv_pre(const params_t* params,
                                               const uint8_t* base_ptr) {
  return base_ptr + params->sig_size - sizeof(uint32_t) - params->lambda_iv_bytes;
}

static inline const uint8_t* dsignature_ctr(const params_t* params,
                                            const uint8_t* base_ptr) {
  return base_ptr + params->sig_size - sizeof(uint32_t);
}

void hash_mu(const params_t* params, uint8_t* mu, const uint8_t* owf_input,
             size_t owf_input_size, const uint8_t* owf_output, size_t owf_output_size,
             const uint8_t* msg, size_t msglen, unsigned int lambda);
void hash_iv(uint8_t* iv, const uint8_t* iv_pre, unsigned int lambda);
void hash_r_iv(const params_t* params, uint8_t* root_key, uint8_t* iv_pre,
               uint8_t* iv, const uint8_t* owf_key, const uint8_t* mu,
               const uint8_t* rho, size_t rho_size, unsigned int lambda);
void hash_challenge_1(const params_t* params, uint8_t* chall_1,
                      const uint8_t* mu, const uint8_t* hcom, const uint8_t* c,
                      const uint8_t* iv, unsigned int lambda, unsigned int ell,
                      unsigned int tau);
void hash_challenge_2_init(H2_context_t* h2_ctx, const uint8_t* chall_1,
                           const uint8_t* u_tilde, unsigned int lambda);
void hash_challenge_2_update_v_tilde(H2_context_t* h2_ctx, const uint8_t* v_tilde,
                                     unsigned int lambda);
void hash_challenge_2_update_v_tilde_batch(H2_context_t* h2_ctx, const uint8_t* v_tilde_batch,
                                           size_t lanes, unsigned int lambda);
void hash_challenge_2_finalize(const params_t* params, uint8_t* chall_2,
                               H2_context_t* h2_ctx, const uint8_t* d,
                               unsigned int lambda, unsigned int ell);
void hash_challenge_3_init(H2_context_t* h2_ctx, const uint8_t* chall_2,
                           const uint8_t* a_tilde, unsigned int lambda,
                           unsigned int deg);
void hash_challenge_3_final(const params_t* params, uint8_t* chall_3,
                            const H2_context_t* ctx, uint32_t ctr,
                            unsigned int lambda);
void hash_challenge_3_final_batch4(const params_t* params, uint8_t* const chall_3[4],
                                   const H2_context_t* ctx, uint32_t ctr_base,
                                   size_t lane_count, unsigned int lambda);
void hash_challenge_3(const params_t* params, uint8_t* chall_3,
                      const uint8_t* chall_2, const uint8_t* a_tilde,
                      const uint8_t* ctr, unsigned int lambda);
bool check_challenge_3(const uint8_t* chall_3, unsigned int start, unsigned int lambda);
void free_pointer_array(uint8_t*** ptr);

#endif
