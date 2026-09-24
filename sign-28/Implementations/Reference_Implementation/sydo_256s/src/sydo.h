/*
 *  SPDX-License-Identifier: MIT
 */

#ifndef SYDO_REF_H
#define SYDO_REF_H

#include <stddef.h>
#include <stdint.h>

#include "macros.h"

SYDO_BEGIN_C_DECL

typedef enum sydo_ref_paramid_t {
  SYDO_REF_PARAM_INVALID = 0,
  SYDO_REF_160S = 1,
  SYDO_REF_160F = 2,
  SYDO_REF_256S = 3,
  SYDO_REF_256F = 4,
  SYDO_REF_512S = 5,
  SYDO_REF_512F = 6,
  SYDO_REF_PARAM_MAX_INDEX = 7
} sydo_ref_paramid_t;

typedef struct sydo_ref_paramset_t {
  sydo_ref_paramid_t id;
  const char* name;
  uint16_t secpar_bits;
  uint16_t tau;
  uint16_t zero_bits_in_delta;
  uint16_t bavc_opening_threshold;
  uint32_t rsd_n;
  uint16_t rsd_w;
  uint16_t rsd_codim;
  uint32_t signature_size;
  uint16_t public_key_size;
  uint16_t secret_key_size;
  uint16_t witness_size;
  uint16_t keygen_seed_size;
} sydo_ref_paramset_t;

const sydo_ref_paramset_t* sydo_ref_get_paramset(sydo_ref_paramid_t id);
const sydo_ref_paramset_t* sydo_ref_get_paramset_by_name(const char* name);

int sydo_ref_keygen_from_seed(const sydo_ref_paramset_t* params, uint8_t* pk, size_t pk_len,
                              uint8_t* sk, size_t sk_len, const uint8_t* seed,
                              size_t seed_len);

int sydo_ref_derive_public_key(const sydo_ref_paramset_t* params, uint8_t* pk, size_t pk_len,
                               const uint8_t* sk, size_t sk_len);

int sydo_ref_validate_keypair(const sydo_ref_paramset_t* params, const uint8_t* pk, size_t pk_len,
                              const uint8_t* sk, size_t sk_len);

int sydo_ref_keygen(const sydo_ref_paramset_t* params, uint8_t* pk, size_t pk_len, uint8_t* sk,
                    size_t sk_len);

int sydo_ref_sign_from_seed(const sydo_ref_paramset_t* params, uint8_t* sig, size_t sig_len,
                            const uint8_t* msg, size_t msg_len, const uint8_t* sk,
                            size_t sk_len, const uint8_t* seed, size_t seed_len);

int sydo_ref_sign(const sydo_ref_paramset_t* params, uint8_t* sig, size_t sig_len,
                  const uint8_t* msg, size_t msg_len, const uint8_t* sk, size_t sk_len);

int sydo_ref_verify(const sydo_ref_paramset_t* params, const uint8_t* pk, size_t pk_len,
                    const uint8_t* sig, size_t sig_len, const uint8_t* msg, size_t msg_len);

SYDO_END_C_DECL

#endif
