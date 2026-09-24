#include "sig_impl_internal.h"
#include "utils_ublock/ublock.h"
#include "utils_ublock/ublockith_ublock_256.h"

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

void ublockith_sign(const params_t* params, uint8_t* sig, const uint8_t* msg, size_t msg_len,
                    const uint8_t* owf_key, const uint8_t* owf_input, const uint8_t* owf_output,
                    const uint8_t* witness, const uint8_t* rho, size_t rholen) {
  const char* proto_name = "ublockith";
  SIG_TSTART(t_sign_total);
  const unsigned int ell = params->ell;
  const unsigned int ell_bytes = params->ell_bytes;
  const unsigned int lambda = params->lambda;
  const unsigned int tau = params->tau;
  const unsigned int ell_hat = params->ell_hat_bits;
  const unsigned int ell_hat_bytes = params->ell_hat_bytes;
  const unsigned int w_grind = params->w_grind;
  const unsigned int utilde_bytes = params->utilde_bytes;

  assert(params->deg == 3);
  SIG_PRINT("[%s sign] start\n", proto_name);

  uint8_t* mu = malloc(params->lambda_bytes * 2);
  assert(mu);
  SIG_TSTART(t_hash_mu);
  hash_mu(params, mu, owf_input, UBLOCK_BLOCK, owf_output, UBLOCK_BLOCK, msg, msg_len, lambda);
  SIG_TEND("sign.hash_mu", t_hash_mu);

  uint8_t* rootkey = malloc(params->lambda_bytes);
  uint8_t iv[IV_SIZE];
  assert(rootkey);
  SIG_TSTART(t_hash_r_iv);
  hash_r_iv(params, rootkey, signature_iv_pre(params, sig), iv, owf_key, mu, rho, rholen, lambda);
  SIG_TEND("sign.hash_r_iv", t_hash_r_iv);

  bavc_t bavc;
  uint8_t* u = malloc(ell_hat_bytes);
  assert(u);
  uint8_t** V = malloc(lambda * sizeof(uint8_t*));
  assert(V);
  V[0] = calloc(lambda, ell_hat_bytes);
  assert(V[0]);
  for (unsigned int i = 1; i < lambda; ++i) {
    V[i] = V[0] + i * ell_hat_bytes;
  }
  SIG_TSTART(t_vole_commit);
  vole_commit(params, rootkey, iv, ell_hat, &bavc, signature_c(params, sig, 0), u, V);
  SIG_TEND("sign.vole_commit", t_vole_commit);

  uint8_t* chall_1 = malloc(5u * params->lambda_bytes + 8u);
  assert(chall_1);
  SIG_TSTART(t_chall_1);
  hash_challenge_1(params, chall_1, mu, bavc.h, signature_c(params, sig, 0), iv, lambda, ell, tau);
  SIG_TEND("sign.hash_challenge_1.pseudoXOF_5lambda+64", t_chall_1);

  SIG_TSTART(t_u_tilde);
  vole_hash(signature_u_tilde(params, sig), chall_1, u, ell, lambda);
  SIG_TEND("sign.vole_hash_u_tilde", t_u_tilde);

  H2_context_t chall_2_ctx;
#ifdef SIG_TIMING
  uint64_t t_vole_hash_ns = 0;
  uint64_t t_pseudoXOF_ns = 0;
  uint64_t t_stage = sig_now_ns();
#endif
  hash_challenge_2_init(&chall_2_ctx, chall_1, signature_u_tilde(params, sig), lambda);
#ifdef SIG_TIMING
  t_pseudoXOF_ns += sig_now_ns() - t_stage;
#endif
  vole_hash_precomp_t vh_precomp_sign;
  const int has_vh_precomp_sign =
      vole_hash_precompute_init(&vh_precomp_sign, chall_1, ell, lambda);

  {
    uint8_t V_tilde_batch[4u * (BF256_NUM_BYTES + UNIVERSAL_HASH_B)];
    uint8_t* V_tilde0 = V_tilde_batch + 0u * utilde_bytes;
    uint8_t* V_tilde1 = V_tilde_batch + 1u * utilde_bytes;
    uint8_t* V_tilde2 = V_tilde_batch + 2u * utilde_bytes;
    uint8_t* V_tilde3 = V_tilde_batch + 3u * utilde_bytes;
    unsigned int i = 0;
    for (; i + 3 < lambda; i += 4) {
#ifdef SIG_TIMING
      t_stage = sig_now_ns();
#endif
      if (has_vh_precomp_sign) {
        vole_hash_4_precomp(V_tilde0, V_tilde1, V_tilde2, V_tilde3, &vh_precomp_sign, V[i],
                            V[i + 1], V[i + 2], V[i + 3]);
      } else {
        vole_hash_4(V_tilde0, V_tilde1, V_tilde2, V_tilde3, chall_1, V[i], V[i + 1], V[i + 2],
                    V[i + 3], ell, lambda);
      }
#ifdef SIG_TIMING
      t_vole_hash_ns += sig_now_ns() - t_stage;
      t_stage = sig_now_ns();
#endif
      hash_challenge_2_update_v_tilde_batch(&chall_2_ctx, V_tilde_batch, 4u, lambda);
#ifdef SIG_TIMING
      t_pseudoXOF_ns += sig_now_ns() - t_stage;
#endif
    }
    if (i + 1 < lambda) {
#ifdef SIG_TIMING
      t_stage = sig_now_ns();
#endif
      if (has_vh_precomp_sign) {
        vole_hash_2_precomp(V_tilde0, V_tilde1, &vh_precomp_sign, V[i], V[i + 1]);
      } else {
        vole_hash_2(V_tilde0, V_tilde1, chall_1, V[i], V[i + 1], ell, lambda);
      }
#ifdef SIG_TIMING
      t_vole_hash_ns += sig_now_ns() - t_stage;
      t_stage = sig_now_ns();
#endif
      hash_challenge_2_update_v_tilde_batch(&chall_2_ctx, V_tilde_batch, 2u, lambda);
#ifdef SIG_TIMING
      t_pseudoXOF_ns += sig_now_ns() - t_stage;
#endif
      i += 2;
    }
    if (i < lambda) {
#ifdef SIG_TIMING
      t_stage = sig_now_ns();
#endif
      if (has_vh_precomp_sign) {
        vole_hash_precomp(V_tilde0, &vh_precomp_sign, V[i]);
      } else {
        vole_hash(V_tilde0, chall_1, V[i], ell, lambda);
      }
#ifdef SIG_TIMING
      t_vole_hash_ns += sig_now_ns() - t_stage;
      t_stage = sig_now_ns();
#endif
      hash_challenge_2_update_v_tilde(&chall_2_ctx, V_tilde0, lambda);
#ifdef SIG_TIMING
      t_pseudoXOF_ns += sig_now_ns() - t_stage;
#endif
    }
  }
  vole_hash_precompute_clear(&vh_precomp_sign);

  xor_u8_array(witness, u, signature_d(params, sig), ell_bytes);

  uint8_t* chall_2 = malloc(3u * params->lambda_bytes + 8u);
  assert(chall_2);
#ifdef SIG_TIMING
  t_stage = sig_now_ns();
#endif
  hash_challenge_2_finalize(params, chall_2, &chall_2_ctx, signature_d(params, sig), lambda, ell);
#ifdef SIG_TIMING
  t_pseudoXOF_ns += sig_now_ns() - t_stage;
  sig_print_timing("sign.hash_challenge_2.vole_hash_V_tillde", t_vole_hash_ns);
  sig_print_timing("sign.hash_challenge_2.challenge_2_pseudoXOF_3lambda+64", t_pseudoXOF_ns);
#endif

  uint8_t* a0_tilde = malloc(params->lambda_bytes);
  assert(a0_tilde);
  SIG_TSTART(t_ublock_prove);
  ublock_256_prover(params, a0_tilde, signature_a1_tilde(params, sig), signature_a2_tilde(params, sig),
               witness, u + ell_bytes, V, owf_input, owf_output, chall_2);
  SIG_TEND("sign.ublock_prove", t_ublock_prove);

  free_pointer_array(&V);
  free(u);
  u = NULL;

  H2_context_t chall_3_ctx;
  SIG_TSTART(t_chall_3);
  hash_challenge_3_init(&chall_3_ctx, chall_2, signature_a1_tilde(params, sig), lambda,
                        params->deg);

  uint8_t chall_3_batch[4][params->lambda_bytes];
  uint8_t* chall_3_ptrs[4] = {
      chall_3_batch[0], chall_3_batch[1], chall_3_batch[2], chall_3_batch[3]};
  uint16_t* decoded_chall_3 = malloc(sizeof(uint16_t) * tau);
  assert(decoded_chall_3);
#ifdef SIG_TIMING
  uint64_t t_chall3_hash_ns = 0;
  uint64_t t_chall3_open_ns = 0;
  uint64_t t_chall3_stage = 0;
#endif

  uint32_t ctr = 0;
  for (;;) {
#ifdef SIG_TIMING
    t_chall3_stage = sig_now_ns();
#endif
    hash_challenge_3_final_batch4(params, chall_3_ptrs, &chall_3_ctx, ctr, 4u, lambda);
#ifdef SIG_TIMING
    t_chall3_hash_ns += sig_now_ns() - t_chall3_stage;
    t_chall3_stage = sig_now_ns();
#endif

    bool done = false;
    for (size_t lane = 0; lane < 4u; ++lane) {
      const uint8_t* chall_3 = chall_3_ptrs[lane];
      if (!check_challenge_3(chall_3, lambda - w_grind, lambda)) {
        continue;
      }
      if (!decode_all_chall_3(params, decoded_chall_3, chall_3)) {
        continue;
      }
      if (bavc_open(params, signature_decom_i(params, sig), &bavc, decoded_chall_3)) {
        memcpy(signature_chall_3(params, sig), chall_3, params->lambda_bytes);
        ctr += (uint32_t)lane;
        done = true;
        break;
      }
    }

    if (done) {
#ifdef SIG_TIMING
      t_chall3_open_ns += sig_now_ns() - t_chall3_stage;
#endif
      break;
    }
#ifdef SIG_TIMING
    t_chall3_open_ns += sig_now_ns() - t_chall3_stage;
#endif
    ctr += 4u;
  }
  free(decoded_chall_3);
  SIG_TEND("sign.challenge_3_loop.pseudoXOF_lambda", t_chall_3);
#ifdef SIG_TIMING
  sig_print_timing("sign.challenge_3_loop.hash_batch4", t_chall3_hash_ns);
  sig_print_timing("sign.challenge_3_loop.check_decode_open", t_chall3_open_ns);
#endif
  new_hash_clear(&chall_3_ctx);
  bavc_clear(&bavc);

  ctr = htole32(ctr);
  memcpy(signature_ctr(params, sig), &ctr, sizeof(ctr));

  free(chall_2);
  free(chall_1);
  free(a0_tilde);
  free(rootkey);
  free(mu);

  SIG_PRINT("[%s sign] end\n", proto_name);
  SIG_TEND("sign.total", t_sign_total);
}

int ublockith_verify(const params_t* params, const uint8_t* msg, size_t msglen,
                     const uint8_t* sig, const uint8_t* owf_input, const uint8_t* owf_output) {
  const char* proto_name = "ublockith";
  SIG_TSTART(t_verify_total);
  const unsigned int ell = params->ell;
  const unsigned int lambda = params->lambda;
  const unsigned int lambda_bytes = params->lambda_bytes;
  const unsigned int tau = params->tau;
  const unsigned int ell_hat = params->ell_hat_bits;
  const unsigned int ell_hat_bytes = params->ell_hat_bytes;
  const unsigned int utilde_bytes = params->utilde_bytes;

  assert(params->deg == 3);
  SIG_PRINT("[%s verify] start\n", proto_name);

  SIG_TSTART(t_check_chall3);
  if (!check_challenge_3(dsignature_chall_3(params, sig), lambda - params->w_grind, lambda)) {
    SIG_TEND("verify.check_challenge_3", t_check_chall3);
    return -1;
  }
  SIG_TEND("verify.check_challenge_3", t_check_chall3);

  uint8_t* mu = malloc(params->lambda_bytes * 2);
  assert(mu);
  SIG_TSTART(t_hash_mu_v);
  hash_mu(params, mu, owf_input, UBLOCK_BLOCK, owf_output, UBLOCK_BLOCK, msg, msglen, lambda);
  SIG_TEND("verify.hash_mu", t_hash_mu_v);

  uint8_t iv[IV_SIZE];
  SIG_TSTART(t_hash_iv);
  hash_iv(iv, dsignature_iv_pre(params, sig), lambda);
  SIG_TEND("verify.hash_iv", t_hash_iv);

  uint8_t** q = malloc(lambda * sizeof(uint8_t*));
  assert(q);
  q[0] = calloc(lambda, ell_hat_bytes);
  assert(q[0]);
  for (unsigned int i = 1; i < lambda; ++i) {
    q[i] = q[0] + i * ell_hat_bytes;
  }
  uint8_t* hcom = malloc(params->lambda_bytes * 2);
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

  uint8_t* chall_1 = malloc(5u * params->lambda_bytes + 8u);
  assert(chall_1);
  SIG_TSTART(t_chall1_v);
  hash_challenge_1(params, chall_1, mu, hcom, dsignature_c(params, sig, 0), iv, lambda, ell, tau);
  SIG_TEND("verify.hash_challenge_1.pseudoXOF_5lambda+64", t_chall1_v);

  H2_context_t chall_2_ctx;
#ifdef SIG_TIMING
  uint64_t t_vole_hash_ns = 0;
  uint64_t t_pseudoXOF_ns = 0;
  uint64_t t_stage = sig_now_ns();
#endif
  hash_challenge_2_init(&chall_2_ctx, chall_1, dsignature_u_tilde(params, sig), lambda);
#ifdef SIG_TIMING
  t_pseudoXOF_ns += sig_now_ns() - t_stage;
#endif
  vole_hash_precomp_t vh_precomp;
  const int has_vh_precomp = vole_hash_precompute_init(&vh_precomp, chall_1, ell, lambda);

  {
    const uint8_t* chall_3 = dsignature_chall_3(params, sig);
    uint8_t Q_tilde_batch[4u * (BF256_NUM_BYTES + UNIVERSAL_HASH_B)];
    uint8_t* Q_tilde0 = Q_tilde_batch + 0u * utilde_bytes;
    uint8_t* Q_tilde1 = Q_tilde_batch + 1u * utilde_bytes;
    uint8_t* Q_tilde2 = Q_tilde_batch + 2u * utilde_bytes;
    uint8_t* Q_tilde3 = Q_tilde_batch + 3u * utilde_bytes;
    const uint8_t* u_tilde = dsignature_u_tilde(params, sig);
    unsigned int i = 0;
    for (; i + 3 < lambda; i += 4) {
#ifdef SIG_TIMING
      t_stage = sig_now_ns();
#endif
      if (has_vh_precomp) {
        vole_hash_4_precomp(Q_tilde0, Q_tilde1, Q_tilde2, Q_tilde3, &vh_precomp, q[i], q[i + 1],
                            q[i + 2], q[i + 3]);
      } else {
        vole_hash_4(Q_tilde0, Q_tilde1, Q_tilde2, Q_tilde3, chall_1, q[i], q[i + 1], q[i + 2],
                    q[i + 3], ell, lambda);
      }
#ifdef SIG_TIMING
      t_vole_hash_ns += sig_now_ns() - t_stage;
      t_stage = sig_now_ns();
#endif
      const uint8_t bits = chall3_get_bits4(chall_3, i, lambda);
      xor_u_tilde_masked_batch4(Q_tilde_batch, utilde_bytes, u_tilde, utilde_bytes, bits);
      hash_challenge_2_update_v_tilde_batch(&chall_2_ctx, Q_tilde_batch, 4u, lambda);
#ifdef SIG_TIMING
      t_pseudoXOF_ns += sig_now_ns() - t_stage;
#endif
    }
    if (i + 1 < lambda) {
#ifdef SIG_TIMING
      t_stage = sig_now_ns();
#endif
      if (has_vh_precomp) {
        vole_hash_2_precomp(Q_tilde0, Q_tilde1, &vh_precomp, q[i], q[i + 1]);
      } else {
        vole_hash_2(Q_tilde0, Q_tilde1, chall_1, q[i], q[i + 1], ell, lambda);
      }
#ifdef SIG_TIMING
      t_vole_hash_ns += sig_now_ns() - t_stage;
      t_stage = sig_now_ns();
#endif
      const uint8_t bits = (uint8_t)(chall3_get_bits4(chall_3, i, lambda) & 0x03u);
      xor_u_tilde_masked_batch4(Q_tilde_batch, utilde_bytes, u_tilde, utilde_bytes, bits);
      hash_challenge_2_update_v_tilde_batch(&chall_2_ctx, Q_tilde_batch, 2u, lambda);
#ifdef SIG_TIMING
      t_pseudoXOF_ns += sig_now_ns() - t_stage;
#endif
      i += 2;
    }
    if (i < lambda) {
#ifdef SIG_TIMING
      t_stage = sig_now_ns();
#endif
      if (has_vh_precomp) {
        vole_hash_precomp(Q_tilde0, &vh_precomp, q[i]);
      } else {
        vole_hash(Q_tilde0, chall_1, q[i], ell, lambda);
      }
#ifdef SIG_TIMING
      t_vole_hash_ns += sig_now_ns() - t_stage;
      t_stage = sig_now_ns();
#endif
      const uint8_t bits = (uint8_t)(chall3_get_bits4(chall_3, i, lambda) & 0x01u);
      xor_u_tilde_masked_batch4(Q_tilde_batch, utilde_bytes, u_tilde, utilde_bytes, bits);
      hash_challenge_2_update_v_tilde(&chall_2_ctx, Q_tilde0, lambda);
#ifdef SIG_TIMING
      t_pseudoXOF_ns += sig_now_ns() - t_stage;
#endif
    }
  }
  vole_hash_precompute_clear(&vh_precomp);

  uint8_t* chall_2 = malloc(3u * params->lambda_bytes + 8u);
  assert(chall_2);
#ifdef SIG_TIMING
  t_stage = sig_now_ns();
#endif
  hash_challenge_2_finalize(params, chall_2, &chall_2_ctx, dsignature_d(params, sig), lambda, ell);
#ifdef SIG_TIMING
  t_pseudoXOF_ns += sig_now_ns() - t_stage;
  sig_print_timing("verify.hash_challenge_2.vole_hash_V_tillde", t_vole_hash_ns);
  sig_print_timing("verify.hash_challenge_2.challenge_2_pseudoXOF_3lambda+64", t_pseudoXOF_ns);
#endif

  const uint8_t* d = dsignature_d(params, sig);
  uint8_t* a0_tilde = malloc(params->lambda_bytes);
  assert(a0_tilde);
  SIG_TSTART(t_ublock_verify);
  ublock_256_verifier(params, a0_tilde, d, q, owf_input, owf_output, chall_2, dsignature_chall_3(params, sig), dsignature_a1_tilde(params, sig), dsignature_a2_tilde(params, sig));
  SIG_TEND("verify.ublock_verify", t_ublock_verify);
  free_pointer_array(&q);

  uint8_t* chall_3 = malloc(params->lambda_bytes);
  assert(chall_3);
  SIG_TSTART(t_hash_chall3);
  hash_challenge_3(params, chall_3, chall_2, dsignature_a1_tilde(params, sig),
                   dsignature_ctr(params, sig), lambda);
  SIG_TEND("verify.hash_challenge_3.pseudoXOF_lambda", t_hash_chall3);

  int result = memcmp(chall_3, dsignature_chall_3(params, sig), lambda_bytes) == 0 ? 0 : -1;
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
