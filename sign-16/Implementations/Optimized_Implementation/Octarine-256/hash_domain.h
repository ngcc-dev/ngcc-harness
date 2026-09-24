/*
 * Domain-separated, length-delimited hash/XOF inputs for ARCANE-Octarine.
 */

#ifndef HASH_DOMAIN_H
#define HASH_DOMAIN_H

#include "parameters.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(__GNUC__)
#define RRLWR_DOMAIN_UNUSED __attribute__((unused))
#else
#define RRLWR_DOMAIN_UNUSED
#endif

#define RRLWR_DOMAIN_PREFIX "ARCANE-Octarine-v1"
#define RRLWR_DOMAIN_PREFIX_LEN ((size_t)(sizeof(RRLWR_DOMAIN_PREFIX) - 1))
#define RRLWR_DOMAIN_LEN_BYTES 8
#define RRLWR_DOMAIN_XOF_PREFIX "AOX1"
#define RRLWR_DOMAIN_XOF_PREFIX_LEN ((size_t)(sizeof(RRLWR_DOMAIN_XOF_PREFIX) - 1))
#define RRLWR_DOMAIN_XOF_LEN_BYTES 1
#define RRLWR_DOMAIN_XOF_PUBLIC_ID 1
#define RRLWR_DOMAIN_XOF_SECRET_ID 2

#define RRLWR_DOMAIN_LABEL_KDF      "H_KDF"
#define RRLWR_DOMAIN_LABEL_TR       "H_TR"
#define RRLWR_DOMAIN_LABEL_MU       "H_MU"
#define RRLWR_DOMAIN_LABEL_RHOPP    "H_RHOPP"
#define RRLWR_DOMAIN_LABEL_CH       "H_CH"
#define RRLWR_DOMAIN_LABEL_SAMPLE_C "XOF_C"
#define RRLWR_DOMAIN_LABEL_PUBLIC   "XOF_A"
#define RRLWR_DOMAIN_LABEL_SECRET   "XOF_S"

#define RRLWR_DOMAIN_LABEL_LEN(label) ((size_t)(sizeof(label) - 1))
#define RRLWR_DOMAIN_HEADER_LEN(label) \
  (RRLWR_DOMAIN_PREFIX_LEN + 1 + RRLWR_DOMAIN_LABEL_LEN(label) + 2 + 1 + 4 + 1)
#define RRLWR_DOMAIN_INPUT_LEN(label, field_bytes, nfields) \
  (RRLWR_DOMAIN_HEADER_LEN(label) + (size_t)(nfields) * RRLWR_DOMAIN_LEN_BYTES + (size_t)(field_bytes))
#define RRLWR_DOMAIN_XOF_HEADER_LEN \
  (RRLWR_DOMAIN_XOF_PREFIX_LEN + 1 + 2 + 1 + 2)
#define RRLWR_DOMAIN_XOF_INPUT_MAX(label, seed_len) \
  (RRLWR_DOMAIN_XOF_HEADER_LEN + RRLWR_DOMAIN_XOF_LEN_BYTES + (size_t)(seed_len) + 2)
#define RRLWR_DOMAIN_XOF_SEED_MAX \
  ((RRLWR_SIGN_RHOPRIMEPRIME_LEN + 4 > RRLWR_SIGN_RHOPRIME_LEN) ? \
   (RRLWR_SIGN_RHOPRIMEPRIME_LEN + 4) : RRLWR_SIGN_RHOPRIME_LEN)

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

#ifdef RRLWR_HASH_DOMAIN_SIGN
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
#endif

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

#ifdef RRLWR_HASH_DOMAIN_SIGN
static RRLWR_DOMAIN_UNUSED void RRLWR_SIGN_HASH_KDF(uint8_t out[RRLWR_SIGN_RHO_LEN + RRLWR_SIGN_RHOPRIME_LEN + RRLWR_SIGN_K_LEN],
                                                    const uint8_t xi[RRLWR_SIGN_XI_LEN])
{
  uint8_t input[RRLWR_DOMAIN_INPUT_LEN(RRLWR_DOMAIN_LABEL_KDF, RRLWR_SIGN_XI_LEN, 1)];
  size_t inlen = rrlwr_domain_encode_1(input,
                                       RRLWR_DOMAIN_LABEL_KDF,
                                       sizeof(RRLWR_DOMAIN_LABEL_KDF) - 1,
                                       RRLWR_SIGN_RHO_LEN + RRLWR_SIGN_RHOPRIME_LEN + RRLWR_SIGN_K_LEN,
                                       xi,
                                       RRLWR_SIGN_XI_LEN);
  RRLWR_SIGN_HASH_H(out, RRLWR_SIGN_RHO_LEN + RRLWR_SIGN_RHOPRIME_LEN + RRLWR_SIGN_K_LEN, input, inlen);
}

static RRLWR_DOMAIN_UNUSED void RRLWR_SIGN_HASH_TR(uint8_t out[RRLWR_SIGN_TR_LEN],
                                                   const uint8_t pk[RRLWR_SIGN_PK_LEN])
{
  uint8_t input[RRLWR_DOMAIN_INPUT_LEN(RRLWR_DOMAIN_LABEL_TR, RRLWR_SIGN_PK_LEN, 1)];
  size_t inlen = rrlwr_domain_encode_1(input,
                                       RRLWR_DOMAIN_LABEL_TR,
                                       sizeof(RRLWR_DOMAIN_LABEL_TR) - 1,
                                       RRLWR_SIGN_TR_LEN,
                                       pk,
                                       RRLWR_SIGN_PK_LEN);
  RRLWR_SIGN_HASH_H(out, RRLWR_SIGN_TR_LEN, input, inlen);
}

static RRLWR_DOMAIN_UNUSED int RRLWR_SIGN_HASH_MU(uint8_t out[RRLWR_SIGN_MU_LEN],
                                                  const uint8_t tr[RRLWR_SIGN_TR_LEN],
                                                  const uint8_t *m,
                                                  unsigned long long m_len)
{
  const size_t fixed_len = RRLWR_DOMAIN_HEADER_LEN(RRLWR_DOMAIN_LABEL_MU) +
                           2 * RRLWR_DOMAIN_LEN_BYTES +
                           RRLWR_SIGN_TR_LEN;
  size_t inlen;
  uint8_t *input;

  if(m_len > (unsigned long long)((size_t)-1 - fixed_len)) {
    return -1;
  }

  inlen = fixed_len + (size_t)m_len;
  input = (uint8_t *)malloc(inlen);
  if(input == NULL) {
    return -1;
  }

  inlen = rrlwr_domain_encode_2(input,
                                RRLWR_DOMAIN_LABEL_MU,
                                sizeof(RRLWR_DOMAIN_LABEL_MU) - 1,
                                RRLWR_SIGN_MU_LEN,
                                tr,
                                RRLWR_SIGN_TR_LEN,
                                m,
                                (size_t)m_len);
  RRLWR_SIGN_HASH_H(out, RRLWR_SIGN_MU_LEN, input, inlen);
  free(input);

  return 0;
}

static RRLWR_DOMAIN_UNUSED void RRLWR_SIGN_HASH_RHOPP(uint8_t out[RRLWR_SIGN_RHOPRIMEPRIME_LEN],
                                                      const uint8_t K[RRLWR_SIGN_K_LEN],
                                                      const uint8_t rnd[RRLWR_SIGN_RND_LEN],
                                                      const uint8_t mu[RRLWR_SIGN_MU_LEN])
{
  uint8_t input[RRLWR_DOMAIN_INPUT_LEN(RRLWR_DOMAIN_LABEL_RHOPP,
                                       RRLWR_SIGN_K_LEN + RRLWR_SIGN_RND_LEN + RRLWR_SIGN_MU_LEN,
                                       3)];
  size_t inlen = rrlwr_domain_encode_3(input,
                                       RRLWR_DOMAIN_LABEL_RHOPP,
                                       sizeof(RRLWR_DOMAIN_LABEL_RHOPP) - 1,
                                       RRLWR_SIGN_RHOPRIMEPRIME_LEN,
                                       K,
                                       RRLWR_SIGN_K_LEN,
                                       rnd,
                                       RRLWR_SIGN_RND_LEN,
                                       mu,
                                       RRLWR_SIGN_MU_LEN);
  RRLWR_SIGN_HASH_H(out, RRLWR_SIGN_RHOPRIMEPRIME_LEN, input, inlen);
}

static RRLWR_DOMAIN_UNUSED void RRLWR_SIGN_HASH_CH(uint8_t out[RRLWR_SIGN_CTILDE_LEN],
                                                   const uint8_t mu[RRLWR_SIGN_MU_LEN],
                                                   const uint8_t w1[RRLWR_SIGN_PACKED_W1_LEN])
{
  uint8_t input[RRLWR_DOMAIN_INPUT_LEN(RRLWR_DOMAIN_LABEL_CH,
                                       RRLWR_SIGN_MU_LEN + RRLWR_SIGN_PACKED_W1_LEN,
                                       2)];
  size_t inlen = rrlwr_domain_encode_2(input,
                                       RRLWR_DOMAIN_LABEL_CH,
                                       sizeof(RRLWR_DOMAIN_LABEL_CH) - 1,
                                       RRLWR_SIGN_CTILDE_LEN,
                                       mu,
                                       RRLWR_SIGN_MU_LEN,
                                       w1,
                                       RRLWR_SIGN_PACKED_W1_LEN);
  RRLWR_SIGN_HASH_H(out, RRLWR_SIGN_CTILDE_LEN, input, inlen);
}

#define RRLWR_SIGN_HASH_CH_INPUT_LEN \
  RRLWR_DOMAIN_INPUT_LEN(RRLWR_DOMAIN_LABEL_CH, \
                         RRLWR_SIGN_MU_LEN + RRLWR_SIGN_PACKED_W1_LEN, \
                         2)

static RRLWR_DOMAIN_UNUSED size_t RRLWR_SIGN_HASH_CH_INIT(uint8_t input[RRLWR_SIGN_HASH_CH_INPUT_LEN],
                                                          const uint8_t mu[RRLWR_SIGN_MU_LEN])
{
  size_t pos = rrlwr_domain_begin(input,
                                  RRLWR_DOMAIN_LABEL_CH,
                                  sizeof(RRLWR_DOMAIN_LABEL_CH) - 1,
                                  RRLWR_SIGN_CTILDE_LEN,
                                  2);
  pos = rrlwr_domain_add_field(input, pos, mu, RRLWR_SIGN_MU_LEN);
  rrlwr_store64_le(input + pos, RRLWR_SIGN_PACKED_W1_LEN);
  return pos + RRLWR_DOMAIN_LEN_BYTES;
}

static RRLWR_DOMAIN_UNUSED void RRLWR_SIGN_HASH_CH_FINAL(uint8_t out[RRLWR_SIGN_CTILDE_LEN],
                                                         const uint8_t input[RRLWR_SIGN_HASH_CH_INPUT_LEN])
{
  RRLWR_SIGN_HASH_H(out, RRLWR_SIGN_CTILDE_LEN, input, RRLWR_SIGN_HASH_CH_INPUT_LEN);
}
#endif

#ifdef RRLWR_HASH_DOMAIN_SAMPLE_C
static RRLWR_DOMAIN_UNUSED void RRLWR_SIGN_HASH_SAMPLE_C_DOMAIN(uint8_t *out,
                                                                size_t outlen,
                                                                const uint8_t ctilde[RRLWR_SIGN_CTILDE_LEN],
                                                                uint8_t stream_id,
                                                                uint32_t counter)
{
  uint8_t stream[1];
  uint8_t counter_bytes[4];
  uint8_t input[RRLWR_DOMAIN_INPUT_LEN(RRLWR_DOMAIN_LABEL_SAMPLE_C,
                                       RRLWR_SIGN_CTILDE_LEN + sizeof(stream) + sizeof(counter_bytes),
                                       3)];
  size_t inlen;

  stream[0] = stream_id;
  rrlwr_store32_le(counter_bytes, counter);
  inlen = rrlwr_domain_encode_3(input,
                                RRLWR_DOMAIN_LABEL_SAMPLE_C,
                                sizeof(RRLWR_DOMAIN_LABEL_SAMPLE_C) - 1,
                                outlen,
                                ctilde,
                                RRLWR_SIGN_CTILDE_LEN,
                                stream,
                                sizeof(stream),
                                counter_bytes,
                                sizeof(counter_bytes));
  RRLWR_SIGN_HASH_SAMPLE_C(out, outlen, input, inlen);
}
#endif

#ifdef RRLWR_HASH_DOMAIN_XOF
static RRLWR_DOMAIN_UNUSED size_t rrlwr_domain_encode_xof(uint8_t *input,
                                                          uint8_t label_id,
                                                          size_t outlen,
                                                          const uint8_t *seed,
                                                          int32_t seed_len,
                                                          uint8_t coeff,
                                                          uint8_t lane)
{
  size_t pos = 0;

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

#ifdef RRLWR_HASH_DOMAIN_XOF_PUBLIC_SINGLE
static RRLWR_DOMAIN_UNUSED void RRLWR_XOF_PUBLIC_DOMAIN(uint8_t *out,
                                                        size_t outlen,
                                                        const uint8_t *seed,
                                                        int32_t seed_len,
                                                        uint8_t coeff,
                                                        uint8_t lane)
{
  uint8_t input[RRLWR_DOMAIN_XOF_INPUT_MAX(RRLWR_DOMAIN_LABEL_PUBLIC, RRLWR_DOMAIN_XOF_SEED_MAX)];
  size_t inlen;

  if(seed_len < 0 || (size_t)seed_len > RRLWR_DOMAIN_XOF_SEED_MAX) {
    memset(out, 0, outlen);
    return;
  }

  inlen = RRLWR_DOMAIN_ENCODE_XOF_PUBLIC(input, outlen, seed, seed_len, coeff, lane);
  RRLWR_XOF_PUBLIC(out, outlen, input, inlen);
}
#endif

#ifdef RRLWR_HASH_DOMAIN_XOF_SECRET_SINGLE
static RRLWR_DOMAIN_UNUSED void RRLWR_XOF_SECRET_DOMAIN(uint8_t *out,
                                                        size_t outlen,
                                                        const uint8_t *seed,
                                                        int32_t seed_len,
                                                        uint8_t coeff,
                                                        uint8_t lane)
{
  uint8_t input[RRLWR_DOMAIN_XOF_INPUT_MAX(RRLWR_DOMAIN_LABEL_SECRET, RRLWR_DOMAIN_XOF_SEED_MAX)];
  size_t inlen;

  if(seed_len < 0 || (size_t)seed_len > RRLWR_DOMAIN_XOF_SEED_MAX) {
    memset(out, 0, outlen);
    return;
  }

  inlen = RRLWR_DOMAIN_ENCODE_XOF_SECRET(input, outlen, seed, seed_len, coeff, lane);
  RRLWR_XOF_SECRET(out, outlen, input, inlen);
}
#endif
#endif

#endif
