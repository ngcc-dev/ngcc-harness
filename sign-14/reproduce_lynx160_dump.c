/* sign-14-6 witness, part 1: run the submitted Lynx-160 code on one (iv, key).
 * Built with the unmodified Lynxer-160f reference sources (default XOF).
 * Usage: reproduce_lynx160_dump <lambda> <iv-hex> <key-hex>
 * Prints the matrices exactly as owf.c reads them (row stride ceil(lambda/64)
 * words over the padded uint64_t L[512][8] array, masked to lambda columns),
 * the constants, the output y of the submitted owf_lynx_<lambda>, and the
 * extended witness k || v1 || v2 of the submitted lynx_extend_witness. The
 * attack in reproduce_lynx160_key_recovery.py uses iv, y, the matrices and
 * the constants; the key and the witness are used only to check the model
 * and the result. Written by an analysis agent and reviewed for this
 * repository. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "lynx_matrices.h"
#include "owf.h"
#include "instances.h"

static lynx_t mats;

static int hex2bin(const char* h, uint8_t* out, size_t n) {
  if (strlen(h) != 2 * n) return -1;
  for (size_t i = 0; i < n; i++) {
    unsigned v;
    if (sscanf(h + 2 * i, "%2x", &v) != 1) return -1;
    out[i] = (uint8_t)v;
  }
  return 0;
}

static void phex(const char* tag, const uint8_t* b, size_t n) {
  printf("%s ", tag);
  for (size_t i = 0; i < n; i++) printf("%02x", b[i]);
  printf("\n");
}

static void dump_used(const char* name, const uint64_t M[LYNX_MAX_CSP][LYNX_MAX_WORDS], unsigned lambda) {
  unsigned words = (lambda + 63) / 64;
  const uint64_t* flat = (const uint64_t*)M;
  for (unsigned r = 0; r < lambda; r++) {
    printf("%s %u", name, r);
    for (unsigned w = 0; w < words; w++) {
      uint64_t v = flat[r * words + w];
      unsigned rem = lambda - w * 64;
      if (rem < 64) v &= (UINT64_C(1) << rem) - 1;
      printf(" %016llx", (unsigned long long)v);
    }
    printf("\n");
  }
}

int main(int argc, char** argv) {
  if (argc != 4) { fprintf(stderr, "usage\n"); return 1; }
  unsigned lambda = (unsigned)atoi(argv[1]);
  unsigned nb = lambda / 8;
  uint8_t iv[64], key[64], y[64], w[3 * 64];
  if (hex2bin(argv[2], iv, nb) || hex2bin(argv[3], key, nb)) { fprintf(stderr, "bad hex\n"); return 1; }
  sig_paramid_t pid;
  switch (lambda) {
  case 160: pid = LYNXER_160F; owf_lynx_160(key, iv, y); break;
  case 256: pid = LYNXER_256F; owf_lynx_256(key, iv, y); break;
  case 384: pid = LYNXER_384F; owf_lynx_384(key, iv, y); break;
  case 512: pid = LYNXER_512F; owf_lynx_512(key, iv, y); break;
  default: return 1;
  }
  const sig_paramset_t* params = sig_get_paramset(pid);
  if (params->csp != lambda || params->owf_input_size != nb) { fprintf(stderr, "param mismatch\n"); return 1; }
  lynx_extend_witness(w, key, iv, params);
  if (lynx_init(&mats, lambda, iv, nb)) { fprintf(stderr, "init failed\n"); return 1; }
  printf("lambda %u\n", lambda);
  phex("iv", iv, nb);
  phex("key", key, nb);
  phex("y", y, nb);
  phex("w", w, 3 * nb);
  phex("C0", mats.C0, nb);
  phex("C1", mats.C1, nb);
  phex("C2", mats.C2, nb);
  dump_used("L0", mats.L0, lambda);
  dump_used("L1", mats.L1, lambda);
  dump_used("L2", mats.L2, lambda);
  dump_used("L3", mats.L3, lambda);
  return 0;
}
