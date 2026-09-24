#ifndef YUANYANG_SYSRNG_H
#define YUANYANG_SYSRNG_H

#include <stddef.h>

/* Fill dst with operating-system randomness. Return 0 on success. */
int yuanyang_sysrng(void *dst, size_t len);

#endif
