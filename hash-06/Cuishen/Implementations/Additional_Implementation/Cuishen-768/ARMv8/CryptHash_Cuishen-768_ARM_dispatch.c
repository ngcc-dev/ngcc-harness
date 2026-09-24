/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ARM optimized Cuishen-768 implementation dispatcher.
*/

#include "CryptHash_Cuishen-768.h"

#if defined(__linux__) && defined(__aarch64__)
#include <sys/auxv.h>
#endif

#define CUISHEN_AARCH64_HWCAP_SHA3 (1UL << 17)

int cuishen768_arm_sha3_impl(int digest_len_bits, const unsigned char *msg,
                             unsigned long long msg_len_bits,
                             unsigned char *digest);
int cuishen768_arm_nosha3_impl(int digest_len_bits, const unsigned char *msg,
                               unsigned long long msg_len_bits,
                               unsigned char *digest);

static int cuishen768_arm_detect_sha3(void) {
#if defined(CUISHEN_ARM_FORCE_NOSHA3)
  return 0;
#elif defined(CUISHEN_ARM_FORCE_SHA3)
  return 1;
#elif defined(__linux__) && defined(__aarch64__) && defined(AT_HWCAP)
  return (getauxval(AT_HWCAP) & CUISHEN_AARCH64_HWCAP_SHA3) != 0UL;
#elif defined(__ARM_FEATURE_SHA3)
  return 1;
#else
  return 0;
#endif
}

static int cuishen768_arm_have_sha3(void) {
#if defined(__GNUC__)
  static int cached = -1;
  int have_sha3 = __atomic_load_n(&cached, __ATOMIC_RELAXED);

  if (have_sha3 < 0) {
    have_sha3 = cuishen768_arm_detect_sha3();
    __atomic_store_n(&cached, have_sha3, __ATOMIC_RELAXED);
  }

  return have_sha3;
#else
  return cuishen768_arm_detect_sha3();
#endif
}

int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest) {
  if (cuishen768_arm_have_sha3()) {
    return cuishen768_arm_sha3_impl(digest_len_bits, msg, msg_len_bits,
                                    digest);
  }

  return cuishen768_arm_nosha3_impl(digest_len_bits, msg, msg_len_bits,
                                    digest);
}
