/* MAMBA-Viper implementation and implementation support layer where applicable.
 * Viper PKE support with the current polynomial-arithmetic backend, public dithered
 * quantization, and an isolated schoolbook oracle for tests/debug only.
 */

#include "viper.h"
#include "viper_arith.h"
#include "viper_message_codec.h"
#include "fips202.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t modq_int(int64_t x) { return (uint16_t)x & VIPER_Q_MASK; }

void viper_pack_bits(unsigned char *out, const uint16_t *in, size_t n, unsigned bits) {
  size_t outlen = (n * bits + 7) / 8;
  uint32_t acc = 0;
  unsigned accbits = 0;
  size_t j = 0;
  uint16_t mask = (uint16_t)((1u << bits) - 1u);
  memset(out, 0, outlen);
  for (size_t i = 0; i < n; i++) {
    acc |= ((uint32_t)in[i] & mask) << accbits;
    accbits += bits;
    while (accbits >= 8) {
      out[j++] = (unsigned char)(acc & 0xffu);
      acc >>= 8;
      accbits -= 8;
    }
  }
  if (accbits) out[j] = (unsigned char)(acc & 0xffu);
}

void viper_unpack_bits(uint16_t *out, const unsigned char *in, size_t n, unsigned bits) {
  uint32_t acc = 0;
  unsigned accbits = 0;
  size_t j = 0;
  uint16_t mask = (uint16_t)((1u << bits) - 1u);
  for (size_t i = 0; i < n; i++) {
    while (accbits < bits) {
      acc |= ((uint32_t)in[j++]) << accbits;
      accbits += 8;
    }
    out[i] = (uint16_t)(acc & mask);
    acc >>= bits;
    accbits -= bits;
  }
}

uint16_t viper_quantize(uint16_t x, uint16_t d, unsigned t) {
  const unsigned shift = VIPER_QLOG - t;
  const uint16_t mask = (uint16_t)((1u << t) - 1u);
  uint16_t y = (uint16_t)((x + d) & VIPER_Q_MASK);
  return (uint16_t)(((y + (1u << (shift - 1u))) >> shift) & mask);
}

uint16_t viper_reconstruct(uint16_t b, uint16_t d, unsigned t) {
  const unsigned shift = VIPER_QLOG - t;
  return (uint16_t)(((b << shift) - d) & VIPER_Q_MASK);
}

static uint16_t quantize_shift(uint16_t x, uint16_t d, unsigned shift, uint16_t mask) {
  uint16_t y = (uint16_t)((x + d) & VIPER_Q_MASK);
  return (uint16_t)(((y + (1u << (shift - 1u))) >> shift) & mask);
}

static uint16_t reconstruct_shift(uint16_t b, uint16_t d, unsigned shift) {
  return (uint16_t)(((uint16_t)(b << shift) - d) & VIPER_Q_MASK);
}

void viper_quantize_pack_t10_array(unsigned char *out, const uint16_t *poly, const uint16_t *dither) {
  for (size_t i = 0, j = 0; i < VIPER_N; i += 4, j += 5) {
    uint16_t q0 = quantize_shift(poly[i + 0], dither[i + 0], 2u, 0x03ffu);
    uint16_t q1 = quantize_shift(poly[i + 1], dither[i + 1], 2u, 0x03ffu);
    uint16_t q2 = quantize_shift(poly[i + 2], dither[i + 2], 2u, 0x03ffu);
    uint16_t q3 = quantize_shift(poly[i + 3], dither[i + 3], 2u, 0x03ffu);
    out[j + 0] = (unsigned char)q0;
    out[j + 1] = (unsigned char)((q0 >> 8) | (q1 << 2));
    out[j + 2] = (unsigned char)((q1 >> 6) | (q2 << 4));
    out[j + 3] = (unsigned char)((q2 >> 4) | (q3 << 6));
    out[j + 4] = (unsigned char)(q3 >> 2);
  }
}

void viper_quantize_pack_t9_array(unsigned char *out, const uint16_t *poly, const uint16_t *dither) {
  for (size_t i = 0, j = 0; i < VIPER_N; i += 8, j += 9) {
    uint16_t q0 = quantize_shift(poly[i + 0], dither[i + 0], 3u, 0x01ffu);
    uint16_t q1 = quantize_shift(poly[i + 1], dither[i + 1], 3u, 0x01ffu);
    uint16_t q2 = quantize_shift(poly[i + 2], dither[i + 2], 3u, 0x01ffu);
    uint16_t q3 = quantize_shift(poly[i + 3], dither[i + 3], 3u, 0x01ffu);
    uint16_t q4 = quantize_shift(poly[i + 4], dither[i + 4], 3u, 0x01ffu);
    uint16_t q5 = quantize_shift(poly[i + 5], dither[i + 5], 3u, 0x01ffu);
    uint16_t q6 = quantize_shift(poly[i + 6], dither[i + 6], 3u, 0x01ffu);
    uint16_t q7 = quantize_shift(poly[i + 7], dither[i + 7], 3u, 0x01ffu);
    out[j + 0] = (unsigned char)q0;
    out[j + 1] = (unsigned char)((q0 >> 8) | (q1 << 1));
    out[j + 2] = (unsigned char)((q1 >> 7) | (q2 << 2));
    out[j + 3] = (unsigned char)((q2 >> 6) | (q3 << 3));
    out[j + 4] = (unsigned char)((q3 >> 5) | (q4 << 4));
    out[j + 5] = (unsigned char)((q4 >> 4) | (q5 << 5));
    out[j + 6] = (unsigned char)((q5 >> 3) | (q6 << 6));
    out[j + 7] = (unsigned char)((q6 >> 2) | (q7 << 7));
    out[j + 8] = (unsigned char)(q7 >> 1);
  }
}

void viper_quantize_pack_t4_array(unsigned char *out, const uint16_t *poly, const uint16_t *dither) {
  for (size_t i = 0, j = 0; i < VIPER_N; i += 8, j += 4) {
    uint16_t q0 = quantize_shift(poly[i + 0], dither[i + 0], 8u, 0x000fu);
    uint16_t q1 = quantize_shift(poly[i + 1], dither[i + 1], 8u, 0x000fu);
    uint16_t q2 = quantize_shift(poly[i + 2], dither[i + 2], 8u, 0x000fu);
    uint16_t q3 = quantize_shift(poly[i + 3], dither[i + 3], 8u, 0x000fu);
    uint16_t q4 = quantize_shift(poly[i + 4], dither[i + 4], 8u, 0x000fu);
    uint16_t q5 = quantize_shift(poly[i + 5], dither[i + 5], 8u, 0x000fu);
    uint16_t q6 = quantize_shift(poly[i + 6], dither[i + 6], 8u, 0x000fu);
    uint16_t q7 = quantize_shift(poly[i + 7], dither[i + 7], 8u, 0x000fu);
    out[j + 0] = (unsigned char)(q0 | (q1 << 4));
    out[j + 1] = (unsigned char)(q2 | (q3 << 4));
    out[j + 2] = (unsigned char)(q4 | (q5 << 4));
    out[j + 3] = (unsigned char)(q6 | (q7 << 4));
  }
}

void viper_quantize_pack_t3_array(unsigned char *out, const uint16_t *poly, const uint16_t *dither) {
  for (size_t i = 0, j = 0; i < VIPER_N; i += 8, j += 3) {
    uint16_t q0 = quantize_shift(poly[i + 0], dither[i + 0], 9u, 0x0007u);
    uint16_t q1 = quantize_shift(poly[i + 1], dither[i + 1], 9u, 0x0007u);
    uint16_t q2 = quantize_shift(poly[i + 2], dither[i + 2], 9u, 0x0007u);
    uint16_t q3 = quantize_shift(poly[i + 3], dither[i + 3], 9u, 0x0007u);
    uint16_t q4 = quantize_shift(poly[i + 4], dither[i + 4], 9u, 0x0007u);
    uint16_t q5 = quantize_shift(poly[i + 5], dither[i + 5], 9u, 0x0007u);
    uint16_t q6 = quantize_shift(poly[i + 6], dither[i + 6], 9u, 0x0007u);
    uint16_t q7 = quantize_shift(poly[i + 7], dither[i + 7], 9u, 0x0007u);
    out[j + 0] = (unsigned char)(q0 | (q1 << 3) | (q2 << 6));
    out[j + 1] = (unsigned char)((q2 >> 2) | (q3 << 1) | (q4 << 4) | (q5 << 7));
    out[j + 2] = (unsigned char)((q5 >> 1) | (q6 << 2) | (q7 << 5));
  }
}

void viper_unpack_reconstruct_t10_array(uint16_t *out, const unsigned char *in, const uint16_t *dither) {
  for (size_t i = 0, j = 0; i < VIPER_N; i += 4, j += 5) {
    uint16_t q0 = (uint16_t)(in[j + 0] | ((uint16_t)(in[j + 1] & 0x03u) << 8));
    uint16_t q1 = (uint16_t)((in[j + 1] >> 2) | ((uint16_t)(in[j + 2] & 0x0fu) << 6));
    uint16_t q2 = (uint16_t)((in[j + 2] >> 4) | ((uint16_t)(in[j + 3] & 0x3fu) << 4));
    uint16_t q3 = (uint16_t)((in[j + 3] >> 6) | ((uint16_t)in[j + 4] << 2));
    out[i + 0] = reconstruct_shift(q0, dither[i + 0], 2u);
    out[i + 1] = reconstruct_shift(q1, dither[i + 1], 2u);
    out[i + 2] = reconstruct_shift(q2, dither[i + 2], 2u);
    out[i + 3] = reconstruct_shift(q3, dither[i + 3], 2u);
  }
}

void viper_unpack_reconstruct_t9_array(uint16_t *out, const unsigned char *in, const uint16_t *dither) {
  for (size_t i = 0, j = 0; i < VIPER_N; i += 8, j += 9) {
    uint16_t q0 = (uint16_t)(in[j + 0] | ((uint16_t)(in[j + 1] & 0x01u) << 8));
    uint16_t q1 = (uint16_t)((in[j + 1] >> 1) | ((uint16_t)(in[j + 2] & 0x03u) << 7));
    uint16_t q2 = (uint16_t)((in[j + 2] >> 2) | ((uint16_t)(in[j + 3] & 0x07u) << 6));
    uint16_t q3 = (uint16_t)((in[j + 3] >> 3) | ((uint16_t)(in[j + 4] & 0x0fu) << 5));
    uint16_t q4 = (uint16_t)((in[j + 4] >> 4) | ((uint16_t)(in[j + 5] & 0x1fu) << 4));
    uint16_t q5 = (uint16_t)((in[j + 5] >> 5) | ((uint16_t)(in[j + 6] & 0x3fu) << 3));
    uint16_t q6 = (uint16_t)((in[j + 6] >> 6) | ((uint16_t)(in[j + 7] & 0x7fu) << 2));
    uint16_t q7 = (uint16_t)((in[j + 7] >> 7) | ((uint16_t)in[j + 8] << 1));
    out[i + 0] = reconstruct_shift(q0, dither[i + 0], 3u);
    out[i + 1] = reconstruct_shift(q1, dither[i + 1], 3u);
    out[i + 2] = reconstruct_shift(q2, dither[i + 2], 3u);
    out[i + 3] = reconstruct_shift(q3, dither[i + 3], 3u);
    out[i + 4] = reconstruct_shift(q4, dither[i + 4], 3u);
    out[i + 5] = reconstruct_shift(q5, dither[i + 5], 3u);
    out[i + 6] = reconstruct_shift(q6, dither[i + 6], 3u);
    out[i + 7] = reconstruct_shift(q7, dither[i + 7], 3u);
  }
}

void viper_unpack_reconstruct_t4_array(uint16_t *out, const unsigned char *in, const uint16_t *dither) {
  for (size_t i = 0, j = 0; i < VIPER_N; i += 8, j += 4) {
    out[i + 0] = reconstruct_shift((uint16_t)(in[j + 0] & 0x0fu), dither[i + 0], 8u);
    out[i + 1] = reconstruct_shift((uint16_t)(in[j + 0] >> 4), dither[i + 1], 8u);
    out[i + 2] = reconstruct_shift((uint16_t)(in[j + 1] & 0x0fu), dither[i + 2], 8u);
    out[i + 3] = reconstruct_shift((uint16_t)(in[j + 1] >> 4), dither[i + 3], 8u);
    out[i + 4] = reconstruct_shift((uint16_t)(in[j + 2] & 0x0fu), dither[i + 4], 8u);
    out[i + 5] = reconstruct_shift((uint16_t)(in[j + 2] >> 4), dither[i + 5], 8u);
    out[i + 6] = reconstruct_shift((uint16_t)(in[j + 3] & 0x0fu), dither[i + 6], 8u);
    out[i + 7] = reconstruct_shift((uint16_t)(in[j + 3] >> 4), dither[i + 7], 8u);
  }
}

void viper_unpack_reconstruct_t3_array(uint16_t *out, const unsigned char *in, const uint16_t *dither) {
  for (size_t i = 0, j = 0; i < VIPER_N; i += 8, j += 3) {
    uint16_t q0 = (uint16_t)(in[j + 0] & 0x07u);
    uint16_t q1 = (uint16_t)((in[j + 0] >> 3) & 0x07u);
    uint16_t q2 = (uint16_t)((in[j + 0] >> 6) | ((uint16_t)(in[j + 1] & 0x01u) << 2));
    uint16_t q3 = (uint16_t)((in[j + 1] >> 1) & 0x07u);
    uint16_t q4 = (uint16_t)((in[j + 1] >> 4) & 0x07u);
    uint16_t q5 = (uint16_t)((in[j + 1] >> 7) | ((uint16_t)(in[j + 2] & 0x03u) << 1));
    uint16_t q6 = (uint16_t)((in[j + 2] >> 2) & 0x07u);
    uint16_t q7 = (uint16_t)((in[j + 2] >> 5) & 0x07u);
    out[i + 0] = reconstruct_shift(q0, dither[i + 0], 9u);
    out[i + 1] = reconstruct_shift(q1, dither[i + 1], 9u);
    out[i + 2] = reconstruct_shift(q2, dither[i + 2], 9u);
    out[i + 3] = reconstruct_shift(q3, dither[i + 3], 9u);
    out[i + 4] = reconstruct_shift(q4, dither[i + 4], 9u);
    out[i + 5] = reconstruct_shift(q5, dither[i + 5], 9u);
    out[i + 6] = reconstruct_shift(q6, dither[i + 6], 9u);
    out[i + 7] = reconstruct_shift(q7, dither[i + 7], 9u);
  }
}

void viper_quantize_pack_array(unsigned char *out, const uint16_t *poly, const uint16_t *dither, unsigned bits) {
  uint16_t q[VIPER_N];
  switch (bits) {
    case 10: viper_quantize_pack_t10_array(out, poly, dither); return;
    case 9: viper_quantize_pack_t9_array(out, poly, dither); return;
    case 4: viper_quantize_pack_t4_array(out, poly, dither); return;
    case 3: viper_quantize_pack_t3_array(out, poly, dither); return;
    default:
      for (size_t i = 0; i < VIPER_N; i++) q[i] = viper_quantize(poly[i], dither[i], bits);
      viper_pack_bits(out, q, VIPER_N, bits);
      return;
  }
}

void viper_unpack_reconstruct_array(uint16_t *out, const unsigned char *in, const uint16_t *dither, unsigned bits) {
  uint16_t q[VIPER_N];
  switch (bits) {
    case 10: viper_unpack_reconstruct_t10_array(out, in, dither); return;
    case 9: viper_unpack_reconstruct_t9_array(out, in, dither); return;
    case 4: viper_unpack_reconstruct_t4_array(out, in, dither); return;
    case 3: viper_unpack_reconstruct_t3_array(out, in, dither); return;
    default:
      viper_unpack_bits(q, in, VIPER_N, bits);
      for (size_t i = 0; i < VIPER_N; i++) out[i] = viper_reconstruct(q[i], dither[i], bits);
      return;
  }
}

static void shake128_label(unsigned char *out, unsigned long long outlen, const char *label, const unsigned char seed[32]) {
  unsigned char in[48];
  size_t l = strlen(label);
  memset(in, 0, sizeof(in));
  if (l > 15) l = 15;
  memcpy(in, label, l);
  memcpy(in + 16, seed, 32);
  shake128(out, outlen, in, sizeof(in));
}

typedef struct {
  const unsigned char *buf;
  size_t pos;
  uint32_t acc;
  unsigned accbits;
} viper_bitreader;

static uint16_t bitreader_read(viper_bitreader *br, unsigned bits) {
  while (br->accbits < bits) {
    br->acc |= ((uint32_t)br->buf[br->pos++]) << br->accbits;
    br->accbits += 8;
  }
  uint16_t mask = (uint16_t)((1u << bits) - 1u);
  uint16_t out = (uint16_t)(br->acc & mask);
  br->acc >>= bits;
  br->accbits -= bits;
  return out;
}

static size_t read_dither2(uint16_t *out, size_t n, const unsigned char *buf, size_t off) {
  for (size_t i = 0; i < n; i += 4, off++) {
    unsigned b = buf[off];
    out[i + 0] = (uint16_t)(b & 3u);
    out[i + 1] = (uint16_t)((b >> 2) & 3u);
    out[i + 2] = (uint16_t)((b >> 4) & 3u);
    out[i + 3] = (uint16_t)((b >> 6) & 3u);
  }
  return off;
}

static size_t read_dither3(uint16_t *out, size_t n, const unsigned char *buf, size_t off) {
  for (size_t i = 0; i < n; i += 8, off += 3) {
    uint32_t w = (uint32_t)buf[off] | ((uint32_t)buf[off + 1] << 8) | ((uint32_t)buf[off + 2] << 16);
    out[i + 0] = (uint16_t)(w & 7u);
    out[i + 1] = (uint16_t)((w >> 3) & 7u);
    out[i + 2] = (uint16_t)((w >> 6) & 7u);
    out[i + 3] = (uint16_t)((w >> 9) & 7u);
    out[i + 4] = (uint16_t)((w >> 12) & 7u);
    out[i + 5] = (uint16_t)((w >> 15) & 7u);
    out[i + 6] = (uint16_t)((w >> 18) & 7u);
    out[i + 7] = (uint16_t)((w >> 21) & 7u);
  }
  return off;
}

static size_t read_dither8(uint16_t *out, size_t n, const unsigned char *buf, size_t off) {
  for (size_t i = 0; i < n; i++, off++) out[i] = buf[off];
  return off;
}

static size_t read_dither_bits(uint16_t *out, size_t n, const unsigned char *buf, size_t off, unsigned bits) {
  viper_bitreader br = {buf + off, 0, 0, 0};
  for (size_t i = 0; i < n; i++) out[i] = bitreader_read(&br, bits);
  return off + (n * bits + 7u) / 8u;
}

void viper_gen_dither(uint16_t du[VIPER_K][VIPER_N], uint16_t dv[VIPER_N], const unsigned char mu[VIPER_MU_BYTES]) {
  const size_t need_bits = VIPER_K * VIPER_N * (VIPER_QLOG - VIPER_T_U) + VIPER_N * (VIPER_QLOG - VIPER_T_V);
  const size_t need = (need_bits + 7u) / 8u;
  unsigned char buf[(VIPER_K * VIPER_N * (VIPER_QLOG - VIPER_T_U) + VIPER_N * (VIPER_QLOG - VIPER_T_V) + 7u) / 8u];
  size_t off = 0;
  shake128_label(buf, need, "ViperDither", mu);
  for (size_t i = 0; i < VIPER_K; i++) {
    if ((VIPER_QLOG - VIPER_T_U) == 2u) off = read_dither2(du[i], VIPER_N, buf, off);
    else if ((VIPER_QLOG - VIPER_T_U) == 3u) off = read_dither3(du[i], VIPER_N, buf, off);
    else off = read_dither_bits(du[i], VIPER_N, buf, off, VIPER_QLOG - VIPER_T_U);
  }
  if ((VIPER_QLOG - VIPER_T_V) == 8u) (void)read_dither8(dv, VIPER_N, buf, off);
  else (void)read_dither_bits(dv, VIPER_N, buf, off, VIPER_QLOG - VIPER_T_V);
}

#define VIPER_PUBLIC_A_POLYBYTES (VIPER_N * VIPER_QLOG / 8u)
#define VIPER_PUBLIC_DPK_POLYBYTES (VIPER_N * 2u / 8u)
#define VIPER_PUBLIC_A_BYTES (VIPER_K * VIPER_K * VIPER_PUBLIC_A_POLYBYTES)
#define VIPER_PUBLIC_DPK_BYTES (VIPER_K * VIPER_PUBLIC_DPK_POLYBYTES)
#define VIPER_PUBLIC_STREAM_BYTES (VIPER_PUBLIC_A_BYTES + VIPER_PUBLIC_DPK_BYTES)
#define VIPER_GENPUBLIC_INPUT_BYTES 36u
#define VIPER_DOMAIN_A 0x41u
#define VIPER_DOMAIN_DPK 0x44u

static void genpublic_input(unsigned char in[VIPER_GENPUBLIC_INPUT_BYTES], const unsigned char rho[32], unsigned domain, size_t i, size_t j) {
  memcpy(in, rho, 32);
  in[32] = (unsigned char)domain;
  in[33] = (unsigned char)i;
  in[34] = (unsigned char)j;
  in[35] = 0;
}

static size_t genpublic_a_offset(size_t i, size_t j) {
  return (i * VIPER_K + j) * VIPER_PUBLIC_A_POLYBYTES;
}

static size_t genpublic_dpk_offset(size_t i) {
  return VIPER_PUBLIC_A_BYTES + i * VIPER_PUBLIC_DPK_POLYBYTES;
}

void viper_gen_public_shake(unsigned char *buf, const unsigned char rho[32]) {
  unsigned char in[VIPER_GENPUBLIC_INPUT_BYTES];
  for (size_t i = 0; i < VIPER_K; i++) {
    for (size_t j = 0; j < VIPER_K; j++) {
      genpublic_input(in, rho, VIPER_DOMAIN_A, i, j);
      shake128(buf + genpublic_a_offset(i, j), VIPER_PUBLIC_A_POLYBYTES, in, sizeof(in));
    }
  }
  for (size_t i = 0; i < VIPER_K; i++) {
    genpublic_input(in, rho, VIPER_DOMAIN_DPK, i, 0);
    shake128(buf + genpublic_dpk_offset(i), VIPER_PUBLIC_DPK_POLYBYTES, in, sizeof(in));
  }
}

static void parse_A_poly(vpoly out, const unsigned char *buf) {
  viper_unpack_bits(out, buf, VIPER_N, VIPER_QLOG);
  for (size_t l = 0; l < VIPER_N; l++) out[l] &= VIPER_QMASK;
}

void viper_gen_public_parse_A(vpoly A[VIPER_K][VIPER_K], const unsigned char *buf) {
  for (size_t i = 0; i < VIPER_K; i++) {
    for (size_t j = 0; j < VIPER_K; j++) parse_A_poly(A[i][j], buf + genpublic_a_offset(i, j));
  }
}

void viper_gen_public_parse_dpk(uint16_t dpk[VIPER_K][VIPER_N], const unsigned char *buf) {
  for (size_t i = 0; i < VIPER_K; i++) {
    const unsigned char *p = buf + genpublic_dpk_offset(i);
    for (size_t l = 0; l < VIPER_N; l += 4, p++) {
      unsigned b = *p;
      dpk[i][l + 0] = (uint16_t)(b & 3u);
      dpk[i][l + 1] = (uint16_t)((b >> 2) & 3u);
      dpk[i][l + 2] = (uint16_t)((b >> 4) & 3u);
      dpk[i][l + 3] = (uint16_t)((b >> 6) & 3u);
    }
  }
}

void viper_gen_public(vpoly A[VIPER_K][VIPER_K], uint16_t dpk[VIPER_K][VIPER_N], const unsigned char rho[32]) {
  unsigned char buf[VIPER_PUBLIC_STREAM_BYTES];
  viper_gen_public_shake(buf, rho);
  viper_gen_public_parse_A(A, buf);
  viper_gen_public_parse_dpk(dpk, buf);
}

static void genpublic_expand_A_poly(vpoly out, const unsigned char rho[32], size_t i, size_t j) {
  unsigned char in[VIPER_GENPUBLIC_INPUT_BYTES];
  unsigned char buf[VIPER_PUBLIC_A_POLYBYTES];
  genpublic_input(in, rho, VIPER_DOMAIN_A, i, j);
  shake128(buf, sizeof(buf), in, sizeof(in));
  parse_A_poly(out, buf);
}

void viper_genpublic_matvec_fused_experiment(vpolyvec out, uint16_t dpk[VIPER_K][VIPER_N], const unsigned char rho[32], const vpolyvec s, int transpose) {
  vpoly a, t;
  unsigned char in[VIPER_GENPUBLIC_INPUT_BYTES];
  unsigned char dbuf[VIPER_PUBLIC_DPK_POLYBYTES];
  memset(out, 0, sizeof(vpolyvec));
  for (size_t i = 0; i < VIPER_K; i++) {
    for (size_t j = 0; j < VIPER_K; j++) {
      genpublic_expand_A_poly(a, rho, transpose ? j : i, transpose ? i : j);
      viper_poly_mul(t, a, s[j]);
      for (size_t l = 0; l < VIPER_N; l++) out[i][l] = (uint16_t)((out[i][l] + t[l]) & VIPER_Q_MASK);
    }
  }
  for (size_t i = 0; i < VIPER_K; i++) {
    genpublic_input(in, rho, VIPER_DOMAIN_DPK, i, 0);
    shake128(dbuf, sizeof(dbuf), in, sizeof(in));
    for (size_t l = 0, off = 0; l < VIPER_N; l += 4, off++) {
      unsigned b = dbuf[off];
      dpk[i][l + 0] = (uint16_t)(b & 3u);
      dpk[i][l + 1] = (uint16_t)((b >> 2) & 3u);
      dpk[i][l + 2] = (uint16_t)((b >> 4) & 3u);
      dpk[i][l + 3] = (uint16_t)((b >> 6) & 3u);
    }
  }
}

void viper_sample_secret(vpolyvec s, const unsigned char seed[32], unsigned eta) {
  if (eta == 2u) {
    uint16_t cbd2_lut[16] = {0, 1, 1, 2, VIPER_QMASK, 0, 0, 1, VIPER_QMASK, 0, 0, 1, VIPER_Q - 2, VIPER_QMASK, VIPER_QMASK, 0};
    unsigned char buf[(VIPER_K * VIPER_N * 4u + 7u) / 8u];
    shake256(buf, sizeof(buf), seed, 32);
    for (size_t i = 0; i < VIPER_K; i++) {
      for (size_t j = 0; j < VIPER_N; j++) {
        size_t idx = i * VIPER_N + j;
        unsigned nibble = (unsigned)(buf[idx >> 1] >> ((idx & 1u) * 4u)) & 0x0fu;
        s[i][j] = cbd2_lut[nibble];
      }
    }
    return;
  }

  unsigned char buf[VIPER_K * VIPER_N];
  shake256(buf, sizeof(buf), seed, 32);
  for (size_t i = 0; i < VIPER_K; i++) {
    for (size_t j = 0; j < VIPER_N; j++) {
      unsigned byte = buf[i * VIPER_N + j];
      int val = 0;
      for (unsigned b = 0; b < eta; b++) val += (int)((byte >> b) & 1u);
      for (unsigned b = 0; b < eta; b++) val -= (int)((byte >> (eta + b)) & 1u);
      s[i][j] = modq_int(val);
    }
  }
}

void viper_poly_mul_schoolbook_oracle(vpoly c, const vpoly a, const vpoly b) {
  int64_t tmp[VIPER_N];
  memset(tmp, 0, sizeof(tmp));
  for (size_t i = 0; i < VIPER_N; i++) {
    int64_t ai = (int64_t)a[i];
    for (size_t j = 0; j < VIPER_N; j++) {
      int64_t prod = ai * (int64_t)b[j];
      size_t idx = i + j;
      if (idx >= VIPER_N) tmp[idx - VIPER_N] -= prod;
      else tmp[idx] += prod;
    }
  }
  for (size_t i = 0; i < VIPER_N; i++) c[i] = modq_int(tmp[i]);
}

static void pack_secret_poly(unsigned char *out, const vpoly s) {
  uint16_t tmp[VIPER_N];
  for (size_t i = 0; i < VIPER_N; i++) {
    int v = (int)(s[i] & VIPER_QMASK);
    if (v >= VIPER_Q / 2) v -= VIPER_Q;
    tmp[i] = (uint16_t)(v & VIPER_Q_MASK);
  }
  viper_pack_bits(out, tmp, VIPER_N, VIPER_QLOG);
}

static void unpack_secret_poly(vpoly out, const unsigned char *in) {
  uint16_t tmp[VIPER_N];
  viper_unpack_bits(tmp, in, VIPER_N, VIPER_QLOG);
  for (size_t i = 0; i < VIPER_N; i++) {
    int v = (int)(tmp[i] & VIPER_Q_MASK);
    if (v & (VIPER_Q >> 1)) v -= VIPER_Q;
    out[i] = modq_int(v);
  }
}

static void poly_add(vpoly r, const vpoly a) { for (size_t i = 0; i < VIPER_N; i++) r[i] = (uint16_t)((r[i] + a[i]) & VIPER_Q_MASK); }
static void poly_sub(vpoly r, const vpoly a) { for (size_t i = 0; i < VIPER_N; i++) r[i] = (uint16_t)((r[i] - a[i]) & VIPER_Q_MASK); }

void viper_encode(vpoly out, const unsigned char m[VIPER_MSGBYTES]) {
  viper_msg_encode(out, m);
}

void viper_decode(unsigned char m[VIPER_MSGBYTES], const vpoly in) {
  viper_msg_decode(m, in);
}

void viper_pke_keypair(unsigned char *pk, unsigned char *skpke, const unsigned char rho[32], const unsigned char sseed[32]) {
  vpoly A[VIPER_K][VIPER_K];
  uint16_t dpk[VIPER_K][VIPER_N];
  vpolyvec s, b;
  memcpy(pk, rho, 32);
  viper_gen_public(A, dpk, rho);
  viper_sample_secret(s, sseed, VIPER_ETA_S);
  viper_matvec(b, A, s);
  for (size_t i = 0; i < VIPER_K; i++) {
    viper_quantize_pack_array(pk + 32 + i * VIPER_PACKED_PK_POLYBYTES, b[i], dpk[i], VIPER_T_PK);
    pack_secret_poly(skpke + i * VIPER_SECRET_POLYBYTES, s[i]);
  }
}

void viper_pke_enc(unsigned char *ct, const unsigned char *pk, const unsigned char m[VIPER_MSGBYTES], const unsigned char omega[VIPER_FALLBACK_KEY_BYTES + VIPER_MU_BYTES]) {
  vpoly A[VIPER_K][VIPER_K], acc, t;
  uint16_t dpk[VIPER_K][VIPER_N], du[VIPER_K][VIPER_N], dv[VIPER_N];
  vpolyvec r, u, bhat;
  const unsigned char *rho = pk;
  const unsigned char *mu = omega + VIPER_FALLBACK_KEY_BYTES;
  viper_gen_public(A, dpk, rho);
  viper_gen_dither(du, dv, mu);
  viper_sample_secret(r, omega, VIPER_ETA_R);
  for (size_t i = 0; i < VIPER_K; i++) {
    viper_unpack_reconstruct_array(bhat[i], pk + 32 + i * VIPER_PACKED_PK_POLYBYTES, dpk[i], VIPER_T_PK);
  }
  viper_matTvec(u, A, r);
  for (size_t i = 0; i < VIPER_K; i++) {
    viper_quantize_pack_array(ct + i * VIPER_PACKED_U_POLYBYTES, u[i], du[i], VIPER_T_U);
  }
  viper_encode(acc, m);
  viper_dot(t, bhat, r);
  poly_add(acc, t);
  viper_quantize_pack_array(ct + VIPER_PACKED_U_BYTES, acc, dv, VIPER_T_V);
  memcpy(ct + VIPER_PACKED_U_BYTES + VIPER_PACKED_V_BYTES, mu, VIPER_MU_BYTES);
}

void viper_pke_dec(unsigned char m[VIPER_MSGBYTES], const unsigned char *skpke, const unsigned char *ct) {
  uint16_t du[VIPER_K][VIPER_N], dv[VIPER_N];
  vpolyvec s, u;
  vpoly w, t;
  const unsigned char *mu = ct + VIPER_PACKED_U_BYTES + VIPER_PACKED_V_BYTES;
  viper_gen_dither(du, dv, mu);
  for (size_t i = 0; i < VIPER_K; i++) {
    unpack_secret_poly(s[i], skpke + i * VIPER_SECRET_POLYBYTES);
    viper_unpack_reconstruct_array(u[i], ct + i * VIPER_PACKED_U_POLYBYTES, du[i], VIPER_T_U);
  }
  viper_unpack_reconstruct_array(w, ct + VIPER_PACKED_U_BYTES, dv, VIPER_T_V);
  viper_dot(t, s, u);
  poly_sub(w, t);
  viper_decode(m, w);
}

static unsigned diff_bytes(const unsigned char *a, const unsigned char *b, size_t n) {
  unsigned diff = 0;
  for (size_t i = 0; i < n; i++) diff |= (unsigned)(a[i] ^ b[i]);
  return diff;
}

int viper_reencrypt_check(const unsigned char *ct, const unsigned char *pk, const unsigned char m[VIPER_MSGBYTES], const unsigned char sigma[VIPER_FALLBACK_KEY_BYTES]) {
  vpoly A[VIPER_K][VIPER_K], acc, t;
  uint16_t dpk[VIPER_K][VIPER_N], du[VIPER_K][VIPER_N], dv[VIPER_N];
  vpolyvec r, u, bhat;
  unsigned char omega[VIPER_FALLBACK_KEY_BYTES + VIPER_MU_BYTES];
  unsigned char packed_u[VIPER_PACKED_U_POLYBYTES];
  unsigned char packed_v[VIPER_PACKED_V_BYTES];
  const unsigned char *rho = pk;
  const unsigned char *mu = ct + VIPER_PACKED_U_BYTES + VIPER_PACKED_V_BYTES;
  unsigned diff = 0;

  memcpy(omega, sigma, VIPER_FALLBACK_KEY_BYTES);
  memcpy(omega + VIPER_FALLBACK_KEY_BYTES, mu, VIPER_MU_BYTES);
  viper_gen_public(A, dpk, rho);
  viper_gen_dither(du, dv, mu);
  viper_sample_secret(r, omega, VIPER_ETA_R);
  for (size_t i = 0; i < VIPER_K; i++) {
    viper_unpack_reconstruct_array(bhat[i], pk + 32 + i * VIPER_PACKED_PK_POLYBYTES, dpk[i], VIPER_T_PK);
  }
  viper_matTvec(u, A, r);
  for (size_t i = 0; i < VIPER_K; i++) {
    viper_quantize_pack_array(packed_u, u[i], du[i], VIPER_T_U);
    diff |= diff_bytes(packed_u, ct + i * VIPER_PACKED_U_POLYBYTES, VIPER_PACKED_U_POLYBYTES);
  }
  viper_encode(acc, m);
  viper_dot(t, bhat, r);
  poly_add(acc, t);
  viper_quantize_pack_array(packed_v, acc, dv, VIPER_T_V);
  diff |= diff_bytes(packed_v, ct + VIPER_PACKED_U_BYTES, VIPER_PACKED_V_BYTES);
  diff |= diff_bytes(mu, ct + VIPER_PACKED_U_BYTES + VIPER_PACKED_V_BYTES, VIPER_MU_BYTES);
  return diff == 0;
}
