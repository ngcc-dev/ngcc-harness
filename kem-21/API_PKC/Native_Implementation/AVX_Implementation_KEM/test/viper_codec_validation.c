#include "../api.h"
#include "../viper.h"
#include "../viper_e8.h"
#include "../viper_message_codec.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

static int fail(const char *what) { fprintf(stderr, "FAIL: %s\n", what); return 1; }

static void fill_msg(unsigned char m[VIPER_MSGBYTES], unsigned seed) {
  for (size_t i = 0; i < VIPER_MSGBYTES; i++) m[i] = (unsigned char)(seed + 131u * i + (i >> 1));
}

static int roundtrip_msg(const unsigned char m[VIPER_MSGBYTES], const char *name) {
  vpoly p;
  unsigned char out[VIPER_MSGBYTES];
  viper_msg_encode(p, m);
  viper_msg_decode(out, p);
  if (memcmp(m, out, VIPER_MSGBYTES) != 0) {
    fprintf(stderr, "message roundtrip mismatch: %s level=%d codec=%s\n", name, VIPER_LEVEL, viper_message_codec_name());
    return 1;
  }
  return 0;
}

static int e8_label_exhaustive(void) {
  const unsigned labels = 1u << VIPER_E8_BITS_PER_BLOCK;
  uint16_t coeffs[8];
  for (unsigned label = 0; label < labels; label++) {
    for (unsigned j = 0; j < 8; j++) coeffs[j] = viper_e8_label_to_coeff(label, j);
    unsigned got = viper_e8_coeffs_to_label(coeffs);
    if (got != label) {
      fprintf(stderr, "E8 label mismatch: level=%d q=%d rate=%d alpha=%d label=%u got=%u coeffs=", VIPER_LEVEL, VIPER_Q, VIPER_E8_RATE, VIPER_E8_ALPHA, label, got);
      for (unsigned j = 0; j < 8; j++) fprintf(stderr, "%u%s", coeffs[j], j == 7 ? "" : ",");
      fprintf(stderr, "\n");
      return 1;
    }
  }
  return 0;
}

static int e8_boundary_labels(void) {
  const unsigned labels = 1u << VIPER_E8_BITS_PER_BLOCK;
  unsigned tests[24];
  unsigned n = 0;
  tests[n++] = 0;
  tests[n++] = 1;
  tests[n++] = labels - 1;
  for (unsigned b = 0; b < VIPER_E8_BITS_PER_BLOCK; b++) tests[n++] = 1u << b;
  uint16_t coeffs[8];
  for (unsigned i = 0; i < n; i++) {
    unsigned label = tests[i] & (labels - 1u);
    for (unsigned j = 0; j < 8; j++) coeffs[j] = viper_e8_label_to_coeff(label, j);
    if (viper_e8_coeffs_to_label(coeffs) != label) return 1;
  }
  return 0;
}

int main(void) {
  printf("codec validation: alg=%s level=%d q=%d msgbytes=%d ssbytes=%d codec=%s use_e8=%d e8_rate=%d e8_bits_per_block=%d e8_active_blocks=%d e8_alpha=%d pk=%d ct=%d sk=%d ss=%d\n",
         CRYPTO_ALGNAME, VIPER_LEVEL, VIPER_Q, VIPER_MSGBYTES, VIPER_SSBYTES,
         viper_message_codec_name(), VIPER_USE_E8_CODEC, VIPER_E8_RATE,
         VIPER_E8_BITS_PER_BLOCK, VIPER_E8_ACTIVE_BLOCKS, VIPER_E8_ALPHA,
         CRYPTO_PUBLICKEYBYTES, CRYPTO_CIPHERTEXTBYTES, CRYPTO_SECRETKEYBYTES, CRYPTO_BYTES);
#if VIPER_USE_E8_CODEC
  printf("E8 labeling: %s\n", viper_e8_labeling_name());
  if (e8_label_exhaustive()) return fail("E8 exhaustive label/delabel");
  if (e8_boundary_labels()) return fail("E8 boundary labels");
#endif
  unsigned char m[VIPER_MSGBYTES];
  memset(m, 0, sizeof(m));
  if (roundtrip_msg(m, "all-zero")) return fail("all-zero roundtrip");
  memset(m, 0xff, sizeof(m));
  if (roundtrip_msg(m, "all-ff")) return fail("all-ff roundtrip");
  for (size_t bit = 0; bit < VIPER_MSGBITS; bit++) {
    memset(m, 0, sizeof(m));
    m[bit >> 3] = (unsigned char)(1u << (bit & 7));
    if (roundtrip_msg(m, "single-bit")) return fail("single-bit roundtrip");
  }
  for (unsigned t = 0; t < 256; t++) {
    fill_msg(m, t * 17u + 5u);
    if (roundtrip_msg(m, "random-pattern")) return fail("random-pattern roundtrip");
  }
  printf("codec validation ok: level=%d codec=%s exhaustive_labels=%u\n", VIPER_LEVEL, viper_message_codec_name(), 1u << VIPER_E8_BITS_PER_BLOCK);
  return 0;
}
