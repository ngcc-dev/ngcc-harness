#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "randombytes.h"

// Platform-independent random bytes using /dev/urandom
// This is ONLY used for non-deterministic functional testing.
// The KAT generation uses the ICCS-provided SM3-based DRNG (drng.c).

static FILE *urandom = NULL;

void randombytes(uint8_t *out, size_t outlen) {
  if (urandom == NULL) {
    urandom = fopen("/dev/urandom", "rb");
    if (urandom == NULL) {
      fprintf(stderr, "ERROR: Cannot open /dev/urandom\n");
      exit(1);
    }
  }
  if (fread(out, 1, outlen, urandom) != outlen) {
    fprintf(stderr, "ERROR: Failed to read from /dev/urandom\n");
    exit(1);
  }
}
