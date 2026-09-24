#ifndef DKE_RANDOMBYTES_H
#define DKE_RANDOMBYTES_H

#include <stddef.h>
#include <stdint.h>

/* Fill pool with system entropy. Call once before any randombytes() calls. */
void randombytes_init(void);

/* Read from pre-filled pool. No system API calls in hot path. */
void randombytes(uint8_t *out, size_t outlen);

#endif
