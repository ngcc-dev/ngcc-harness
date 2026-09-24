#include <stddef.h>
#include <stdint.h>

#include "drng.h"
#include "sign.h"

DRNG_ctx drng_algorithm;
static volatile uintptr_t exported_symbols[3];

/*
 * Load-only binary for static memory accounting.
 * It links the full algorithm implementation but intentionally executes no
 * key generation, signing, or verification logic.
 */
int main(void) {
  exported_symbols[0] = (uintptr_t)&crypto_sign_keypair;
  exported_symbols[1] = (uintptr_t)&crypto_sign_signature;
  exported_symbols[2] = (uintptr_t)&crypto_sign_verify;
  return exported_symbols[0] == 0U;
}
