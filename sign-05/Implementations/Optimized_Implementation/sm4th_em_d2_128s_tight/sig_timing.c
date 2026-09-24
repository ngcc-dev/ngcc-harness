#include "sig_timing.h"

#if SM4TH_COMPONENT_TIMING
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifndef SM4TH_COMPONENT_TIMING_VERBOSE
#define SM4TH_COMPONENT_TIMING_VERBOSE 0
#endif

enum {
  SIG_TIMING_BAVC = 0,
  SIG_TIMING_VOLE,
  SIG_TIMING_OWF,
  SIG_TIMING_FS_HASH,
  SIG_TIMING_MISC,
  SIG_TIMING_BUCKETS
};

static struct {
  char op[16];
  double bucket[SIG_TIMING_BUCKETS];
  double total_ms;
} g_sig_timing;

double sm4th_timing_now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static int label_has(const char* label, const char* needle) {
  return strstr(label, needle) != NULL;
}

static int timing_bucket_for_label(const char* label) {
  if (label_has(label, ".total")) {
    return -1;
  }
  if (label_has(label, "sign.bavc_commit") || label_has(label, "verify.bavc_reconstruct")) {
    return -1;
  }
  if (label_has(label, "vole_commit") || label_has(label, "vole_reconstruct")) {
    return -1;
  }
  if (label_has(label, "bavc")) {
    return SIG_TIMING_BAVC;
  }
  if (label_has(label, "vole_convert")) {
    return SIG_TIMING_VOLE;
  }
  if (label_has(label, "sm4_prove") || label_has(label, "sm4_verify")) {
    return SIG_TIMING_OWF;
  }
  if (label_has(label, "misc") || label_has(label, "cleanup") ||
      label_has(label, "alloc") || label_has(label, "memcpy") ||
      label_has(label, "xor")) {
    return SIG_TIMING_MISC;
  }
  if (label_has(label, "hash") || label_has(label, "challenge") ||
      label_has(label, "chall")) {
    return SIG_TIMING_FS_HASH;
  }
  return SIG_TIMING_MISC;
}

void sm4th_timing_reset(const char* op) {
  memset(&g_sig_timing, 0, sizeof(g_sig_timing));
  if (op) {
    snprintf(g_sig_timing.op, sizeof(g_sig_timing.op), "%s", op);
  }
}

void sm4th_timing_record(const char* label, double elapsed_ms) {
  if (!label) {
    return;
  }
  if (label_has(label, ".total")) {
    g_sig_timing.total_ms += elapsed_ms;
  } else {
    const int bucket = timing_bucket_for_label(label);
    if (bucket >= 0 && bucket < SIG_TIMING_BUCKETS) {
      g_sig_timing.bucket[bucket] += elapsed_ms;
    }
  }
#if SM4TH_COMPONENT_TIMING_VERBOSE
  fprintf(stderr, "[sm4th timing] %-56s %9.6f ms\n", label, elapsed_ms);
#endif
}

void sm4th_timing_report(const char* op) {
  const char* name = op ? op : g_sig_timing.op;
  const double bavc = g_sig_timing.bucket[SIG_TIMING_BAVC];
  const double vole = g_sig_timing.bucket[SIG_TIMING_VOLE];
  const double owf = g_sig_timing.bucket[SIG_TIMING_OWF];
  const double fs_hash = g_sig_timing.bucket[SIG_TIMING_FS_HASH];
  const double explicit_misc = g_sig_timing.bucket[SIG_TIMING_MISC];
  const double measured_total = g_sig_timing.total_ms;
  const double named_without_misc = bavc + vole + owf + fs_hash;
  double misc = explicit_misc;

  if (measured_total > named_without_misc) {
    misc = measured_total - named_without_misc;
  }

  fprintf(stderr, "[sm4th timing] %s components (ms)\n", name ? name : "op");
  fprintf(stderr, "  BAVC commit/reconstruct:      %9.6f\n", bavc);
  fprintf(stderr, "  VOLE convert/reconstruct:     %9.6f\n", vole);
  fprintf(stderr, "  OWF proof/verify:             %9.6f\n", owf);
  fprintf(stderr, "  Fiat-Shamir/hash:             %9.6f\n", fs_hash);
  fprintf(stderr, "  misc/memcpy/allocation:       %9.6f\n", misc);
  fprintf(stderr, "  total measured:               %9.6f\n", measured_total);
}
#endif
