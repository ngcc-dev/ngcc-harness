#ifndef SM4TH_X86_CAPS_H
#define SM4TH_X86_CAPS_H

/*
 * Shared x86 feature probing helpers.
 * We cache detection once per process and reuse it across PRG/SM4 paths.
 */

enum {
  SM4TH_X86_CAP_AES   = 1u << 0,
  SM4TH_X86_CAP_SSSE3 = 1u << 1,
  SM4TH_X86_CAP_AVX2  = 1u << 2,
};

static inline unsigned int sm4th_x86_get_caps(void) {
#if defined(__x86_64__) || defined(__i386__)
  static unsigned int caps = 0;
  static int initialized = 0;

  if (!initialized) {
#if defined(__GNUC__) || defined(__clang__)
    __builtin_cpu_init();
    if (__builtin_cpu_supports("aes")) {
      caps |= SM4TH_X86_CAP_AES;
    }
    if (__builtin_cpu_supports("ssse3")) {
      caps |= SM4TH_X86_CAP_SSSE3;
    }
    if (__builtin_cpu_supports("avx2")) {
      caps |= SM4TH_X86_CAP_AVX2;
    }
#endif
    initialized = 1;
  }
  return caps;
#else
  return 0;
#endif
}

static inline int sm4th_x86_has_aes_ssse3(void) {
  const unsigned int caps = sm4th_x86_get_caps();
  const unsigned int want = SM4TH_X86_CAP_AES | SM4TH_X86_CAP_SSSE3;
  return (caps & want) == want;
}

static inline int sm4th_x86_has_avx2(void) {
  return (sm4th_x86_get_caps() & SM4TH_X86_CAP_AVX2) != 0;
}

#endif
