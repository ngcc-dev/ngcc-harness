#ifndef PARAMS_H
#define PARAMS_H

#include "macros.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UNIVERSAL_HASH_B_BITS 16
#define UNIVERSAL_HASH_B (UNIVERSAL_HASH_B_BITS / 8)
/* IV_PRE_SIZE is the H3 output stored in signatures before H4.
 * LAMBDA_IV_SIZE/IV_SIZE is the H4-derived IV length used by PRG and hash domains;
 * it is independent of the SM4th field width lambda_f.
 * SALT_SIZE is kept as a compatibility alias for older code.
 */
#define IV_PRE_SIZE 16
#define LAMBDA_IV_SIZE 16
#define IV_SIZE LAMBDA_IV_SIZE
#define SALT_SIZE IV_PRE_SIZE

typedef struct {
  unsigned int lambda;
  unsigned int lambda_q;
  unsigned int lambda_f;
  unsigned int lambda_iv;
  unsigned int ell;
  unsigned int ske;
  unsigned int senc;
  unsigned int r;
  unsigned int lke;
  unsigned int lenc;

  unsigned int tau;
  unsigned int w_grind;
  unsigned int t_open;
  unsigned int k;
  unsigned int tau0;
  unsigned int tau1;
  unsigned int L;
  unsigned int n_leaf;
  bool use_em_bavc;
  unsigned int deg;
  unsigned int sig_size;

  unsigned int lambda_bytes;
  unsigned int lambda_q_bytes;
  unsigned int lambda_f_bytes;
  unsigned int lambda_iv_bytes;
  unsigned int iv_bytes;
  unsigned int ell_bytes;
  unsigned int ell_hat_bits;
  unsigned int ell_hat_bytes;
  unsigned int utilde_bytes;
} params_t;

const params_t* sm4th_params_d3_f(void);
unsigned int compute_sig_size(const params_t* params);

#define is_em(params) ((params)->ske == 0)

#endif
