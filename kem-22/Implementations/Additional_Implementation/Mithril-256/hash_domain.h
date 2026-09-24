/*
 * Domain-separated, length-delimited hash/XOF inputs for ARCANE-Mithril.
 */

#ifndef HASH_DOMAIN_H
#define HASH_DOMAIN_H

#include "parameters.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(__GNUC__)
#define RRLWR_DOMAIN_UNUSED __attribute__((unused))
#else
#define RRLWR_DOMAIN_UNUSED
#endif

#define RRLWR_DOMAIN_PREFIX "ARCANE-Mithril-v1"
#define RRLWR_DOMAIN_PREFIX_LEN ((size_t)(sizeof(RRLWR_DOMAIN_PREFIX) - 1))
#define RRLWR_DOMAIN_LEN_BYTES 8
#define RRLWR_DOMAIN_XOF_PREFIX "AMX1"
#define RRLWR_DOMAIN_XOF_PREFIX_LEN ((size_t)(sizeof(RRLWR_DOMAIN_XOF_PREFIX) - 1))
#define RRLWR_DOMAIN_XOF_LEN_BYTES 1
#define RRLWR_DOMAIN_XOF_PUBLIC_ID 1
#define RRLWR_DOMAIN_XOF_SECRET_ID 2

#define RRLWR_DOMAIN_LABEL_F      "H_F"
#define RRLWR_DOMAIN_LABEL_G      "H_G"
#define RRLWR_DOMAIN_LABEL_H      "H_H"
#define RRLWR_DOMAIN_LABEL_PUBLIC "XOF_A"
#define RRLWR_DOMAIN_LABEL_SECRET "XOF_S"

#define RRLWR_DOMAIN_LABEL_LEN(label) ((size_t)(sizeof(label) - 1))
#define RRLWR_DOMAIN_HEADER_LEN(label) \
  (RRLWR_DOMAIN_PREFIX_LEN + 1 + RRLWR_DOMAIN_LABEL_LEN(label) + 2 + 1 + 4 + 1)
#define RRLWR_DOMAIN_INPUT_LEN(label, field_bytes, nfields) \
  (RRLWR_DOMAIN_HEADER_LEN(label) + (size_t)(nfields) * RRLWR_DOMAIN_LEN_BYTES + (size_t)(field_bytes))
#define RRLWR_DOMAIN_XOF_HEADER_LEN \
  (RRLWR_DOMAIN_XOF_PREFIX_LEN + 1 + 2 + 1 + 2)
#define RRLWR_DOMAIN_XOF_INPUT_MAX(label, seed_len) \
  (RRLWR_DOMAIN_XOF_HEADER_LEN + RRLWR_DOMAIN_XOF_LEN_BYTES + (size_t)(seed_len) + 2)
#define RRLWR_DOMAIN_XOF_SEED_MAX ((size_t)128)

#if defined(RRLWR_SM3_XOF)
#define RRLWR_DOMAIN_HASH_RAW(output, output_len, input, input_len) \
  RRLWR_SM3_XOF((output), (output_len), (input), (input_len))
#define RRLWR_DOMAIN_XOF_PUBLIC_RAW(output, output_len, input, input_len) \
  RRLWR_XOF((output), (output_len), (input), (input_len))
#define RRLWR_DOMAIN_XOF_SECRET_RAW(output, output_len, input, input_len) \
  RRLWR_XOF((output), (output_len), (input), (input_len))
#elif defined(RRLWR_XOF_SECRET)
#define RRLWR_DOMAIN_HASH_RAW(output, output_len, input, input_len) \
  RRLWR_XOF_SECRET((output), (output_len), (input), (input_len))
#if defined(RRLWR_XOF_PUBLIC)
#define RRLWR_DOMAIN_XOF_PUBLIC_RAW(output, output_len, input, input_len) \
  RRLWR_XOF_PUBLIC((output), (output_len), (input), (input_len))
#else
#define RRLWR_DOMAIN_XOF_PUBLIC_RAW(output, output_len, input, input_len) \
  RRLWR_XOF_SECRET((output), (output_len), (input), (input_len))
#endif
#define RRLWR_DOMAIN_XOF_SECRET_RAW(output, output_len, input, input_len) \
  RRLWR_XOF_SECRET((output), (output_len), (input), (input_len))
#else
#error "No raw Mithril XOF is available for domain wrappers"
#endif

#ifdef RRLWR_KEM_HASH_F
#undef RRLWR_KEM_HASH_F
#endif
#ifdef RRLWR_KEM_HASH_G
#undef RRLWR_KEM_HASH_G
#endif
#ifdef RRLWR_KEM_HASH_H
#undef RRLWR_KEM_HASH_H
#endif

static RRLWR_DOMAIN_UNUSED void rrlwr_store16_le(uint8_t out[2], uint16_t x)
{
  out[0] = (uint8_t)x;
  out[1] = (uint8_t)(x >> 8);
}

static RRLWR_DOMAIN_UNUSED void rrlwr_store32_le(uint8_t out[4], uint32_t x)
{
  out[0] = (uint8_t)x;
  out[1] = (uint8_t)(x >> 8);
  out[2] = (uint8_t)(x >> 16);
  out[3] = (uint8_t)(x >> 24);
}

static RRLWR_DOMAIN_UNUSED void rrlwr_store64_le(uint8_t out[8], uint64_t x)
{
  out[0] = (uint8_t)x;
  out[1] = (uint8_t)(x >> 8);
  out[2] = (uint8_t)(x >> 16);
  out[3] = (uint8_t)(x >> 24);
  out[4] = (uint8_t)(x >> 32);
  out[5] = (uint8_t)(x >> 40);
  out[6] = (uint8_t)(x >> 48);
  out[7] = (uint8_t)(x >> 56);
}

static RRLWR_DOMAIN_UNUSED size_t rrlwr_domain_begin(uint8_t *buf,
                                                     const char *label,
                                                     size_t label_len,
                                                     size_t outlen,
                                                     uint8_t nfields)
{
  size_t pos = 0;

  memcpy(buf + pos, RRLWR_DOMAIN_PREFIX, RRLWR_DOMAIN_PREFIX_LEN);
  pos += RRLWR_DOMAIN_PREFIX_LEN;
  buf[pos++] = (uint8_t)label_len;
  memcpy(buf + pos, label, label_len);
  pos += label_len;
  rrlwr_store16_le(buf + pos, (uint16_t)RRLWR_SECURITY_LEVEL);
  pos += 2;
  buf[pos++] = (uint8_t)RRLWR_K;
  rrlwr_store32_le(buf + pos, (uint32_t)outlen);
  pos += 4;
  buf[pos++] = nfields;

  return pos;
}

static RRLWR_DOMAIN_UNUSED size_t rrlwr_domain_add_field(uint8_t *buf,
                                                         size_t pos,
                                                         const uint8_t *field,
                                                         size_t field_len)
{
  rrlwr_store64_le(buf + pos, (uint64_t)field_len);
  pos += RRLWR_DOMAIN_LEN_BYTES;
  if(field_len != 0) {
    memcpy(buf + pos, field, field_len);
  }
  return pos + field_len;
}

static RRLWR_DOMAIN_UNUSED size_t rrlwr_domain_encode_1(uint8_t *buf,
                                                        const char *label,
                                                        size_t label_len,
                                                        size_t outlen,
                                                        const uint8_t *f0,
                                                        size_t f0_len)
{
  size_t pos = rrlwr_domain_begin(buf, label, label_len, outlen, 1);
  return rrlwr_domain_add_field(buf, pos, f0, f0_len);
}

static RRLWR_DOMAIN_UNUSED size_t rrlwr_domain_encode_2(uint8_t *buf,
                                                        const char *label,
                                                        size_t label_len,
                                                        size_t outlen,
                                                        const uint8_t *f0,
                                                        size_t f0_len,
                                                        const uint8_t *f1,
                                                        size_t f1_len)
{
  size_t pos = rrlwr_domain_begin(buf, label, label_len, outlen, 2);
  pos = rrlwr_domain_add_field(buf, pos, f0, f0_len);
  return rrlwr_domain_add_field(buf, pos, f1, f1_len);
}

static RRLWR_DOMAIN_UNUSED size_t rrlwr_domain_encode_3(uint8_t *buf,
                                                        const char *label,
                                                        size_t label_len,
                                                        size_t outlen,
                                                        const uint8_t *f0,
                                                        size_t f0_len,
                                                        const uint8_t *f1,
                                                        size_t f1_len,
                                                        const uint8_t *f2,
                                                        size_t f2_len)
{
  size_t pos = rrlwr_domain_begin(buf, label, label_len, outlen, 3);
  pos = rrlwr_domain_add_field(buf, pos, f0, f0_len);
  pos = rrlwr_domain_add_field(buf, pos, f1, f1_len);
  return rrlwr_domain_add_field(buf, pos, f2, f2_len);
}

static RRLWR_DOMAIN_UNUSED void rrlwr_domain_check_seed_len(int32_t seed_len)
{
  if(seed_len < 0 || (size_t)seed_len > RRLWR_DOMAIN_XOF_SEED_MAX) {
    abort();
  }
}

static RRLWR_DOMAIN_UNUSED void rrlwr_domain_check_xof_args(size_t outlen, int32_t seed_len)
{
  rrlwr_domain_check_seed_len(seed_len);
  if(outlen > UINT16_MAX) {
    abort();
  }
}

static RRLWR_DOMAIN_UNUSED size_t rrlwr_domain_encode_xof(uint8_t *input,
                                                          uint8_t label_id,
                                                          size_t outlen,
                                                          const uint8_t *seed,
                                                          int32_t seed_len,
                                                          uint8_t coeff,
                                                          uint8_t lane)
{
  size_t pos = 0;

  rrlwr_domain_check_xof_args(outlen, seed_len);
  memcpy(input + pos, RRLWR_DOMAIN_XOF_PREFIX, RRLWR_DOMAIN_XOF_PREFIX_LEN);
  pos += RRLWR_DOMAIN_XOF_PREFIX_LEN;
  input[pos++] = label_id;
  rrlwr_store16_le(input + pos, (uint16_t)RRLWR_SECURITY_LEVEL);
  pos += 2;
  input[pos++] = (uint8_t)RRLWR_K;
  rrlwr_store16_le(input + pos, (uint16_t)outlen);
  pos += 2;
  input[pos++] = (uint8_t)seed_len;
  memcpy(input + pos, seed, (size_t)seed_len);
  pos += (size_t)seed_len;
  input[pos++] = coeff;
  input[pos++] = lane;

  return pos;
}

static RRLWR_DOMAIN_UNUSED void RRLWR_KEM_HASH_F(uint8_t out[RRLWR_KEM_HPK_LEN],
                                                 const uint8_t pk[RRLWR_PKE_PK_LEN])
{
  uint8_t input[RRLWR_DOMAIN_INPUT_LEN(RRLWR_DOMAIN_LABEL_F, RRLWR_PKE_PK_LEN, 1)];
  size_t inlen = rrlwr_domain_encode_1(input,
                                       RRLWR_DOMAIN_LABEL_F,
                                       sizeof(RRLWR_DOMAIN_LABEL_F) - 1,
                                       RRLWR_KEM_HPK_LEN,
                                       pk,
                                       RRLWR_PKE_PK_LEN);
  RRLWR_DOMAIN_HASH_RAW(out, RRLWR_KEM_HPK_LEN, input, inlen);
}

static RRLWR_DOMAIN_UNUSED void RRLWR_KEM_HASH_G(uint8_t *out,
                                                 size_t outlen,
                                                 const uint8_t hpk[RRLWR_KEM_HPK_LEN],
                                                 const uint8_t m[RRLWR_PKE_MESSAGE_LEN])
{
  uint8_t input[RRLWR_DOMAIN_INPUT_LEN(RRLWR_DOMAIN_LABEL_G,
                                       RRLWR_KEM_HPK_LEN + RRLWR_PKE_MESSAGE_LEN,
                                       2)];
  size_t inlen = rrlwr_domain_encode_2(input,
                                       RRLWR_DOMAIN_LABEL_G,
                                       sizeof(RRLWR_DOMAIN_LABEL_G) - 1,
                                       outlen,
                                       hpk,
                                       RRLWR_KEM_HPK_LEN,
                                       m,
                                       RRLWR_PKE_MESSAGE_LEN);
  RRLWR_DOMAIN_HASH_RAW(out, outlen, input, inlen);
}

static RRLWR_DOMAIN_UNUSED void RRLWR_KEM_HASH_H(uint8_t *out,
                                                 size_t outlen,
                                                 const uint8_t ct[RRLWR_KEM_CT_LEN],
                                                 const uint8_t z[RRLWR_KEM_SEED_Z_LEN])
{
  uint8_t input[RRLWR_DOMAIN_INPUT_LEN(RRLWR_DOMAIN_LABEL_H,
                                       RRLWR_KEM_CT_LEN + RRLWR_KEM_SEED_Z_LEN,
                                       2)];
  size_t inlen = rrlwr_domain_encode_2(input,
                                       RRLWR_DOMAIN_LABEL_H,
                                       sizeof(RRLWR_DOMAIN_LABEL_H) - 1,
                                       outlen,
                                       ct,
                                       RRLWR_KEM_CT_LEN,
                                       z,
                                       RRLWR_KEM_SEED_Z_LEN);
  RRLWR_DOMAIN_HASH_RAW(out, outlen, input, inlen);
}

static RRLWR_DOMAIN_UNUSED size_t RRLWR_DOMAIN_ENCODE_XOF_PUBLIC(uint8_t *input,
                                                                 size_t outlen,
                                                                 const uint8_t *seed,
                                                                 int32_t seed_len,
                                                                 uint8_t coeff,
                                                                 uint8_t lane)
{
  return rrlwr_domain_encode_xof(input,
                                 RRLWR_DOMAIN_XOF_PUBLIC_ID,
                                 outlen,
                                 seed,
                                 seed_len,
                                 coeff,
                                 lane);
}

static RRLWR_DOMAIN_UNUSED size_t RRLWR_DOMAIN_ENCODE_XOF_SECRET(uint8_t *input,
                                                                 size_t outlen,
                                                                 const uint8_t *seed,
                                                                 int32_t seed_len,
                                                                 uint8_t coeff,
                                                                 uint8_t lane)
{
  return rrlwr_domain_encode_xof(input,
                                 RRLWR_DOMAIN_XOF_SECRET_ID,
                                 outlen,
                                 seed,
                                 seed_len,
                                 coeff,
                                 lane);
}

static RRLWR_DOMAIN_UNUSED void RRLWR_XOF_PUBLIC_DOMAIN(uint8_t *out,
                                                        size_t outlen,
                                                        const uint8_t *seed,
                                                        int32_t seed_len,
                                                        uint8_t coeff,
                                                        uint8_t lane)
{
  uint8_t input[RRLWR_DOMAIN_XOF_INPUT_MAX(RRLWR_DOMAIN_LABEL_PUBLIC, RRLWR_DOMAIN_XOF_SEED_MAX)];
  size_t inlen;

  inlen = RRLWR_DOMAIN_ENCODE_XOF_PUBLIC(input, outlen, seed, seed_len, coeff, lane);
  RRLWR_DOMAIN_XOF_PUBLIC_RAW(out, outlen, input, inlen);
}

static RRLWR_DOMAIN_UNUSED void RRLWR_XOF_SECRET_DOMAIN(uint8_t *out,
                                                        size_t outlen,
                                                        const uint8_t *seed,
                                                        int32_t seed_len,
                                                        uint8_t coeff,
                                                        uint8_t lane)
{
  uint8_t input[RRLWR_DOMAIN_XOF_INPUT_MAX(RRLWR_DOMAIN_LABEL_SECRET, RRLWR_DOMAIN_XOF_SEED_MAX)];
  size_t inlen;

  inlen = RRLWR_DOMAIN_ENCODE_XOF_SECRET(input, outlen, seed, seed_len, coeff, lane);
  RRLWR_DOMAIN_XOF_SECRET_RAW(out, outlen, input, inlen);
}

#endif
