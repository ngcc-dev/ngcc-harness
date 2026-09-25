#ifndef PARAMS_H
#define PARAMS_H

#include "macros.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UNIVERSAL_HASH_B_BITS 16
#define UNIVERSAL_HASH_B (UNIVERSAL_HASH_B_BITS / 8)
#define IV_SIZE 32
#define SALT_SIZE 32

typedef struct {
  unsigned int lambda;
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
  unsigned int ell_bytes;
  unsigned int ell_hat_bits;
  unsigned int ell_hat_bytes;
  unsigned int utilde_bytes;
} params_t;

const params_t* ublockith_params_d3_256s(void);
unsigned int compute_sig_size(const params_t* params);

#define is_em(params) ((params)->ske == 0)

#endif
