/*
 *  SPDX-License-Identifier: MIT
 */

#include "SIG_AlgorithmInstance.h"
#include "drng.h"
#include "owf.h"
#include "parameters.h"
#include "voleith_impl.h"
#include "instances.h"
#include "endian_compat.h"

#include <string.h>

extern DRNG_ctx drng_algorithm;

static_assert(96 == 48 + 48, "invalid public key size");
static_assert(96 == 48 + 384 / 8, "invalid secret key size");

unsigned long long sig_get_pk_len_bytes(void) {
  return 96;
}

unsigned long long sig_get_sk_len_bytes(void) {
  return 96;
}

unsigned long long sig_get_sn_len_bytes(void) {
  return 34109;
}

int sig_keygen(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes)
{
  if (!pk || !pk_len_bytes || !sk || !sk_len_bytes) {
    return -1;
  }

  const unsigned int csp          = 384;
  const unsigned int input_size   = 48;

  // memory layout of the secret key: OWF input || OWF key
  unsigned char *sk_key = sk + input_size;

  // Generate OWF key
  get_random_number(&drng_algorithm, sk_key, csp);

  // Generate OWF input
  get_random_number(&drng_algorithm, sk, input_size * 8);

  // memory layout of the public key: OWF input || OWF output
  memcpy(pk, sk, input_size);
  owf_384(sk_key, sk, pk + input_size);

  *pk_len_bytes = 96;
  *sk_len_bytes = 96;

  return 0;
}

int sig_sign(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes,
    unsigned char *sn, unsigned long long *sn_len_bytes)
{
  if (!sk || !sn || !sn_len_bytes || (!m && m_len_bytes)) {
    return -1;
  }

  if (sk_len_bytes != 96) {
    return -1;
  }

  const unsigned int input_size = 48;

  unsigned char rho[384 / 8];
  get_random_number(&drng_algorithm, rho, 384);

  // Unpack secret key: OWF input || OWF key
  const unsigned char *owf_input = sk;
  const unsigned char *owf_key   = sk + input_size;

  // Compute OWF output and witness
  unsigned char owf_output[48];
  owf_384(owf_key, owf_input, owf_output);

  unsigned char witness[1152 / 8];
  const sig_paramset_t* params = sig_get_paramset(LYNXER_384F);
  lynx_extend_witness(witness, owf_key, owf_input, params);

  *sn_len_bytes = 34109;
  voleith_sign(sn, m, m_len_bytes, owf_key, owf_input, owf_output, witness,
               rho, sizeof(rho), params);

  memset(witness, 0, sizeof(witness));
  memset(owf_output, 0, sizeof(owf_output));

  return 0;
}

int sig_verify(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *sn, unsigned long long sn_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes)
{
  if (!pk || !sn || (!m && m_len_bytes)) {
    return -1;
  }

  if (pk_len_bytes != 96) {
    return -1;
  }

  if (sn_len_bytes != 34109) {
    return -1;
  }

  const sig_paramset_t* params = sig_get_paramset(LYNXER_384F);
  return voleith_verify(m, m_len_bytes, sn, pk, pk + 48, params);
}
