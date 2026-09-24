#ifndef RANDOMBYTES_H
#define RANDOMBYTES_H

#include <stddef.h>
#include <stdint.h>

void randombytes(uint8_t *out, size_t outlen);


uint32_t randombytes_uniform(uint32_t upper_bound);
#endif
