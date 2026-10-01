#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "sydo.h"

int main(void) {
  const sydo_ref_paramset_t *params = sydo_ref_get_paramset(SYDO_REF_160S);
  uint8_t *pk, *sk;
  if (!params) return 2;
  pk = malloc(params->public_key_size);
  sk = malloc(params->secret_key_size);
  if (!pk || !sk) return 3;
  if (sydo_ref_keygen(params, pk, params->public_key_size,
                      sk, params->secret_key_size) != 0) return 4;
  if (fwrite(pk, 1, params->public_key_size, stdout) != params->public_key_size) return 5;
  if (fwrite(sk, 1, params->secret_key_size, stdout) != params->secret_key_size) return 6;
  free(pk);
  free(sk);
  return 0;
}
