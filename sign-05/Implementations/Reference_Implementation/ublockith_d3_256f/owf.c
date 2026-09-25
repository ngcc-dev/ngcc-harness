/* SPDX-License-Identifier: MIT */

#include "owf.h"

#include "utils.h"
#include "utils_ublock/ublock.h"

#include <assert.h>
#include <string.h>

#define OWF_HAS_X86_UBLOCK_ACCE 0

#if OWF_HAS_X86_UBLOCK_ACCE
extern void uBlock_256_KeySchedule(unsigned char* key) __attribute__((weak));
extern void uBlock_256_Encrypt(unsigned char* plain, unsigned char* cipher,
                               int round) __attribute__((weak));
#endif

static inline __attribute__((unused)) int owf_ublock_has_x86_fastpath_runtime(void) {
#if OWF_HAS_X86_UBLOCK_ACCE
  return ublockith_x86_has_avx2() && (uBlock_256_KeySchedule != NULL) &&
         (uBlock_256_Encrypt != NULL);
#else
  return 0;
#endif
}

void owf_ublock_256(const uint8_t* key, const uint8_t* input, uint8_t* output) {
#if OWF_HAS_X86_UBLOCK_ACCE
  if (owf_ublock_has_x86_fastpath_runtime()) {
    uint8_t input_pair[2 * UBLOCK_BLOCK] = {0};
    uint8_t output_pair[2 * UBLOCK_BLOCK];
    memcpy(input_pair, input, UBLOCK_BLOCK);
    uBlock_256_KeySchedule((unsigned char*)key);
    uBlock_256_Encrypt((unsigned char*)input_pair, output_pair, UBLOCK_ROUNDS);
    memcpy(output, output_pair, UBLOCK_BLOCK);
    return;
  }
#endif

  ublock256_key_t ks;
  const int ret = ublock256_set_key(key, &ks);
  assert(ret == 0);
  (void)ret;

  ublock256_256_encrypt(input, output, &ks);
}

void owf_ublock_em_256(const uint8_t* key, const uint8_t* input, uint8_t* output) {
  owf_ublock_256(input, key, output);
  xor_u8_array(output, key, output, UBLOCK_KEY);
}
