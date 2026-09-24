/*
 * SPDX-License-Identifier: MIT
 */

#include <assert.h>

#include "owf.h"
#include "utils_sm4/sm4.h"
#include "utils.h"

void owf_sm4_128(const uint8_t* key, const uint8_t* input, uint8_t* output) {
  generic_sm4_ecb_t ctx;
  int ret = generic_sm4_ecb_new(&ctx, key, 128);
  assert(ret == 0);
  (void)ret;

  ret = generic_sm4_ecb_encrypt(&ctx, output, input, 1);
  assert(ret == 0);
  (void)ret;

  generic_sm4_ecb_free(&ctx);
}

void owf_sm4_em_128(const uint8_t* key, const uint8_t* input, uint8_t* output) {
  /* Same as owf_sm4_128 with swapped keys and additional xor. */
  owf_sm4_128(input, key, output);
  xor_u8_array(output, key, output, 16);
}
