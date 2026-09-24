#ifndef RANDOMBYTES_H
#define RANDOMBYTES_H

#include <stddef.h>
#include <stdint.h>

/* OS-entropy based randombytes for auxiliary test programs only.
   KAT generation uses the deterministic DRNG (drng.c) instead. */
void randombytes(uint8_t *out, size_t outlen);

#endif
