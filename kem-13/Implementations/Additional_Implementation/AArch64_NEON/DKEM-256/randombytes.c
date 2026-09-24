#include "parameters.h"

#if DKE_RANDOM == 1

#include <stddef.h>
#include <stdint.h>
#include "randombytes.h"

/* Defined in randombytes_sys.c (compiled with /GL-) */
extern void randombytes_sys_fill(uint8_t *out, size_t outlen);

void randombytes(uint8_t *out, size_t outlen) {
    randombytes_sys_fill(out, outlen);
}

void randombytes_init(void) { }

#endif
