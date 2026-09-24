#include "params.h"

#include <assert.h>
#include <stdint.h>

static params_t ublockith_params_d3_256f_g = {
    .lambda = 256,
    .ell = 4608,
    .ske = 384,
    .senc = 1536,
    .r = 24,
    .lke = 1792,
    .lenc = 2816,
    .tau = 32,
    .w_grind = 8,
    .t_open = 246,
    .n_leaf = 2,
    .use_em_bavc = true,
    .deg = 3,
    .k = 0,
    .tau0 = 0,
    .tau1 = 0,
    .L = 0,
    .sig_size = 0,
    .lambda_bytes = 256 / 8,
    .ell_bytes = 4608 / 8,
    .ell_hat_bits = 4608 + 3 * 256 + UNIVERSAL_HASH_B_BITS,
    .ell_hat_bytes = (4608 + 3 * 256 + UNIVERSAL_HASH_B_BITS) / 8,
    .utilde_bytes = (256 / 8) + UNIVERSAL_HASH_B,
};

static void init_params(params_t* params) {
  const unsigned int tau = params->tau;
  const unsigned int lambda = params->lambda;
  const unsigned int w_grind = params->w_grind;

  unsigned int total = (lambda > w_grind) ? (lambda - w_grind) : 0;

  unsigned int k = 0;
  if (tau == 0) {
    params->tau1 = 0;
    params->tau0 = 0;
  } else {
    k = (total / tau) + 1;
    params->tau1 = (total % tau);
    params->tau0 = tau - params->tau1;
  }
  params->k = k;

  assert(params->deg >= 2);

  params->ell_hat_bits = params->ell + params->deg * params->lambda + UNIVERSAL_HASH_B_BITS;
  params->ell_hat_bytes = params->ell_hat_bits / 8;

  {
    const uint64_t pow_k = (k >= 64) ? 0ULL : ((uint64_t)1 << k);
    const uint64_t pow_km1 = (k == 0 || (k - 1) >= 64) ? 0ULL : ((uint64_t)1 << (k - 1));
    const uint64_t L = (uint64_t)params->tau1 * pow_k + (uint64_t)params->tau0 * pow_km1;
    params->L = (unsigned int)L;
  }

  {
    const unsigned int computed_sig_size = compute_sig_size(params);
    if (params->sig_size == 0) {
      params->sig_size = computed_sig_size;
    } else {
      assert(computed_sig_size == params->sig_size);
    }
  }
}

const params_t* ublockith_params_d3_256f(void) {
  static int initialized = 0;
  if (!initialized) {
    init_params(&ublockith_params_d3_256f_g);
    initialized = 1;
  }
  return &ublockith_params_d3_256f_g;
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
