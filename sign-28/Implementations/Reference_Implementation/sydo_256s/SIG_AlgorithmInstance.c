/*
 *  SPDX-License-Identifier: MIT
 */

#include "SIG_AlgorithmInstance.h"
#include "lib/drng.h"
#include "src/sydo.h"

#include <stddef.h>
#include <stdlib.h>

extern DRNG_ctx drng_algorithm;

static const sydo_ref_paramset_t* selected_params(void) {
  return sydo_ref_get_paramset_by_name(ALGORITHM_INSTANCE);
}

unsigned long long sig_get_pk_len_bytes(void) {
  const sydo_ref_paramset_t* params = selected_params();
  return params ? params->public_key_size : 0;
}

unsigned long long sig_get_sk_len_bytes(void) {
  const sydo_ref_paramset_t* params = selected_params();
  return params ? params->secret_key_size : 0;
}

unsigned long long sig_get_sn_len_bytes(void) {
  const sydo_ref_paramset_t* params = selected_params();
  return params ? params->signature_size : 0;
}

int sig_keygen(unsigned char* pk, unsigned long long* pk_len_bytes, unsigned char* sk,
               unsigned long long* sk_len_bytes) {
  const sydo_ref_paramset_t* params = selected_params();
  unsigned char* random_seed = NULL;
  int ret = -1;

  if (!params || !pk || !pk_len_bytes || !sk || !sk_len_bytes) {
    return -1;
  }
  random_seed = (unsigned char*)malloc(params->keygen_seed_size);
  if (!random_seed) {
    return -2;
  }
  if (get_random_number(&drng_algorithm, random_seed,
                        (unsigned long long)params->keygen_seed_size * 8) != 0) {
    free(random_seed);
    return -3;
  }

  ret = sydo_ref_keygen_from_seed(params, pk, (size_t)*pk_len_bytes, sk, (size_t)*sk_len_bytes,
                                  random_seed, params->keygen_seed_size);
  if (ret == 0) {
    *pk_len_bytes = params->public_key_size;
    *sk_len_bytes = params->secret_key_size;
  }
  free(random_seed);
  return ret;
}

int sig_sign(unsigned char* sk, unsigned long long sk_len_bytes, unsigned char* m,
             unsigned long long m_len_bytes, unsigned char* sn,
             unsigned long long* sn_len_bytes) {
  const sydo_ref_paramset_t* params = selected_params();
  unsigned char* random_seed = NULL;
  int ret = -1;

  if (!params || !sk || !m || !sn || !sn_len_bytes) {
    return -1;
  }
  random_seed = (unsigned char*)malloc(params->secpar_bits / 8);
  if (!random_seed) {
    return -2;
  }
  if (get_random_number(&drng_algorithm, random_seed, (unsigned long long)params->secpar_bits) !=
      0) {
    free(random_seed);
    return -3;
  }

  ret = sydo_ref_sign_from_seed(params, sn, (size_t)*sn_len_bytes, m, (size_t)m_len_bytes, sk,
                                (size_t)sk_len_bytes, random_seed, params->secpar_bits / 8);
  if (ret == 0) {
    *sn_len_bytes = params->signature_size;
  }
  free(random_seed);
  return ret;
}

int sig_verify(unsigned char* pk, unsigned long long pk_len_bytes, unsigned char* sn,
               unsigned long long sn_len_bytes, unsigned char* m, unsigned long long m_len_bytes) {
  const sydo_ref_paramset_t* params = selected_params();
  if (!params) {
    return -1;
  }
  return sydo_ref_verify(params, pk, (size_t)pk_len_bytes, sn, (size_t)sn_len_bytes, m,
                         (size_t)m_len_bytes);
}
