#include "params.h"

#include <assert.h>
#include <stdint.h>

/*
 * Fixed parameter set for vistrutith_d3_512s.
 * Chosen to mirror the ublockith_d3_256s style:
 * - tau = 44
 * - T_open = 480
 * - with lambda=512 and w_grind=5:
 *   k = 12, k1=tau1=23, k0=tau0=21
 */
static params_t vistrutith_params_d3_512s_g = {
    .lambda = 512,
    .ell = 6912,
    .ske = 0,
    .senc = 1152,
    .r = 18,
    .lke = 0,
    .lenc = 6912,
    .tau = 44,
    .w_grind = 5,
    .t_open = 480,
    .n_leaf = 2,
    .use_em_bavc = false,
    .deg = 3,
    .k = 0,
    .tau0 = 0,
    .tau1 = 0,
    .L = 0,
    .sig_size = 0,
    .lambda_bytes = 512 / 8,
    .ell_bytes = 6912 / 8,
    .ell_hat_bits = 6912 + 3 * 512 + UNIVERSAL_HASH_B_BITS,
    .ell_hat_bytes = (6912 + 3 * 512 + UNIVERSAL_HASH_B_BITS) / 8,
    .utilde_bytes = (512 / 8) + UNIVERSAL_HASH_B,
};

static void init_params(params_t* params) {
  const unsigned int tau = params->tau;
  const unsigned int lambda = params->lambda;
  const unsigned int w_grind = params->w_grind;

  unsigned int total = 0;
  if (lambda > w_grind) total = lambda - w_grind;

  if (tau == 0) {
    params->k = 0;
    params->tau1 = 0;
    params->tau0 = 0;
  } else {
    params->k = (total / tau) + 1;
    params->tau1 = (total % tau);
    params->tau0 = tau - params->tau1;
  }

  assert(params->deg >= 2);
  params->ell_hat_bits = params->ell + params->deg * params->lambda + UNIVERSAL_HASH_B_BITS;
  params->ell_hat_bytes = params->ell_hat_bits / 8;

  const uint64_t pow_k = (params->k >= 64) ? 0ULL : ((uint64_t)1 << params->k);
  const uint64_t pow_km1 =
      (params->k == 0 || (params->k - 1) >= 64) ? 0ULL : ((uint64_t)1 << (params->k - 1));
  params->L = (unsigned int)((uint64_t)params->tau1 * pow_k + (uint64_t)params->tau0 * pow_km1);

  if (params->sig_size == 0) {
    params->sig_size = compute_sig_size(params);
  } else {
    assert(params->sig_size == compute_sig_size(params));
  }
}

const params_t* vistrutith_params_d3_512s(void) {
  static int initialized = 0;
  if (!initialized) {
    init_params(&vistrutith_params_d3_512s_g);
    initialized = 1;
  }
  return &vistrutith_params_d3_512s_g;
}

unsigned int compute_sig_size(const params_t* params) {
  const unsigned int lambda_bytes = params->lambda_bytes;
  const unsigned int ell_hat_bytes = params->ell_hat_bytes;
  const unsigned int ell_bytes = params->ell_bytes;
  const unsigned int utilde_bytes = params->utilde_bytes;
  const unsigned int tau = params->tau;
  const unsigned int t_open = params->t_open;

  const unsigned int n_leaf = params->n_leaf;

  const unsigned int A = (tau - 1) * ell_hat_bytes;
  const unsigned int B = utilde_bytes;
  const unsigned int C = ell_bytes;
  const unsigned int D = lambda_bytes * (params->deg - 1);
  const unsigned int E = (n_leaf * lambda_bytes) * tau + t_open * lambda_bytes;
  const unsigned int F = lambda_bytes;
  const unsigned int G = SALT_SIZE;
  const unsigned int H = (unsigned int)sizeof(uint32_t);

  return A + B + C + D + E + F + G + H;
}
