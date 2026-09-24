#ifndef RANDOMBYTES_H
#define RANDOMBYTES_H

#include <stddef.h>
#include <stdint.h>

// Obtain cryptographic random numbers from the system entropy pool
void CryptoRandomBytes(uint8_t *out, size_t outlen);

#endif
