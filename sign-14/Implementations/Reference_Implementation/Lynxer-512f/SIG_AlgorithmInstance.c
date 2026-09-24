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

static_assert(128 == 64 + 64, "invalid public key size");
static_assert(128 == 64 + 512 / 8, "invalid secret key size");

unsigned long long sig_get_pk_len_bytes(void) {
  return 128;
}

unsigned long long sig_get_sk_len_bytes(void) {
  return 128;
}

unsigned long long sig_get_sn_len_bytes(void) {
  return 61091;
}

int sig_keygen(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes)
{
  if (!pk || !pk_len_bytes || !sk || !sk_len_bytes) {
    return -1;
  }

  const unsigned int csp          = 512;
  const unsigned int input_size   = 64;

  // memory layout of the secret key: OWF input || OWF key
  unsigned char *sk_key = sk + input_size;

  // Generate OWF key
  get_random_number(&drng_algorithm, sk_key, csp);

  // Generate OWF input
  get_random_number(&drng_algorithm, sk, input_size * 8);

  // memory layout of the public key: OWF input || OWF output
  memcpy(pk, sk, input_size);
  owf_512(sk_key, sk, pk + input_size);

  *pk_len_bytes = 128;
  *sk_len_bytes = 128;

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

  if (sk_len_bytes != 128) {
    return -1;
  }

  const unsigned int input_size = 64;

  unsigned char rho[512 / 8];
  get_random_number(&drng_algorithm, rho, 512);

  // Unpack secret key: OWF input || OWF key
  const unsigned char *owf_input = sk;
  const unsigned char *owf_key   = sk + input_size;

  // Compute OWF output and witness
  unsigned char owf_output[64];
  owf_512(owf_key, owf_input, owf_output);

  unsigned char witness[1536 / 8];
  const sig_paramset_t* params = sig_get_paramset(LYNXER_512F);
  lynx_extend_witness(witness, owf_key, owf_input, params);

  *sn_len_bytes = 61091;
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

  if (pk_len_bytes != 128) {
    return -1;
  }

  if (sn_len_bytes != 61091) {
    return -1;
  }

  const sig_paramset_t* params = sig_get_paramset(LYNXER_512F);
  return voleith_verify(m, m_len_bytes, sn, pk, pk + 64, params);
}
